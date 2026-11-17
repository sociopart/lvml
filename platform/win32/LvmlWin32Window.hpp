// lvml — optional Win32 host for LVGL's Windows driver (LV_USE_WINDOWS).
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <lvgl.h>
#include <atomic>

// Frameless LVGL window: caption drag area, mouse-wheel scrolling of the scrollable object under
// the cursor, sleep-until-input main loop, rounded corners.
//
//   lv_init();
//   CLvmlWin32Window window;
//   window.Create(L"App", 1000, 600);
//   CLvmlContext ui(…); ui.SetScale(window.GetScale());
//   while (!window.IsCloseRequested()) {
//       lv_lock(); window.Pump(); /* update UI */ lv_unlock();
//       window.Wait(lv_timer_handler());
//   }
//
// The window belongs to the driver's thread: its handlers take lv_lock, so from the main loop
// (while holding lv_lock) only post to it — never block on it (modal dialogs: release the lock first).
class CLvmlWin32Window
{
public:
    CLvmlWin32Window() = default;
    ~CLvmlWin32Window();
    CLvmlWin32Window(const CLvmlWin32Window&)            = delete;
    CLvmlWin32Window& operator=(const CLvmlWin32Window&) = delete;

    // `width`/`height` are logical (96 DPI) pixels; the display is created at physical resolution.
    bool Create(const wchar_t* title, int width, int height);

    lv_display_t* GetDisplay() const { return m_display; }
    HWND          GetHwnd() const { return m_hwnd; }
    int           GetDpi() const { return m_dpi; }
    float         GetScale() const { return float(m_dpi) / 96.0f; } // pass to CLvmlContext::SetScale

    // Drag area: the top `height` logical px, minus `rightExclude` logical px at the right (buttons).
    void SetCaption(int height, int rightExclude);
    // Rounded window corners (Windows 11 DWM; region fallback with `fallbackRadius` logical px).
    void SetRoundedCorners(int fallbackRadius);
    void SetIcon(HICON bigIcon, HICON smallIcon);

    void Show();
    void Minimize();
    void BringToFront();
    void RequestClose();
    bool IsCloseRequested() const { return m_close; }
    // Alt+F4 / taskbar close: true once per press since the last call — the app decides what to do.
    bool TakeClosePress() { return m_closePressed.exchange(false); }

    // Call under lv_lock() every iteration: wheel scrolling, redraw after restore, idle input pause.
    void Pump();
    // Sleeps until input, Wake() or `ms` elapsed (capped at 1 s).
    void Wait(uint32_t ms);
    // Thread-safe: wakes the main loop (e.g. from a worker thread after changing shared state).
    static void Wake();

private:
    static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static int              QueryDpi(HWND hwnd);
    static lv_obj_t*        ObjectAt(lv_obj_t* obj, const lv_point_t& p);
    static void             AnimScroll(void* obj, int32_t y);
    void                    ScrollAt(POINT p, int delta);
    int                     Px(int logical) const { return MulDiv(logical, m_dpi, 96); }

    static CLvmlWin32Window* s_self;
    static HANDLE            s_wakeEvent;

    lv_display_t*      m_display  = nullptr;
    lv_indev_t*        m_pointer  = nullptr;
    HWND               m_hwnd     = nullptr;
    WNDPROC            m_original = nullptr;
    int                m_dpi = 96, m_width = 0, m_height = 0;
    int                m_captionHeight = 0, m_captionExclude = 0;
    std::atomic<bool>  m_close{ false }, m_closePressed{ false }, m_redrawAll{ false };
    std::atomic<DWORD> m_lastInput{ 0 };
    std::atomic<int>   m_wheel{ 0 };
    std::atomic<LONG>  m_wheelX{ 0 }, m_wheelY{ 0 };
    bool               m_pointerPaused = false;
    lv_obj_t*          m_scrollObj     = nullptr;
    int32_t            m_scrollTarget  = 0;
};
