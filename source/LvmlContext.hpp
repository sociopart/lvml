// lvml — LVGL markup & style sheets.
#pragma once
#include "LvmlFontManager.hpp"
#include "LvmlResources.hpp"
#include "LvssStyleSheet.hpp"
#include <lvgl.h>
#include <functional>
#include <memory>
#include <string>

class CLvmlScene;
class CLvmlDocument;

// Entry point: owns the resource provider, fonts and settings shared by all scenes.
//
//   lv_init();  … create a display …
//   CLvmlContext ui(std::make_shared<CLvmlDirectoryResources>("ui"));
//   ui.SetScale(dpi / 96.0f);
//   auto scene = ui.LoadScene("main/scene.lvml");
//   scene->On("submit", [](CLvmlEvent& e) { … });
class CLvmlContext
{
public:
    explicit CLvmlContext(std::shared_ptr<CLvmlResources> resources, char driveLetter = 'L');
    ~CLvmlContext();
    CLvmlContext(const CLvmlContext&)            = delete;
    CLvmlContext& operator=(const CLvmlContext&) = delete;

    CLvmlResources&   GetResources() { return *m_resources; }
    CLvmlFontManager& GetFonts() { return m_fonts; }

    // Multiplier for px/dp lengths and font sizes (e.g. window DPI / 96).
    void  SetScale(float scale) { m_scale = scale > 0 ? scale : 1.0f; }
    float GetScale() const { return m_scale; }
    int32_t ScalePx(float px) const;

    // Warnings (unknown properties, missing files…). Default: LV_LOG_WARN.
    void SetLogger(std::function<void(const std::string&)> logger) { m_logger = std::move(logger); }
    void Log(const std::string& message) const;

    // Loads a .lvss file and everything it @imports, appended to `into` (imports first).
    bool LoadStyleSheet(const std::string& path, CLvssStyleSheet& into);

    // Builds a scene under `parent` (the active screen if null). Returns null on failure.
    std::unique_ptr<CLvmlScene> LoadScene(const std::string& path, lv_obj_t* parent = nullptr);
    std::unique_ptr<CLvmlScene> LoadSceneFromText(const std::string& text, const std::string& path,
                                                  lv_obj_t* parent = nullptr);

    // Font for a family and a CSS size ("13px", "1.2em", "") — size scaled; empty size → fallbackPx
    // (already scaled) or the default size.
    const lv_font_t* ResolveFont(const std::string& family, const std::string& size, int32_t fallbackPx);
    // Pixel size for a CSS size; em/% are relative to `parentPx`. Empty/invalid → parentPx.
    int32_t ResolveFontPx(const std::string& size, int32_t parentPx);
    // "600 13px/1.4 Inter, sans-serif" → family "Inter, sans-serif", size "13px"
    static void ParseFontShorthand(const std::string& value, std::string& family, std::string& size);

private:
    bool LoadStyleSheetRecursive(const std::string& path, CLvssStyleSheet& into, int depth);
    void RegisterFonts(const CLvssStyleSheet& sheet);

    std::shared_ptr<CLvmlResources>          m_resources;
    CLvmlFontManager                         m_fonts;
    float                                    m_scale = 1.0f;
    std::function<void(const std::string&)>  m_logger;
};
