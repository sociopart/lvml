// lvml — html2lvml: CSS → LVSS.
#pragma once
#include "ConversionReport.hpp"
#include "LvssStyleSheet.hpp"
#include <map>
#include <string>
#include <vector>

// Rewrites CSS declarations into the LVSS subset LVGL can express.
class CCssConverter
{
public:
    CCssConverter() = delete;

    // One rule's (or one style="" attribute's) declarations.
    static std::vector<CLvssDeclaration> ConvertDeclarations(const std::vector<CLvssDeclaration>& in,
                                                             CConversionReport& report);
    // Every rule of a sheet; variables and font faces are kept.
    static CLvssStyleSheet ConvertStyleSheet(const CLvssStyleSheet& in, CConversionReport& report);

    // .svg / .webp / .gif (no LVGL decoder in the default build) → .png; the report lists them.
    static std::string RewriteAssetUrl(const std::string& url, CConversionReport& report);
    static std::string RewriteUrls(const std::string& value, CConversionReport& report);

private:
    static bool IsPassThrough(const std::string& property);
    static bool IsSilentDrop(const std::string& property);
    static void ConvertPosition(std::vector<CLvssDeclaration>& out, const std::map<std::string, std::string>& box,
                                bool absolute, CConversionReport& report);
};
