#include "LvmlParser.hpp"
#include "TextUtils.hpp"

bool CLvmlParser::IsVoidElement(std::string_view tag)
{
    static const char* kVoid[] = { "area", "base", "br", "col", "embed", "hr", "img", "input",
                                   "link", "meta", "param", "source", "track", "wbr" };
    for (const char* v : kVoid)
        if (CTextUtils::IEquals(tag, v)) return true;
    return false;
}

bool CLvmlParser::IsRawTextElement(std::string_view tag)
{
    return CTextUtils::IEquals(tag, "style") || CTextUtils::IEquals(tag, "script");
}

bool CLvmlParser::IsNameChar(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           c == '-' || c == '_' || c == ':' || c == '.' || c == '@';
}

void CLvmlParser::Error(SState& s, const std::string& message, bool fatal)
{
    if (s.errors) s.errors->push_back("line " + std::to_string(s.line) + ": " + message);
    if (fatal) s.fatal = true;
}

void CLvmlParser::Advance(SState& s, size_t count)
{
    for (size_t i = 0; i < count && s.pos < s.text.size(); ++i, ++s.pos)
        if (s.text[s.pos] == '\n') ++s.line;
}

bool CLvmlParser::Consume(SState& s, std::string_view token)
{
    if (s.text.compare(s.pos, token.size(), token) != 0) return false;
    Advance(s, token.size());
    return true;
}

void CLvmlParser::SkipSpaces(SState& s)
{
    while (s.pos < s.text.size() && CTextUtils::IsSpace(s.text[s.pos])) Advance(s, 1);
}

std::string CLvmlParser::ReadName(SState& s)
{
    const size_t start = s.pos;
    while (s.pos < s.text.size() && IsNameChar(s.text[s.pos])) ++s.pos;
    std::string name(s.text.substr(start, s.pos - start));
    return s.options.html ? CTextUtils::ToLower(name) : name;
}

bool CLvmlParser::SkipUntil(SState& s, std::string_view terminator)
{
    const size_t hit = s.text.find(terminator, s.pos);
    if (hit == std::string_view::npos) { Advance(s, s.text.size() - s.pos); return false; }
    Advance(s, hit + terminator.size() - s.pos);
    return true;
}

bool CLvmlParser::ReadAttributes(SState& s, CLvmlNode& node, bool& selfClosing)
{
    selfClosing = false;
    for (;;) {
        SkipSpaces(s);
        if (s.pos >= s.text.size()) { Error(s, "unexpected end of file inside <" + node.GetTag() + ">", true); return false; }
        if (Consume(s, "/>")) { selfClosing = true; return true; }
        if (Consume(s, ">")) return true;

        std::string name = ReadName(s);
        if (name.empty()) {
            Error(s, std::string("unexpected character '") + s.text[s.pos] + "' in <" + node.GetTag() + ">", !s.options.html);
            Advance(s, 1);
            if (s.fatal) return false;
            continue;
        }
        SkipSpaces(s);
        std::string value;
        if (Consume(s, "=")) {
            SkipSpaces(s);
            const char q = s.pos < s.text.size() ? s.text[s.pos] : 0;
            if (q == '"' || q == '\'') {
                Advance(s, 1);
                const size_t end = s.text.find(q, s.pos);
                if (end == std::string_view::npos) { Error(s, "unterminated attribute value", true); return false; }
                value = CTextUtils::DecodeEntities(s.text.substr(s.pos, end - s.pos));
                Advance(s, end - s.pos + 1);
            } else {
                if (!s.options.html) Error(s, "attribute '" + name + "' value must be quoted", false);
                const size_t start = s.pos;
                while (s.pos < s.text.size() && !CTextUtils::IsSpace(s.text[s.pos]) && s.text[s.pos] != '>' &&
                       !(s.text[s.pos] == '/' && s.pos + 1 < s.text.size() && s.text[s.pos + 1] == '>'))
                    Advance(s, 1);
                value = CTextUtils::DecodeEntities(s.text.substr(start, s.pos - start));
            }
        }
        node.SetAttribute(name, std::move(value));
    }
}

