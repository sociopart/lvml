// lvml — LVGL markup & style sheets.
#pragma once
#include "LvmlDocument.hpp"
#include <string>
#include <string_view>
#include <vector>

// XML-subset parser for .lvml. With `html = true` it is lenient enough for real-world HTML:
// void elements (<br>, <img>, <input>…), unquoted / boolean attributes, case-insensitive tags,
// raw <script>/<style> bodies and unbalanced end tags.
class CLvmlParser
{
public:
    struct SOptions {
        bool html           = false; // lenient HTML mode
        bool keepWhitespace = false; // keep whitespace-only text nodes
    };

    CLvmlParser() = delete;

    // Returns false on a fatal syntax error (strict mode only); `errors` collects all problems.
    static bool Parse(std::string_view text, CLvmlDocument& doc, std::vector<std::string>* errors = nullptr);
    static bool Parse(std::string_view text, CLvmlDocument& doc, const SOptions& options,
                      std::vector<std::string>* errors = nullptr);

private:
    struct SState {
        std::string_view          text;
        size_t                    pos  = 0;
        size_t                    line = 1;
        SOptions                  options;
        std::vector<std::string>* errors = nullptr;
        bool                      fatal  = false;
    };

    static bool IsVoidElement(std::string_view tag);
    static bool IsRawTextElement(std::string_view tag);
    static bool IsNameChar(char c);

    static void        Error(SState& s, const std::string& message, bool fatal);
    static void        Advance(SState& s, size_t count);
    static bool        Consume(SState& s, std::string_view token);
    static void        SkipSpaces(SState& s);
    static std::string ReadName(SState& s);
    static bool        SkipUntil(SState& s, std::string_view terminator);
    static bool        ReadAttributes(SState& s, CLvmlNode& node, bool& selfClosing);
    static void        AddText(SState& s, CLvmlNode& parent, std::string_view raw);
};
