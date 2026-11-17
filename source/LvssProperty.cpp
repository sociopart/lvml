#include "LvssProperty.hpp"
#include "LvmlContext.hpp"
#include "LvmlFileSystem.hpp"
#include "LvmlStorage.hpp"
#include "LvssStyleSheet.hpp"
#include "LvssValue.hpp"
#include "TextUtils.hpp"
#include <cctype>
#include <cmath>
#include <iterator>

// ---------------------------------------------------------------------------------------------
// enum tables
// ---------------------------------------------------------------------------------------------
#define LVML_ENUM(table) table, sizeof(table) / sizeof(table[0])

static const CLvssProperty::SEnumValue kAlign[] = {
    { "default", LV_ALIGN_DEFAULT },         { "top-left", LV_ALIGN_TOP_LEFT },
    { "top-mid", LV_ALIGN_TOP_MID },         { "top-center", LV_ALIGN_TOP_MID },     { "top", LV_ALIGN_TOP_MID },
    { "top-right", LV_ALIGN_TOP_RIGHT },     { "bottom-left", LV_ALIGN_BOTTOM_LEFT },
    { "bottom-mid", LV_ALIGN_BOTTOM_MID },   { "bottom-center", LV_ALIGN_BOTTOM_MID }, { "bottom", LV_ALIGN_BOTTOM_MID },
    { "bottom-right", LV_ALIGN_BOTTOM_RIGHT }, { "left-mid", LV_ALIGN_LEFT_MID },  { "left-center", LV_ALIGN_LEFT_MID },
    { "left", LV_ALIGN_LEFT_MID },           { "right-mid", LV_ALIGN_RIGHT_MID },    { "right-center", LV_ALIGN_RIGHT_MID },
    { "right", LV_ALIGN_RIGHT_MID },         { "center", LV_ALIGN_CENTER },
    { "out-top-left", LV_ALIGN_OUT_TOP_LEFT }, { "out-top-mid", LV_ALIGN_OUT_TOP_MID }, { "out-top-right", LV_ALIGN_OUT_TOP_RIGHT },
    { "out-bottom-left", LV_ALIGN_OUT_BOTTOM_LEFT }, { "out-bottom-mid", LV_ALIGN_OUT_BOTTOM_MID },
    { "out-bottom-right", LV_ALIGN_OUT_BOTTOM_RIGHT }, { "out-left-top", LV_ALIGN_OUT_LEFT_TOP },
    { "out-left-mid", LV_ALIGN_OUT_LEFT_MID }, { "out-left-bottom", LV_ALIGN_OUT_LEFT_BOTTOM },
    { "out-right-top", LV_ALIGN_OUT_RIGHT_TOP }, { "out-right-mid", LV_ALIGN_OUT_RIGHT_MID },
    { "out-right-bottom", LV_ALIGN_OUT_RIGHT_BOTTOM },
};
static const CLvssProperty::SEnumValue kTextAlign[] = {
    { "auto", LV_TEXT_ALIGN_AUTO }, { "left", LV_TEXT_ALIGN_LEFT },   { "start", LV_TEXT_ALIGN_LEFT },
    { "justify", LV_TEXT_ALIGN_LEFT }, { "center", LV_TEXT_ALIGN_CENTER }, { "right", LV_TEXT_ALIGN_RIGHT },
    { "end", LV_TEXT_ALIGN_RIGHT },
};
static const CLvssProperty::SEnumValue kGradDir[] = {
    { "none", LV_GRAD_DIR_NONE }, { "ver", LV_GRAD_DIR_VER }, { "vertical", LV_GRAD_DIR_VER },
    { "hor", LV_GRAD_DIR_HOR },   { "horizontal", LV_GRAD_DIR_HOR },
};
static const CLvssProperty::SEnumValue kBorderSide[] = {
    { "none", LV_BORDER_SIDE_NONE }, { "bottom", LV_BORDER_SIDE_BOTTOM }, { "top", LV_BORDER_SIDE_TOP },
    { "left", LV_BORDER_SIDE_LEFT }, { "right", LV_BORDER_SIDE_RIGHT },   { "full", LV_BORDER_SIDE_FULL },
    { "all", LV_BORDER_SIDE_FULL },  { "internal", LV_BORDER_SIDE_INTERNAL },
};
static const CLvssProperty::SEnumValue kTextDecor[] = {
    { "none", LV_TEXT_DECOR_NONE }, { "underline", LV_TEXT_DECOR_UNDERLINE },
    { "strikethrough", LV_TEXT_DECOR_STRIKETHROUGH }, { "line-through", LV_TEXT_DECOR_STRIKETHROUGH },
};
static const CLvssProperty::SEnumValue kBlend[] = {
    { "normal", LV_BLEND_MODE_NORMAL },       { "additive", LV_BLEND_MODE_ADDITIVE },
    { "subtractive", LV_BLEND_MODE_SUBTRACTIVE }, { "multiply", LV_BLEND_MODE_MULTIPLY },
    { "difference", LV_BLEND_MODE_DIFFERENCE },
};
static const CLvssProperty::SEnumValue kBaseDir[] = {
    { "ltr", LV_BASE_DIR_LTR }, { "rtl", LV_BASE_DIR_RTL }, { "auto", LV_BASE_DIR_AUTO },
};
static const CLvssProperty::SEnumValue kLayout[] = {
    { "none", LV_LAYOUT_NONE }, { "flex", LV_LAYOUT_FLEX },
#if LV_USE_GRID
    { "grid", LV_LAYOUT_GRID },
#endif
};
static const CLvssProperty::SEnumValue kFlexFlow[] = {
    { "row", LV_FLEX_FLOW_ROW },                   { "column", LV_FLEX_FLOW_COLUMN },
    { "row-wrap", LV_FLEX_FLOW_ROW_WRAP },         { "row-reverse", LV_FLEX_FLOW_ROW_REVERSE },
    { "row-wrap-reverse", LV_FLEX_FLOW_ROW_WRAP_REVERSE }, { "column-wrap", LV_FLEX_FLOW_COLUMN_WRAP },
    { "column-reverse", LV_FLEX_FLOW_COLUMN_REVERSE }, { "column-wrap-reverse", LV_FLEX_FLOW_COLUMN_WRAP_REVERSE },
};
static const CLvssProperty::SEnumValue kFlexAlign[] = {
    { "start", LV_FLEX_ALIGN_START },   { "flex-start", LV_FLEX_ALIGN_START }, { "left", LV_FLEX_ALIGN_START },
    { "top", LV_FLEX_ALIGN_START },     { "normal", LV_FLEX_ALIGN_START },     { "stretch", LV_FLEX_ALIGN_START },
    { "baseline", LV_FLEX_ALIGN_START }, { "end", LV_FLEX_ALIGN_END },         { "flex-end", LV_FLEX_ALIGN_END },
    { "right", LV_FLEX_ALIGN_END },     { "bottom", LV_FLEX_ALIGN_END },       { "center", LV_FLEX_ALIGN_CENTER },
    { "space-evenly", LV_FLEX_ALIGN_SPACE_EVENLY }, { "space-around", LV_FLEX_ALIGN_SPACE_AROUND },
    { "space-between", LV_FLEX_ALIGN_SPACE_BETWEEN },
};

