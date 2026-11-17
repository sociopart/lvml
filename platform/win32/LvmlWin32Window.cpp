#include "LvmlWin32Window.hpp"
#include <dwmapi.h>
#include <windowsx.h>
#include <algorithm>

CLvmlWin32Window* CLvmlWin32Window::s_self      = nullptr;
HANDLE            CLvmlWin32Window::s_wakeEvent = nullptr;

static constexpr UINT kLvmlShowMessage   = WM_APP + 0x4C;
static constexpr UINT kLvmlCursorMessage = WM_APP + 0x4D;

CLvmlWin32Window::~CLvmlWin32Window()
{
    if (s_self == this) s_self = nullptr;
}

bool CLvmlWin32Window::Create(const wchar_t* title, int width, int height)
{
    s_self = this;
    if (!s_wakeEvent) s_wakeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    m_width  = width;
    m_height = height;

    // Application mode: LVGL renders at the window's physical resolution (crisp at 125–200 %).
    m_display = lv_windows_create_display(title, width, height, 100, false, false);
    if (!m_display) return false;
    m_hwnd    = lv_windows_get_display_window_handle(m_display);
    m_pointer = lv_windows_acquire_pointer_indev(m_display);
    m_dpi     = QueryDpi(m_hwnd);

    ShowWindow(m_hwnd, SW_HIDE);
    SetWindowLongPtrW(m_hwnd, GWL_STYLE, WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN);
    SetWindowLongPtrW(m_hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
    m_original = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SubclassProc)));

    const int w = Px(width), h = Px(height);
    RECT work{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    SetWindowPos(m_hwnd, nullptr, work.left + (work.right - work.left - w) / 2, work.top + (work.bottom - work.top - h) / 2,
                 w, h, SWP_NOZORDER | SWP_FRAMECHANGED);
    const BOOL dark = TRUE;
    DwmSetWindowAttribute(m_hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &dark, sizeof dark);
    return true;
}

int CLvmlWin32Window::QueryDpi(HWND hwnd)
{
    // GetDpiForWindow is Windows 10+; resolve it dynamically to keep Windows 7 working.
    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
    static const auto getDpiForWindow =
        reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    if (getDpiForWindow) {
        const UINT dpi = getDpiForWindow(hwnd);
        if (dpi) return int(dpi);
    }
    HDC dc = GetDC(nullptr);
    const int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSX) : 96;
    if (dc) ReleaseDC(nullptr, dc);
    return dpi > 0 ? dpi : 96;
}

void CLvmlWin32Window::SetCaption(int height, int rightExclude)
{
    m_captionHeight  = height;
    m_captionExclude = rightExclude;
}

void CLvmlWin32Window::SetRoundedCorners(int fallbackRadius)
{
    const DWORD corner = 2; // DWMWCP_ROUND
    const HRESULT hr = DwmSetWindowAttribute(m_hwnd, 33 /*DWMWA_WINDOW_CORNER_PREFERENCE*/, &corner, sizeof corner);
    if (SUCCEEDED(hr)) {
        const MARGINS margins{ 1, 1, 1, 1 }; // keeps the DWM shadow on a WS_POPUP window
        DwmExtendFrameIntoClientArea(m_hwnd, &margins);
        return;
    }
    // Windows 10 and older: clip to a rounded region.
    const int r = Px(fallbackRadius) * 2;
    HRGN rgn = CreateRoundRectRgn(0, 0, Px(m_width) + 1, Px(m_height) + 1, r, r);
    SetWindowRgn(m_hwnd, rgn, TRUE);
}

