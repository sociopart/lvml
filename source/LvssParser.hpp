// lvml — LVGL markup & style sheets.
#pragma once
#include "LvssStyleSheet.hpp"
#include <string>
#include <string_view>
#include <vector>

// Parser for .lvss — CSS syntax with LVGL-flavoured properties.
// Unknown at-rules (@media, @keyframes…) and invalid selectors are skipped with a warning.
class CLvssParser
{
public:
    CLvssParser() = delete;

    static bool Parse(std::string_view text, CLvssStyleSheet& out, std::vector<std::string>* warnings = nullptr);

    // Body of a block or of a style="" attribute.
    static std::vector<CLvssDeclaration> ParseDeclarations(std::string_view block, size_t line = 0,
                                                           std::vector<std::string>* warnings = nullptr);

    static std::string StripComments(std::string_view text);

private:
    static size_t FindBlockEnd(std::string_view text, size_t open);
    static size_t CountLines(std::string_view text, size_t from, size_t to);
    static void   Warn(std::vector<std::string>* warnings, size_t line, const std::string& message);
};