// ---------------------------------------------------------------------------------------------
// property table
// ---------------------------------------------------------------------------------------------
#define P(names, prop, type) { names, prop, CLvssProperty::EType::type, LV_STYLE_PROP_INV, -1, nullptr, 0 }
#define PC(names, prop, opa, ifOpaque) { names, prop, CLvssProperty::EType::Color, opa, ifOpaque, nullptr, 0 }
#define PE(names, prop, type, table) { names, prop, CLvssProperty::EType::type, LV_STYLE_PROP_INV, -1, LVML_ENUM(table) }

const CLvssProperty::SEntry CLvssProperty::kEntries[] = {
    // size & position
    P("width", LV_STYLE_WIDTH, Size),
    P("height", LV_STYLE_HEIGHT, Size),
    P("min-width", LV_STYLE_MIN_WIDTH, Size),
    P("max-width", LV_STYLE_MAX_WIDTH, Size),
    P("min-height", LV_STYLE_MIN_HEIGHT, Size),
    P("max-height", LV_STYLE_MAX_HEIGHT, Size),
    P("x|left", LV_STYLE_X, Coord),
    P("y|top", LV_STYLE_Y, Coord),
    PE("align", LV_STYLE_ALIGN, Enum, kAlign),
    P("transform-width", LV_STYLE_TRANSFORM_WIDTH, Coord),
    P("transform-height", LV_STYLE_TRANSFORM_HEIGHT, Coord),
    P("translate-x", LV_STYLE_TRANSLATE_X, Coord),
    P("translate-y", LV_STYLE_TRANSLATE_Y, Coord),
    P("transform-scale-x|scale-x", LV_STYLE_TRANSFORM_SCALE_X, Scale),
    P("transform-scale-y|scale-y", LV_STYLE_TRANSFORM_SCALE_Y, Scale),
    P("transform-rotation|rotate|rotation", LV_STYLE_TRANSFORM_ROTATION, Angle),
    P("transform-pivot-x", LV_STYLE_TRANSFORM_PIVOT_X, Coord),
    P("transform-pivot-y", LV_STYLE_TRANSFORM_PIVOT_Y, Coord),
    P("transform-skew-x", LV_STYLE_TRANSFORM_SKEW_X, Angle),
    P("transform-skew-y", LV_STYLE_TRANSFORM_SKEW_Y, Angle),

    // padding & margin
    P("pad-top|padding-top", LV_STYLE_PAD_TOP, Pixels),
    P("pad-bottom|padding-bottom", LV_STYLE_PAD_BOTTOM, Pixels),
    P("pad-left|padding-left", LV_STYLE_PAD_LEFT, Pixels),
    P("pad-right|padding-right", LV_STYLE_PAD_RIGHT, Pixels),
    P("pad-row|row-gap", LV_STYLE_PAD_ROW, Pixels),
    P("pad-column|column-gap", LV_STYLE_PAD_COLUMN, Pixels),
    P("margin-top", LV_STYLE_MARGIN_TOP, Pixels),
    P("margin-bottom", LV_STYLE_MARGIN_BOTTOM, Pixels),
    P("margin-left", LV_STYLE_MARGIN_LEFT, Pixels),
    P("margin-right", LV_STYLE_MARGIN_RIGHT, Pixels),

    // background
    PC("bg-color|background-color", LV_STYLE_BG_COLOR, LV_STYLE_BG_OPA, LV_OPA_COVER),
    P("bg-opa|background-opacity", LV_STYLE_BG_OPA, Opa),
    PC("bg-grad-color", LV_STYLE_BG_GRAD_COLOR, LV_STYLE_PROP_INV, -1),
    PE("bg-grad-dir", LV_STYLE_BG_GRAD_DIR, Enum, kGradDir),
    P("bg-main-stop", LV_STYLE_BG_MAIN_STOP, Int),
    P("bg-grad-stop", LV_STYLE_BG_GRAD_STOP, Int),
    P("bg-main-opa", LV_STYLE_BG_MAIN_OPA, Opa),
    P("bg-grad-opa", LV_STYLE_BG_GRAD_OPA, Opa),
    P("bg-image|bg-image-src|background-image", LV_STYLE_BG_IMAGE_SRC, Image),
    P("bg-image-opa", LV_STYLE_BG_IMAGE_OPA, Opa),
    PC("bg-image-recolor", LV_STYLE_BG_IMAGE_RECOLOR, LV_STYLE_BG_IMAGE_RECOLOR_OPA, LV_OPA_COVER),
    P("bg-image-recolor-opa", LV_STYLE_BG_IMAGE_RECOLOR_OPA, Opa),
    P("bg-image-tiled", LV_STYLE_BG_IMAGE_TILED, Bool),

    // border & outline
    P("border-width", LV_STYLE_BORDER_WIDTH, Pixels),
    PC("border-color", LV_STYLE_BORDER_COLOR, LV_STYLE_BORDER_OPA, -1),
    P("border-opa", LV_STYLE_BORDER_OPA, Opa),
    PE("border-side", LV_STYLE_BORDER_SIDE, Flags, kBorderSide),
    P("border-post", LV_STYLE_BORDER_POST, Bool),
    P("outline-width", LV_STYLE_OUTLINE_WIDTH, Pixels),
    PC("outline-color", LV_STYLE_OUTLINE_COLOR, LV_STYLE_OUTLINE_OPA, -1),
    P("outline-opa", LV_STYLE_OUTLINE_OPA, Opa),
    P("outline-pad|outline-offset", LV_STYLE_OUTLINE_PAD, Pixels),
    P("radius|border-radius", LV_STYLE_RADIUS, Radius),
    P("clip-corner", LV_STYLE_CLIP_CORNER, Bool),

    // shadow
    P("shadow-width", LV_STYLE_SHADOW_WIDTH, Pixels),
    P("shadow-offset-x", LV_STYLE_SHADOW_OFFSET_X, Pixels),
    P("shadow-offset-y", LV_STYLE_SHADOW_OFFSET_Y, Pixels),
    P("shadow-spread", LV_STYLE_SHADOW_SPREAD, Pixels),
    PC("shadow-color", LV_STYLE_SHADOW_COLOR, LV_STYLE_SHADOW_OPA, -1),
    P("shadow-opa", LV_STYLE_SHADOW_OPA, Opa),

    // image widget
    P("image-opa", LV_STYLE_IMAGE_OPA, Opa),
    PC("image-recolor|image-color", LV_STYLE_IMAGE_RECOLOR, LV_STYLE_IMAGE_RECOLOR_OPA, LV_OPA_COVER),
    P("image-recolor-opa", LV_STYLE_IMAGE_RECOLOR_OPA, Opa),

    // line & arc
    P("line-width", LV_STYLE_LINE_WIDTH, Pixels),
    P("line-dash-width", LV_STYLE_LINE_DASH_WIDTH, Pixels),
    P("line-dash-gap", LV_STYLE_LINE_DASH_GAP, Pixels),
    P("line-rounded", LV_STYLE_LINE_ROUNDED, Bool),
    PC("line-color", LV_STYLE_LINE_COLOR, LV_STYLE_LINE_OPA, -1),
    P("line-opa", LV_STYLE_LINE_OPA, Opa),
    P("arc-width", LV_STYLE_ARC_WIDTH, Pixels),
    P("arc-rounded", LV_STYLE_ARC_ROUNDED, Bool),
    PC("arc-color", LV_STYLE_ARC_COLOR, LV_STYLE_ARC_OPA, -1),
    P("arc-opa", LV_STYLE_ARC_OPA, Opa),
    P("arc-image|arc-image-src", LV_STYLE_ARC_IMAGE_SRC, Image),

    // text
    PC("text-color|color", LV_STYLE_TEXT_COLOR, LV_STYLE_TEXT_OPA, -1),
    P("text-opa", LV_STYLE_TEXT_OPA, Opa),
    P("text-letter-space|letter-spacing", LV_STYLE_TEXT_LETTER_SPACE, Pixels),
    P("text-line-space|line-space", LV_STYLE_TEXT_LINE_SPACE, Pixels),
    PE("text-align", LV_STYLE_TEXT_ALIGN, Enum, kTextAlign),
    PE("text-decor|text-decoration|text-decoration-line", LV_STYLE_TEXT_DECOR, Flags, kTextDecor),
    P("text-outline-stroke-width", LV_STYLE_TEXT_OUTLINE_STROKE_WIDTH, Pixels),
    PC("text-outline-stroke-color", LV_STYLE_TEXT_OUTLINE_STROKE_COLOR, LV_STYLE_TEXT_OUTLINE_STROKE_OPA, -1),
    P("text-outline-stroke-opa", LV_STYLE_TEXT_OUTLINE_STROKE_OPA, Opa),

    // misc
    P("opa|opacity", LV_STYLE_OPA, Opa),
    P("opa-layered", LV_STYLE_OPA_LAYERED, Opa),
    PC("recolor", LV_STYLE_RECOLOR, LV_STYLE_RECOLOR_OPA, LV_OPA_COVER),
    P("recolor-opa", LV_STYLE_RECOLOR_OPA, Opa),
    P("anim-duration|anim-time", LV_STYLE_ANIM_DURATION, Time),
    PE("blend-mode|mix-blend-mode", LV_STYLE_BLEND_MODE, Enum, kBlend),
    PE("base-dir|direction", LV_STYLE_BASE_DIR, Enum, kBaseDir),
    P("blur-radius", LV_STYLE_BLUR_RADIUS, Pixels),
    P("blur-backdrop", LV_STYLE_BLUR_BACKDROP, Bool),
    P("drop-shadow-radius", LV_STYLE_DROP_SHADOW_RADIUS, Pixels),
    P("drop-shadow-offset-x", LV_STYLE_DROP_SHADOW_OFFSET_X, Pixels),
    P("drop-shadow-offset-y", LV_STYLE_DROP_SHADOW_OFFSET_Y, Pixels),
    PC("drop-shadow-color", LV_STYLE_DROP_SHADOW_COLOR, LV_STYLE_DROP_SHADOW_OPA, -1),
    P("drop-shadow-opa", LV_STYLE_DROP_SHADOW_OPA, Opa),

    // layout
    PE("layout", LV_STYLE_LAYOUT, Enum, kLayout),
    PE("flex-flow", LV_STYLE_FLEX_FLOW, Enum, kFlexFlow),
    PE("flex-main-place|justify-content", LV_STYLE_FLEX_MAIN_PLACE, Enum, kFlexAlign),
    PE("flex-cross-place|align-items", LV_STYLE_FLEX_CROSS_PLACE, Enum, kFlexAlign),
    PE("flex-track-place|align-content", LV_STYLE_FLEX_TRACK_PLACE, Enum, kFlexAlign),
    P("flex-grow", LV_STYLE_FLEX_GROW, Int),
};
const size_t CLvssProperty::kEntryCount = sizeof(kEntries) / sizeof(kEntries[0]);

