#include "LvssParser.hpp"
#include "LvssValue.hpp"
#include "TextUtils.hpp"
#include <cctype>

void CLvssParser::Warn(std::vector<std::string>* warnings, size_t line, const std::string& message)
{
    if (warnings) warnings->push_back("line " + std::to_string(line) + ": " + message);
}

std::string CLvssParser::StripComments(std::string_view text)
{
    // Comments become spaces of equal length (newlines kept), so line numbers stay valid.
    std::string out(text);
    char quote = 0;
    for (size_t i = 0; i < out.size(); ++i) {
        const char c = out[i];
        if (quote) { if (c == '\\') ++i; else if (c == quote) quote = 0; continue; }
        if (c == '"' || c == '\'') { quote = c; continue; }
        if (c == '/' && i + 1 < out.size() && out[i + 1] == '*') {
            size_t end = out.find("*/", i + 2);
            end = end == std::string::npos ? out.size() : end + 2;
            for (size_t k = i; k < end; ++k)
                if (out[k] != '\n') out[k] = ' ';
            i = end - 1;
        }
    }
    return out;
}

size_t CLvssParser::FindBlockEnd(std::string_view text, size_t open)
{
    int depth = 0;
    char quote = 0;
    for (size_t i = open; i < text.size(); ++i) {
        const char c = text[i];
        if (quote) { if (c == '\\') ++i; else if (c == quote) quote = 0; continue; }
        if (c == '"' || c == '\'') { quote = c; continue; }
        if (c == '{') ++depth;
        else if (c == '}' && --depth == 0) return i;
    }
    return std::string_view::npos;
}

size_t CLvssParser::CountLines(std::string_view text, size_t from, size_t to)
{
    size_t n = 0;
    for (size_t i = from; i < to && i < text.size(); ++i)
        if (text[i] == '\n') ++n;
    return n;
}

std::vector<CLvssDeclaration> CLvssParser::ParseDeclarations(std::string_view block, size_t line,
                                                             std::vector<std::string>* warnings)
{
    std::vector<CLvssDeclaration> out;
    const std::string clean = StripComments(block);
    for (const std::string& item : CTextUtils::SplitTopLevel(clean, ';')) {
        const size_t colon = item.find(':');
        if (colon == std::string::npos) {
            Warn(warnings, line, "declaration without ':' — '" + item + "'");
            continue;
        }
        CLvssDeclaration d;
        d.property = CTextUtils::ToLower(CTextUtils::Trim(std::string_view(item).substr(0, colon)));
        std::string value(CTextUtils::Trim(std::string_view(item).substr(colon + 1)));
        const size_t bang = value.rfind("!important");
        if (bang != std::string::npos) {
            d.important = true;
            value = std::string(CTextUtils::Trim(std::string_view(value).substr(0, bang)));
        }
        d.value = std::move(value);
        d.line  = line;
        if (d.property.empty()) continue;
        out.push_back(std::move(d));
    }
    return out;
}

