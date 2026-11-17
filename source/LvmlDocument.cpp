#include "LvmlDocument.hpp"
#include "TextUtils.hpp"

CLvmlDocument::CLvmlDocument() : m_root(CLvmlNode::EType::Element)
{
    m_root.SetTag("#document");
}

CLvmlNode* CLvmlDocument::GetHead()
{
    return m_root.FindFirst("head");
}

CLvmlNode* CLvmlDocument::GetBody()
{
    if (CLvmlNode* screen = m_root.FindFirst("screen")) return screen;
    return m_root.FindFirst("body");
}

std::string CLvmlDocument::GetTitle()
{
    CLvmlNode* head  = GetHead();
    CLvmlNode* title = head ? head->FindFirst("title") : nullptr;
    return title ? title->GetInnerText() : std::string();
}

std::vector<std::string> CLvmlDocument::GetStyleSheetLinks()
{
    std::vector<std::string> out;
    CLvmlNode* head = GetHead();
    if (!head) return out;
    for (const auto& c : head->GetChildren()) {
        if (c->IsTag("link") && CTextUtils::IEquals(c->GetAttribute("rel", "stylesheet"), "stylesheet")) {
            const std::string href = c->GetAttribute("href");
            if (!href.empty()) out.push_back(href);
        } else if (c->IsTag("style") && c->HasAttribute("src")) {
            out.push_back(c->GetAttribute("src"));
        }
    }
    return out;
}

std::vector<std::string> CLvmlDocument::GetInlineStyles()
{
    std::vector<std::string> out;
    std::vector<CLvmlNode*>  styles;
    m_root.FindAll("style", styles);
    for (CLvmlNode* s : styles) {
        if (s->HasAttribute("src")) continue;
        std::string body;
        for (const auto& c : s->GetChildren())
            if (c->IsText()) body += c->GetText();
        if (!CTextUtils::Trim(body).empty()) out.push_back(body);
    }
    return out;
}

std::vector<CLvmlDocument::SFontLink> CLvmlDocument::GetFontLinks()
{
    std::vector<SFontLink> out;
    std::vector<CLvmlNode*> fonts;
    m_root.FindAll("font", fonts);
    for (CLvmlNode* f : fonts) {
        SFontLink link{ f->GetAttribute("family"), f->GetAttribute("src") };
        if (!link.family.empty() && !link.src.empty()) out.push_back(std::move(link));
    }
    return out;
}

std::string CLvmlDocument::ToString() const
{
    std::string out;
    for (const auto& c : m_root.GetChildren()) out += c->ToString(0);
    return out;
}
