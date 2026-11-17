#include "LvmlStyleSet.hpp"
#include "LvmlContext.hpp"
#include "LvmlStorage.hpp"
#include "TextUtils.hpp"
#include <algorithm>

CLvmlStyleSet::CLvmlStyleSet(CLvmlContext& context, CLvmlStorage& storage, CLvssStyleSheet sheet)
    : m_context(context), m_storage(storage), m_sheet(std::move(sheet))
{
    Compile();
}

lv_state_t CLvmlStyleSet::StateFromName(const std::string& name)
{
    if (name == "hovered") return LV_STATE_HOVERED;
    if (name == "pressed") return LV_STATE_PRESSED;
    if (name == "checked") return LV_STATE_CHECKED;
    if (name == "focused") return LV_STATE_FOCUSED;
    if (name == "focus-key") return LV_STATE_FOCUS_KEY;
    if (name == "edited") return LV_STATE_EDITED;
    if (name == "disabled") return LV_STATE_DISABLED;
    if (name == "scrolled") return LV_STATE_SCROLLED;
    if (name == "alt") return LV_STATE_ALT;
    if (name == "user1") return LV_STATE_USER_1;
    if (name == "user2") return LV_STATE_USER_2;
    if (name == "user3") return LV_STATE_USER_3;
    if (name == "user4") return LV_STATE_USER_4;
    return LV_STATE_DEFAULT;
}

lv_part_t CLvmlStyleSet::PartFromName(const std::string& name)
{
    if (name == "scrollbar") return LV_PART_SCROLLBAR;
    if (name == "indicator") return LV_PART_INDICATOR;
    if (name == "knob") return LV_PART_KNOB;
    if (name == "selected") return LV_PART_SELECTED;
    if (name == "items") return LV_PART_ITEMS;
    if (name == "cursor") return LV_PART_CURSOR;
    return LV_PART_MAIN;
}

lv_style_selector_t CLvmlStyleSet::ToLvSelector(const CLvssSelector& selector)
{
    lv_style_selector_t sel = PartFromName(selector.GetPart());
    for (const std::string& s : selector.GetStates()) sel |= StateFromName(s);
    return sel;
}

CLvssProperty::SEnv CLvmlStyleSet::MakeEnv(const std::string& basePath, const std::vector<CLvssDeclaration>* siblings) const
{
    CLvssProperty::SEnv env;
    env.context  = &m_context;
    env.storage  = &m_storage;
    env.sheet    = &m_sheet;
    env.basePath = basePath;
    env.siblings = siblings;
    return env;
}

void CLvmlStyleSet::Compile()
{
    for (const CLvssRule& rule : m_sheet.GetRules()) {
        bool anyNonDefault = false;
        for (const CLvssSelector& sel : rule.selectors)
            if (!sel.GetStates().empty() || !sel.GetPart().empty()) anyNonDefault = true;

        lv_style_t* style = nullptr;
        auto ensureStyle = [&]() {
            if (!style) style = m_storage.NewStyle();
            return style;
        };
        const CLvssProperty::SEnv env = MakeEnv(rule.source, &rule.declarations);

        std::string fontFamily, fontSize;
        for (const CLvssDeclaration& d : rule.declarations) {
            const CLvssProperty::EKind kind = CLvssProperty::Classify(d.property);
            const std::string where = (rule.source.empty() ? std::string("<inline>") : rule.source) + ":" +
                                      std::to_string(d.line) + ": " + d.property + ": ";
            switch (kind) {
            case CLvssProperty::EKind::Style: {
                CLvssProperty::CStyleSink sink(ensureStyle());
                std::string error;
                if (!CLvssProperty::Apply(d, env, sink, &error) || !error.empty()) m_context.Log(where + error);
                break;
            }
            case CLvssProperty::EKind::Object:
                if (d.property == "display") { // the layout half of display:flex
                    CLvssProperty::CStyleSink sink(ensureStyle());
                    CLvssProperty::Apply(d, env, sink, nullptr);
                }
                break;
            case CLvssProperty::EKind::Font:
                if (anyNonDefault) {
                    if (d.property == "font-family") fontFamily = m_sheet.ResolveVariables(d.value);
                    else if (d.property == "font-size") fontSize = m_sheet.ResolveVariables(d.value);
                    else {
                        std::string fam, size;
                        CLvmlContext::ParseFontShorthand(m_sheet.ResolveVariables(d.value), fam, size);
                        if (!fam.empty()) fontFamily = fam;
                        if (!size.empty()) fontSize = size;
                    }
                }
                break;
            case CLvssProperty::EKind::Ignored:
                break;
            case CLvssProperty::EKind::Unknown:
                m_context.Log(where + "unknown property");
                break;
            }
        }
        // State/part rules carry their own font (default rules get theirs per object, with inheritance).
        if (anyNonDefault && (!fontFamily.empty() || !fontSize.empty())) {
            const lv_font_t* font = m_context.ResolveFont(fontFamily, fontSize, 0);
            lv_style_set_text_font(ensureStyle(), font);
        }

        for (const CLvssSelector& sel : rule.selectors) {
            SEntry e;
            e.rule        = &rule;
            e.selector    = &sel;
            e.style       = style;
            e.lvSelector  = ToLvSelector(sel);
            e.specificity = sel.GetSpecificity();
            e.order       = rule.order;
            e.isDefault   = sel.GetStates().empty() && sel.GetPart().empty();
            for (const std::string& s : sel.GetStates())
                if (s == "hovered" || s == "pressed") e.needsInput = true;
            if (sel.HasAncestorStates()) m_hasAncestorStates = true;
            m_entries.push_back(e);
        }
    }
}

void CLvmlStyleSet::Match(const CLvmlNode& node, const CLvssSelector::StateQuery* query, std::vector<const SEntry*>& out) const
{
    out.clear();
    for (const SEntry& e : m_entries)
        if (e.selector->Matches(node, query)) out.push_back(&e);
    std::stable_sort(out.begin(), out.end(), [](const SEntry* a, const SEntry* b) {
        if (a->specificity != b->specificity) return a->specificity < b->specificity;
        return a->order < b->order;
    });
}

bool CLvmlStyleSet::DependsOnAncestorState(const CLvmlNode& node) const
{
    if (!m_hasAncestorStates) return false;
    const CLvssSelector::StateQuery any = [](const CLvmlNode&, const std::vector<std::string>&) { return true; };
    for (const SEntry& e : m_entries)
        if (e.selector->HasAncestorStates() && e.selector->Matches(node, &any)) return true;
    return false;
}
