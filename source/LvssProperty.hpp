// lvml — LVGL markup & style sheets.
#pragma once
#include "LvssDeclaration.hpp"
#include <lvgl.h>
#include <string>
#include <string_view>
#include <vector>

class CLvmlContext;
class CLvmlStorage;
class CLvssStyleSheet;

// Maps .lvss declarations onto LVGL style properties.
//
// Names are LVGL style names with dashes (bg-color, pad-top, text-letter-space…) plus the
// common CSS spellings (background-color, padding-top, letter-spacing, border-radius…) and
// shorthands (padding, margin, gap, border, outline, box-shadow, transition, flex…).
class CLvssProperty
{
public:
    // What kind of declaration a property name is.
    enum class EKind {
        Style,   // compiled into lv_style_t
        Object,  // applied per object: display, overflow, image-fit, long-mode, position, line-height…
        Font,    // font-family / font-size / font — resolved per object with inheritance
        Ignored, // valid CSS without an LVGL meaning (cursor, box-sizing…) — dropped silently
        Unknown,
    };

    // Receiver of compiled values: a shared lv_style_t or local properties of one object.
    class CSink
    {
    public:
        virtual ~CSink() = default;
        virtual void Set(lv_style_prop_t prop, lv_style_value_t value) = 0;
    };

    class CStyleSink : public CSink
    {
    public:
        explicit CStyleSink(lv_style_t* style) : m_style(style) {}
        void Set(lv_style_prop_t prop, lv_style_value_t value) override { lv_style_set_prop(m_style, prop, value); }

    private:
        lv_style_t* m_style;
    };

    class CObjectSink : public CSink
    {
    public:
        CObjectSink(lv_obj_t* obj, lv_style_selector_t selector) : m_obj(obj), m_selector(selector) {}
        void Set(lv_style_prop_t prop, lv_style_value_t value) override
        {
            lv_obj_set_local_style_prop(m_obj, prop, value, m_selector);
        }

    private:
        lv_obj_t*           m_obj;
        lv_style_selector_t m_selector;
    };

    struct SEnv {
        CLvmlContext*                        context  = nullptr;
        CLvmlStorage*                        storage  = nullptr;
        const CLvssStyleSheet*               sheet    = nullptr; // for var()
        std::string                          basePath;           // for relative url()
        const std::vector<CLvssDeclaration>* siblings = nullptr; // the whole block, for longhand merging
    };

    // Property table types (public so the tables can live in the .cpp).
    enum class EType { Coord, Size, Pixels, Int, Color, Opa, Bool, Enum, Flags, Image, Time, Angle, Scale, Radius };

    struct SEnumValue {
        const char* name;
        int32_t     value;
    };

    struct SEntry {
        const char*       names; // '|'-separated aliases
        lv_style_prop_t   prop;
        EType             type;
        lv_style_prop_t   opaProp;     // colors: alpha channel goes here
        int32_t           opaIfOpaque; // colors: value for opaProp when no alpha is given (-1 = leave)
        const SEnumValue* enums;
        size_t            enumCount;
    };

    CLvssProperty() = delete;

    static EKind Classify(std::string_view name);

    // Compiles a Style declaration. Returns false (and fills `error`) if it was not understood.
    static bool Apply(const CLvssDeclaration& decl, const SEnv& env, CSink& sink, std::string* error = nullptr);

    // --- value helpers shared with the scene ---
    // px/dp/rem/em/vw/vh scaled to physical pixels; % → lv_pct(); content/auto → LV_SIZE_CONTENT.
    static bool ToCoord(std::string_view value, const SEnv& env, bool allowPercent, bool allowContent, int32_t& out);
    static bool ToColor(std::string_view value, lv_color_t& color, lv_opa_t& opa, bool& hasAlpha);
    static bool ToImageSource(std::string_view value, const SEnv& env, const char*& out);
    static lv_style_prop_t FindStyleProp(std::string_view name);

private:
    static const SEntry* FindEntry(std::string_view name);
    static bool          NameMatches(const char* names, std::string_view name);
    static bool          ApplyEntry(const SEntry& e, std::string_view value, const SEnv& env, CSink& sink, std::string* error);
    static bool          ApplyShorthand(const std::string& name, const std::string& value, const SEnv& env, CSink& sink,
                                        std::string* error, bool& handled);
    static bool          ApplyBoxShorthand(const std::string& value, const SEnv& env, CSink& sink, lv_style_prop_t top,
                                           lv_style_prop_t right, lv_style_prop_t bottom, lv_style_prop_t left);
    static bool          ApplyBorder(const std::string& value, const SEnv& env, CSink& sink, lv_style_prop_t width,
                                     lv_style_prop_t color, lv_style_prop_t opa, std::string* error);
    static bool          ApplyTransition(const SEnv& env, CSink& sink, std::string* error);
    static bool          ApplyTransform(const std::string& value, const SEnv& env, CSink& sink, std::string* error);
    static bool          ApplyGradient(const std::string& value, const SEnv& env, CSink& sink, std::string* error);
    static std::string   Sibling(const SEnv& env, std::string_view name);
    static lv_anim_path_cb_t ParseEasing(std::string_view name);

    static void SetNum(CSink& sink, lv_style_prop_t prop, int32_t v);
    static void SetColor(CSink& sink, lv_style_prop_t prop, lv_color_t c);

    static const SEntry kEntries[];
    static const size_t kEntryCount;
};
