// lvml — LVGL markup & style sheets.
#pragma once
#include "LvssRule.hpp"
#include <map>
#include <string>
#include <string_view>
#include <vector>

// A parsed .lvss file (or several merged ones).
//
//   @import "common.lvss";
//   @font-face { font-family: inter; src: url(fonts/Inter.ttf); }
//   :root { --accent: #EB2E4A; }
//   #close:hover { image-recolor: var(--accent); }
class CLvssStyleSheet
{
public:
    struct SFontFace {
        std::string family;
        std::string src;
        std::string basePath; // file the rule came from, for relative src
    };

    const std::vector<CLvssRule>&   GetRules() const { return m_rules; }
    const std::vector<SFontFace>&   GetFontFaces() const { return m_fontFaces; }
    const std::vector<std::string>& GetImports() const { return m_imports; }
    const std::string&              GetPath() const { return m_path; }
    void                            SetPath(std::string path) { m_path = std::move(path); }

    void AddRule(CLvssRule rule);
    void AddFontFace(SFontFace face) { m_fontFaces.push_back(std::move(face)); }
    void AddImport(std::string path) { m_imports.push_back(std::move(path)); }
    void SetVariable(const std::string& name, std::string value) { m_variables[name] = std::move(value); }

    const std::map<std::string, std::string>& GetVariables() const { return m_variables; }

    // Appends rules/fonts/variables of `other` after the current ones (imports are not copied).
    void Append(const CLvssStyleSheet& other);

    // Substitutes var(--name[, fallback]) recursively.
    std::string ResolveVariables(std::string_view value) const;

    std::string ToString() const;

private:
    std::vector<CLvssRule>             m_rules;
    std::vector<SFontFace>             m_fontFaces;
    std::vector<std::string>           m_imports;
    std::map<std::string, std::string> m_variables;
    std::string                        m_path;
    size_t                             m_nextOrder = 0;
};
