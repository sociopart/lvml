// lvml — LVGL markup & style sheets.
#pragma once
#include <string>

// "property: value [!important]"
class CLvssDeclaration
{
public:
    std::string property; // lower-case
    std::string value;
    bool        important = false;
    size_t      line      = 0;

    std::string ToString() const { return property + ": " + value + (important ? " !important" : "") + ";"; }
};
