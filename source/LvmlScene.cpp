#include "LvmlScene.hpp"
#include "LvmlContext.hpp"
#include "LvmlFileSystem.hpp"
#include "LvssParser.hpp"
#include "LvssValue.hpp"
#include "TextUtils.hpp"
#include <cmath>

// Attribute → LVGL event.
const CLvmlScene::SEventName CLvmlScene::kEventNames[] = {
    { "onclick", LV_EVENT_CLICKED },         { "on-click", LV_EVENT_CLICKED },
    { "onchange", LV_EVENT_VALUE_CHANGED },  { "on-change", LV_EVENT_VALUE_CHANGED },
    { "onpress", LV_EVENT_PRESSED },         { "on-press", LV_EVENT_PRESSED },
    { "onrelease", LV_EVENT_RELEASED },      { "on-release", LV_EVENT_RELEASED },
    { "onlongpress", LV_EVENT_LONG_PRESSED }, { "on-long-press", LV_EVENT_LONG_PRESSED },
    { "onfocus", LV_EVENT_FOCUSED },         { "onblur", LV_EVENT_DEFOCUSED },
    { "onready", LV_EVENT_READY },           { "onsubmit", LV_EVENT_READY },
    { "onhover", LV_EVENT_HOVER_OVER },      { "onleave", LV_EVENT_HOVER_LEAVE },
    { "onscroll", LV_EVENT_SCROLL },
};

CLvmlScene::CLvmlScene(CLvmlContext& context, std::unique_ptr<CLvmlDocument> document, CLvssStyleSheet sheet)
    : m_context(context), m_document(std::move(document)), m_storage(std::make_unique<CLvmlStorage>())
{
    m_styles   = std::make_unique<CLvmlStyleSet>(context, *m_storage, std::move(sheet));
    m_rootBase = m_storage->NewStyle();
    lv_style_set_width(m_rootBase, lv_pct(100));
    lv_style_set_height(m_rootBase, lv_pct(100));
    m_containerBase = m_storage->NewStyle();
    lv_style_set_width(m_containerBase, LV_SIZE_CONTENT);
    lv_style_set_height(m_containerBase, LV_SIZE_CONTENT);
}

CLvmlScene::~CLvmlScene()
{
    m_destroying = true;
    if (m_root) {
        lv_obj_t* root = m_root;
        m_root = nullptr;
        lv_obj_delete(root);
    }
    m_objects.clear();
    m_nodeObjects.clear();
    // m_styles, then m_storage (lv_style_reset) go after the objects are gone.
}

// ---------------------------------------------------------------------------------------------
// lookup
// ---------------------------------------------------------------------------------------------

CLvmlNode* CLvmlScene::FindNode(std::string_view id) const
{
    return m_document->GetRoot().FindById(id);
}

lv_obj_t* CLvmlScene::Find(std::string_view id) const
{
    return GetObject(FindNode(id));
}

CLvmlNode* CLvmlScene::GetNode(lv_obj_t* obj) const
{
    const SObject* o = Info(obj);
    return o ? o->node : nullptr;
}

lv_obj_t* CLvmlScene::GetObject(const CLvmlNode* node) const
{
    if (!node) return nullptr;
    const auto it = m_nodeObjects.find(node);
    return it == m_nodeObjects.end() ? nullptr : it->second;
}

CLvmlScene::SObject* CLvmlScene::Info(lv_obj_t* obj)
{
    const auto it = m_objects.find(obj);
    return it == m_objects.end() ? nullptr : &it->second;
}

const CLvmlScene::SObject* CLvmlScene::Info(lv_obj_t* obj) const
{
    const auto it = m_objects.find(obj);
    return it == m_objects.end() ? nullptr : &it->second;
}

CLvmlScene::SObject* CLvmlScene::Register(lv_obj_t* obj, CLvmlNode* node, const CLvmlWidgetFactory::SWidget* widget)
{
    SObject& o = m_objects[obj];
    o.obj    = obj;
    o.node   = node;
    o.widget = widget;
    const std::string theme = CTextUtils::ToLower(node->GetAttribute("theme"));
    o.keepTheme      = theme == "keep" || theme == "default";
    o.dependsOnState = m_styles->DependsOnAncestorState(*node);
    m_nodeObjects[node] = obj;
    lv_obj_add_event_cb(obj, OnDeleted, LV_EVENT_DELETE, this);
    if (m_styles->HasAncestorStateRules()) lv_obj_add_event_cb(obj, OnStateChanged, LV_EVENT_STATE_CHANGED, this);
    return &o;
}