bool CLvssParser::Parse(std::string_view input, CLvssStyleSheet& out, std::vector<std::string>* warnings)
{
    const std::string text = StripComments(input);
    size_t pos = 0, line = 1;
    bool ok = true;

    auto advanceTo = [&](size_t to) { line += CountLines(text, pos, to); pos = to; };

    while (pos < text.size()) {
        while (pos < text.size() && CTextUtils::IsSpace(text[pos])) advanceTo(pos + 1);
        if (pos >= text.size()) break;

        // ---- at-rules ----
        if (text[pos] == '@') {
            size_t nameEnd = pos + 1;
            while (nameEnd < text.size() && (std::isalnum((unsigned char)text[nameEnd]) || text[nameEnd] == '-')) ++nameEnd;
            const std::string name = CTextUtils::ToLower(std::string_view(text).substr(pos + 1, nameEnd - pos - 1));
            const size_t semi  = text.find(';', nameEnd);
            const size_t brace = text.find('{', nameEnd);

            if (name == "import" || name == "charset") {
                const size_t end = semi == std::string::npos ? text.size() : semi;
                if (name == "import") {
                    std::string path;
                    if (CLvssValue::ParseUrl(std::string_view(text).substr(nameEnd, end - nameEnd), path))
                        out.AddImport(path);
                    else
                        Warn(warnings, line, "bad @import");
                }
                advanceTo(end == text.size() ? end : end + 1);
                continue;
            }
            if (brace == std::string::npos) { Warn(warnings, line, "@" + name + " without a block"); ok = false; break; }
            const size_t close = FindBlockEnd(text, brace);
            if (close == std::string::npos) { Warn(warnings, line, "unterminated @" + name); ok = false; break; }

            if (name == "font-face") {
                CLvssStyleSheet::SFontFace face;
                face.basePath = out.GetPath();
                for (const CLvssDeclaration& d : ParseDeclarations(std::string_view(text).substr(brace + 1, close - brace - 1), line, warnings)) {
                    if (d.property == "font-family") face.family = CTextUtils::ToLower(CTextUtils::Unquote(d.value));
                    else if (d.property == "src") {
                        // src: url(a.woff2) format("woff2"), url(a.ttf) — take the first ttf/otf if any.
                        for (const std::string& item : CTextUtils::SplitTopLevel(d.value, ',')) {
                            std::string url;
                            if (!CLvssValue::ParseUrl(CTextUtils::SplitWhitespace(item).front(), url)) continue;
                            const std::string lower = CTextUtils::ToLower(url);
                            if (face.src.empty() || CTextUtils::EndsWith(lower, ".ttf") || CTextUtils::EndsWith(lower, ".otf")) {
                                face.src = url;
                                if (CTextUtils::EndsWith(lower, ".ttf") || CTextUtils::EndsWith(lower, ".otf")) break;
                            }
                        }
                    }
                }
                if (face.family.empty() || face.src.empty()) Warn(warnings, line, "@font-face needs font-family and src");
                else out.AddFontFace(std::move(face));
            } else {
                Warn(warnings, line, "@" + name + " is not supported, block skipped");
            }
            advanceTo(close + 1);
            continue;
        }

        // ---- qualified rule ----
        const size_t brace = text.find('{', pos);
        if (brace == std::string::npos) { Warn(warnings, line, "trailing garbage"); ok = false; break; }
        const size_t close = FindBlockEnd(text, brace);
        if (close == std::string::npos) { Warn(warnings, line, "unterminated block"); ok = false; break; }

        const std::string prelude(CTextUtils::Trim(std::string_view(text).substr(pos, brace - pos)));
        const size_t ruleLine = line;
        CLvssRule rule;
        rule.line = ruleLine;
        rule.source = out.GetPath();
        rule.declarations = ParseDeclarations(std::string_view(text).substr(brace + 1, close - brace - 1),
                                              ruleLine + CountLines(text, pos, brace), warnings);

        bool rootOnly = true;
        for (const std::string& selText : CTextUtils::SplitTopLevel(prelude, ',')) {
            CLvssSelector sel;
            std::string err;
            if (!CLvssSelector::Parse(selText, sel, &err)) { Warn(warnings, ruleLine, err); continue; }
            const auto& comps = sel.GetCompounds();
            const bool isRoot = comps.size() == 1 && comps[0].tag.empty() && comps[0].id.empty() &&
                                comps[0].classes.empty() && comps[0].structural.size() == 1 && comps[0].structural[0] == "root";
            if (!isRoot) rootOnly = false;
            rule.selectors.push_back(std::move(sel));
        }

        // Custom properties are global variables, wherever they are declared.
        std::vector<CLvssDeclaration> regular;
        for (CLvssDeclaration& d : rule.declarations) {
            if (CTextUtils::StartsWith(d.property, "--")) out.SetVariable(d.property, d.value);
            else regular.push_back(std::move(d));
        }
        rule.declarations = std::move(regular);

        if (!rule.selectors.empty() && !rule.declarations.empty() && !(rootOnly && rule.declarations.empty()))
            out.AddRule(std::move(rule));
        advanceTo(close + 1);
    }
    return ok;
}
