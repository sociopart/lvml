#include "CssConverter.hpp"
#include "LvssValue.hpp"
#include "TextUtils.hpp"

// ---------------------------------------------------------------------------------------------
// declarations
// ---------------------------------------------------------------------------------------------

bool CCssConverter::IsPassThrough(const std::string& p)
{
    static const char* kNames[] = {
        "width", "height", "min-width", "max-width", "min-height", "max-height",
        "padding", "padding-top", "padding-bottom", "padding-left", "padding-right",
        "margin", "margin-top", "margin-bottom", "margin-left", "margin-right",
        "gap", "row-gap", "column-gap",
        "background", "background-color", "background-image",
        "color", "opacity", "visibility", "mix-blend-mode",
        "border", "border-width", "border-color", "border-radius", "border-top", "border-bottom", "border-left",
        "border-right", "outline", "outline-width", "outline-color", "outline-offset", "box-shadow",
        "transition", "transition-property", "transition-duration", "transition-timing-function", "transition-delay",
        "transform", "translate", "scale", "rotate",
        "letter-spacing", "line-height", "text-align", "text-decoration", "text-decoration-line", "text-overflow",
        "font", "font-family", "font-size",
        "flex", "flex-direction", "flex-wrap", "flex-flow", "flex-grow", "justify-content", "align-items",
        "align-content", "place-content", "place-items",
        "overflow", "overflow-x", "overflow-y", "pointer-events", "object-fit",
    };
    for (const char* n : kNames) if (p == n) return true;
    return false;
}

bool CCssConverter::IsSilentDrop(const std::string& p)
{
    static const char* kNames[] = { "cursor", "user-select", "box-sizing", "font-weight", "font-style", "outline-style",
                                    "border-style", "list-style", "list-style-type", "resize", "appearance",
                                    "text-rendering", "will-change", "content", "isolation", "touch-action",
                                    "scroll-behavior", "overscroll-behavior", "font-feature-settings", "caret-color",
                                    "-webkit-tap-highlight-color", "backface-visibility", "flex-shrink", "flex-basis" };
    for (const char* n : kNames) if (p == n) return true;
    return CTextUtils::StartsWith(p, "-webkit-") || CTextUtils::StartsWith(p, "-moz-") || CTextUtils::StartsWith(p, "-ms-");
}

std::string CCssConverter::RewriteAssetUrl(const std::string& url, CConversionReport& report)
{
    if (CTextUtils::StartsWith(url, "data:")) {
        report.Drop("data: URL", "inline images are not supported — extract them to files");
        return std::string();
    }
    std::string clean = url;
    const size_t q = clean.find_first_of("?#");
    if (q != std::string::npos) clean = clean.substr(0, q);
    const std::string lower = CTextUtils::ToLower(clean);
    for (const char* ext : { ".svg", ".webp", ".gif", ".jpeg", ".jpg", ".avif" }) {
        if (!CTextUtils::EndsWith(lower, ext)) continue;
        // JPEG needs LV_USE_TJPGD/LIBJPEG; the default lvml config decodes PNG only.
        const std::string png = clean.substr(0, clean.size() - std::char_traits<char>::length(ext)) + ".png";
        report.Asset(clean, png);
        return png;
    }
    return clean;
}

std::string CCssConverter::RewriteUrls(const std::string& value, CConversionReport& report)
{
    std::string out;
    size_t pos = 0;
    const std::string lower = CTextUtils::ToLower(value);
    for (;;) {
        const size_t at = lower.find("url(", pos);
        if (at == std::string::npos) { out += value.substr(pos); break; }
        // url( "…" ) — quoted contents may contain ')' (e.g. data: SVGs), so honour the quotes.
        size_t i = at + 4;
        while (i < value.size() && CTextUtils::IsSpace(value[i])) ++i;
        size_t close = std::string::npos;
        std::string url;
        if (i < value.size() && (value[i] == '"' || value[i] == '\'')) {
            const char q = value[i];
            const size_t end = value.find(q, i + 1);
            if (end != std::string::npos) {
                url = value.substr(i + 1, end - i - 1);
                close = value.find(')', end);
            }
        } else {
            close = value.find(')', i);
            if (close != std::string::npos) url = std::string(CTextUtils::Trim(value.substr(i, close - i)));
        }
        if (close == std::string::npos) { out += value.substr(pos); break; }
        out += value.substr(pos, at - pos);
        const std::string rewritten = RewriteAssetUrl(url, report);
        out += rewritten.empty() ? std::string("none") : "url(\"" + rewritten + "\")";
        pos = close + 1;
    }
    return out;
}