// ---------------------------------------------------------------------------------------------
// events
// ---------------------------------------------------------------------------------------------

void CLvmlScene::On(const std::string& action, Handler handler)
{
    m_handlers[action] = std::move(handler);
}

void CLvmlScene::Bind(SObject& o)
{
    CLvmlNode& node = *o.node;
    bool clickBound = false;
    for (const CLvmlNode::Attribute& a : node.GetAttributes()) {
        const std::string name = CTextUtils::ToLower(a.first);
        for (const SEventName& ev : kEventNames) {
            if (name != ev.name) continue;
            m_bindings.push_back(SBinding{ this, &node, a.second, node.GetAttribute("href"), false });
            lv_obj_add_event_cb(o.obj, OnLvEvent, ev.code, &m_bindings.back());
            o.hasHandler = true;
            if (ev.code == LV_EVENT_CLICKED) clickBound = true;
        }
    }
    if (!clickBound && node.HasAttribute("href") && o.widget->content != CLvmlWidgetFactory::EContent::Spans) {
        m_bindings.push_back(SBinding{ this, &node, "link", node.GetAttribute("href"), false });
        lv_obj_add_event_cb(o.obj, OnLvEvent, LV_EVENT_CLICKED, &m_bindings.back());
        o.hasHandler = true;
    }
}

void CLvmlScene::OnLvEvent(lv_event_t* e)
{
    auto* b = static_cast<SBinding*>(lv_event_get_user_data(e));
    if (!b || !b->scene || b->scene->m_destroying) return;
    CLvmlScene& scene = *b->scene;
    auto* obj = static_cast<lv_obj_t*>(lv_event_get_current_target(e));

    if (b->spans) {
#if LV_USE_SPAN
        lv_indev_t* indev = lv_indev_active();
        if (!indev) return;
        lv_point_t p;
        lv_indev_get_point(indev, &p);
        lv_span_t* span = lv_spangroup_get_span_by_point(obj, &p);
        const auto it = scene.m_spanLinks.find(span);
        if (it == scene.m_spanLinks.end()) return;
        CLvmlEvent event(scene, e, obj, it->second.node, it->second.action, it->second.href);
        scene.Dispatch(event);
#endif
        return;
    }
    CLvmlEvent event(scene, e, obj, b->node, b->action, b->href);
    scene.Dispatch(event);
}

void CLvmlScene::Dispatch(CLvmlEvent& event)
{
    const auto it = m_handlers.find(event.GetAction());
    if (it != m_handlers.end() && it->second) {
        it->second(event);
        return;
    }
    if (m_fallback) m_fallback(event);
}

void CLvmlScene::OnDeleted(lv_event_t* e)
{
    auto* scene = static_cast<CLvmlScene*>(lv_event_get_user_data(e));
    auto* obj   = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    if (!scene) return;
    const auto it = scene->m_objects.find(obj);
    if (it != scene->m_objects.end()) {
        scene->m_nodeObjects.erase(it->second.node);
        scene->m_objects.erase(it);
    }
    if (obj == scene->m_root) scene->m_root = nullptr;
}

void CLvmlScene::OnStateChanged(lv_event_t* e)
{
    auto* scene = static_cast<CLvmlScene*>(lv_event_get_user_data(e));
    if (!scene || scene->m_destroying) return;
    scene->RestyleDependents(static_cast<lv_obj_t*>(lv_event_get_current_target(e)));
}

void CLvmlScene::RestyleDependents(lv_obj_t* obj)
{
    const uint32_t count = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < count; ++i) {
        lv_obj_t* child = lv_obj_get_child(obj, int32_t(i));
        if (SObject* o = Info(child)) {
            if (o->dependsOnState) ApplyStyle(*o);
        }
        RestyleDependents(child);
    }
}

// ---------------------------------------------------------------------------------------------
// styling
// ---------------------------------------------------------------------------------------------

