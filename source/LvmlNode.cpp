#include "LvmlNode.hpp"
#include "TextUtils.hpp"
#include <algorithm>

CLvmlNode::CLvmlNode(EType type) : m_type(type) {}

bool CLvmlNode::IsTag(std::string_view tag) const
{
    return IsElement() && CTextUtils::IEquals(m_tag, tag);
}

const std::string* CLvmlNode::FindAttribute(std::string_view name) const
{
    for (const Attribute& a : m_attributes)
        if (CTextUtils::IEquals(a.first, name)) return &a.second;
    return nullptr;
}

std::string CLvmlNode::GetAttribute(std::string_view name, std::string_view fallback) const
{
    const std::string* v = FindAttribute(name);
    return v ? *v : std::string(fallback);
}

bool CLvmlNode::GetBoolAttribute(std::string_view name, bool fallback) const
{
    const std::string* v = FindAttribute(name);
    if (!v) return fallback;
    const std::string s = CTextUtils::ToLower(CTextUtils::Trim(*v));
    // <checkbox checked> and checked="checked" are both "true".
    return !(s == "false" || s == "0" || s == "no" || s == "off");
}

void CLvmlNode::SetAttribute(std::string_view name, std::string value)
{
    for (Attribute& a : m_attributes)
        if (CTextUtils::IEquals(a.first, name)) { a.second = std::move(value); return; }
    m_attributes.emplace_back(std::string(name), std::move(value));
}

void CLvmlNode::RemoveAttribute(std::string_view name)
{
    m_attributes.erase(std::remove_if(m_attributes.begin(), m_attributes.end(),
        [&](const Attribute& a) { return CTextUtils::IEquals(a.first, name); }), m_attributes.end());
}

std::vector<std::string> CLvmlNode::GetClasses() const
{
    const std::string* v = FindAttribute("class");
    return v ? CTextUtils::SplitWhitespace(*v) : std::vector<std::string>{};
}

bool CLvmlNode::HasClass(std::string_view name) const
{
    for (const std::string& c : GetClasses())
        if (c == name) return true;
    return false;
}

bool CLvmlNode::AddClass(std::string_view name)
{
    if (name.empty() || HasClass(name)) return false;
    std::string v = GetAttribute("class");
    if (!v.empty()) v += ' ';
    v += name;
    SetAttribute("class", std::move(v));
    return true;
}

bool CLvmlNode::RemoveClass(std::string_view name)
{
    std::vector<std::string> classes = GetClasses();
    const auto it = std::find(classes.begin(), classes.end(), name);
    if (it == classes.end()) return false;
    classes.erase(it);
    std::string v;
    for (const std::string& c : classes) { if (!v.empty()) v += ' '; v += c; }
    if (v.empty()) RemoveAttribute("class");
    else SetAttribute("class", std::move(v));
    return true;
}

CLvmlNode* CLvmlNode::AppendChild(std::unique_ptr<CLvmlNode> child)
{
    child->m_parent = this;
    m_children.push_back(std::move(child));
    return m_children.back().get();
}

CLvmlNode* CLvmlNode::AppendElement(std::string tag)
{
    auto node = std::make_unique<CLvmlNode>(EType::Element);
    node->SetTag(std::move(tag));
    return AppendChild(std::move(node));
}

CLvmlNode* CLvmlNode::AppendText(std::string text)
{
    auto node = std::make_unique<CLvmlNode>(EType::Text);
    node->SetText(std::move(text));
    return AppendChild(std::move(node));
}

std::unique_ptr<CLvmlNode> CLvmlNode::RemoveChild(CLvmlNode* child)
{
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        if (it->get() != child) continue;
        std::unique_ptr<CLvmlNode> owned = std::move(*it);
        m_children.erase(it);
        owned->m_parent = nullptr;
        return owned;
    }
    return nullptr;
}

size_t CLvmlNode::GetIndexInParent() const
{
    if (!m_parent) return 0;
    size_t index = 0;
    for (const auto& c : m_parent->m_children) {
        if (c.get() == this) return index;
        if (c->IsElement()) ++index;
    }
    return index;
}

size_t CLvmlNode::GetElementSiblingCount() const
{
    if (!m_parent) return 1;
    size_t count = 0;
    for (const auto& c : m_parent->m_children)
        if (c->IsElement()) ++count;
    return count;
}

CLvmlNode* CLvmlNode::FindById(std::string_view id)
{
    if (IsElement()) {
        const std::string* v = FindAttribute("id");
        if (v && *v == id) return this;
    }
    for (const auto& c : m_children)
        if (CLvmlNode* hit = c->FindById(id)) return hit;
    return nullptr;
}

CLvmlNode* CLvmlNode::FindFirst(std::string_view tag)
{
    if (IsTag(tag)) return this;
    for (const auto& c : m_children)
        if (CLvmlNode* hit = c->FindFirst(tag)) return hit;
    return nullptr;
}

void CLvmlNode::FindAll(std::string_view tag, std::vector<CLvmlNode*>& out)
{
    if (IsTag(tag)) out.push_back(this);
    for (const auto& c : m_children) c->FindAll(tag, out);
}

bool CLvmlNode::HasElementChildren() const
{
    for (const auto& c : m_children)
        if (c->IsElement()) return true;
    return false;
}

void CLvmlNode::AppendInnerText(std::string& out) const
{
    if (IsText()) { out += CTextUtils::CollapseWhitespace(m_text); return; }
    if (IsTag("br")) { out += '\n'; return; }
    for (const auto& c : m_children) c->AppendInnerText(out);
}

std::string CLvmlNode::GetInnerText() const
{
    std::string raw;
    for (const auto& c : m_children) c->AppendInnerText(raw);

    // Trim spaces around line breaks and at the ends, like a browser does for block text.
    std::string out;
    out.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        const char c = raw[i];
        if (c == ' ' && (out.empty() || out.back() == '\n' || out.back() == ' ')) continue;
        if (c == '\n' && !out.empty() && out.back() == ' ') out.pop_back();
        out += c;
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

std::string CLvmlNode::ToString(int indent) const
{
    const std::string pad(size_t(indent) * 4, ' ');
    if (IsText()) {
        const std::string_view t = CTextUtils::Trim(m_text);
        return t.empty() ? std::string() : pad + CTextUtils::EscapeXml(CTextUtils::CollapseWhitespace(t), false) + "\n";
    }

    std::string out = pad + "<" + m_tag;
    for (const Attribute& a : m_attributes) {
        out += ' ';
        out += a.first;
        out += "=\"";
        out += CTextUtils::EscapeXml(a.second, true);
        out += '"';
    }
    if (m_children.empty()) return out + "/>\n";

    // Short text-only content stays on one line.
    if (m_children.size() == 1 && m_children[0]->IsText()) {
        const std::string text = CTextUtils::CollapseWhitespace(CTextUtils::Trim(m_children[0]->GetText()));
        if (text.size() < 80)
            return out + ">" + CTextUtils::EscapeXml(text, false) + "</" + m_tag + ">\n";
    }

    // <style> keeps its raw body.
    if (IsTag("style")) {
        out += ">\n";
        for (const auto& c : m_children) out += c->GetText();
        return out + "\n" + pad + "</" + m_tag + ">\n";
    }

    out += ">\n";
    for (const auto& c : m_children) out += c->ToString(indent + 1);
    return out + pad + "</" + m_tag + ">\n";
}