void CCssConverter::ConvertPosition(std::vector<CLvssDeclaration>& out, const std::map<std::string, std::string>& box,
                                    bool absolute, CConversionReport& report)
{
    auto get = [&](const char* k) {
        const auto it = box.find(k);
        return it == box.end() ? std::string() : it->second;
    };
    auto negate = [](const std::string& v) {
        const std::string t(CTextUtils::Trim(v));
        if (t.empty() || t == "0" || t == "auto") return std::string("0");
        return t[0] == '-' ? t.substr(1) : "-" + t;
    };
    std::string left = get("left"), right = get("right"), top = get("top"), bottom = get("bottom");
    if (const std::string inset = get("inset"); !inset.empty()) {
        const std::vector<std::string> v = CTextUtils::SplitWhitespace(inset);
        top = v[0];
        right = v.size() > 1 ? v[1] : v[0];
        bottom = v.size() > 2 ? v[2] : v[0];
        left = v.size() > 3 ? v[3] : (v.size() > 1 ? v[1] : v[0]);
    }
    if (left == "auto") left.clear();
    if (right == "auto") right.clear();
    if (top == "auto") top.clear();
    if (bottom == "auto") bottom.clear();
    if (left.empty() && right.empty() && top.empty() && bottom.empty()) return;

    // LVGL has x/y plus an alignment reference: right/bottom become negative offsets from that side.
    const bool fromRight = left.empty() && !right.empty();
    const bool fromBottom = top.empty() && !bottom.empty();
    if (!left.empty()) out.push_back({ "x", left, false, 0 });
    else if (fromRight) out.push_back({ "x", negate(right), false, 0 });
    if (!top.empty()) out.push_back({ "y", top, false, 0 });
    else if (fromBottom) out.push_back({ "y", negate(bottom), false, 0 });
    if (fromRight || fromBottom)
        out.push_back({ "align", std::string(fromBottom ? "bottom" : "top") + "-" + (fromRight ? "right" : "left"), false, 0 });

    // left + right (or top + bottom) stretch the box in CSS.
    const bool hasWidth = !get("width").empty(), hasHeight = !get("height").empty();
    if (!left.empty() && !right.empty() && !hasWidth) {
        if ((left == "0" || left == "0px") && (right == "0" || right == "0px")) out.push_back({ "width", "100%", false, 0 });
        else report.Drop("left+right", "stretching between two offsets needs a width");
    }
    if (!top.empty() && !bottom.empty() && !hasHeight) {
        if ((top == "0" || top == "0px") && (bottom == "0" || bottom == "0px")) out.push_back({ "height", "100%", false, 0 });
        else report.Drop("top+bottom", "stretching between two offsets needs a height");
    }
    if (!absolute) report.Note("position: relative offsets are emitted as x/y (LVGL has no relative shift)");
}

