// lvml — LVGL markup & style sheets.
#pragma once
#include <lvgl.h>
#include <string_view>
#include <unordered_map>

// Mouse cursor per object (LVGL has none). Filled from the lvss `cursor` property by CLvmlScene,
// read by the platform host (CLvmlWin32Window) for the object under the pointer.
class CLvmlCursor
{
public:
    enum class ECursor { Default, Pointer, Text, Move, NotAllowed, Wait, Progress, Crosshair, Help };

    CLvmlCursor() = delete;

    // CSS names: pointer, text, move, grab, not-allowed, wait, progress, crosshair, help, default, auto.
    static bool Parse(std::string_view name, ECursor& out);

    static void Set(lv_obj_t* obj, ECursor cursor);
    static void Clear(lv_obj_t* obj);
    // Cursor of `obj` or of its closest ancestor that has one.
    static ECursor Resolve(lv_obj_t* obj);

private:
    static std::unordered_map<const lv_obj_t*, ECursor>& Map();
};