#undef P
#undef PC
#undef PE

// ---------------------------------------------------------------------------------------------

CLvssProperty::EKind CLvssProperty::Classify(std::string_view name)
{
    static const char* kObject[] = { "display", "overflow", "overflow-x", "overflow-y", "scrollbar-mode",
                                     "image-fit", "object-fit", "long-mode", "text-overflow", "position",
                                     "pointer-events", "line-height", "scroll-snap-x", "scroll-snap-y",
                                     "white-space", "inner-align", "image-align" };
    static const char* kFont[] = { "font", "font-family", "font-size" };
    static const char* kIgnored[] = { "cursor", "user-select", "box-sizing", "font-weight", "font-style",
                                      "z-index", "will-change", "content", "outline-style", "border-style",
                                      "list-style", "list-style-type", "text-transform", "vertical-align",
                                      "resize", "appearance", "-webkit-appearance", "-webkit-user-select",
                                      "-webkit-font-smoothing", "-moz-osx-font-smoothing", "text-rendering",
                                      "backface-visibility", "isolation", "touch-action", "scroll-behavior",
                                      "overscroll-behavior", "font-feature-settings", "word-break",
                                      "overflow-wrap", "word-wrap", "text-shadow", "filter", "backdrop-filter",
                                      "animation", "animation-name", "animation-duration", "object-position",
                                      "background-repeat", "background-position", "background-size",
                                      "scrollbar-width", "scrollbar-color", "flex-shrink", "flex-basis",
                                      "align-self", "order", "grid-template-columns", "grid-template-rows",
                                      "grid-column", "grid-row", "inset", "aspect-ratio", "caret-color",
                                      "text-indent", "transform-origin", "-webkit-user-drag" };
    for (const char* n : kObject) if (name == n) return EKind::Object;
    for (const char* n : kFont) if (name == n) return EKind::Font;
    for (const char* n : kIgnored) if (name == n) return EKind::Ignored;
    if (CTextUtils::StartsWith(name, "-webkit-") || CTextUtils::StartsWith(name, "-moz-") ||
        CTextUtils::StartsWith(name, "-ms-"))
        return EKind::Ignored;

    static const char* kShorthands[] = { "padding", "pad", "pad-all", "margin", "gap", "pad-gap", "border",
                                         "border-top", "border-bottom", "border-left", "border-right", "outline",
                                         "box-shadow", "shadow", "transition", "transition-property",
                                         "transition-duration", "transition-timing-function", "transition-delay",
                                         "background", "flex", "flex-direction", "flex-wrap", "visibility",
                                         "size", "transform", "transform-scale", "scale", "translate", "pad-hor",
                                         "pad-ver", "place-content", "place-items" };
    for (const char* n : kShorthands) if (name == n) return EKind::Style;
    return FindEntry(name) ? EKind::Style : EKind::Unknown;
}

