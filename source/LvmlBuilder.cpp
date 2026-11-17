#include "LvmlBuilder.hpp"
#include "LvmlContext.hpp"
#include "LvmlFileSystem.hpp"
#include "TextUtils.hpp"

std::unique_ptr<CLvmlScene> CLvmlBuilder::Build(CLvmlContext& context, std::unique_ptr<CLvmlDocument> document,
                                                CLvssStyleSheet sheet, lv_obj_t* parent)
{
    CLvmlNode* body = document->GetBody();
    if (!body) {
        context.Log(document->GetPath() + ": no <screen> (or <body>) element");
        return nullptr;
    }
    std::unique_ptr<CLvmlScene> scene(new CLvmlScene(context, std::move(document), std::move(sheet)));
    if (!parent) parent = lv_screen_active();
    BuildElement(*scene, *body, parent);
    return scene;
}

bool CLvmlBuilder::IsInlineContent(const CLvmlNode& node)
{
    // <div>Hello <b>you</b><br/>there</div> → one label.
    static const char* kInline[] = { "br", "b", "strong", "i", "em", "u", "s", "small", "span", "a", "code",
                                     "mark", "sub", "sup", "font", "nobr" };
    bool hasText = false;
    for (const auto& c : node.GetChildren()) {
        if (c->IsText()) {
            if (!CTextUtils::Trim(c->GetText()).empty()) hasText = true;
            continue;
        }
        bool isInline = false;
        for (const char* t : kInline) if (c->IsTag(t)) { isInline = true; break; }
        if (!isInline || c->HasAttribute("id") || c->HasAttribute("class") || c->HasAttribute("onclick")) return false;
        if (!IsInlineContent(*c) && c->HasElementChildren()) return false;
    }
    return hasText;
}

