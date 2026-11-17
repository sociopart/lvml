#include "LvssRule.hpp"

std::string CLvssRule::ToString() const
{
    std::string out;
    for (size_t i = 0; i < selectors.size(); ++i) {
        if (i) out += ",\n";
        out += selectors[i].ToString();
    }
    out += " {\n";
    for (const CLvssDeclaration& d : declarations) out += "    " + d.ToString() + "\n";
    out += "}\n";
    return out;
}