void CLvmlScene::Restyle(lv_obj_t* obj, bool recursive)
{
    if (SObject* o = Info(obj)) ApplyStyle(*o);
    if (!recursive) return;
    const uint32_t count = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < count; ++i) Restyle(lv_obj_get_child(obj, int32_t(i)), true);
}

void CLvmlScene::ApplyStyle(SObject& o)
{
    lv_obj_t*  obj  = o.obj;
    CLvmlNode& node = *o.node;

    lv_obj_remove_style_all(obj);
    if (o.keepTheme) lv_theme_apply(obj);
    if (obj == m_root) lv_obj_add_style(obj, m_rootBase, 0);
    else if (o.widget->container) lv_obj_add_style(obj, m_containerBase, 0);

    const CLvssSelector::StateQuery query = [this](const CLvmlNode& n, const std::vector<std::string>& states) {
        lv_obj_t* target = GetObject(&n);
        if (!target) return false;
        for (const std::string& s : states)
            if (!lv_obj_has_state(target, CLvmlStyleSet::StateFromName(s))) return false;
        return true;
    };
    std::vector<const CLvmlStyleSet::SEntry*> entries;
    m_styles->Match(node, &query, entries);

    bool needsInput = false;
    std::map<std::string, std::string> objectDecls; // Object + Font declarations, cascade resolved
    for (const CLvmlStyleSet::SEntry* e : entries) {
        if (e->style) lv_obj_add_style(obj, e->style, e->lvSelector);
        if (e->needsInput) needsInput = true;
        if (!e->isDefault) continue;
        for (const CLvssDeclaration& d : e->rule->declarations) {
            const CLvssProperty::EKind k = CLvssProperty::Classify(d.property);
            if (k == CLvssProperty::EKind::Object || k == CLvssProperty::EKind::Font)
                objectDecls[d.property] = m_styles->GetSheet().ResolveVariables(d.value);
        }
    }

    // style="…" — local properties, highest priority.
    o.inlineDecls.clear();
    if (const std::string* inlineStyle = node.FindAttribute("style")) {
        o.inlineDecls = CLvssParser::ParseDeclarations(*inlineStyle);
        const CLvssProperty::SEnv env = m_styles->MakeEnv(m_document->GetPath(), &o.inlineDecls);
        CLvssProperty::CObjectSink sink(obj, LV_PART_MAIN | LV_STATE_DEFAULT);
        for (const CLvssDeclaration& d : o.inlineDecls) {
            const CLvssProperty::EKind k = CLvssProperty::Classify(d.property);
            if (k == CLvssProperty::EKind::Style || d.property == "display") {
                std::string error;
                if (!CLvssProperty::Apply(d, env, sink, &error) || !error.empty())
                    m_context.Log(m_document->GetPath() + ": style=\"" + d.property + "\": " + error);
            }
            if (k == CLvssProperty::EKind::Object || k == CLvssProperty::EKind::Font)
                objectDecls[d.property] = m_styles->GetSheet().ResolveVariables(d.value);
            if (k == CLvssProperty::EKind::Unknown)
                m_context.Log(m_document->GetPath() + ": style=\"" + d.property + "\": unknown property");
        }
    }

    ApplyFont(o, objectDecls);
    ApplyObjectProperties(o, objectDecls, needsInput);
}

void CLvmlScene::ApplyFont(SObject& o, const std::map<std::string, std::string>& decls)
{
    // Inherit from the closest styled ancestor.
    std::string family = m_context.GetFonts().GetDefaultFamily();
    int32_t     px     = m_context.ScalePx(float(m_context.GetFonts().GetDefaultSize()));
    for (const CLvmlNode* p = o.node->GetParent(); p; p = p->GetParent()) {
        if (const SObject* po = Info(GetObject(p))) {
            family = po->fontFamily;
            px     = po->fontPx;
            break;
        }
    }

    bool explicitFont = o.obj == m_root;
    std::string size;
    if (const auto it = decls.find("font"); it != decls.end()) {
        std::string f;
        CLvmlContext::ParseFontShorthand(it->second, f, size);
        if (!f.empty()) family = f;
        explicitFont = true;
    }
    if (const auto it = decls.find("font-family"); it != decls.end()) { family = it->second; explicitFont = true; }
    if (const auto it = decls.find("font-size"); it != decls.end()) { size = it->second; explicitFont = true; }
    if (!size.empty()) px = m_context.ResolveFontPx(size, px);

    o.fontFamily = family;
    o.fontPx     = px;
    if (explicitFont) lv_obj_set_style_text_font(o.obj, m_context.GetFonts().GetFont(family, px), 0);
}

