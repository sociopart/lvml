// lvml — LVGL markup & style sheets.
#pragma once
#include "LvmlNode.hpp"
#include <string>
#include <vector>

// A parsed .lvml file.
//
//   <lvml>
//     <head>
//       <title>…</title>
//       <link rel="stylesheet" href="common.lvss"/>   (or <style src="…"/>)
//       <font family="inter" src="fonts/Inter.ttf"/>
//       <style> /* inline lvss */ </style>
//     </head>
//     <screen> … widgets … </screen>                  (<body> is accepted too)
//   </lvml>
class CLvmlDocument
{
public:
    struct SFontLink {
        std::string family;
        std::string src;
    };

    CLvmlDocument();

    // Synthetic "#document" element that holds the top-level nodes.
    CLvmlNode&       GetRoot()       { return m_root; }
    const CLvmlNode& GetRoot() const { return m_root; }

    CLvmlNode* GetHead();
    CLvmlNode* GetBody(); // <screen> or <body>

    std::string              GetTitle();
    std::vector<std::string> GetStyleSheetLinks(); // hrefs in document order
    std::vector<std::string> GetInlineStyles();    // <style> bodies in document order
    std::vector<SFontLink>   GetFontLinks();

    const std::string& GetPath() const { return m_path; }
    void               SetPath(std::string path) { m_path = std::move(path); }

    std::string ToString() const;

private:
    CLvmlNode   m_root;
    std::string m_path;
};
