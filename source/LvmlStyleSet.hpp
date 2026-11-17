// lvml — LVGL markup & style sheets.
#pragma once
#include "LvmlNode.hpp"
#include "LvssProperty.hpp"
#include "LvssStyleSheet.hpp"
#include <lvgl.h>
#include <string>
#include <vector>

class CLvmlContext;
class CLvmlStorage;

// A style sheet compiled for LVGL: one lv_style_t per rule, one entry per selector.
class CLvmlStyleSet
{
public:
    struct SEntry {
        const CLvssRule*     rule        = nullptr;
        const CLvssSelector* selector    = nullptr;
        lv_style_t*          style       = nullptr; // null if the rule has no Style declarations
        lv_style_selector_t  lvSelector  = 0;       // part | state
        uint32_t             specificity = 0;
        size_t               order       = 0;
        bool                 isDefault   = true;    // main part, no state: its Object/Font declarations apply
        bool                 needsInput  = false;   // :hover / :pressed — the object must be clickable
    };

    CLvmlStyleSet(CLvmlContext& context, CLvmlStorage& storage, CLvssStyleSheet sheet);
    CLvmlStyleSet(const CLvmlStyleSet&)            = delete;
    CLvmlStyleSet& operator=(const CLvmlStyleSet&) = delete;

    // Matching entries in cascade order (low → high priority).
    void Match(const CLvmlNode& node, const CLvssSelector::StateQuery* query, std::vector<const SEntry*>& out) const;

    bool HasAncestorStateRules() const { return m_hasAncestorStates; }
    // True if some "ancestor:state … node" rule could apply to `node`.
    bool DependsOnAncestorState(const CLvmlNode& node) const;

    const CLvssStyleSheet& GetSheet() const { return m_sheet; }
    CLvssProperty::SEnv    MakeEnv(const std::string& basePath, const std::vector<CLvssDeclaration>* siblings) const;

    static lv_style_selector_t ToLvSelector(const CLvssSelector& selector);
    static lv_state_t          StateFromName(const std::string& name);
    static lv_part_t           PartFromName(const std::string& name);

private:
    void Compile();

    CLvmlContext&       m_context;
    CLvmlStorage&       m_storage;
    CLvssStyleSheet     m_sheet;
    std::vector<SEntry> m_entries;
    bool                m_hasAncestorStates = false;
};