std::vector<CLvssDeclaration> CCssConverter::ConvertDeclarations(const std::vector<CLvssDeclaration>& in,
                                                                 CConversionReport& report)
{
    std::vector<CLvssDeclaration> out;
    std::map<std::string, std::string> box;
    std::string position;

    for (const CLvssDeclaration& d : in) {
        if (d.property == "left" || d.property == "right" || d.property == "top" || d.property == "bottom" ||
            d.property == "inset") {
            box[d.property] = d.value;
            continue;
        }
        if (d.property == "width" || d.property == "height") box[d.property] = d.value;
        if (d.property == "position") { position = CTextUtils::ToLower(CTextUtils::Trim(d.value)); continue; }
        if (CTextUtils::StartsWith(d.property, "--")) { out.push_back(d); continue; }

        CLvssDeclaration c = d;
        if (CTextUtils::ToLower(d.value).find("data:") != std::string::npos) {
            report.Drop(d.property + ": url(data:…)", "inline images are not supported — extract them to files");
            continue;
        }
        c.value = RewriteUrls(d.value, report);
        std::string lower = CTextUtils::ToLower(c.value);
        if (CTextUtils::StartsWith(d.property, "border") || CTextUtils::StartsWith(d.property, "outline")) {
            // currentColor has no LVSS spelling: keep width/style, let the colour default.
            if (lower.find("currentcolor") != std::string::npos) {
                std::string kept;
                for (const std::string& part : CTextUtils::SplitWhitespace(c.value))
                    if (!CTextUtils::IEquals(part, "currentcolor")) kept += (kept.empty() ? "" : " ") + part;
                if (kept.empty()) continue;
                c.value = kept;
                lower = CTextUtils::ToLower(kept);
            }
        }

        if (lower.find("calc(") != std::string::npos || lower.find("clamp(") != std::string::npos ||
            lower.find("min(") != std::string::npos || lower.find("max(") != std::string::npos) {
            report.Drop(d.property + ": " + d.value, "calc()/clamp()/min()/max()");
            continue;
        }
        if (d.property == "display") {
            if (lower == "none" || lower == "flex" || lower == "inline-flex" || lower == "grid") out.push_back(c);
            continue; // block/inline/… are handled by the flow classes
        }
        if (d.property == "z-index") { report.Note("z-index is ignored: LVGL draws children in document order"); continue; }
        if (d.property == "white-space") continue;
        if (d.property == "text-transform") { report.Drop("text-transform", "transform the text in the source"); continue; }
        if (d.property == "align-self" || d.property == "order" || d.property == "justify-self") {
            report.Drop(d.property, "per-item flex alignment");
            continue;
        }
        if (d.property == "background" || d.property == "background-image") {
            if (lower.find("radial-gradient") != std::string::npos || lower.find("conic-gradient") != std::string::npos) {
                report.Drop(d.property + ": radial/conic gradient", "only linear-gradient is supported");
                continue;
            }
            if (CTextUtils::SplitTopLevel(c.value, ',').size() > 1 && lower.find("gradient") == std::string::npos) {
                report.Drop(d.property + " (several layers)", "first layer only");
                c.value = CTextUtils::SplitTopLevel(c.value, ',').front();
            }
        }
        if (d.property == "background-size") continue; // cover/contain handled with an <img class="lv-bg">
        if (d.property == "background-position" || d.property == "background-repeat") {
            report.Drop(d.property, "bg images are drawn centred and unscaled");
            continue;
        }
        if (d.property == "filter" || d.property == "backdrop-filter" || d.property == "text-shadow" ||
            d.property == "clip-path" || d.property == "mask" || d.property == "animation" ||
            CTextUtils::StartsWith(d.property, "animation-") || CTextUtils::StartsWith(d.property, "grid-")) {
            report.Drop(d.property, "no LVGL equivalent");
            continue;
        }
        if (IsSilentDrop(d.property)) continue;
        if (!IsPassThrough(d.property)) {
            report.Drop(d.property, "unknown to lvss");
            continue;
        }
        out.push_back(std::move(c));
    }

    // Scaled backgrounds become an <img class="lv-bg"> (see CHtmlConverter); drop the unscaled copy.
    for (const CLvssDeclaration& d : in) {
        const std::string v = CTextUtils::ToLower(CTextUtils::Trim(d.value));
        if (d.property != "background-size" || !(v == "cover" || v == "contain" || CTextUtils::StartsWith(v, "100%"))) continue;
        for (CLvssDeclaration& o : out)
            if ((o.property == "background-image" || o.property == "background") &&
                CTextUtils::ToLower(o.value).find("url(") != std::string::npos && o.value.find("gradient") == std::string::npos) {
                o.property = "background-image";
                o.value    = "none";
            }
    }

    const bool absolute = position == "absolute" || position == "fixed";
    if (absolute) out.push_back({ "position", position, false, 0 });
    if (absolute || position == "relative") ConvertPosition(out, box, absolute, report);
    return out;
}

CLvssStyleSheet CCssConverter::ConvertStyleSheet(const CLvssStyleSheet& in, CConversionReport& report)
{
    CLvssStyleSheet out;
    for (const auto& v : in.GetVariables()) out.SetVariable(v.first, RewriteUrls(v.second, report));
    for (const CLvssStyleSheet::SFontFace& f : in.GetFontFaces()) {
        CLvssStyleSheet::SFontFace face = f;
        const std::string lower = CTextUtils::ToLower(f.src);
        if (!CTextUtils::EndsWith(lower, ".ttf") && !CTextUtils::EndsWith(lower, ".otf")) {
            const size_t dot = f.src.find_last_of('.');
            face.src = (dot == std::string::npos ? f.src : f.src.substr(0, dot)) + ".ttf";
            report.Asset(f.src, face.src);
        }
        out.AddFontFace(std::move(face));
    }
    for (const CLvssRule& r : in.GetRules()) {
        CLvssRule rule = r;
        rule.declarations = ConvertDeclarations(r.declarations, report);
        if (!rule.declarations.empty()) out.AddRule(std::move(rule));
    }
    return out;
}
