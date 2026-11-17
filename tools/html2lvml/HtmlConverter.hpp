// lvml — html2lvml: HTML + CSS → .lvml + .lvss.
#pragma once
#include "ConversionReport.hpp"
#include "LvmlDocument.hpp"
#include "LvssStyleSheet.hpp"
#include <string>
#include <unordered_map>
#include <vector>

// Converts a rendered HTML page (a DOM snapshot of an SPA works best: script output is not
// executed here) into an lvml scene.
//
//  * tags keep their names where lvml knows them (div, p, h1, span, a, button, img…), so CSS
//    tag selectors keep matching; form controls map to LVGL widgets;
//  * CSS goes through CCssConverter; <style> and <link rel=stylesheet> are both read;
//  * normal block/inline flow has no LVGL counterpart, so containers in flow get generated
//    flex classes (.lv-col / .lv-row) and block children .lv-fill (width: 100%).
class CHtmlConverter
{
public:
    struct SOptions {
        std::string styleSheetName = "scene.lvss"; // href written into the .lvml
        std::string title;                         // defaults to <title>
        bool        flowLayout = true;
    };

    struct SResult {
        std::string       lvml;
        std::string       lvss;
        CConversionReport report;
    };

    CHtmlConverter() = delete;

    // `htmlDir` resolves <link href> and absolute ("/…") stylesheet paths.
    static bool Convert(const std::string& html, const std::string& htmlDir, const SOptions& options, SResult& out);

private:
    struct SContext {
        const CLvssStyleSheet* css = nullptr; // raw (unconverted) CSS — used for the cascade
        CConversionReport*     report = nullptr;
        const SOptions*        options = nullptr;
        std::unordered_map<const CLvmlNode*, const CLvmlNode*> sources; // output element → input element
    };

    static bool        ReadFile(const std::string& path, std::string& out);
    static void        CollectStyles(CLvmlNode& root, const std::string& htmlDir, CLvssStyleSheet& css, CConversionReport& report);
    static std::string Effective(const CLvmlNode& node, const std::string& property, const SContext& ctx);
    static std::string DisplayOf(const CLvmlNode& node, const SContext& ctx);
    static bool        IsOutOfFlow(const CLvmlNode& node, const SContext& ctx);
    static bool        IsSkipped(const CLvmlNode& node);
    static std::string MapTag(const CLvmlNode& node, SContext& ctx);
    static void        CopyAttributes(const CLvmlNode& in, CLvmlNode& out, const std::string& tag, SContext& ctx);
    static void        ConvertChildren(const CLvmlNode& in, CLvmlNode& out, SContext& ctx);
    static void        ConvertElement(const CLvmlNode& in, CLvmlNode& parentOut, SContext& ctx);
    static void        ApplyFlow(const CLvmlNode& in, CLvmlNode& out, const std::string& tag, SContext& ctx);
};