void CLvmlParser::AddText(SState& s, CLvmlNode& parent, std::string_view raw)
{
    if (raw.empty()) return;
    if (!s.options.keepWhitespace && CTextUtils::Trim(raw).empty()) return;
    parent.AppendText(CTextUtils::DecodeEntities(raw));
}

bool CLvmlParser::Parse(std::string_view text, CLvmlDocument& doc, std::vector<std::string>* errors)
{
    return Parse(text, doc, SOptions{}, errors);
}

bool CLvmlParser::Parse(std::string_view text, CLvmlDocument& doc, const SOptions& options, std::vector<std::string>* errors)
{
    SState s;
    s.text    = text;
    s.options = options;
    s.errors  = errors;

    // UTF-8 BOM
    if (CTextUtils::StartsWith(s.text, "\xEF\xBB\xBF")) s.pos = 3;

    doc.GetRoot().ClearChildren();
    std::vector<CLvmlNode*> stack{ &doc.GetRoot() };

    while (s.pos < s.text.size() && !s.fatal) {
        const size_t lt = s.text.find('<', s.pos);
        if (lt == std::string_view::npos) {
            AddText(s, *stack.back(), s.text.substr(s.pos));
            Advance(s, s.text.size() - s.pos);
            break;
        }
        if (lt > s.pos) {
            AddText(s, *stack.back(), s.text.substr(s.pos, lt - s.pos));
            Advance(s, lt - s.pos);
        }

        if (Consume(s, "<!--")) {
            if (!SkipUntil(s, "-->")) Error(s, "unterminated comment", !options.html);
            continue;
        }
        if (Consume(s, "<![CDATA[")) {
            const size_t start = s.pos;
            const size_t end   = s.text.find("]]>", s.pos);
            const size_t stop  = end == std::string_view::npos ? s.text.size() : end;
            stack.back()->AppendText(std::string(s.text.substr(start, stop - start)));
            Advance(s, stop - s.pos);
            Consume(s, "]]>");
            continue;
        }
        if (Consume(s, "<?") || Consume(s, "<!")) { // <?xml …?>, <!DOCTYPE …>
            SkipUntil(s, ">");
            continue;
        }

        if (Consume(s, "</")) {
            const std::string name = ReadName(s);
            SkipUntil(s, ">");
            size_t match = stack.size();
            for (size_t i = stack.size(); i-- > 1;)
                if (CTextUtils::IEquals(stack[i]->GetTag(), name)) { match = i; break; }
            if (match == stack.size()) {
                Error(s, "unexpected </" + name + ">", !options.html);
                continue;
            }
            if (match != stack.size() - 1 && !options.html)
                Error(s, "<" + stack.back()->GetTag() + "> is not closed before </" + name + ">", true);
            stack.resize(match);
            continue;
        }

        Advance(s, 1); // '<'
        std::string tag = ReadName(s);
        if (tag.empty()) { // a stray '<' in text
            Error(s, "stray '<'", !options.html);
            stack.back()->AppendText("<");
            continue;
        }
        auto node = std::make_unique<CLvmlNode>(CLvmlNode::EType::Element);
        node->SetTag(tag);
        node->SetLine(s.line);
        bool selfClosing = false;
        if (!ReadAttributes(s, *node, selfClosing)) break;

        CLvmlNode* added = stack.back()->AppendChild(std::move(node));
        if (selfClosing || (options.html && IsVoidElement(tag))) continue;

        if (IsRawTextElement(tag)) {
            const std::string close = "</" + tag;
            size_t end = s.text.find(close, s.pos);
            if (options.html && end == std::string_view::npos) end = s.text.find("</" + CTextUtils::ToLower(tag), s.pos);
            if (end == std::string_view::npos) { Error(s, "<" + tag + "> is not closed", true); break; }
            if (end > s.pos) added->AppendText(std::string(s.text.substr(s.pos, end - s.pos)));
            Advance(s, end - s.pos);
            SkipUntil(s, ">");
            continue;
        }
        stack.push_back(added);
    }

    if (stack.size() > 1 && !s.fatal) {
        if (options.html) {
            // Browsers close everything at EOF.
        } else {
            Error(s, "<" + stack.back()->GetTag() + "> is not closed", true);
        }
    }
    return !s.fatal;
}