void CLvmlScene::ApplyLineHeight(SObject& o, const std::string& value)
{
    const std::string v = CTextUtils::ToLower(CTextUtils::Trim(value));
    if (v.empty() || v == "normal") return;
    const lv_font_t* font = lv_obj_get_style_text_font(o.obj, LV_PART_MAIN);
    if (!font) return;
    const float fontPx = float(o.fontPx > 0 ? o.fontPx : font->line_height);
    float total = 0;
    CLvssValue::SLength len;
    float factor = 0;
    if (CTextUtils::ParseFloat(v, factor)) total = factor * fontPx;            // 1.5
    else if (CLvssValue::ParseLength(v, len)) {
        if (len.unit == CLvssValue::EUnit::Percent) total = len.value * fontPx / 100.0f;
        else if (len.unit == CLvssValue::EUnit::Em) total = len.value * fontPx;
        else total = float(m_context.ScalePx(len.unit == CLvssValue::EUnit::Rem ? len.value * 16.0f : len.value));
    } else {
        m_context.Log("bad line-height '" + value + "'");
        return;
    }
    lv_obj_set_style_text_line_space(o.obj, int32_t(std::lround(total)) - font->line_height, 0);
}

void CLvmlScene::ApplyObjectProperties(SObject& o, const std::map<std::string, std::string>& decls, bool needsInput)
{
    lv_obj_t*  obj  = o.obj;
    CLvmlNode& node = *o.node;
    auto get = [&](const char* name) {
        const auto it = decls.find(name);
        return it == decls.end() ? std::string() : CTextUtils::ToLower(CTextUtils::Trim(it->second));
    };

    // visibility: hidden="…" wins over display:none
    bool hidden = get("display") == "none";
    if (node.HasAttribute("hidden")) hidden = node.GetBoolAttribute("hidden");
    if (hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);

    // overflow
    std::string ox = get("overflow"), oy = ox;
    if (const std::string x = get("overflow-x"); !x.empty()) ox = x;
    if (const std::string y = get("overflow-y"); !y.empty()) oy = y;
    // "overflow: hidden scroll" two-value form
    if (const std::vector<std::string> two = CTextUtils::SplitWhitespace(get("overflow")); two.size() == 2) {
        if (get("overflow-x").empty()) ox = two[0];
        if (get("overflow-y").empty()) oy = two[1];
    }
    const bool overflowSet = !ox.empty() || !oy.empty();
    if (overflowSet || o.widget->container || node.HasAttribute("scrollable")) {
        lv_dir_t dir = LV_DIR_NONE;
        const bool sx = ox == "scroll" || ox == "auto", sy = oy == "scroll" || oy == "auto";
        if (sx) dir = lv_dir_t(dir | LV_DIR_HOR);
        if (sy) dir = lv_dir_t(dir | LV_DIR_VER);
        bool scrollable = dir != LV_DIR_NONE;
        if (node.HasAttribute("scrollable")) {
            scrollable = node.GetBoolAttribute("scrollable");
            if (scrollable && dir == LV_DIR_NONE) dir = LV_DIR_ALL;
        }
        if (scrollable) {
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_scroll_dir(obj, dir);
            lv_obj_set_scrollbar_mode(obj, (ox == "scroll" || oy == "scroll") ? LV_SCROLLBAR_MODE_ON : LV_SCROLLBAR_MODE_AUTO);
        } else if (overflowSet || o.widget->container) {
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        }
        if (ox == "visible" && oy == "visible") lv_obj_add_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        else lv_obj_remove_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    }
    if (const std::string m = get("scrollbar-mode"); !m.empty()) {
        lv_obj_set_scrollbar_mode(obj, m == "off" || m == "none" ? LV_SCROLLBAR_MODE_OFF
                                     : m == "on" ? LV_SCROLLBAR_MODE_ON
                                     : m == "active" ? LV_SCROLLBAR_MODE_ACTIVE : LV_SCROLLBAR_MODE_AUTO);
    }
    auto snap = [](const std::string& v) {
        return v == "start" ? LV_SCROLL_SNAP_START : v == "end" ? LV_SCROLL_SNAP_END
             : v == "center" ? LV_SCROLL_SNAP_CENTER : LV_SCROLL_SNAP_NONE;
    };
    if (const std::string s = get("scroll-snap-x"); !s.empty()) lv_obj_set_scroll_snap_x(obj, snap(s));
    if (const std::string s = get("scroll-snap-y"); !s.empty()) lv_obj_set_scroll_snap_y(obj, snap(s));

    // position: absolute/fixed take the element out of flex/grid flow
    const std::string pos = get("position");
    if (pos == "absolute" || pos == "fixed") lv_obj_add_flag(obj, LV_OBJ_FLAG_IGNORE_LAYOUT);
    else lv_obj_remove_flag(obj, LV_OBJ_FLAG_IGNORE_LAYOUT);
    if (pos == "fixed") lv_obj_add_flag(obj, LV_OBJ_FLAG_FLOATING);
    else lv_obj_remove_flag(obj, LV_OBJ_FLAG_FLOATING);

    // image fit
    std::string fit = get("image-fit");
    if (fit.empty()) fit = get("object-fit");
    if (fit.empty()) fit = get("inner-align");
    if (fit.empty()) fit = get("image-align");
    if (!fit.empty() && lv_obj_check_type(obj, &lv_image_class)) {
        struct SFit { const char* name; lv_image_align_t align; };
        static const SFit kFits[] = {
            { "fill", LV_IMAGE_ALIGN_STRETCH }, { "stretch", LV_IMAGE_ALIGN_STRETCH }, { "contain", LV_IMAGE_ALIGN_CONTAIN },
            { "cover", LV_IMAGE_ALIGN_COVER },  { "tile", LV_IMAGE_ALIGN_TILE },       { "none", LV_IMAGE_ALIGN_CENTER },
            { "center", LV_IMAGE_ALIGN_CENTER }, { "scale-down", LV_IMAGE_ALIGN_CONTAIN },
            { "top-left", LV_IMAGE_ALIGN_TOP_LEFT }, { "top-mid", LV_IMAGE_ALIGN_TOP_MID }, { "top-right", LV_IMAGE_ALIGN_TOP_RIGHT },
            { "bottom-left", LV_IMAGE_ALIGN_BOTTOM_LEFT }, { "bottom-mid", LV_IMAGE_ALIGN_BOTTOM_MID },
            { "bottom-right", LV_IMAGE_ALIGN_BOTTOM_RIGHT }, { "left-mid", LV_IMAGE_ALIGN_LEFT_MID },
            { "right-mid", LV_IMAGE_ALIGN_RIGHT_MID },
        };
        bool found = false;
        for (const SFit& f : kFits)
            if (fit == f.name) { lv_image_set_inner_align(obj, f.align); found = true; break; }
        if (!found) m_context.Log("unknown image-fit '" + fit + "'");
    }

    // label long mode
    std::string longMode = get("long-mode");
    if (longMode.empty()) longMode = get("text-overflow");
    if (!longMode.empty() && lv_obj_check_type(obj, &lv_label_class)) {
        lv_label_long_mode_t m = LV_LABEL_LONG_MODE_WRAP;
        if (longMode == "dots" || longMode == "dot" || longMode == "ellipsis") m = LV_LABEL_LONG_MODE_DOTS;
        else if (longMode == "scroll") m = LV_LABEL_LONG_MODE_SCROLL;
        else if (longMode == "scroll-circular" || longMode == "marquee") m = LV_LABEL_LONG_MODE_SCROLL_CIRCULAR;
        else if (longMode == "clip") m = LV_LABEL_LONG_MODE_CLIP;
        lv_label_set_long_mode(obj, m);
    }

    // clickability
    bool clickable = o.widget->interactive || o.hasHandler || needsInput || lv_obj_has_flag(obj, LV_OBJ_FLAG_CHECKABLE);
    if (node.HasAttribute("clickable")) clickable = node.GetBoolAttribute("clickable");
    if (get("pointer-events") == "none") clickable = false;
    if (clickable) lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    else lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);

    if (const auto it = decls.find("line-height"); it != decls.end()) ApplyLineHeight(o, it->second);
}

