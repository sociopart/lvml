#include "LvmlContext.hpp"
#include "LvmlBuilder.hpp"
#include "LvmlFileSystem.hpp"
#include "LvmlParser.hpp"
#include "LvmlScene.hpp"
#include "LvssParser.hpp"
#include "LvssValue.hpp"
#include "TextUtils.hpp"
#include <cmath>

CLvmlContext::CLvmlContext(std::shared_ptr<CLvmlResources> resources, char driveLetter)
    : m_resources(std::move(resources)), m_fonts(m_resources.get())
{
    CLvmlFileSystem::Register(driveLetter, m_resources.get());
}

CLvmlContext::~CLvmlContext() = default;

int32_t CLvmlContext::ScalePx(float px) const
{
    return int32_t(std::lround(px * m_scale));
}

void CLvmlContext::Log(const std::string& message) const
{
    if (m_logger) m_logger(message);
    else LV_LOG_WARN("lvml: %s", message.c_str());
}

bool CLvmlContext::LoadStyleSheet(const std::string& path, CLvssStyleSheet& into)
{
    return LoadStyleSheetRecursive(CLvmlResources::NormalizePath(path), into, 0);
}

bool CLvmlContext::LoadStyleSheetRecursive(const std::string& path, CLvssStyleSheet& into, int depth)
{
    if (depth > 8) { Log(path + ": @import nesting too deep"); return false; }
    std::string text;
    if (!m_resources->ReadText(path, text)) { Log(path + ": not found"); return false; }

    CLvssStyleSheet sheet;
    sheet.SetPath(path);
    std::vector<std::string> warnings;
    const bool ok = CLvssParser::Parse(text, sheet, &warnings);
    for (const std::string& w : warnings) Log(path + ": " + w);

    for (const std::string& import : sheet.GetImports())
        LoadStyleSheetRecursive(CLvmlResources::ResolvePath(path, import), into, depth + 1);
    into.Append(sheet);
    return ok;
}

void CLvmlContext::RegisterFonts(const CLvssStyleSheet& sheet)
{
    for (const CLvssStyleSheet::SFontFace& f : sheet.GetFontFaces())
        m_fonts.RegisterFamily(f.family, CLvmlResources::ResolvePath(f.basePath, f.src));
}

void CLvmlContext::ParseFontShorthand(const std::string& value, std::string& family, std::string& size)
{
    family.clear();
    size.clear();
    const std::vector<std::string> words = CTextUtils::SplitWhitespace(value);
    size_t i = 0;
    for (; i < words.size(); ++i) {
        std::string w = words[i];
        const size_t slash = w.find('/'); // 13px/1.4
        if (slash != std::string::npos) w = w.substr(0, slash);
        CLvssValue::SLength len;
        const bool hasUnit = w.find_first_of("abcdefghijklmnopqrstuvwxyz%") != std::string::npos;
        if (hasUnit && CLvssValue::ParseLength(w, len) && len.unit != CLvssValue::EUnit::Auto &&
            len.unit != CLvssValue::EUnit::Content) {
            size = w;
            ++i;
            break;
        }
    }
    for (; i < words.size(); ++i) {
        if (!family.empty()) family += ' ';
        family += words[i];
    }
}

int32_t CLvmlContext::ResolveFontPx(const std::string& size, int32_t parentPx)
{
    int32_t px = parentPx > 0 ? parentPx : ScalePx(float(m_fonts.GetDefaultSize()));
    if (size.empty()) return px;
    CLvssValue::SLength len;
    if (!CLvssValue::ParseLength(size, len)) {
        Log("bad font size '" + size + "'");
        return px;
    }
    switch (len.unit) {
    case CLvssValue::EUnit::Px:      return ScalePx(len.value);
    case CLvssValue::EUnit::Rem:     return ScalePx(len.value * 16.0f);
    case CLvssValue::EUnit::Em:      return int32_t(std::lround(len.value * float(px)));
    case CLvssValue::EUnit::Percent: return int32_t(std::lround(len.value * float(px) / 100.0f));
    default:                         return px;
    }
}

const lv_font_t* CLvmlContext::ResolveFont(const std::string& family, const std::string& size, int32_t fallbackPx)
{
    return m_fonts.GetFont(family, ResolveFontPx(size, fallbackPx));
}

std::unique_ptr<CLvmlScene> CLvmlContext::LoadScene(const std::string& path, lv_obj_t* parent)
{
    std::string text;
    const std::string normalized = CLvmlResources::NormalizePath(path);
    if (!m_resources->ReadText(normalized, text)) { Log(normalized + ": not found"); return nullptr; }
    return LoadSceneFromText(text, normalized, parent);
}

std::unique_ptr<CLvmlScene> CLvmlContext::LoadSceneFromText(const std::string& text, const std::string& path, lv_obj_t* parent)
{
    auto doc = std::make_unique<CLvmlDocument>();
    doc->SetPath(path);
    std::vector<std::string> errors;
    const bool ok = CLvmlParser::Parse(text, *doc, &errors);
    for (const std::string& e : errors) Log(path + ": " + e);
    if (!ok) return nullptr;

    CLvssStyleSheet sheet;
    sheet.SetPath(path);
    for (const std::string& href : doc->GetStyleSheetLinks())
        LoadStyleSheetRecursive(CLvmlResources::ResolvePath(path, href), sheet, 0);
    for (const std::string& inlineStyle : doc->GetInlineStyles()) {
        CLvssStyleSheet part;
        part.SetPath(path);
        std::vector<std::string> warnings;
        CLvssParser::Parse(inlineStyle, part, &warnings);
        for (const std::string& w : warnings) Log(path + ": <style>: " + w);
        for (const std::string& import : part.GetImports())
            LoadStyleSheetRecursive(CLvmlResources::ResolvePath(path, import), sheet, 1);
        sheet.Append(part);
    }
    for (const CLvmlDocument::SFontLink& f : doc->GetFontLinks())
        m_fonts.RegisterFamily(f.family, CLvmlResources::ResolvePath(path, f.src));
    RegisterFonts(sheet);

    return CLvmlBuilder::Build(*this, std::move(doc), std::move(sheet), parent);
}
