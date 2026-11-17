#include "TextUtils.hpp"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

bool CTextUtils::IsSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

std::string_view CTextUtils::Trim(std::string_view s)
{
    size_t b = 0, e = s.size();
    while (b < e && IsSpace(s[b])) ++b;
    while (e > b && IsSpace(s[e - 1])) --e;
    return s.substr(b, e - b);
}

std::string CTextUtils::ToLower(std::string_view s)
{
    std::string out(s);
    for (char& c : out)
        if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
    return out;
}

bool CTextUtils::IEquals(std::string_view a, std::string_view b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        char x = a[i], y = b[i];
        if (x >= 'A' && x <= 'Z') x = char(x - 'A' + 'a');
        if (y >= 'A' && y <= 'Z') y = char(y - 'A' + 'a');
        if (x != y) return false;
    }
    return true;
}

bool CTextUtils::StartsWith(std::string_view s, std::string_view prefix)
{
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool CTextUtils::EndsWith(std::string_view s, std::string_view suffix)
{
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::vector<std::string> CTextUtils::SplitTopLevel(std::string_view s, char sep, bool skipEmpty)
{
    std::vector<std::string> out;
    int depth = 0;
    char quote = 0;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        const char c = i < s.size() ? s[i] : sep;
        if (i < s.size()) {
            if (quote) { if (c == '\\') { ++i; continue; } if (c == quote) quote = 0; continue; }
            if (c == '"' || c == '\'') { quote = c; continue; }
            if (c == '(' || c == '[') { ++depth; continue; }
            if ((c == ')' || c == ']') && depth > 0) { --depth; continue; }
        }
        if (c == sep && depth == 0) {
            std::string_view part = Trim(s.substr(start, i - start));
            if (!part.empty() || !skipEmpty) out.emplace_back(part);
            start = i + 1;
        }
    }
    return out;
}

std::vector<std::string> CTextUtils::SplitWhitespace(std::string_view s)
{
    std::vector<std::string> out;
    int depth = 0;
    char quote = 0;
    std::string cur;
    for (size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if (quote) { cur += c; if (c == quote) quote = 0; continue; }
        if (c == '"' || c == '\'') { quote = c; cur += c; continue; }
        if (c == '(') ++depth;
        if (c == ')' && depth > 0) --depth;
        if (IsSpace(c) && depth == 0) {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
            continue;
        }
        cur += c;
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

std::string CTextUtils::CollapseWhitespace(std::string_view s)
{
    std::string out;
    out.reserve(s.size());
    bool space = false;
    for (char c : s) {
        if (IsSpace(c)) { space = true; continue; }
        if (space) { out += ' '; space = false; }
        out += c;
    }
    if (space) out += ' ';
    return out;
}

std::string CTextUtils::Unquote(std::string_view s)
{
    s = Trim(s);
    if (s.size() >= 2 && (s.front() == '"' || s.front() == '\'') && s.back() == s.front())
        s = s.substr(1, s.size() - 2);
    return std::string(s);
}

std::string CTextUtils::Replace(std::string_view s, std::string_view from, std::string_view to)
{
    std::string out;
    if (from.empty()) return std::string(s);
    size_t pos = 0;
    for (;;) {
        const size_t hit = s.find(from, pos);
        if (hit == std::string_view::npos) { out.append(s.substr(pos)); break; }
        out.append(s.substr(pos, hit - pos));
        out.append(to);
        pos = hit + from.size();
    }
    return out;
}

void CTextUtils::AppendUtf8(std::string& out, uint32_t cp)
{
    if (cp < 0x80) out += char(cp);
    else if (cp < 0x800) { out += char(0xC0 | (cp >> 6)); out += char(0x80 | (cp & 0x3F)); }
    else if (cp < 0x10000) {
        out += char(0xE0 | (cp >> 12)); out += char(0x80 | ((cp >> 6) & 0x3F)); out += char(0x80 | (cp & 0x3F));
    } else {
        out += char(0xF0 | (cp >> 18)); out += char(0x80 | ((cp >> 12) & 0x3F));
        out += char(0x80 | ((cp >> 6) & 0x3F)); out += char(0x80 | (cp & 0x3F));
    }
}

std::string CTextUtils::DecodeEntities(std::string_view s)
{
    struct SEntity { const char* name; uint32_t cp; };
    static const SEntity kEntities[] = {
        { "amp", '&' }, { "lt", '<' }, { "gt", '>' }, { "quot", '"' }, { "apos", '\'' },
        { "nbsp", 0xA0 }, { "copy", 0xA9 }, { "reg", 0xAE }, { "trade", 0x2122 }, { "laquo", 0xAB },
        { "raquo", 0xBB }, { "mdash", 0x2014 }, { "ndash", 0x2013 }, { "hellip", 0x2026 }, { "bull", 0x2022 },
        { "middot", 0xB7 }, { "times", 0xD7 }, { "deg", 0xB0 }, { "euro", 0x20AC }, { "rarr", 0x2192 },
        { "larr", 0x2190 }, { "check", 0x2713 },
    };

    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '&') { out += s[i]; continue; }
        const size_t semi = s.find(';', i + 1);
        if (semi == std::string_view::npos || semi - i > 12) { out += '&'; continue; }
        std::string_view name = s.substr(i + 1, semi - i - 1);
        bool ok = false;
        if (!name.empty() && name[0] == '#') {
            const bool hex = name.size() > 1 && (name[1] == 'x' || name[1] == 'X');
            const std::string digits(name.substr(hex ? 2 : 1));
            char* end = nullptr;
            const unsigned long cp = std::strtoul(digits.c_str(), &end, hex ? 16 : 10);
            if (!digits.empty() && end && *end == 0) { AppendUtf8(out, uint32_t(cp)); ok = true; }
        } else {
            for (const SEntity& e : kEntities)
                if (name == e.name) { AppendUtf8(out, e.cp); ok = true; break; }
        }
        if (ok) i = semi;
        else out += '&';
    }
    return out;
}

std::string CTextUtils::EscapeXml(std::string_view s, bool attribute)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': if (attribute) out += "&quot;"; else out += c; break;
        case '\n': if (attribute) out += "&#10;"; else out += c; break;
        default: out += c;
        }
    }
    return out;
}

bool CTextUtils::ParseFloat(std::string_view s, float& out)
{
    s = Trim(s);
    if (s.empty() || s.size() > 63) return false;
    char buf[64];
    std::memcpy(buf, s.data(), s.size());
    buf[s.size()] = 0;
    char* end = nullptr;
    const float v = std::strtof(buf, &end);
    if (end == buf || *end != 0) return false;
    out = v;
    return true;
}

bool CTextUtils::ParseInt(std::string_view s, long& out)
{
    s = Trim(s);
    if (s.empty() || s.size() > 31) return false;
    char buf[32];
    std::memcpy(buf, s.data(), s.size());
    buf[s.size()] = 0;
    char* end = nullptr;
    const long v = std::strtol(buf, &end, 10);
    if (end == buf || *end != 0) return false;
    out = v;
    return true;
}

std::string CTextUtils::FormatFloat(float value)
{
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.3f", value);
    std::string s(buf);
    while (!s.empty() && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == '.') s.pop_back();
    if (s == "-0") s = "0";
    return s;
}
