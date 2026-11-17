// lvml — LVGL markup & style sheets.
#pragma once
#include "LvssDeclaration.hpp"
#include "LvssSelector.hpp"
#include <string>
#include <vector>

// "selector, selector { declarations }"
class CLvssRule
{
public:
    std::vector<CLvssSelector>    selectors;
    std::vector<CLvssDeclaration> declarations;
    size_t                        order = 0; // source order across all merged sheets
    size_t                        line  = 0;
    std::string                   source;    // file the rule came from (base for relative url())

    std::string ToString() const;
};
