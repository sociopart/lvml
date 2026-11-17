// lvml — LVGL markup & style sheets.
#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// One node of a parsed .lvml (or HTML) tree: an element with attributes and children, or a text run.
class CLvmlNode
{
public:
    enum class EType { Element, Text };
    using Attribute = std::pair<std::string, std::string>;

    explicit CLvmlNode(EType type);

    EType GetType() const   { return m_type; }
    bool  IsElement() const { return m_type == EType::Element; }
    bool  IsText() const    { return m_type == EType::Text; }

    const std::string& GetTag() const { return m_tag; }
    void               SetTag(std::string tag) { m_tag = std::move(tag); }
    bool               IsTag(std::string_view tag) const;

    // Text of a text node (entities already decoded).
    const std::string& GetText() const { return m_text; }
    void               SetText(std::string text) { m_text = std::move(text); }

    size_t GetLine() const { return m_line; }
    void   SetLine(size_t line) { m_line = line; }

    // --- attributes ---
    const std::vector<Attribute>& GetAttributes() const { return m_attributes; }
    const std::string*            FindAttribute(std::string_view name) const;
    bool                          HasAttribute(std::string_view name) const { return FindAttribute(name) != nullptr; }
    std::string                   GetAttribute(std::string_view name, std::string_view fallback = {}) const;
    bool                          GetBoolAttribute(std::string_view name, bool fallback = false) const;
    void                          SetAttribute(std::string_view name, std::string value);
    void                          RemoveAttribute(std::string_view name);

    std::string              GetId() const { return GetAttribute("id"); }
    std::vector<std::string> GetClasses() const;
    bool                     HasClass(std::string_view name) const;
    bool                     AddClass(std::string_view name);
    bool                     RemoveClass(std::string_view name);

    // --- tree ---
    CLvmlNode*                                     GetParent() const { return m_parent; }
    const std::vector<std::unique_ptr<CLvmlNode>>& GetChildren() const { return m_children; }
    CLvmlNode*                                     AppendChild(std::unique_ptr<CLvmlNode> child);
    CLvmlNode*                                     AppendElement(std::string tag);
    CLvmlNode*                                     AppendText(std::string text);
    std::unique_ptr<CLvmlNode>                     RemoveChild(CLvmlNode* child);
    void                                           ClearChildren() { m_children.clear(); }

    size_t     GetIndexInParent() const;       // among element siblings
    size_t     GetElementSiblingCount() const; // element siblings including this one
    CLvmlNode* FindById(std::string_view id);
    CLvmlNode* FindFirst(std::string_view tag);
    void       FindAll(std::string_view tag, std::vector<CLvmlNode*>& out);
    bool       HasElementChildren() const;

    // Text content with HTML whitespace rules: runs collapse, <br> becomes '\n'.
    std::string GetInnerText() const;

    // Serialises back to markup.
    std::string ToString(int indent = 0) const;

private:
    void AppendInnerText(std::string& out) const;

    EType                                   m_type;
    std::string                             m_tag;
    std::string                             m_text;
    std::vector<Attribute>                  m_attributes;
    std::vector<std::unique_ptr<CLvmlNode>> m_children;
    CLvmlNode*                              m_parent = nullptr;
    size_t                                  m_line   = 0;
};