bool CLvssProperty::NameMatches(const char* names, std::string_view name)
{
    const char* start = names;
    for (const char* p = names;; ++p) {
        if (*p == '|' || *p == 0) {
            if (size_t(p - start) == name.size() && std::string_view(start, name.size()) == name) return true;
            if (*p == 0) return false;
            start = p + 1;
        }
    }
}

const CLvssProperty::SEntry* CLvssProperty::FindEntry(std::string_view name)
{
    for (size_t i = 0; i < kEntryCount; ++i)
        if (NameMatches(kEntries[i].names, name)) return &kEntries[i];
    return nullptr;
}

lv_style_prop_t CLvssProperty::FindStyleProp(std::string_view name)
{
    const SEntry* e = FindEntry(name);
    if (e) return e->prop;
    if (name == "image-color") return LV_STYLE_IMAGE_RECOLOR;
    return LV_STYLE_PROP_INV;
}

void CLvssProperty::SetNum(CSink& sink, lv_style_prop_t prop, int32_t v)
{
    lv_style_value_t value;
    value.num = v;
    sink.Set(prop, value);
}

void CLvssProperty::SetColor(CSink& sink, lv_style_prop_t prop, lv_color_t c)
{
    lv_style_value_t value;
    value.color = c;
    sink.Set(prop, value);
}

bool CLvssProperty::ToCoord(std::string_view value, const SEnv& env, bool allowPercent, bool allowContent, int32_t& out)
{
    CLvssValue::SLength len;
    if (!CLvssValue::ParseLength(value, len)) return false;
    const float scale = env.context ? env.context->GetScale() : 1.0f;
    switch (len.unit) {
    case CLvssValue::EUnit::Px:      out = int32_t(std::lround(len.value * scale)); return true;
    case CLvssValue::EUnit::Rem:     out = int32_t(std::lround(len.value * 16.0f * scale)); return true;
    case CLvssValue::EUnit::Em: {
        const float base = env.context ? float(env.context->GetFonts().GetDefaultSize()) : 16.0f;
        out = int32_t(std::lround(len.value * base * scale));
        return true;
    }
    case CLvssValue::EUnit::Percent:
        if (!allowPercent) return false;
        out = lv_pct(int32_t(std::lround(len.value)));
        return true;
    case CLvssValue::EUnit::Vw:
    case CLvssValue::EUnit::Vh: {
        lv_display_t* d = lv_display_get_default();
        const int32_t res = d ? (len.unit == CLvssValue::EUnit::Vw ? lv_display_get_horizontal_resolution(d)
                                                                   : lv_display_get_vertical_resolution(d))
                              : 0;
        out = int32_t(std::lround(len.value * float(res) / 100.0f));
        return true;
    }
    case CLvssValue::EUnit::Content:
        if (!allowContent) return false;
        out = LV_SIZE_CONTENT;
        return true;
    case CLvssValue::EUnit::Auto:
        out = allowContent ? LV_SIZE_CONTENT : 0;
        return true;
    }
    return false;
}

bool CLvssProperty::ToColor(std::string_view value, lv_color_t& color, lv_opa_t& opa, bool& hasAlpha)
{
    uint32_t argb = 0;
    if (!CLvssValue::ParseColor(value, argb, &hasAlpha)) return false;
    color = lv_color_hex(argb & 0xFFFFFF);
    opa   = lv_opa_t(argb >> 24);
    return true;
}

bool CLvssProperty::ToImageSource(std::string_view value, const SEnv& env, const char*& out)
{
    const std::string v = CTextUtils::ToLower(CTextUtils::Trim(value));
    if (v == "none" || v.empty()) { out = nullptr; return true; }
    std::string url;
    if (!CLvssValue::ParseUrl(value, url)) return false;
    std::string full;
    if (url.size() > 2 && url[1] == ':' && std::isalpha((unsigned char)url[0]) && url.find("://") == std::string::npos)
        full = url; // already an LVGL drive path ("A:/x.png")
    else
        full = CLvmlFileSystem::MakePath(CLvmlResources::ResolvePath(env.basePath, url));
    out = env.storage ? env.storage->Intern(full) : nullptr;
    return out != nullptr;
}

lv_anim_path_cb_t CLvssProperty::ParseEasing(std::string_view name)
{
    const std::string n = CTextUtils::ToLower(name);
    if (n == "linear") return lv_anim_path_linear;
    if (n == "ease-in") return lv_anim_path_ease_in;
    if (n == "ease-out" || n == "ease") return lv_anim_path_ease_out;
    if (n == "ease-in-out") return lv_anim_path_ease_in_out;
    if (n == "overshoot") return lv_anim_path_overshoot;
    if (n == "bounce") return lv_anim_path_bounce;
    if (n == "step" || n == "step-end" || n == "step-start") return lv_anim_path_step;
    return nullptr;
}

std::string CLvssProperty::Sibling(const SEnv& env, std::string_view name)
{
    std::string out;
    if (env.siblings)
        for (const CLvssDeclaration& d : *env.siblings)
            if (d.property == name) out = d.value;
    if (env.sheet) out = env.sheet->ResolveVariables(out);
    return out;
}