void CLvmlWin32Window::SetIcon(HICON bigIcon, HICON smallIcon)
{
    if (bigIcon) {
        SetClassLongPtrW(m_hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(bigIcon));
        SendMessageW(m_hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(bigIcon));
    }
    if (smallIcon) {
        SetClassLongPtrW(m_hwnd, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(smallIcon));
        SendMessageW(m_hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
    }
}

void CLvmlWin32Window::Show()
{
    PostMessageW(m_hwnd, kLvmlShowMessage, 0, 0);
}

void CLvmlWin32Window::Minimize()
{
    PostMessageW(m_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
}

void CLvmlWin32Window::BringToFront()
{
    PostMessageW(m_hwnd, kLvmlShowMessage, 1, 0);
}

void CLvmlWin32Window::RequestClose()
{
    m_close = true;
    Wake();
}

void CLvmlWin32Window::Wake()
{
    if (s_wakeEvent) SetEvent(s_wakeEvent);
}

void CLvmlWin32Window::Wait(uint32_t ms)
{
    WaitForSingleObject(s_wakeEvent, std::min<uint32_t>(ms, 1000));
}

LRESULT CALLBACK CLvmlWin32Window::SubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CLvmlWin32Window* s = s_self;
    if (!s) return DefWindowProcW(hwnd, msg, wp, lp);
    switch (msg) {
    case WM_NCHITTEST: {
        POINT p{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        ScreenToClient(hwnd, &p);
        RECT rc;
        GetClientRect(hwnd, &rc);
        if (p.y >= 0 && p.y < s->Px(s->m_captionHeight) && p.x < rc.right - s->Px(s->m_captionExclude)) return HTCAPTION;
        return HTCLIENT;
    }
    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT) {
            SetCursor(LoadCursorW(nullptr, CursorId(CLvmlCursor::ECursor(s->m_cursor.load()))));
            return TRUE;
        }
        return CallWindowProcW(s->m_original, hwnd, msg, wp, lp);
    case kLvmlCursorMessage: {
        // Pump() found a different cursor under the pointer: apply it now if the pointer is in the client area.
        POINT p;
        GetCursorPos(&p);
        if (WindowFromPoint(p) == hwnd && SendMessageW(hwnd, WM_NCHITTEST, 0, MAKELPARAM(p.x, p.y)) == HTCLIENT)
            SetCursor(LoadCursorW(nullptr, CursorId(CLvmlCursor::ECursor(s->m_cursor.load()))));
        return 0;
    }
    case WM_CLOSE:
        s->m_closePressed = true;
        Wake();
        return 0;
    case kLvmlShowMessage:
        ShowWindow(hwnd, IsIconic(hwnd) ? SW_RESTORE : SW_SHOW);
        SetForegroundWindow(hwnd);
        if (wp) BringWindowToTop(hwnd);
        s->m_redrawAll = true;
        Wake();
        return 0;
    case WM_MOUSEWHEEL: {
        POINT p{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        ScreenToClient(hwnd, &p);
        s->m_wheelX = p.x;
        s->m_wheelY = p.y;
        s->m_wheel += GET_WHEEL_DELTA_WPARAM(wp);
        s->m_lastInput = GetTickCount();
        Wake();
        return 0;
    }
    case WM_SIZE:
        // Minimising reports 0×0: the driver would rebuild a zero-size frame buffer.
        if (wp == SIZE_MINIMIZED) return 0;
        s->m_redrawAll = true;
        // Restoring reports the unchanged size; the driver would still swap in a fresh (black) frame
        // buffer, and LVGL redraws only dirty areas into it. Same size → keep the buffer.
        if (s->m_clientSize.exchange(LONG(lp)) == LONG(lp)) {
            Wake();
            return 0;
        }
        [[fallthrough]];
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSELEAVE:
    case WM_KEYDOWN:
    case WM_ACTIVATE: {
        if (msg == WM_ACTIVATE && LOWORD(wp) != WA_INACTIVE) s->m_redrawAll = true;
        const LRESULT r = CallWindowProcW(s->m_original, hwnd, msg, wp, lp);
        s->m_lastInput = GetTickCount();
        Wake();
        return r;
    }
    default:
        return CallWindowProcW(s->m_original, hwnd, msg, wp, lp);
    }
}

lv_obj_t* CLvmlWin32Window::ObjectAt(lv_obj_t* obj, const lv_point_t& p)
{
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return nullptr;
    lv_area_t area;
    lv_obj_get_coords(obj, &area);
    if (p.x < area.x1 || p.x > area.x2 || p.y < area.y1 || p.y > area.y2) return nullptr;
    for (int32_t i = int32_t(lv_obj_get_child_count(obj)) - 1; i >= 0; --i)
        if (lv_obj_t* hit = ObjectAt(lv_obj_get_child(obj, i), p)) return hit;
    return obj;
}

void CLvmlWin32Window::AnimScroll(void* obj, int32_t y)
{
    lv_obj_scroll_to_y(static_cast<lv_obj_t*>(obj), y, LV_ANIM_OFF);
}

void CLvmlWin32Window::ScrollAt(POINT p, int delta)
{
    const lv_point_t pt{ int32_t(p.x), int32_t(p.y) };
    lv_obj_t* obj = ObjectAt(lv_layer_top(), pt);
    if (!obj || obj == lv_layer_top()) obj = ObjectAt(lv_screen_active(), pt);
    for (; obj; obj = lv_obj_get_parent(obj))
        if (lv_obj_has_flag(obj, LV_OBJ_FLAG_SCROLLABLE) && (lv_obj_get_scroll_top(obj) > 0 || lv_obj_get_scroll_bottom(obj) > 0))
            break;
    if (!obj) return;

    const int32_t current = lv_obj_get_scroll_y(obj);
    const int32_t maxY    = current + lv_obj_get_scroll_bottom(obj);
    // Keep adding to the target while an animation runs, so fast wheel turns are not lost.
    const bool    running = obj == m_scrollObj && lv_anim_get(obj, AnimScroll) != nullptr;
    const int32_t base    = running ? m_scrollTarget : current;
    const int32_t step    = MulDiv(-delta, 110 * m_dpi / 96, WHEEL_DELTA); // ≈110 px per notch, like browsers
    const int32_t target  = std::max<int32_t>(0, std::min<int32_t>(maxY, base + step));
    m_scrollObj    = obj;
    m_scrollTarget = target;
    if (target == current) return;

    lv_anim_delete(obj, AnimScroll);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, AnimScroll);
    lv_anim_set_values(&a, current, target);
    lv_anim_set_duration(&a, 160);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

LPCWSTR CLvmlWin32Window::CursorId(CLvmlCursor::ECursor cursor)
{
    // Numeric ids: IDC_* expand to narrow strings when UNICODE is not defined.
    switch (cursor) {
    case CLvmlCursor::ECursor::Pointer:    return MAKEINTRESOURCEW(32649); // IDC_HAND
    case CLvmlCursor::ECursor::Text:       return MAKEINTRESOURCEW(32513); // IDC_IBEAM
    case CLvmlCursor::ECursor::Move:       return MAKEINTRESOURCEW(32646); // IDC_SIZEALL
    case CLvmlCursor::ECursor::NotAllowed: return MAKEINTRESOURCEW(32648); // IDC_NO
    case CLvmlCursor::ECursor::Wait:       return MAKEINTRESOURCEW(32514); // IDC_WAIT
    case CLvmlCursor::ECursor::Progress:   return MAKEINTRESOURCEW(32650); // IDC_APPSTARTING
    case CLvmlCursor::ECursor::Crosshair:  return MAKEINTRESOURCEW(32515); // IDC_CROSS
    case CLvmlCursor::ECursor::Help:       return MAKEINTRESOURCEW(32651); // IDC_HELP
    default:                               return MAKEINTRESOURCEW(32512); // IDC_ARROW
    }
}

void CLvmlWin32Window::UpdateCursor()
{
    // The clickable object under the pointer (the same one LVGL hovers), top layer first.
    lv_point_t p;
    lv_indev_get_point(m_pointer, &p);
    lv_obj_t* obj = lv_indev_search_obj(lv_layer_top(), &p);
    if (!obj) obj = lv_indev_search_obj(lv_screen_active(), &p);
    const int cursor = int(CLvmlCursor::Resolve(obj));
    if (m_cursor.exchange(cursor) != cursor) PostMessageW(m_hwnd, kLvmlCursorMessage, 0, 0);
}

void CLvmlWin32Window::Pump()
{
    if (m_redrawAll.exchange(false)) {
        lv_obj_invalidate(lv_screen_active());
        lv_obj_invalidate(lv_layer_top());
    }
    if (const int d = m_wheel.exchange(0)) ScrollAt(POINT{ m_wheelX, m_wheelY }, d);
    if (!m_pointerPaused) UpdateCursor();

    // Poll the mouse only while it is used; pause the input timer when idle.
    lv_timer_t* t = lv_indev_get_read_timer(m_pointer);
    const bool idle = GetTickCount() - m_lastInput > 1500;
    if (idle && !m_pointerPaused) {
        lv_timer_pause(t);
        m_pointerPaused = true;
    } else if (!idle && m_pointerPaused) {
        lv_timer_resume(t);
        lv_timer_ready(t);
        m_pointerPaused = false;
    }
}
