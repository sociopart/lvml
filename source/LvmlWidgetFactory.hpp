// lvml — LVGL markup & style sheets.
#pragma once
#include "LvmlNode.hpp"
#include <lvgl.h>
#include <functional>
#include <map>
#include <string>

// Tag → LVGL widget. Built-in tags:
//   obj (div, view, panel, screen, body), label (text), button, image (img), checkbox, bar (progress),
//   slider, switch, textarea (input), dropdown (select), richtext (spangroup)
// Applications can register their own tags.
class CLvmlWidgetFactory
{
public:
    enum class EContent {
        Children, // child elements become child objects; bare text becomes anonymous labels
        Text,     // inner text is the widget's text (label, checkbox, textarea)
        Spans,    // inner elements become spans of an lv_spangroup
        None,     // content ignored
    };

    struct SWidget {
        std::function<lv_obj_t*(lv_obj_t* parent, const CLvmlNode& node)> create;
        // Sets the widget's text (null = not a text widget).
        std::function<void(lv_obj_t* obj, const std::string& text)> setText;
        std::function<std::string(lv_obj_t* obj)>                   getText;
        EContent content     = EContent::Children;
        bool     interactive = false; // clickable even without a handler (button, checkbox…)
        bool     container   = false; // plain container: not scrollable unless overflow says so
    };

    CLvmlWidgetFactory() = delete;

    static void           Register(const std::string& tag, SWidget widget);
    static const SWidget* Find(const std::string& tag);
    static const SWidget& Fallback(); // used for unknown tags (a plain container)

    // Value helpers for bar/slider/switch/checkbox; return false if the widget has no such value.
    static bool SetValue(lv_obj_t* obj, int32_t value, bool animate);
    static bool GetValue(lv_obj_t* obj, int32_t& value);

private:
    static std::map<std::string, SWidget>& Registry();
    static void                             RegisterBuiltins(std::map<std::string, SWidget>& registry);
    static void                             SetLabelText(lv_obj_t* label, const std::string& text);
};
