#include "LvmlWidgetFactory.hpp"
#include "TextUtils.hpp"

std::map<std::string, CLvmlWidgetFactory::SWidget>& CLvmlWidgetFactory::Registry()
{
    static std::map<std::string, SWidget> registry;
    static bool initialised = false;
    if (!initialised) {
        initialised = true;
        RegisterBuiltins(registry);
    }
    return registry;
}

void CLvmlWidgetFactory::Register(const std::string& tag, SWidget widget)
{
    Registry()[CTextUtils::ToLower(tag)] = std::move(widget);
}

const CLvmlWidgetFactory::SWidget* CLvmlWidgetFactory::Find(const std::string& tag)
{
    auto& r = Registry();
    const auto it = r.find(CTextUtils::ToLower(tag));
    return it == r.end() ? nullptr : &it->second;
}

const CLvmlWidgetFactory::SWidget& CLvmlWidgetFactory::Fallback()
{
    return *Find("obj");
}

void CLvmlWidgetFactory::SetLabelText(lv_obj_t* label, const std::string& text)
{
    lv_label_set_text(label, text.c_str());
}

bool CLvmlWidgetFactory::SetValue(lv_obj_t* obj, int32_t value, bool animate)
{
    const lv_anim_enable_t anim = animate ? LV_ANIM_ON : LV_ANIM_OFF;
#if LV_USE_SLIDER
    if (lv_obj_check_type(obj, &lv_slider_class)) { lv_slider_set_value(obj, value, anim); return true; }
#endif
#if LV_USE_BAR
    if (lv_obj_check_type(obj, &lv_bar_class)) { lv_bar_set_value(obj, value, anim); return true; }
#endif
#if LV_USE_DROPDOWN
    if (lv_obj_check_type(obj, &lv_dropdown_class)) { lv_dropdown_set_selected(obj, uint32_t(value)); return true; }
#endif
    (void)anim;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CHECKABLE)) {
        if (value) lv_obj_add_state(obj, LV_STATE_CHECKED);
        else lv_obj_remove_state(obj, LV_STATE_CHECKED);
        return true;
    }
    return false;
}

bool CLvmlWidgetFactory::GetValue(lv_obj_t* obj, int32_t& value)
{
#if LV_USE_SLIDER
    if (lv_obj_check_type(obj, &lv_slider_class)) { value = lv_slider_get_value(obj); return true; }
#endif
#if LV_USE_BAR
    if (lv_obj_check_type(obj, &lv_bar_class)) { value = lv_bar_get_value(obj); return true; }
#endif
#if LV_USE_DROPDOWN
    if (lv_obj_check_type(obj, &lv_dropdown_class)) { value = int32_t(lv_dropdown_get_selected(obj)); return true; }
#endif
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CHECKABLE)) { value = lv_obj_has_state(obj, LV_STATE_CHECKED) ? 1 : 0; return true; }
    return false;
}