bool CLvssProperty::ApplyEntry(const SEntry& e, std::string_view rawValue, const SEnv& env, CSink& sink, std::string* error)
{
    const std::string value(CTextUtils::Trim(rawValue));
    const std::string lower = CTextUtils::ToLower(value);
    auto fail = [&](const char* what) {
        if (error) *error = std::string("bad ") + what + " '" + value + "'";
        return false;
    };

    switch (e.type) {
    case EType::Coord:
    case EType::Size:
    case EType::Pixels: {
        int32_t v = 0;
        if (!ToCoord(value, env, e.type != EType::Pixels, e.type == EType::Size, v)) return fail("length");
        SetNum(sink, e.prop, v);
        return true;
    }
    case EType::Int: {
        float v = 0;
        if (!CTextUtils::ParseFloat(value, v)) return fail("number");
        SetNum(sink, e.prop, int32_t(std::lround(v)));
        return true;
    }
    case EType::Color: {
        lv_color_t c;
        lv_opa_t opa;
        bool hasAlpha = false;
        if (!ToColor(value, c, opa, hasAlpha)) return fail("color");
        SetColor(sink, e.prop, c);
        if (e.opaProp != LV_STYLE_PROP_INV) {
            if (hasAlpha) SetNum(sink, e.opaProp, opa);
            else if (e.opaIfOpaque >= 0) SetNum(sink, e.opaProp, e.opaIfOpaque);
        }
        return true;
    }
    case EType::Opa: {
        uint8_t o = 0;
        if (!CLvssValue::ParseOpacity(value, o)) return fail("opacity");
        SetNum(sink, e.prop, o);
        return true;
    }
    case EType::Bool: {
        if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") { SetNum(sink, e.prop, 1); return true; }
        if (lower == "false" || lower == "0" || lower == "no" || lower == "off") { SetNum(sink, e.prop, 0); return true; }
        return fail("boolean");
    }
    case EType::Enum:
    case EType::Flags: {
        int32_t result = 0;
        const std::vector<std::string> words = e.type == EType::Flags ? CTextUtils::SplitWhitespace(lower)
                                                                      : std::vector<std::string>{ lower };
        for (const std::string& w : words) {
            bool found = false;
            for (size_t i = 0; i < e.enumCount; ++i)
                if (w == e.enums[i].name) { result |= e.enums[i].value; found = true; break; }
            if (!found) return fail("keyword");
        }
        SetNum(sink, e.prop, result);
        return true;
    }
    case EType::Image: {
        if (CTextUtils::StartsWith(lower, "linear-gradient")) return ApplyGradient(value, env, sink, error);
        const char* src = nullptr;
        if (!ToImageSource(value, env, src)) return fail("image");
        lv_style_value_t v;
        v.ptr = src;
        sink.Set(e.prop, v);
        return true;
    }
    case EType::Time: {
        uint32_t ms = 0;
        if (!CLvssValue::ParseTime(value, ms)) return fail("time");
        SetNum(sink, e.prop, int32_t(ms));
        return true;
    }
    case EType::Angle: {
        int32_t a = 0;
        if (!CLvssValue::ParseAngle(value, a)) return fail("angle");
        SetNum(sink, e.prop, a);
        return true;
    }
    case EType::Scale: {
        float v = 0;
        const bool pct = CTextUtils::EndsWith(lower, "%");
        if (!CTextUtils::ParseFloat(pct ? lower.substr(0, lower.size() - 1) : lower, v)) return fail("scale");
        if (pct) v /= 100.0f;
        SetNum(sink, e.prop, int32_t(std::lround(v * 256.0f)));
        return true;
    }
    case EType::Radius: {
        if (lower == "circle" || lower == "round") { SetNum(sink, e.prop, LV_RADIUS_CIRCLE); return true; }
        // "8px 8px 0 0" — LVGL has one radius: take the largest.
        int32_t best = 0;
        for (const std::string& part : CTextUtils::SplitWhitespace(CTextUtils::Replace(lower, "/", " "))) {
            CLvssValue::SLength len;
            if (!CLvssValue::ParseLength(part, len)) return fail("radius");
            if (len.unit == CLvssValue::EUnit::Percent) {
                if (len.value >= 50.0f) { best = LV_RADIUS_CIRCLE; break; }
                continue;
            }
            int32_t v = 0;
            if (!ToCoord(part, env, false, false, v)) return fail("radius");
            if (v > best) best = v;
        }
        SetNum(sink, e.prop, best);
        return true;
    }
    }
    return false;
}

bool CLvssProperty::ApplyBoxShorthand(const std::string& value, const SEnv& env, CSink& sink, lv_style_prop_t top,
                                      lv_style_prop_t right, lv_style_prop_t bottom, lv_style_prop_t left)
{
    const std::vector<std::string> parts = CTextUtils::SplitWhitespace(value);
    if (parts.empty() || parts.size() > 4) return false;
    int32_t v[4] = {};
    for (size_t i = 0; i < parts.size(); ++i)
        if (!ToCoord(parts[i], env, false, false, v[i])) return false;
    int32_t t = v[0], r = v[0], b = v[0], l = v[0];
    if (parts.size() == 2) { r = l = v[1]; }
    if (parts.size() == 3) { r = l = v[1]; b = v[2]; }
    if (parts.size() == 4) { r = v[1]; b = v[2]; l = v[3]; }
    if (top != LV_STYLE_PROP_INV) SetNum(sink, top, t);
    if (right != LV_STYLE_PROP_INV) SetNum(sink, right, r);
    if (bottom != LV_STYLE_PROP_INV) SetNum(sink, bottom, b);
    if (left != LV_STYLE_PROP_INV) SetNum(sink, left, l);
    return true;
}

bool CLvssProperty::ApplyBorder(const std::string& value, const SEnv& env, CSink& sink, lv_style_prop_t width,
                                lv_style_prop_t color, lv_style_prop_t opa, std::string* error)
{
    const std::string lower = CTextUtils::ToLower(CTextUtils::Trim(value));
    if (lower == "none" || lower == "0") { SetNum(sink, width, 0); return true; }
    static const char* kStyles[] = { "solid", "dashed", "dotted", "double", "groove", "ridge", "inset", "outset", "hidden" };
    for (const std::string& part : CTextUtils::SplitWhitespace(value)) {
        bool isStyle = false;
        for (const char* s : kStyles) if (CTextUtils::IEquals(part, s)) isStyle = true;
        if (isStyle) continue;
        int32_t w = 0;
        lv_color_t c;
        lv_opa_t o;
        bool alpha = false;
        if (ToCoord(part, env, false, false, w)) SetNum(sink, width, w);
        else if (ToColor(part, c, o, alpha)) {
            SetColor(sink, color, c);
            SetNum(sink, opa, alpha ? o : LV_OPA_COVER);
        } else {
            if (error) *error = "bad border part '" + part + "'";
            return false;
        }
    }
    return true;
}

bool CLvssProperty::ApplyGradient(const std::string& value, const SEnv& env, CSink& sink, std::string* error)
{
    (void)env;
    // linear-gradient([to bottom|to right|<angle>,] color [stop%], color [stop%])
    const size_t open = value.find('('), close = value.rfind(')');
    if (open == std::string::npos || close == std::string::npos) { if (error) *error = "bad gradient"; return false; }
    std::vector<std::string> args = CTextUtils::SplitTopLevel(value.substr(open + 1, close - open - 1), ',');
    int32_t dir = LV_GRAD_DIR_VER;
    if (!args.empty()) {
        const std::string first = CTextUtils::ToLower(args[0]);
        int32_t angle = 0;
        if (CTextUtils::StartsWith(first, "to ")) {
            dir = (first.find("right") != std::string::npos || first.find("left") != std::string::npos) ? LV_GRAD_DIR_HOR : LV_GRAD_DIR_VER;
            args.erase(args.begin());
        } else if (CLvssValue::ParseAngle(first, angle) && first.find("deg") != std::string::npos) {
            const int32_t deg = ((angle / 10) % 360 + 360) % 360;
            dir = (deg == 90 || deg == 270) ? LV_GRAD_DIR_HOR : LV_GRAD_DIR_VER;
            args.erase(args.begin());
        }
    }
    if (args.size() < 2) { if (error) *error = "gradient needs two colors"; return false; }
    auto stop = [&](const std::string& arg, lv_color_t& c, lv_opa_t& o, int32_t& pos, int32_t def) {
        const std::vector<std::string> parts = CTextUtils::SplitWhitespace(arg);
        bool alpha = false;
        if (parts.empty() || !ToColor(parts[0], c, o, alpha)) return false;
        pos = def;
        CLvssValue::SLength len;
        if (parts.size() > 1 && CLvssValue::ParseLength(parts[1], len) && len.unit == CLvssValue::EUnit::Percent)
            pos = int32_t(std::lround(len.value * 255.0f / 100.0f));
        return true;
    };
    lv_color_t c1, c2;
    lv_opa_t o1, o2;
    int32_t s1, s2;
    if (!stop(args.front(), c1, o1, s1, 0) || !stop(args.back(), c2, o2, s2, 255)) {
        if (error) *error = "bad gradient color";
        return false;
    }
    SetColor(sink, LV_STYLE_BG_COLOR, c1);
    SetColor(sink, LV_STYLE_BG_GRAD_COLOR, c2);
    SetNum(sink, LV_STYLE_BG_GRAD_DIR, dir);
    SetNum(sink, LV_STYLE_BG_MAIN_STOP, s1);
    SetNum(sink, LV_STYLE_BG_GRAD_STOP, s2);
    SetNum(sink, LV_STYLE_BG_MAIN_OPA, o1);
    SetNum(sink, LV_STYLE_BG_GRAD_OPA, o2);
    SetNum(sink, LV_STYLE_BG_OPA, LV_OPA_COVER);
    return true;
}

