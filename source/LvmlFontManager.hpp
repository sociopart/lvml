// lvml — LVGL markup & style sheets.
#pragma once
#include "LvmlResources.hpp"
#include <lvgl.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Font families → lv_font_t per pixel size. TTF/OTF families are rendered with tiny_ttf
// (LV_USE_TINY_TTF) and created lazily; built-in bitmap fonts can be registered per size.
class CLvmlFontManager
{
public:
    explicit CLvmlFontManager(CLvmlResources* resources);
    ~CLvmlFontManager();
    CLvmlFontManager(const CLvmlFontManager&)            = delete;
    CLvmlFontManager& operator=(const CLvmlFontManager&) = delete;

    // Family from a resource file (loaded on first use). Re-registering the same family is a no-op.
    void RegisterFamily(const std::string& family, const std::string& path);
    void RegisterFamilyData(const std::string& family, std::vector<uint8_t> data);
    void RegisterStatic(const std::string& family, int32_t pixelSize, const lv_font_t* font);
    bool HasFamily(const std::string& family) const;

    // Falls back to the default family, then to LV_FONT_DEFAULT. Never returns null.
    const lv_font_t* GetFont(const std::string& family, int32_t pixelSize);
    // Pixel size a font returned by GetFont() was created for (0 if unknown).
    int32_t GetPixelSize(const lv_font_t* font) const;

    void               SetDefaultFamily(const std::string& family);
    const std::string& GetDefaultFamily() const { return m_defaultFamily; }
    void               SetDefaultSize(int32_t pixelSize) { m_defaultSize = pixelSize; }
    int32_t            GetDefaultSize() const { return m_defaultSize; }
    // Font used for glyphs missing in every family (e.g. an icon font).
    void SetFallbackFamily(const std::string& family) { m_fallbackFamily = Key(family); }

private:
    struct SFamily {
        std::string                         path;
        std::vector<uint8_t>                data;
        bool                                loaded = false;
        std::map<int32_t, lv_font_t*>       ttf;     // owned
        std::map<int32_t, const lv_font_t*> statics; // not owned
    };

    static std::string Key(const std::string& family);
    SFamily*           Find(const std::string& family);
    const lv_font_t*   Create(SFamily& family, const std::string& key, int32_t pixelSize);

    CLvmlResources*                       m_resources;
    std::map<std::string, SFamily>        m_families;
    std::map<const lv_font_t*, int32_t>   m_sizes;
    std::string                           m_defaultFamily;
    std::string                           m_fallbackFamily;
    int32_t                               m_defaultSize = 14;
    bool                                  m_inFallback  = false;
};
