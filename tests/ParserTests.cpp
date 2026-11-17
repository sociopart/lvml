#include "ParserTests.hpp"
#include "LvmlParser.hpp"
#include "LvssParser.hpp"
#include "LvssValue.hpp"
#include <cstdio>

int CParserTests::s_failed = 0;
int CParserTests::s_passed = 0;

void CParserTests::Check(bool condition, const std::string& what)
{
    if (condition) { ++s_passed; return; }
    ++s_failed;
    std::printf("FAIL: %s\n", what.c_str());
}

void CParserTests::TestMarkup()
{
    const char* src = R"(<?xml version="1.0"?>
<lvml>
  <head>
    <title>Path</title>
    <link rel="stylesheet" href="../common.lvss"/>
    <font family="inter" src="fonts/Inter.ttf"/>
    <style> #a { width: 10px; } </style>
  </head>
  <!-- comment -->
  <screen class="dark main">
    <obj id="row" checkable checked="true">
      <label id="txt">Hello &amp; <br/>  world&#33;</label>
    </obj>
    <button id="ok" onclick="submit">OK</button>
  </screen>
</lvml>)";
    CLvmlDocument doc;
    std::vector<std::string> errors;
    Check(CLvmlParser::Parse(src, doc, &errors), "lvml parses");
    Check(errors.empty(), "no lvml errors");
    Check(doc.GetTitle() == "Path", "title");
    Check(doc.GetStyleSheetLinks().size() == 1 && doc.GetStyleSheetLinks()[0] == "../common.lvss", "stylesheet link");
    Check(doc.GetInlineStyles().size() == 1, "inline style");
    Check(doc.GetFontLinks().size() == 1 && doc.GetFontLinks()[0].family == "inter", "font link");
    CLvmlNode* body = doc.GetBody();
    Check(body && body->IsTag("screen"), "body is screen");
    Check(body && body->HasClass("dark") && body->HasClass("main"), "classes");
    CLvmlNode* txt = doc.GetRoot().FindById("txt");
    Check(txt && txt->GetInnerText() == "Hello &\nworld!", "inner text: '" + (txt ? txt->GetInnerText() : "") + "'");
    CLvmlNode* row = doc.GetRoot().FindById("row");
    Check(row && row->GetBoolAttribute("checkable") && row->GetBoolAttribute("checked"), "boolean attributes");
    Check(doc.GetRoot().FindById("ok")->GetAttribute("onclick") == "submit", "onclick");

    CLvmlDocument bad;
    errors.clear();
    Check(!CLvmlParser::Parse("<lvml><screen><obj></screen></lvml>", bad, &errors), "strict mode rejects mismatched tags");
    Check(!errors.empty(), "mismatch reported");
}

void CParserTests::TestHtmlMode()
{
    const char* src = R"(<!doctype html><HTML><head><meta charset=utf-8><style>p{color:red}</style>
<script>if (a < b && c > d) {}</script></head>
<body><div class=card id=x><img src="a.png"><p>One<br>Two</p><input type=checkbox checked></div></body></html>)";
    CLvmlDocument doc;
    CLvmlParser::SOptions opt;
    opt.html = true;
    std::vector<std::string> errors;
    Check(CLvmlParser::Parse(src, doc, opt, &errors), "html parses");
    CLvmlNode* x = doc.GetRoot().FindById("x");
    Check(x && x->IsTag("div") && x->HasClass("card"), "unquoted attributes");
    Check(x && x->GetChildren().size() == 3, "void elements do not nest");
    std::vector<CLvmlNode*> scripts;
    doc.GetRoot().FindAll("script", scripts);
    Check(scripts.size() == 1 && scripts[0]->GetChildren().size() == 1 &&
          scripts[0]->GetChildren()[0]->GetText().find("a < b") != std::string::npos, "raw script text");
    CLvmlNode* p = doc.GetRoot().FindFirst("p");
    Check(p && p->GetInnerText() == "One\nTwo", "br in html");
    CLvmlNode* input = doc.GetRoot().FindFirst("input");
    Check(input && input->HasAttribute("checked") && input->GetAttribute("type") == "checkbox", "boolean html attribute");
}

