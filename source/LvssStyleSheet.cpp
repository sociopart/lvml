#include "LvssStyleSheet.hpp"
#include "TextUtils.hpp"

void CLvssStyleSheet::AddRule(CLvssRule rule)
{
    rule.order = m_nextOrder++;
    m_rules.push_back(std::move(rule));
}

void CLvssStyleSheet::Append(const CLvssStyleSheet& other)
{
    for (const CLvssRule& r : other.m_rules) AddRule(r);
    for (const SFontFace& f : other.m_fontFaces) m_fontFaces.push_back(f);
    for (const auto& v : other.m_variables) m_variables[v.first] = v.second;
}

std::string CLvssStyleSheet::ResolveVariables(std::string_view value) const
{
    std::string out(value);
    for (int guard = 0; guard < 16; ++guard) {
        const size_t at = out.find("var(");
        if (at == std::string::npos) break;
        int depth = 0;
        size_t end = at + 3;
        for (; end < out.size(); ++end) {
            if (out[end] == '(') ++depth;
            else if (out[end] == ')' && --depth == 0) break;
        }
        if (end >= out.size()) break;
        const std::string inner = out.substr(at + 4, end - at - 4);
        const std::vector<std::string> parts = CTextUtils::SplitTopLevel(inner, ',', false);
        std::string name = parts.empty() ? std::string() : std::string(CTextUtils::Trim(parts[0]));
        std::string replacement;
        const auto it = m_variables.find(name);
        if (it != m_variables.end()) replacement = it->second;
        else if (parts.size() > 1) {
            // Fallback may itself contain commas: take everything after the first one.
            replacement = std::string(CTextUtils::Trim(std::string_view(inner).substr(inner.find(',') + 1)));
        }
        out.replace(at, end - at + 1, replacement);
    }
    return out;
}

std::string CLvssStyleSheet::ToString() const
{
    std::string out;
    for (const std::string& i : m_imports) out += "@import \"" + i + "\";\n";
    for (const SFontFace& f : m_fontFaces)
        out += "@font-face { font-family: " + f.family + "; src: url(\"" + f.src + "\"); }\n";
    if (!m_variables.empty()) {
        out += ":root {\n";
        for (const auto& v : m_variables) out += "    " + v.first + ": " + v.second + ";\n";
        out += "}\n";
    }
    if (!out.empty()) out += "\n";
    for (const CLvssRule& r : m_rules) out += r.ToString() + "\n";
    return out;
}
