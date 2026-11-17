// lvml — LVGL markup & style sheets.
#pragma once
#include "LvmlNode.hpp"
#include <functional>
#include <string>
#include <string_view>
#include <vector>

// A complex selector: compounds joined by descendant (" ") or child (">") combinators.
//
//   tag  #id  .class  *                      — structure, matched against the node tree
//   :first-child :last-child :only-child :root — structural pseudo-classes
//   :hover :pressed :checked :focused …       — LVGL states
//   ::scrollbar ::indicator ::knob …          — LVGL parts (subject compound only)
//
// States on the subject compound become the LVGL style selector (LVGL switches them itself).
// States on an ancestor compound ("#row:checked > #box") are evaluated through a callback
// against the live object state, and the scene restyles the subtree when that state changes.
class CLvssSelector
{
public:
    enum class ECombinator { Descendant, Child };

    struct SCompound {
        std::string              tag;     // empty or "*" = any
        std::string              id;
        std::vector<std::string> classes;
        std::vector<std::string> states;     // canonical LVGL state names
        std::vector<std::string> structural; // first-child, last-child, only-child, root
        std::string              part;       // canonical LVGL part name, subject only
    };

    // Asked for ancestor compounds that carry states: does `node` currently have all `states`?
    using StateQuery = std::function<bool(const CLvmlNode& node, const std::vector<std::string>& states)>;

    static bool Parse(std::string_view text, CLvssSelector& out, std::string* error = nullptr);

    bool Matches(const CLvmlNode& node, const StateQuery* query = nullptr) const;

    // a*10000 + b*100 + c  (ids, classes/pseudo, tags)
    uint32_t GetSpecificity() const;

    const std::vector<SCompound>&   GetCompounds() const { return m_compounds; }
    const SCompound&                GetSubject() const { return m_compounds.back(); }
    const std::vector<std::string>& GetStates() const { return GetSubject().states; }
    const std::string&              GetPart() const { return GetSubject().part; }
    bool                            HasAncestorStates() const;

    std::string ToString() const;

    static const char* CanonicalState(std::string_view name); // nullptr if not a state
    static const char* CanonicalPart(std::string_view name);  // nullptr if not a part

private:
    static bool MatchCompound(const SCompound& c, const CLvmlNode& node, const StateQuery* query);
    bool        MatchFrom(size_t index, const CLvmlNode& node, const StateQuery* query) const;

    std::vector<SCompound>   m_compounds;
    std::vector<ECombinator> m_combinators; // m_combinators[i] joins compounds i and i+1
};
