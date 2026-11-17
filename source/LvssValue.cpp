#include "LvssValue.hpp"
#include "TextUtils.hpp"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <vector>

bool CLvssValue::ParseLength(std::string_view s, SLength& out)
{
    s = CTextUtils::Trim(s);
    if (s.empty()) return false;
    const std::string lower = CTextUtils::ToLower(s);
    if (lower == "content" || lower == "fit-content" || lower == "max-content" || lower == "min-content") {
        out = { 0, EUnit::Content };
        return true;
    }
    if (lower == "auto") { out = { 0, EUnit::Auto }; return true; }

    struct SSuffix { const char* text; EUnit unit; };
    static const SSuffix kSuffixes[] = {
        { "px", EUnit::Px }, { "dp", EUnit::Px }, { "rem", EUnit::Rem }, { "em", EUnit::Em },
        { "%", EUnit::Percent }, { "vw", EUnit::Vw }, { "vh", EUnit::Vh },
    };
    std::string_view number = lower;
    EUnit unit = EUnit::Px;
    for (const SSuffix& suf : kSuffixes) {
        if (CTextUtils::EndsWith(lower, suf.text)) {
            number = std::string_view(lower).substr(0, lower.size() - std::char_traits<char>::length(suf.text));
            unit = suf.unit;
            break;
        }
    }
    float v = 0;
    if (!CTextUtils::ParseFloat(number, v)) return false;
    out = { v, unit };
    return true;
}

bool CLvssValue::ParseColor(std::string_view s, uint32_t& argb, bool* hasAlpha)
{
    s = CTextUtils::Trim(s);
    if (hasAlpha) *hasAlpha = false;
    if (s.empty()) return false;
    const std::string lower = CTextUtils::ToLower(s);

    if (lower[0] == '#') {
        const std::string hex = lower.substr(1);
        for (char c : hex)
            if (!std::isxdigit((unsigned char)c)) return false;
        auto nib = [&](size_t i) { return uint32_t(std::stoul(hex.substr(i, 1), nullptr, 16)); };
        auto byte = [&](size_t i) { return uint32_t(std::stoul(hex.substr(i, 2), nullptr, 16)); };
        uint32_t r, g, b, a = 0xFF;
        if (hex.size() == 3 || hex.size() == 4) {
            r = nib(0) * 17; g = nib(1) * 17; b = nib(2) * 17;
            if (hex.size() == 4) { a = nib(3) * 17; if (hasAlpha) *hasAlpha = true; }
        } else if (hex.size() == 6 || hex.size() == 8) {
            r = byte(0); g = byte(2); b = byte(4);
            if (hex.size() == 8) { a = byte(6); if (hasAlpha) *hasAlpha = true; }
        } else {
            return false;
        }
        argb = (a << 24) | (r << 16) | (g << 8) | b;
        return true;
    }

    if (CTextUtils::StartsWith(lower, "rgb")) {
        const size_t open = lower.find('('), close = lower.rfind(')');
        if (open == std::string::npos || close == std::string::npos || close < open) return false;
        std::string inner = lower.substr(open + 1, close - open - 1);
        for (char& c : inner) if (c == ',' || c == '/') c = ' ';
        const std::vector<std::string> parts = CTextUtils::SplitWhitespace(inner);
        if (parts.size() < 3) return false;
        uint32_t ch[3];
        for (int i = 0; i < 3; ++i) {
            float v = 0;
            const bool pct = CTextUtils::EndsWith(parts[size_t(i)], "%");
            if (!CTextUtils::ParseFloat(pct ? parts[size_t(i)].substr(0, parts[size_t(i)].size() - 1) : parts[size_t(i)], v)) return false;
            if (pct) v = v * 255.0f / 100.0f;
            ch[i] = uint32_t(std::lround(v < 0 ? 0 : v > 255 ? 255 : v));
        }
        uint8_t a = 0xFF;
        if (parts.size() >= 4) {
            if (!ParseOpacity(parts[3], a)) return false;
            if (hasAlpha) *hasAlpha = true;
        }
        argb = (uint32_t(a) << 24) | (ch[0] << 16) | (ch[1] << 8) | ch[2];
        return true;
    }

    struct SNamed { const char* name; uint32_t argb; };
    static const SNamed kNamed[] = {
        { "transparent", 0x00000000 }, { "black", 0xFF000000 }, { "white", 0xFFFFFFFF },
        { "red", 0xFFFF0000 },         { "green", 0xFF008000 }, { "lime", 0xFF00FF00 },
        { "blue", 0xFF0000FF },        { "yellow", 0xFFFFFF00 }, { "orange", 0xFFFFA500 },
        { "gray", 0xFF808080 },        { "grey", 0xFF808080 },  { "silver", 0xFFC0C0C0 },
        { "purple", 0xFF800080 },      { "cyan", 0xFF00FFFF },  { "magenta", 0xFFFF00FF },
        { "pink", 0xFFFFC0CB },        { "navy", 0xFF000080 },  { "teal", 0xFF008080 },
        { "maroon", 0xFF800000 },      { "olive", 0xFF808000 }, { "gold", 0xFFFFD700 },
    };
    for (const SNamed& n : kNamed) {
        if (lower == n.name) {
            argb = n.argb;
            if (hasAlpha) *hasAlpha = lower == "transparent";
            return true;
        }
    }
    return false;
}

