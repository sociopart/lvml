// lvml — LVGL markup & style sheets.
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// String helpers shared by the parsers, the builder and the converter.
class CTextUtils
{
public:
    CTextUtils() = delete;

    static bool             IsSpace(char c);
    static std::string_view Trim(std::string_view s);
    static std::string      ToLower(std::string_view s);
    static bool             IEquals(std::string_view a, std::string_view b);
    static bool             StartsWith(std::string_view s, std::string_view prefix);
    static bool             EndsWith(std::string_view s, std::string_view suffix);

    // Splits on `sep`, ignoring separators inside quotes and parentheses.
    static std::vector<std::string> SplitTopLevel(std::string_view s, char sep, bool skipEmpty = true);
    // Splits on whitespace, ignoring whitespace inside quotes and parentheses.
    static std::vector<std::string> SplitWhitespace(std::string_view s);

    // Collapses runs of whitespace into one space (HTML text semantics). Does not trim.
    static std::string CollapseWhitespace(std::string_view s);
    static std::string Unquote(std::string_view s);
    static std::string Replace(std::string_view s, std::string_view from, std::string_view to);

    static void        AppendUtf8(std::string& out, uint32_t codepoint);
    static std::string DecodeEntities(std::string_view s);
    static std::string EscapeXml(std::string_view s, bool attribute);

    static bool ParseFloat(std::string_view s, float& out);
    static bool ParseInt(std::string_view s, long& out);
    static std::string FormatFloat(float value);
};
