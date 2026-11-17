#include "LvssSelector.hpp"
#include "TextUtils.hpp"

const char* CLvssSelector::CanonicalState(std::string_view name)
{
    struct SAlias { const char* alias; const char* state; };
    static const SAlias kStates[] = {
        { "hover", "hovered" },     { "hovered", "hovered" },     { "active", "pressed" },
        { "pressed", "pressed" },   { "checked", "checked" },     { "focus", "focused" },
        { "focused", "focused" },   { "focus-visible", "focus-key" }, { "focus-key", "focus-key" },
        { "edited", "edited" },     { "disabled", "disabled" },   { "scrolled", "scrolled" },
        { "alt", "alt" },           { "user1", "user1" },         { "user2", "user2" },
        { "user3", "user3" },       { "user4", "user4" },
    };
    for (const SAlias& a : kStates)
        if (CTextUtils::IEquals(name, a.alias)) return a.state;
    return nullptr;
}

const char* CLvssSelector::CanonicalPart(std::string_view name)
{
    static const char* kParts[] = { "main", "scrollbar", "indicator", "knob", "selected", "items", "cursor" };
    for (const char* p : kParts)
        if (CTextUtils::IEquals(name, p)) return p;
    if (CTextUtils::IEquals(name, "-webkit-scrollbar-thumb") || CTextUtils::IEquals(name, "thumb")) return "scrollbar";
    return nullptr;
}

bool CLvssSelector::Parse(std::string_view text, CLvssSelector& out, std::string* error)
{
    out = CLvssSelector();
    auto fail = [&](const std::string& msg) {
        if (error) *error = "selector '" + std::string(CTextUtils::Trim(text)) + "': " + msg;
        return false;
    };
    auto isIdent = [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
               (unsigned char)c >= 0x80;
    };

    const std::string_view s = CTextUtils::Trim(text);
    size_t i = 0;
    SCompound cur;
    bool haveCur = false;
    ECombinator pending = ECombinator::Descendant;

    auto readIdent = [&]() {
        const size_t start = i;
        while (i < s.size() && isIdent(s[i])) ++i;
        return std::string(s.substr(start, i - start));
    };
    auto flush = [&]() {
        if (!haveCur) return;
        if (!out.m_compounds.empty()) out.m_combinators.push_back(pending);
        out.m_compounds.push_back(std::move(cur));
        cur = SCompound();
        haveCur = false;
        pending = ECombinator::Descendant;
    };

    while (i < s.size()) {
        const char c = s[i];
        if (CTextUtils::IsSpace(c) || c == '>') {
            bool child = false;
            while (i < s.size() && (CTextUtils::IsSpace(s[i]) || s[i] == '>')) { if (s[i] == '>') child = true; ++i; }
            if (!haveCur) {
                if (out.m_compounds.empty()) return fail("starts with a combinator");
                if (child) pending = ECombinator::Child;
                continue;
            }
            flush();
            pending = child ? ECombinator::Child : ECombinator::Descendant;
            continue;
        }
        if (c == '+' || c == '~' || c == '[') return fail(std::string("'") + c + "' is not supported");
        haveCur = true;
        if (c == '*') { cur.tag = "*"; ++i; continue; }
        if (c == '#') { ++i; cur.id = readIdent(); if (cur.id.empty()) return fail("empty id"); continue; }
        if (c == '.') { ++i; std::string cls = readIdent(); if (cls.empty()) return fail("empty class"); cur.classes.push_back(cls); continue; }
        if (c == ':') {
            ++i;
            const bool isPart = i < s.size() && s[i] == ':';
            if (isPart) ++i;
            const std::string name = readIdent();
            if (i < s.size() && s[i] == '(') return fail(":" + name + "() is not supported");
            if (const char* part = CanonicalPart(name); part && (isPart || name == "scrollbar")) { cur.part = part; continue; }
            if (isPart) return fail("unknown part ::" + name);
            if (const char* state = CanonicalState(name)) { cur.states.emplace_back(state); continue; }
            if (name == "first-child" || name == "last-child" || name == "only-child" || name == "root") {
                cur.structural.push_back(name);
                continue;
            }
            return fail("unknown pseudo-class :" + name);
        }
        if (isIdent(c)) { cur.tag = CTextUtils::ToLower(readIdent()); continue; }
        return fail(std::string("unexpected '") + c + "'");
    }
    flush();
    if (out.m_compounds.empty()) return fail("empty");
    for (size_t k = 0; k + 1 < out.m_compounds.size(); ++k)
        if (!out.m_compounds[k].part.empty()) return fail("a part is allowed only on the last compound");
    return true;
}