bool CLvssValue::ParseOpacity(std::string_view s, uint8_t& out)
{
    s = CTextUtils::Trim(s);
    const std::string lower = CTextUtils::ToLower(s);
    if (lower == "cover") { out = 255; return true; }
    if (lower == "transp" || lower == "transparent") { out = 0; return true; }
    float v = 0;
    if (CTextUtils::EndsWith(lower, "%")) {
        if (!CTextUtils::ParseFloat(std::string_view(lower).substr(0, lower.size() - 1), v)) return false;
        v = v * 255.0f / 100.0f;
    } else {
        if (!CTextUtils::ParseFloat(lower, v)) return false;
        if (lower.find('.') != std::string::npos || v <= 1.0f) {
            // 0.6 → 60 %. A bare integer 1 is treated as fully opaque too.
            if (v <= 1.0f) v *= 255.0f;
        }
    }
    out = uint8_t(std::lround(v < 0 ? 0 : v > 255 ? 255 : v));
    return true;
}

bool CLvssValue::ParseTime(std::string_view s, uint32_t& ms)
{
    const std::string lower = CTextUtils::ToLower(CTextUtils::Trim(s));
    float v = 0;
    if (CTextUtils::EndsWith(lower, "ms")) {
        if (!CTextUtils::ParseFloat(std::string_view(lower).substr(0, lower.size() - 2), v)) return false;
    } else if (CTextUtils::EndsWith(lower, "s")) {
        if (!CTextUtils::ParseFloat(std::string_view(lower).substr(0, lower.size() - 1), v)) return false;
        v *= 1000.0f;
    } else if (!CTextUtils::ParseFloat(lower, v)) {
        return false;
    }
    ms = uint32_t(std::lround(v < 0 ? 0 : v));
    return true;
}

bool CLvssValue::ParseAngle(std::string_view s, int32_t& decidegrees)
{
    const std::string lower = CTextUtils::ToLower(CTextUtils::Trim(s));
    float v = 0;
    float k = 1.0f;
    std::string_view num = lower;
    if (CTextUtils::EndsWith(lower, "deg")) num = std::string_view(lower).substr(0, lower.size() - 3);
    else if (CTextUtils::EndsWith(lower, "turn")) { num = std::string_view(lower).substr(0, lower.size() - 4); k = 360.0f; }
    else if (CTextUtils::EndsWith(lower, "rad")) { num = std::string_view(lower).substr(0, lower.size() - 3); k = 57.2957795f; }
    if (!CTextUtils::ParseFloat(num, v)) return false;
    decidegrees = int32_t(std::lround(v * k * 10.0f));
    return true;
}

bool CLvssValue::ParseUrl(std::string_view s, std::string& out)
{
    s = CTextUtils::Trim(s);
    if (CTextUtils::StartsWith(CTextUtils::ToLower(s.substr(0, 4)), "url(")) {
        const size_t close = s.rfind(')');
        if (close == std::string_view::npos) return false;
        s = s.substr(4, close - 4);
    }
    out = CTextUtils::Unquote(s);
    return !out.empty();
}

std::string CLvssValue::ColorToHex(uint32_t argb, bool withAlpha)
{
    char buf[16];
    if (withAlpha && (argb >> 24) != 0xFF)
        std::snprintf(buf, sizeof(buf), "#%06X%02X", argb & 0xFFFFFF, argb >> 24);
    else
        std::snprintf(buf, sizeof(buf), "#%06X", argb & 0xFFFFFF);
    return buf;
}