void CLvmlBuilder::AddAnonymousLabel(lv_obj_t* parent, const std::string& text)
{
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_remove_style_all(l);
    lv_obj_remove_flag(l, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(l, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(l, text.c_str());
    // A sized parent wraps its text like a browser block does; a content-sized one grows with it.
    if (lv_obj_get_style_width(parent, LV_PART_MAIN) != LV_SIZE_CONTENT) lv_obj_set_width(l, lv_pct(100));
}

void CLvmlBuilder::ApplyAttributes(CLvmlScene& scene, lv_obj_t* obj, CLvmlNode& node)
{
    if (node.GetBoolAttribute("checkable")) lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
    if (node.GetBoolAttribute("checked")) lv_obj_add_state(obj, LV_STATE_CHECKED);
    if (node.GetBoolAttribute("disabled")) lv_obj_add_state(obj, LV_STATE_DISABLED);
    if (node.HasAttribute("focusable") && !node.GetBoolAttribute("focusable"))
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    if (const std::string* src = node.FindAttribute("src"); src && lv_obj_check_type(obj, &lv_image_class)) {
        const std::string full = CLvmlFileSystem::MakePath(CLvmlResources::ResolvePath(scene.m_document->GetPath(), *src));
        lv_image_set_src(obj, scene.m_storage->Intern(full));
    }
#if LV_USE_SWITCH
    if (lv_obj_check_type(obj, &lv_switch_class)) lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
#endif
}

lv_obj_t* CLvmlBuilder::BuildElement(CLvmlScene& scene, CLvmlNode& node, lv_obj_t* parent)
{
    const CLvmlWidgetFactory::SWidget* widget = CLvmlWidgetFactory::Find(node.GetTag());
    if (!widget) {
        scene.m_context.Log(scene.m_document->GetPath() + ":" + std::to_string(node.GetLine()) + ": unknown tag <" +
                            node.GetTag() + ">, created as <obj>");
        widget = &CLvmlWidgetFactory::Fallback();
    }

    lv_obj_t* obj = widget->create(parent, node);
    if (!scene.m_root) scene.m_root = obj; // the root gets its 100% × 100% base style in ApplyStyle
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    CLvmlScene::SObject* o = scene.Register(obj, &node, widget);
    ApplyAttributes(scene, obj, node);
    scene.Bind(*o);

    // Text content first: a richtext with links needs to know before styling decides clickability.
    if (widget->content == CLvmlWidgetFactory::EContent::Spans) {
        bool hasLinks = false;
        std::vector<CLvmlNode*> links;
        node.FindAll("a", links);
        for (const auto& c : node.GetChildren())
            if (c->IsElement() && (c->HasAttribute("href") || c->HasAttribute("onclick"))) hasLinks = true;
        if (hasLinks || !links.empty()) {
            scene.m_bindings.push_back(CLvmlScene::SBinding{ &scene, &node, std::string(), std::string(), true });
            lv_obj_add_event_cb(obj, CLvmlScene::OnLvEvent, LV_EVENT_CLICKED, &scene.m_bindings.back());
            o->hasHandler = true;
        }
    }

    scene.ApplyStyle(*o); // before children: they inherit the font from here

    switch (widget->content) {
    case CLvmlWidgetFactory::EContent::Text: {
        const std::string text = node.HasAttribute("text") ? node.GetAttribute("text") : node.GetInnerText();
        if (widget->setText) widget->setText(obj, text);
        break;
    }
    case CLvmlWidgetFactory::EContent::Children:
        BuildChildren(scene, *o);
        break;
    case CLvmlWidgetFactory::EContent::Spans: {
        SSpanState state;
        for (const auto& c : node.GetChildren()) BuildSpans(scene, *o, *c, nullptr, nullptr, state);
#if LV_USE_SPAN
        lv_spangroup_refresh(obj);
#endif
        break;
    }
    case CLvmlWidgetFactory::EContent::None:
        break;
    }
    return obj;
}

void CLvmlBuilder::BuildChildren(CLvmlScene& scene, CLvmlScene::SObject& o)
{
    CLvmlNode& node = *o.node;
    if (node.HasAttribute("text") || IsInlineContent(node)) {
        const std::string text = node.HasAttribute("text") ? node.GetAttribute("text") : node.GetInnerText();
        if (o.widget->setText) o.widget->setText(o.obj, text);
        else AddAnonymousLabel(o.obj, text);
        return;
    }
    for (const auto& c : node.GetChildren()) {
        if (c->IsElement()) {
            if (c->IsTag("option") || c->IsTag("br")) continue; // <br> only matters inside text
            BuildElement(scene, *c, o.obj);
            continue;
        }
        const std::string text(CTextUtils::Trim(CTextUtils::CollapseWhitespace(c->GetText())));
        if (text.empty()) continue;
        if (o.widget->setText) o.widget->setText(o.obj, text);
        else AddAnonymousLabel(o.obj, text);
    }
}

void CLvmlBuilder::BuildSpans(CLvmlScene& scene, CLvmlScene::SObject& group, CLvmlNode& node, lv_style_t* style,
                              const CLvmlScene::SSpanLink* link, SSpanState& state)
{
#if LV_USE_SPAN
    auto addSpan = [&](const std::string& text) {
        lv_span_t* span = lv_spangroup_add_span(group.obj);
        lv_span_set_text(span, text.c_str());
        if (style) lv_spangroup_set_span_style(group.obj, span, style);
        if (link) scene.m_spanLinks[span] = *link;
    };

    if (node.IsText()) {
        std::string text = CTextUtils::CollapseWhitespace(node.GetText());
        if (state.atLineStart) {
            while (!text.empty() && text.front() == ' ') text.erase(text.begin());
        }
        if (text.empty()) return;
        addSpan(text);
        state.atLineStart = false;
        return;
    }
    if (node.IsTag("br")) {
        addSpan("\n");
        state.atLineStart = true;
        return;
    }

    CLvmlScene::SSpanLink ownLink;
    const CLvmlScene::SSpanLink* childLink = link;
    if (node.HasAttribute("href") || node.HasAttribute("onclick")) {
        ownLink.node   = &node;
        ownLink.href   = node.GetAttribute("href");
        ownLink.action = node.HasAttribute("onclick") ? node.GetAttribute("onclick") : std::string("link");
        childLink      = &ownLink;
    }
    lv_style_t* childStyle = scene.SpanStyle(node, group);
    for (const auto& c : node.GetChildren()) BuildSpans(scene, group, *c, childStyle, childLink, state);
#else
    (void)scene; (void)group; (void)node; (void)style; (void)link; (void)state;
#endif
}