void CLvmlScene::CollectStyle(const CLvmlNode& node, CLvssProperty::CSink& sink, std::string& family, std::string& size)
{
    std::vector<const CLvmlStyleSet::SEntry*> entries;
    m_styles->Match(node, nullptr, entries);
    for (const CLvmlStyleSet::SEntry* e : entries) {
        if (!e->isDefault) continue;
        const CLvssProperty::SEnv env = m_styles->MakeEnv(e->rule->source, &e->rule->declarations);
        for (const CLvssDeclaration& d : e->rule->declarations) {
            const CLvssProperty::EKind k = CLvssProperty::Classify(d.property);
            if (k == CLvssProperty::EKind::Style) CLvssProperty::Apply(d, env, sink, nullptr);
            else if (d.property == "font-family") family = m_styles->GetSheet().ResolveVariables(d.value);
            else if (d.property == "font-size") size = m_styles->GetSheet().ResolveVariables(d.value);
            else if (d.property == "font") CLvmlContext::ParseFontShorthand(m_styles->GetSheet().ResolveVariables(d.value), family, size);
        }
    }
    if (const std::string* inlineStyle = node.FindAttribute("style")) {
        const std::vector<CLvssDeclaration> decls = CLvssParser::ParseDeclarations(*inlineStyle);
        const CLvssProperty::SEnv env = m_styles->MakeEnv(m_document->GetPath(), &decls);
        for (const CLvssDeclaration& d : decls) {
            if (CLvssProperty::Classify(d.property) == CLvssProperty::EKind::Style) CLvssProperty::Apply(d, env, sink, nullptr);
            else if (d.property == "font-family") family = d.value;
            else if (d.property == "font-size") size = d.value;
        }
    }
}

