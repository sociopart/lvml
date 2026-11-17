// lvml — headless renderer: .lvml → .bmp (for previews and visual regression checks).
#pragma once
#include <lvgl.h>
#include <string>

class CSceneRenderer
{
public:
    CSceneRenderer() = delete;

    // Renders `scenePath` (relative to `root`) at width×height and writes a 32-bit BMP.
    // `clickId` (optional) sends a click to that element before rendering.
    static int Run(const std::string& root, const std::string& scenePath, const std::string& outPath, int width,
                   int height, float scale, const std::string& clickId, int treeDepth = 0);

private:
    static void FlushNoop(lv_display_t* display, const lv_area_t* area, uint8_t* px);
    static void DumpTree(class CLvmlScene& scene, lv_obj_t* obj, int depth, int maxDepth);
    static bool WriteBmp(const std::string& path, const void* argb, int width, int height, int stride);
};