bool CLvssProperty::ApplyTransition(const SEnv& env, CSink& sink, std::string* error)
{
    static const lv_style_prop_t kAll[] = {
        LV_STYLE_BG_COLOR, LV_STYLE_BG_OPA, LV_STYLE_BG_IMAGE_RECOLOR, LV_STYLE_BG_IMAGE_RECOLOR_OPA,
        LV_STYLE_BG_IMAGE_OPA, LV_STYLE_BORDER_COLOR, LV_STYLE_BORDER_OPA, LV_STYLE_TEXT_COLOR, LV_STYLE_TEXT_OPA,
        LV_STYLE_IMAGE_RECOLOR, LV_STYLE_IMAGE_RECOLOR_OPA, LV_STYLE_IMAGE_OPA, LV_STYLE_OPA, LV_STYLE_SHADOW_COLOR,
        LV_STYLE_SHADOW_OPA, LV_STYLE_SHADOW_WIDTH, LV_STYLE_TRANSLATE_X, LV_STYLE_TRANSLATE_Y,
        LV_STYLE_TRANSFORM_SCALE_X, LV_STYLE_TRANSFORM_SCALE_Y, LV_STYLE_TRANSFORM_ROTATION, LV_STYLE_OUTLINE_COLOR,
        LV_STYLE_OUTLINE_OPA, LV_STYLE_RECOLOR, LV_STYLE_RECOLOR_OPA,
    };

    std::vector<lv_style_prop_t> props;
    uint32_t time = 0, delay = 0;
    bool haveTime = false;
    lv_anim_path_cb_t path = nullptr;

    auto addProp = [&](const std::string& rawName) {
        const std::string name = CTextUtils::ToLower(rawName);
        if (name == "all" || name == "none") {
            if (name == "all") props.insert(props.end(), std::begin(kAll), std::end(kAll));
            return true;
        }
        // CSS / lvss name → the props it touches.
        if (name == "background" || name == "background-color" || name == "bg-color") {
            props.push_back(LV_STYLE_BG_COLOR); props.push_back(LV_STYLE_BG_OPA); return true;
        }
        if (name == "image-color" || name == "image-recolor" || name == "filter") {
            props.push_back(LV_STYLE_IMAGE_RECOLOR); props.push_back(LV_STYLE_IMAGE_RECOLOR_OPA);
            props.push_back(LV_STYLE_BG_IMAGE_RECOLOR); props.push_back(LV_STYLE_BG_IMAGE_RECOLOR_OPA);
            return true;
        }
        if (name == "transform") {
            props.push_back(LV_STYLE_TRANSFORM_SCALE_X); props.push_back(LV_STYLE_TRANSFORM_SCALE_Y);
            props.push_back(LV_STYLE_TRANSLATE_X); props.push_back(LV_STYLE_TRANSLATE_Y);
            props.push_back(LV_STYLE_TRANSFORM_ROTATION);
            return true;
        }
        if (name == "box-shadow") { props.push_back(LV_STYLE_SHADOW_COLOR); props.push_back(LV_STYLE_SHADOW_OPA); props.push_back(LV_STYLE_SHADOW_WIDTH); return true; }
        if (name == "border") { props.push_back(LV_STYLE_BORDER_COLOR); props.push_back(LV_STYLE_BORDER_OPA); props.push_back(LV_STYLE_BORDER_WIDTH); return true; }
        const lv_style_prop_t p = FindStyleProp(name);
        if (p == LV_STYLE_PROP_INV) return false;
        props.push_back(p);
        if (const SEntry* e = FindEntry(name); e && e->opaProp != LV_STYLE_PROP_INV) props.push_back(e->opaProp);
        return true;
    };

    const std::string shorthand = Sibling(env, "transition");
    for (const std::string& item : CTextUtils::SplitTopLevel(shorthand, ',')) {
        int timesSeen = 0;
        for (const std::string& word : CTextUtils::SplitWhitespace(item)) {
            uint32_t ms = 0;
            const bool isTime = (CTextUtils::EndsWith(word, "s")) && CLvssValue::ParseTime(word, ms);
            if (isTime) {
                if (timesSeen++ == 0) { if (!haveTime) { time = ms; haveTime = true; } }
                else delay = ms;
                continue;
            }
            if (lv_anim_path_cb_t p = ParseEasing(word)) { if (!path) path = p; continue; }
            if (CTextUtils::StartsWith(word, "cubic-bezier")) { if (!path) path = lv_anim_path_ease_in_out; continue; }
            if (!addProp(word) && error) *error = "unknown transition property '" + word + "'";
        }
        if (timesSeen && props.empty() && CTextUtils::SplitWhitespace(item).size() <= 3)
            props.insert(props.end(), std::begin(kAll), std::end(kAll)); // "transition: 0.15s"
    }
    for (const std::string& name : CTextUtils::SplitTopLevel(Sibling(env, "transition-property"), ',')) addProp(name);
    if (const std::string d = Sibling(env, "transition-duration"); !d.empty()) {
        uint32_t ms = 0;
        if (CLvssValue::ParseTime(CTextUtils::SplitTopLevel(d, ',').front(), ms)) { time = ms; haveTime = true; }
    }
    if (const std::string d = Sibling(env, "transition-delay"); !d.empty()) {
        uint32_t ms = 0;
        if (CLvssValue::ParseTime(CTextUtils::SplitTopLevel(d, ',').front(), ms)) delay = ms;
    }
    if (const std::string f = Sibling(env, "transition-timing-function"); !f.empty())
        if (lv_anim_path_cb_t p = ParseEasing(CTextUtils::SplitTopLevel(f, ',').front())) path = p;

    if (!haveTime || time == 0) return true; // "transition: none" or no duration: nothing to animate
    if (props.empty()) props.insert(props.end(), std::begin(kAll), std::end(kAll));
    if (!env.storage) return false;
    lv_style_value_t v;
    v.ptr = env.storage->NewTransition(std::move(props), path ? path : lv_anim_path_linear, time, delay);
    sink.Set(LV_STYLE_TRANSITION, v);
    return true;
}