void CLvmlWidgetFactory::RegisterBuiltins(std::map<std::string, SWidget>& r)
{
    auto num = [](const CLvmlNode& n, const char* name, int32_t def) {
        long v = def;
        if (const std::string* s = n.FindAttribute(name)) CTextUtils::ParseInt(*s, v);
        return int32_t(v);
    };

    // ---- containers ----
    SWidget obj;
    obj.create    = [](lv_obj_t* parent, const CLvmlNode&) { return lv_obj_create(parent); };
    obj.content   = EContent::Children;
    obj.container = true;
    for (const char* tag : { "obj", "div", "view", "panel", "screen", "body", "section", "row", "column", "box" })
        r[tag] = obj;
    // HTML block/inline tags keep their names (so CSS tag selectors still match) and act as containers;
    // text-only content becomes an anonymous label inside.
    for (const char* tag : { "p", "h1", "h2", "h3", "h4", "h5", "h6", "span", "a", "ul", "ol", "li", "header",
                             "footer", "nav", "main", "aside", "article", "form", "figure", "figcaption", "strong",
                             "b", "i", "em", "small", "fieldset", "legend", "dl", "dt", "dd", "blockquote", "pre",
                             "code", "table", "tr", "td", "th", "thead", "tbody" })
        r[tag] = obj;

    // ---- label ----
    SWidget label;
    label.create  = [](lv_obj_t* parent, const CLvmlNode&) { return lv_label_create(parent); };
    label.setText = [](lv_obj_t* o, const std::string& t) { SetLabelText(o, t); };
    label.getText = [](lv_obj_t* o) { return std::string(lv_label_get_text(o)); };
    label.content = EContent::Text;
    for (const char* tag : { "label", "text" }) r[tag] = label;

    // ---- button: text-only content becomes a centred child label ----
    SWidget button;
    button.create = [](lv_obj_t* parent, const CLvmlNode&) { return lv_button_create(parent); };
    button.setText = [](lv_obj_t* o, const std::string& t) {
        lv_obj_t* l = nullptr;
        for (uint32_t i = 0; i < lv_obj_get_child_count(o); ++i) {
            lv_obj_t* c = lv_obj_get_child(o, int32_t(i));
            if (lv_obj_check_type(c, &lv_label_class)) { l = c; break; }
        }
        if (!l) {
            l = lv_label_create(o);
            lv_obj_remove_style_all(l);
            lv_obj_center(l);
            lv_obj_remove_flag(l, LV_OBJ_FLAG_CLICKABLE);
        }
        SetLabelText(l, t);
    };
    button.getText = [](lv_obj_t* o) {
        for (uint32_t i = 0; i < lv_obj_get_child_count(o); ++i) {
            lv_obj_t* c = lv_obj_get_child(o, int32_t(i));
            if (lv_obj_check_type(c, &lv_label_class)) return std::string(lv_label_get_text(c));
        }
        return std::string();
    };
    button.content     = EContent::Children;
    button.interactive = true;
    button.container   = true;
    for (const char* tag : { "button", "btn" }) r[tag] = button;

    // ---- image ----
    SWidget image;
    image.create  = [](lv_obj_t* parent, const CLvmlNode&) { return lv_image_create(parent); };
    image.content = EContent::Children; // overlays (labels on a picture) are allowed
    for (const char* tag : { "image", "img" }) r[tag] = image;

#if LV_USE_CHECKBOX
    SWidget checkbox;
    checkbox.create  = [](lv_obj_t* parent, const CLvmlNode&) { return lv_checkbox_create(parent); };
    checkbox.setText = [](lv_obj_t* o, const std::string& t) { lv_checkbox_set_text(o, t.c_str()); };
    checkbox.getText = [](lv_obj_t* o) { return std::string(lv_checkbox_get_text(o)); };
    checkbox.content     = EContent::Text;
    checkbox.interactive = true;
    r["checkbox"] = checkbox;
#endif

#if LV_USE_BAR
    SWidget bar;
    bar.create = [num](lv_obj_t* parent, const CLvmlNode& n) {
        lv_obj_t* o = lv_bar_create(parent);
        lv_bar_set_range(o, num(n, "min", 0), num(n, "max", 100));
        lv_bar_set_value(o, num(n, "value", 0), LV_ANIM_OFF);
        return o;
    };
    bar.content = EContent::None;
    for (const char* tag : { "bar", "progress" }) r[tag] = bar;
#endif

#if LV_USE_SLIDER
    SWidget slider;
    slider.create = [num](lv_obj_t* parent, const CLvmlNode& n) {
        lv_obj_t* o = lv_slider_create(parent);
        lv_slider_set_range(o, num(n, "min", 0), num(n, "max", 100));
        lv_slider_set_value(o, num(n, "value", 0), LV_ANIM_OFF);
        return o;
    };
    slider.content     = EContent::None;
    slider.interactive = true;
    r["slider"] = slider;
#endif

#if LV_USE_SWITCH
    SWidget sw;
    sw.create      = [](lv_obj_t* parent, const CLvmlNode&) { return lv_switch_create(parent); };
    sw.content     = EContent::None;
    sw.interactive = true;
    r["switch"] = sw;
#endif

#if LV_USE_TEXTAREA
    SWidget textarea;
    textarea.create = [num](lv_obj_t* parent, const CLvmlNode& n) {
        lv_obj_t* o = lv_textarea_create(parent);
        const std::string type = CTextUtils::ToLower(n.GetAttribute("type"));
        if (n.IsTag("input") || n.GetBoolAttribute("one-line") || n.GetBoolAttribute("oneline")) lv_textarea_set_one_line(o, true);
        if (type == "password" || n.GetBoolAttribute("password")) lv_textarea_set_password_mode(o, true);
        if (const std::string* p = n.FindAttribute("placeholder")) lv_textarea_set_placeholder_text(o, p->c_str());
        if (const std::string* a = n.FindAttribute("accepted-chars")) lv_textarea_set_accepted_chars(o, a->c_str());
        const int32_t maxLen = num(n, "max-length", num(n, "maxlength", 0));
        if (maxLen > 0) lv_textarea_set_max_length(o, uint32_t(maxLen));
        if (const std::string* v = n.FindAttribute("value")) lv_textarea_set_text(o, v->c_str());
        return o;
    };
    textarea.setText     = [](lv_obj_t* o, const std::string& t) { lv_textarea_set_text(o, t.c_str()); };
    textarea.getText     = [](lv_obj_t* o) { return std::string(lv_textarea_get_text(o)); };
    textarea.content     = EContent::Text;
    textarea.interactive = true;
    for (const char* tag : { "textarea", "input" }) r[tag] = textarea;
#endif

#if LV_USE_DROPDOWN
    SWidget dropdown;
    dropdown.create = [num](lv_obj_t* parent, const CLvmlNode& n) {
        lv_obj_t* o = lv_dropdown_create(parent);
        std::string options;
        int32_t selected = -1, index = 0;
        for (const auto& c : n.GetChildren()) {
            if (!c->IsTag("option")) continue;
            if (!options.empty()) options += '\n';
            options += c->GetInnerText();
            if (c->HasAttribute("selected")) selected = index;
            ++index;
        }
        lv_dropdown_set_options(o, options.c_str());
        const int32_t sel = num(n, "selected", selected);
        if (sel >= 0) lv_dropdown_set_selected(o, uint32_t(sel));
        return o;
    };
    dropdown.getText = [](lv_obj_t* o) {
        char buf[256];
        lv_dropdown_get_selected_str(o, buf, sizeof(buf));
        return std::string(buf);
    };
    dropdown.content     = EContent::None;
    dropdown.interactive = true;
    for (const char* tag : { "dropdown", "select" }) r[tag] = dropdown;
#endif

#if LV_USE_SPAN
    SWidget spans;
    spans.create = [](lv_obj_t* parent, const CLvmlNode&) {
        lv_obj_t* o = lv_spangroup_create(parent);
        lv_spangroup_set_mode(o, LV_SPAN_MODE_BREAK);
        return o;
    };
    spans.content = EContent::Spans;
    for (const char* tag : { "richtext", "spangroup", "spans" }) r[tag] = spans;
#endif
}
