// lvml — LVGL markup & style sheets.
#pragma once
#include "LvmlResources.hpp"
#include <lvgl.h>
#include <string>
#include <string_view>

// Exposes a CLvmlResources to LVGL as a drive letter, so image sources like "L:img/a.png"
// are read through it (PNG decoding is done by LVGL's lodepng decoder).
class CLvmlFileSystem
{
public:
    CLvmlFileSystem() = delete;

    // Registers (once) or re-targets the driver. Call after lv_init().
    static void Register(char letter, CLvmlResources* resources);
    static char GetLetter() { return s_letter; }
    static std::string MakePath(std::string_view resourcePath); // "L:" + path

private:
    struct SFile {
        std::vector<uint8_t>        owned;
        const std::vector<uint8_t>* data = nullptr;
        uint32_t                    pos  = 0;
    };

    static bool        Ready(lv_fs_drv_t* drv);
    static void*       Open(lv_fs_drv_t* drv, const char* path, lv_fs_mode_t mode);
    static lv_fs_res_t Close(lv_fs_drv_t* drv, void* file);
    static lv_fs_res_t Read(lv_fs_drv_t* drv, void* file, void* buf, uint32_t btr, uint32_t* br);
    static lv_fs_res_t Seek(lv_fs_drv_t* drv, void* file, uint32_t pos, lv_fs_whence_t whence);
    static lv_fs_res_t Tell(lv_fs_drv_t* drv, void* file, uint32_t* pos);

    static lv_fs_drv_t     s_driver;
    static CLvmlResources* s_resources;
    static char            s_letter;
    static bool            s_registered;
};