lv_style_t* CLvmlScene::SpanStyle(CLvmlNode& node, const SObject& group)
{
    // Spans cannot nest styles: fold the chain group → … → node into one style.
    std::vector<const CLvmlNode*> chain;
    for (const CLvmlNode* p = &node; p && p != group.node; p = p->GetParent()) chain.insert(chain.begin(), p);

    lv_style_t* style = m_storage->NewStyle();
    CLvssProperty::CStyleSink sink(style);
    std::string family = group.fontFamily, size;
    int32_t px = group.fontPx;
    for (const CLvmlNode* n : chain) {
        std::string f, s;
        CollectStyle(*n, sink, f, s);
        if (!f.empty()) family = f;
        if (!s.empty()) px = m_context.ResolveFontPx(s, px);
    }
    if (family != group.fontFamily || px != group.fontPx)
        lv_style_set_text_font(style, m_context.GetFonts().GetFont(family, px));
    return style;
}

// ---------------------------------------------------------------------------------------------
// runtime helpers
// ---------------------------------------------------------------------------------------------

bool CLvmlScene::SetText(lv_obj_t* obj, const std::string& text)
{
    const SObject* o = Info(obj);
    if (!o) return false;
#if LV_USE_SPAN
    if (o->widget->content == CLvmlWidgetFactory::EContent::Spans) {
        while (lv_spangroup_get_span_count(obj) > 0) lv_spangroup_delete_span(obj, lv_spangroup_get_child(obj, 0));
        lv_span_t* span = lv_spangroup_add_span(obj);
        lv_span_set_text(span, text.c_str());
        lv_spangroup_refresh(obj);
        return true;
    }
#endif
    if (o->widget->setText) { o->widget->setText(obj, text); return true; }
    // A container: put the text into its first (anonymous) label.
    for (uint32_t i = 0; i < lv_obj_get_child_count(obj); ++i) {
        lv_obj_t* c = lv_obj_get_child(obj, int32_t(i));
        if (lv_obj_check_type(c, &lv_label_class) && !Info(c)) { lv_label_set_text(c, text.c_str()); return true; }
    }
    lv_obj_t* l = lv_label_create(obj);
    lv_obj_remove_style_all(l);
    lv_obj_remove_flag(l, LV_OBJ_FLAG_CLICKABLE);
    lv_label_set_text(l, text.c_str());
    return true;
}

std::string CLvmlScene::GetText(lv_obj_t* obj) const
{
    const SObject* o = Info(obj);
    if (!o) return std::string();
    if (o->widget->getText) return o->widget->getText(obj);
    for (uint32_t i = 0; i < lv_obj_get_child_count(obj); ++i) {
        lv_obj_t* c = lv_obj_get_child(obj, int32_t(i));
        if (lv_obj_check_type(c, &lv_label_class) && !Info(c)) return lv_label_get_text(c);
    }
    return std::string();
}