bool CLvssSelector::HasAncestorStates() const
{
    for (size_t k = 0; k + 1 < m_compounds.size(); ++k)
        if (!m_compounds[k].states.empty()) return true;
    return false;
}

bool CLvssSelector::MatchCompound(const SCompound& c, const CLvmlNode& node, const StateQuery* query)
{
    if (!node.IsElement()) return false;
    if (!c.tag.empty() && c.tag != "*" && !CTextUtils::IEquals(c.tag, node.GetTag())) return false;
    if (!c.id.empty()) {
        const std::string* id = node.FindAttribute("id");
        if (!id || *id != c.id) return false;
    }
    if (!c.classes.empty()) {
        const std::vector<std::string> classes = node.GetClasses();
        for (const std::string& want : c.classes) {
            bool found = false;
            for (const std::string& have : classes) if (have == want) { found = true; break; }
            if (!found) return false;
        }
    }
    for (const std::string& p : c.structural) {
        if (p == "first-child" && node.GetIndexInParent() != 0) return false;
        if (p == "last-child" && node.GetIndexInParent() + 1 != node.GetElementSiblingCount()) return false;
        if (p == "only-child" && node.GetElementSiblingCount() != 1) return false;
        if (p == "root") {
            const CLvmlNode* parent = node.GetParent();
            if (!parent || parent->GetParent() != nullptr) return false; // child of #document
        }
    }
    if (query) {
        if (!c.states.empty() && !(*query)(node, c.states)) return false;
    }
    return true;
}

bool CLvssSelector::MatchFrom(size_t index, const CLvmlNode& node, const StateQuery* query) const
{
    // index > 0 here: an ancestor compound; states must hold, or there is no live state to ask.
    const SCompound& c = m_compounds[index];
    if (!c.states.empty() && !query) return false;
    if (!MatchCompound(c, node, query)) return false;
    if (index == 0) return true;

    const ECombinator comb = m_combinators[index - 1];
    for (const CLvmlNode* p = node.GetParent(); p; p = p->GetParent()) {
        if (MatchFrom(index - 1, *p, query)) return true;
        if (comb == ECombinator::Child) break;
    }
    return false;
}

bool CLvssSelector::Matches(const CLvmlNode& node, const StateQuery* query) const
{
    if (m_compounds.empty()) return false;
    const size_t last = m_compounds.size() - 1;
    // Subject states are left to LVGL: match the structure only.
    if (!MatchCompound(m_compounds[last], node, nullptr)) return false;
    if (last == 0) return true;

    const ECombinator comb = m_combinators[last - 1];
    for (const CLvmlNode* p = node.GetParent(); p; p = p->GetParent()) {
        if (MatchFrom(last - 1, *p, query)) return true;
        if (comb == ECombinator::Child) break;
    }
    return false;
}

uint32_t CLvssSelector::GetSpecificity() const
{
    uint32_t a = 0, b = 0, c = 0;
    for (const SCompound& k : m_compounds) {
        if (!k.id.empty()) ++a;
        b += uint32_t(k.classes.size() + k.states.size() + k.structural.size());
        if (!k.tag.empty() && k.tag != "*") ++c;
        if (!k.part.empty()) ++c;
    }
    return a * 10000 + b * 100 + c;
}

std::string CLvssSelector::ToString() const
{
    std::string out;
    for (size_t k = 0; k < m_compounds.size(); ++k) {
        if (k > 0) out += m_combinators[k - 1] == ECombinator::Child ? " > " : " ";
        const SCompound& c = m_compounds[k];
        std::string part = c.tag;
        if (!c.id.empty()) part += "#" + c.id;
        for (const std::string& cls : c.classes) part += "." + cls;
        for (const std::string& st : c.structural) part += ":" + st;
        for (const std::string& st : c.states) part += ":" + st;
        if (!c.part.empty()) part += "::" + c.part;
        out += part.empty() ? "*" : part;
    }
    return out;
}