bool CLvssProperty::ApplyTransform(const std::string& value, const SEnv& env, CSink& sink, std::string* error)
{
    if (CTextUtils::IEquals(CTextUtils::Trim(value), "none")) {
        SetNum(sink, LV_STYLE_TRANSFORM_SCALE_X, LV_SCALE_NONE);
        SetNum(sink, LV_STYLE_TRANSFORM_SCALE_Y, LV_SCALE_NONE);
        SetNum(sink, LV_STYLE_TRANSLATE_X, 0);
        SetNum(sink, LV_STYLE_TRANSLATE_Y, 0);
        SetNum(sink, LV_STYLE_TRANSFORM_ROTATION, 0);
        return true;
    }
    for (const std::string& fn : CTextUtils::SplitWhitespace(value)) {
        const size_t open = fn.find('('), close = fn.rfind(')');
        if (open == std::string::npos || close == std::string::npos) { if (error) *error = "bad transform"; return false; }
        const std::string name = CTextUtils::ToLower(fn.substr(0, open));
        const std::vector<std::string> args = CTextUtils::SplitTopLevel(fn.substr(open + 1, close - open - 1), ',');
        if (args.empty()) continue;
        auto coord = [&](const std::string& a, lv_style_prop_t p) {
            int32_t v = 0;
            if (ToCoord(a, env, true, false, v)) SetNum(sink, p, v);
        };
        auto scale = [&](const std::string& a, lv_style_prop_t p) {
            float v = 1;
            if (CTextUtils::ParseFloat(a, v)) SetNum(sink, p, int32_t(std::lround(v * 256.0f)));
        };
        if (name == "translate") { coord(args[0], LV_STYLE_TRANSLATE_X); if (args.size() > 1) coord(args[1], LV_STYLE_TRANSLATE_Y); }
        else if (name == "translatex") coord(args[0], LV_STYLE_TRANSLATE_X);
        else if (name == "translatey") coord(args[0], LV_STYLE_TRANSLATE_Y);
        else if (name == "scale") { scale(args[0], LV_STYLE_TRANSFORM_SCALE_X); scale(args.size() > 1 ? args[1] : args[0], LV_STYLE_TRANSFORM_SCALE_Y); }
        else if (name == "scalex") scale(args[0], LV_STYLE_TRANSFORM_SCALE_X);
        else if (name == "scaley") scale(args[0], LV_STYLE_TRANSFORM_SCALE_Y);
        else if (name == "rotate") {
            int32_t a = 0;
            if (CLvssValue::ParseAngle(args[0], a)) SetNum(sink, LV_STYLE_TRANSFORM_ROTATION, a);
        } else if (error) *error = "transform function " + name + "() is not supported";
    }
    return true;
}