void CParserTests::TestStyleSheet()
{
    const char* src = R"(
@import "base.lvss";
@font-face { font-family: "Inter"; src: url(fonts/Inter.woff2) format("woff2"), url('fonts/Inter.ttf'); }
:root { --accent: #EB2E4A; --pad: 4px; }
/* comment { not a rule } */
#close:hover, .icon::scrollbar { image-recolor: var(--accent); padding: var(--pad) 8px !important; }
@media (max-width: 100px) { a { color: red; } }
screen > obj.row:checked #box { bg-image: url("a b.png"); }
bad[attr] { color: red; }
)";
    CLvssStyleSheet sheet;
    std::vector<std::string> warnings;
    Check(CLvssParser::Parse(src, sheet, &warnings), "lvss parses");
    Check(sheet.GetImports().size() == 1 && sheet.GetImports()[0] == "base.lvss", "import");
    Check(sheet.GetFontFaces().size() == 1 && sheet.GetFontFaces()[0].family == "inter" &&
          sheet.GetFontFaces()[0].src == "fonts/Inter.ttf", "font-face picks ttf");
    Check(sheet.GetVariables().size() == 2, "variables");
    Check(sheet.GetRules().size() == 2, "rules (media and bad selector skipped): " + std::to_string(sheet.GetRules().size()));
    Check(warnings.size() == 2, "two warnings: " + std::to_string(warnings.size()));
    if (sheet.GetRules().size() == 2) {
        const CLvssRule& r = sheet.GetRules()[0];
        Check(r.selectors.size() == 2, "selector list");
        Check(r.declarations.size() == 2 && r.declarations[1].important, "!important");
        Check(sheet.ResolveVariables(r.declarations[0].value) == "#EB2E4A", "var()");
        Check(sheet.ResolveVariables("var(--nope, 1px 2px)") == "1px 2px", "var() fallback");
        Check(r.selectors[0].GetStates().size() == 1 && r.selectors[0].GetStates()[0] == "hovered", "hover state");
        Check(r.selectors[1].GetPart() == "scrollbar", "part");
        Check(sheet.GetRules()[1].selectors[0].HasAncestorStates(), "ancestor state");
    }
    const auto decls = CLvssParser::ParseDeclarations("width: 10px; background: url(a;b.png) ; ;color:#fff");
    Check(decls.size() == 3 && decls[1].value == "url(a;b.png)", "inline declarations");
}

void CParserTests::TestSelectors()
{
    CLvmlDocument doc;
    CLvmlParser::Parse(R"(<lvml><screen><obj id="row" class="a b"><obj id="box"/><label id="l1"/></obj><obj id="z"/></screen></lvml>)", doc);
    CLvmlNode* box = doc.GetRoot().FindById("box");
    CLvmlNode* l1  = doc.GetRoot().FindById("l1");
    CLvmlNode* z   = doc.GetRoot().FindById("z");

    auto sel = [](const char* text) { CLvssSelector s; CLvssSelector::Parse(text, s); return s; };
    Check(sel("#box").Matches(*box), "#id");
    Check(sel("screen #box").Matches(*box), "descendant");
    Check(sel("screen > obj > #box").Matches(*box), "child chain");
    Check(!sel("screen > #box").Matches(*box), "child mismatch");
    Check(sel(".a.b > obj:first-child").Matches(*box), "first-child");
    Check(!sel("obj:first-child").Matches(*l1), "not first-child");
    Check(sel("label:last-child").Matches(*l1), "last-child");
    Check(sel("#z:hover").Matches(*z), "subject state ignored for structure");
    Check(!sel("#row:checked > #box").Matches(*box), "ancestor state without query fails");

    CLvssSelector::StateQuery yes = [](const CLvmlNode&, const std::vector<std::string>&) { return true; };
    CLvssSelector::StateQuery no  = [](const CLvmlNode&, const std::vector<std::string>&) { return false; };
    Check(sel("#row:checked > #box").Matches(*box, &yes), "ancestor state query true");
    Check(!sel("#row:checked > #box").Matches(*box, &no), "ancestor state query false");
    Check(sel("#a").GetSpecificity() > sel(".a.b.c").GetSpecificity(), "specificity id > classes");
    Check(sel(".a:hover").GetSpecificity() > sel(".a").GetSpecificity(), "specificity state");
}

void CParserTests::TestValues()
{
    CLvssValue::SLength len;
    Check(CLvssValue::ParseLength("12.5px", len) && len.value == 12.5f && len.unit == CLvssValue::EUnit::Px, "px");
    Check(CLvssValue::ParseLength("50%", len) && len.unit == CLvssValue::EUnit::Percent, "%");
    Check(CLvssValue::ParseLength("content", len) && len.unit == CLvssValue::EUnit::Content, "content");
    Check(CLvssValue::ParseLength("1.5rem", len) && len.unit == CLvssValue::EUnit::Rem, "rem");
    uint32_t c = 0;
    bool alpha = false;
    Check(CLvssValue::ParseColor("#EB2E4A", c, &alpha) && c == 0xFFEB2E4A && !alpha, "#rrggbb");
    Check(CLvssValue::ParseColor("#00000099", c, &alpha) && c == 0x99000000 && alpha, "#rrggbbaa");
    Check(CLvssValue::ParseColor("#fff", c) && c == 0xFFFFFFFF, "#rgb");
    Check(CLvssValue::ParseColor("rgba(255, 0, 0, 0.5)", c, &alpha) && c == 0x80FF0000 && alpha, "rgba");
    Check(CLvssValue::ParseColor("transparent", c, &alpha) && c == 0 && alpha, "transparent");
    uint8_t o = 0;
    Check(CLvssValue::ParseOpacity("50%", o) && o == 128, "opa %");
    Check(CLvssValue::ParseOpacity("0.6", o) && o == 153, "opa float");
    Check(CLvssValue::ParseOpacity("200", o) && o == 200, "opa int");
    uint32_t ms = 0;
    Check(CLvssValue::ParseTime("0.15s", ms) && ms == 150, "time s");
    Check(CLvssValue::ParseTime("200ms", ms) && ms == 200, "time ms");
    std::string url;
    Check(CLvssValue::ParseUrl("url( \"x/y.png\" )", url) && url == "x/y.png", "url");
}

int CParserTests::Run()
{
    TestMarkup();
    TestHtmlMode();
    TestStyleSheet();
    TestSelectors();
    TestValues();
    std::printf("%d passed, %d failed\n", s_passed, s_failed);
    return s_failed;
}