int32_t CLvmlScene::GetValue(lv_obj_t* obj) const
{
    int32_t v = 0;
    if (obj) CLvmlWidgetFactory::GetValue(obj, v);
    return v;
}

bool CLvmlScene::SetText(std::string_view id, const std::string& text)
{
    lv_obj_t* obj = Find(id);
    return obj && SetText(obj, text);
}

std::string CLvmlScene::GetText(std::string_view id) const
{
    return GetText(Find(id));
}

bool CLvmlScene::SetVisible(std::string_view id, bool visible)
{
    CLvmlNode* node = FindNode(id);
    lv_obj_t*  obj  = GetObject(node);
    if (!obj) return false;
    node->SetAttribute("hidden", visible ? "false" : "true");
    if (visible) lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    return true;
}

bool CLvmlScene::IsVisible(std::string_view id) const
{
    lv_obj_t* obj = Find(id);
    return obj && !lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

bool CLvmlScene::SetChecked(std::string_view id, bool checked)
{
    lv_obj_t* obj = Find(id);
    if (!obj) return false;
    if (checked) lv_obj_add_state(obj, LV_STATE_CHECKED);
    else lv_obj_remove_state(obj, LV_STATE_CHECKED);
    return true;
}

bool CLvmlScene::IsChecked(std::string_view id) const
{
    lv_obj_t* obj = Find(id);
    return obj && lv_obj_has_state(obj, LV_STATE_CHECKED);
}

bool CLvmlScene::SetEnabled(std::string_view id, bool enabled)
{
    lv_obj_t* obj = Find(id);
    if (!obj) return false;
    if (enabled) lv_obj_remove_state(obj, LV_STATE_DISABLED);
    else lv_obj_add_state(obj, LV_STATE_DISABLED);
    return true;
}

bool CLvmlScene::SetValue(std::string_view id, int32_t value, bool animate)
{
    lv_obj_t* obj = Find(id);
    return obj && CLvmlWidgetFactory::SetValue(obj, value, animate);
}

int32_t CLvmlScene::GetValue(std::string_view id) const
{
    return GetValue(Find(id));
}

bool CLvmlScene::SetImage(std::string_view id, const std::string& path)
{
    CLvmlNode* node = FindNode(id);
    lv_obj_t*  obj  = GetObject(node);
    if (!obj || !lv_obj_check_type(obj, &lv_image_class)) return false;
    node->SetAttribute("src", path);
    const std::string full = CLvmlFileSystem::MakePath(CLvmlResources::ResolvePath(m_document->GetPath(), path));
    lv_image_set_src(obj, m_storage->Intern(full));
    return true;
}

bool CLvmlScene::AddClass(std::string_view id, std::string_view cls)
{
    CLvmlNode* node = FindNode(id);
    lv_obj_t*  obj  = GetObject(node);
    if (!obj) return false;
    if (node->AddClass(cls)) Restyle(obj, true);
    return true;
}

bool CLvmlScene::RemoveClass(std::string_view id, std::string_view cls)
{
    CLvmlNode* node = FindNode(id);
    lv_obj_t*  obj  = GetObject(node);
    if (!obj) return false;
    if (node->RemoveClass(cls)) Restyle(obj, true);
    return true;
}

bool CLvmlScene::ToggleClass(std::string_view id, std::string_view cls, bool on)
{
    return on ? AddClass(id, cls) : RemoveClass(id, cls);
}

bool CLvmlScene::HasClass(std::string_view id, std::string_view cls) const
{
    const CLvmlNode* node = FindNode(id);
    return node && node->HasClass(cls);
}

bool CLvmlScene::SetStyle(std::string_view id, const std::string& declarations)
{
    return SetAttribute(id, "style", declarations);
}

bool CLvmlScene::SetAttribute(std::string_view id, const std::string& name, const std::string& value)
{
    CLvmlNode* node = FindNode(id);
    lv_obj_t*  obj  = GetObject(node);
    if (!obj) return false;
    node->SetAttribute(name, value);
    Restyle(obj, true);
    return true;
}