bool CLvssProperty::ApplyShorthand(const std::string& name, const std::string& value, const SEnv& env, CSink& sink,
                                   std::string* error, bool& handled)
{
    handled = true;
    const std::string lower = CTextUtils::ToLower(CTextUtils::Trim(value));

    if (name == "padding" || name == "pad" || name == "pad-all")
        return ApplyBoxShorthand(value, env, sink, LV_STYLE_PAD_TOP, LV_STYLE_PAD_RIGHT, LV_STYLE_PAD_BOTTOM, LV_STYLE_PAD_LEFT);
    if (name == "pad-hor") return ApplyBoxShorthand("0 " + value, env, sink, LV_STYLE_PROP_INV, LV_STYLE_PAD_RIGHT, LV_STYLE_PROP_INV, LV_STYLE_PAD_LEFT);
    if (name == "pad-ver") return ApplyBoxShorthand(value + " 0", env, sink, LV_STYLE_PAD_TOP, LV_STYLE_PROP_INV, LV_STYLE_PAD_BOTTOM, LV_STYLE_PROP_INV);
    if (name == "margin") {
        // "0 auto" centring has no LVGL equivalent: auto becomes 0.
        return ApplyBoxShorthand(CTextUtils::Replace(lower, "auto", "0"), env, sink, LV_STYLE_MARGIN_TOP, LV_STYLE_MARGIN_RIGHT,
                                 LV_STYLE_MARGIN_BOTTOM, LV_STYLE_MARGIN_LEFT);
    }
    if (name == "gap" || name == "pad-gap") {
        const std::vector<std::string> parts = CTextUtils::SplitWhitespace(value);
        int32_t row = 0, col = 0;
        if (parts.empty() || !ToCoord(parts[0], env, false, false, row)) return false;
        col = row;
        if (parts.size() > 1 && !ToCoord(parts[1], env, false, false, col)) return false;
        SetNum(sink, LV_STYLE_PAD_ROW, row);
        SetNum(sink, LV_STYLE_PAD_COLUMN, col);
        return true;
    }
    if (name == "size") {
        const std::vector<std::string> parts = CTextUtils::SplitWhitespace(value);
        int32_t w = 0, h = 0;
        if (parts.empty() || !ToCoord(parts[0], env, true, true, w)) return false;
        h = w;
        if (parts.size() > 1 && !ToCoord(parts[1], env, true, true, h)) return false;
        SetNum(sink, LV_STYLE_WIDTH, w);
        SetNum(sink, LV_STYLE_HEIGHT, h);
        return true;
    }
    if (name == "border") return ApplyBorder(value, env, sink, LV_STYLE_BORDER_WIDTH, LV_STYLE_BORDER_COLOR, LV_STYLE_BORDER_OPA, error);
    if (name == "border-top" || name == "border-bottom" || name == "border-left" || name == "border-right") {
        if (!ApplyBorder(value, env, sink, LV_STYLE_BORDER_WIDTH, LV_STYLE_BORDER_COLOR, LV_STYLE_BORDER_OPA, error)) return false;
        // All one-sided borders of the block are OR-ed together.
        int32_t side = 0;
        for (const char* s : { "border-top", "border-bottom", "border-left", "border-right" }) {
            const std::string v = CTextUtils::ToLower(Sibling(env, s));
            if (v.empty() || v == "none" || v == "0") continue;
            side |= std::string(s) == "border-top" ? LV_BORDER_SIDE_TOP : std::string(s) == "border-bottom" ? LV_BORDER_SIDE_BOTTOM
                  : std::string(s) == "border-left" ? LV_BORDER_SIDE_LEFT : LV_BORDER_SIDE_RIGHT;
        }
        SetNum(sink, LV_STYLE_BORDER_SIDE, side);
        return true;
    }
    if (name == "outline") return ApplyBorder(value, env, sink, LV_STYLE_OUTLINE_WIDTH, LV_STYLE_OUTLINE_COLOR, LV_STYLE_OUTLINE_OPA, error);
    if (name == "box-shadow" || name == "shadow") {
        if (lower == "none") { SetNum(sink, LV_STYLE_SHADOW_WIDTH, 0); return true; }
        // First shadow only: [inset] x y [blur [spread]] color
        const std::string first = CTextUtils::SplitTopLevel(value, ',').front();
        std::vector<int32_t> lengths;
        for (const std::string& part : CTextUtils::SplitWhitespace(first)) {
            if (CTextUtils::IEquals(part, "inset")) continue;
            int32_t v = 0;
            lv_color_t c;
            lv_opa_t o;
            bool alpha = false;
            if (ToCoord(part, env, false, false, v)) lengths.push_back(v);
            else if (ToColor(part, c, o, alpha)) {
                SetColor(sink, LV_STYLE_SHADOW_COLOR, c);
                SetNum(sink, LV_STYLE_SHADOW_OPA, alpha ? o : LV_OPA_COVER);
            } else { if (error) *error = "bad box-shadow part '" + part + "'"; return false; }
        }
        if (lengths.size() > 0) SetNum(sink, LV_STYLE_SHADOW_OFFSET_X, lengths[0]);
        if (lengths.size() > 1) SetNum(sink, LV_STYLE_SHADOW_OFFSET_Y, lengths[1]);
        SetNum(sink, LV_STYLE_SHADOW_WIDTH, lengths.size() > 2 ? lengths[2] : 0);
        if (lengths.size() > 3) SetNum(sink, LV_STYLE_SHADOW_SPREAD, lengths[3]);
        return true;
    }
    if (name == "transition" || name == "transition-property" || name == "transition-duration" ||
        name == "transition-timing-function" || name == "transition-delay") {
        // Compiled once from the whole block, at the first transition* declaration.
        if (env.siblings) {
            for (const CLvssDeclaration& d : *env.siblings) {
                if (!CTextUtils::StartsWith(d.property, "transition")) continue;
                if (d.property != name) return true;
                break;
            }
        }
        return ApplyTransition(env, sink, error);
    }
    if (name == "background") {
        // background: <color> | url(x) | linear-gradient(...) | none — simple single-layer forms.
        if (lower == "none" || lower == "transparent") { SetNum(sink, LV_STYLE_BG_OPA, LV_OPA_TRANSP); return true; }
        if (CTextUtils::StartsWith(lower, "linear-gradient")) return ApplyGradient(value, env, sink, error);
        bool any = false;
        for (const std::string& part : CTextUtils::SplitWhitespace(value)) {
            lv_color_t c;
            lv_opa_t o;
            bool alpha = false;
            const std::string pl = CTextUtils::ToLower(part);
            if (ToColor(part, c, o, alpha)) {
                SetColor(sink, LV_STYLE_BG_COLOR, c);
                SetNum(sink, LV_STYLE_BG_OPA, alpha ? o : LV_OPA_COVER);
                any = true;
            } else if (CTextUtils::StartsWith(pl, "url(")) {
                const char* src = nullptr;
                if (ToImageSource(part, env, src)) {
                    lv_style_value_t v;
                    v.ptr = src;
                    sink.Set(LV_STYLE_BG_IMAGE_SRC, v);
                    any = true;
                }
            }
        }
        if (!any && error) *error = "unsupported background '" + value + "'";
        return any;
    }
    if (name == "flex") {
        // flex: <grow> [shrink] [basis] | none | auto
        if (lower == "none") { SetNum(sink, LV_STYLE_FLEX_GROW, 0); return true; }
        if (lower == "auto") { SetNum(sink, LV_STYLE_FLEX_GROW, 1); return true; }
        float g = 0;
        if (!CTextUtils::ParseFloat(CTextUtils::SplitWhitespace(lower).front(), g)) return false;
        SetNum(sink, LV_STYLE_FLEX_GROW, int32_t(std::lround(g)));
        return true;
    }
    if (name == "flex-direction" || name == "flex-wrap") {
        const std::string dir  = CTextUtils::ToLower(Sibling(env, "flex-direction"));
        const std::string wrap = CTextUtils::ToLower(Sibling(env, "flex-wrap"));
        int32_t flow = (dir == "column" || dir == "column-reverse") ? LV_FLEX_FLOW_COLUMN : LV_FLEX_FLOW_ROW;
        if (CTextUtils::EndsWith(dir, "-reverse")) flow |= LV_FLEX_REVERSE;
        if (wrap == "wrap" || wrap == "wrap-reverse") flow |= LV_FLEX_WRAP;
        SetNum(sink, LV_STYLE_FLEX_FLOW, flow);
        return true;
    }
    if (name == "place-content" || name == "place-items") {
        const SEntry* e = FindEntry(name == "place-content" ? "justify-content" : "align-items");
        const std::string first = CTextUtils::SplitWhitespace(value).front();
        if (!ApplyEntry(*e, first, env, sink, error)) return false;
        if (name == "place-content") return ApplyEntry(*FindEntry("align-content"), first, env, sink, error);
        return true;
    }
    if (name == "visibility") {
        SetNum(sink, LV_STYLE_OPA, lower == "hidden" || lower == "collapse" ? LV_OPA_TRANSP : LV_OPA_COVER);
        return true;
    }
    if (name == "transform") return ApplyTransform(value, env, sink, error);
    if (name == "transform-scale" || name == "scale") {
        const std::vector<std::string> parts = CTextUtils::SplitWhitespace(value);
        if (!ApplyEntry(*FindEntry("transform-scale-x"), parts[0], env, sink, error)) return false;
        return ApplyEntry(*FindEntry("transform-scale-y"), parts.size() > 1 ? parts[1] : parts[0], env, sink, error);
    }
    if (name == "translate") {
        const std::vector<std::string> parts = CTextUtils::SplitWhitespace(value);
        if (!ApplyEntry(*FindEntry("translate-x"), parts[0], env, sink, error)) return false;
        return parts.size() < 2 || ApplyEntry(*FindEntry("translate-y"), parts[1], env, sink, error);
    }
    if (name == "display") {
        // Only the layout part; hidden/visible is an object flag.
        if (lower == "flex" || lower == "inline-flex") SetNum(sink, LV_STYLE_LAYOUT, LV_LAYOUT_FLEX);
#if LV_USE_GRID
        else if (lower == "grid" || lower == "inline-grid") SetNum(sink, LV_STYLE_LAYOUT, LV_LAYOUT_GRID);
#endif
        return true;
    }
    handled = false;
    return false;
}

bool CLvssProperty::Apply(const CLvssDeclaration& decl, const SEnv& env, CSink& sink, std::string* error)
{
    const std::string value = env.sheet ? env.sheet->ResolveVariables(decl.value) : decl.value;

    bool handled = false;
    const bool ok = ApplyShorthand(decl.property, value, env, sink, error, handled);
    if (handled) {
        if (!ok && error && error->empty()) *error = "bad value '" + value + "'";
        return ok;
    }
    if (const SEntry* e = FindEntry(decl.property)) return ApplyEntry(*e, value, env, sink, error);
    if (error) *error = "unknown property";
    return false;
}
