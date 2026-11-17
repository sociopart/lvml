// lvml — LVGL markup & style sheets.
#pragma once
#include <cstdint>
#include <string>
#include <string_view>

// Parsers for scalar .lvss values. LVGL-independent so the converter can use them too.
class CLvssValue
{
public:
    enum class EUnit { Px, Percent, Content, Em, Rem, Vw, Vh, Auto };

    struct SLength {
        float value = 0.0f;
        EUnit unit  = EUnit::Px;
    };

    CLvssValue() = delete;

    // 12, 12px, 12dp, 50%, content, auto, 1.5em, 2rem, 10vw
    static bool ParseLength(std::string_view s, SLength& out);
    // #rgb #rgba #rrggbb #rrggbbaa rgb() rgba() hsl()-less named colors, transparent.
    // Result is 0xAARRGGBB; hasAlpha tells whether the source spelled an alpha channel.
    static bool ParseColor(std::string_view s, uint32_t& argb, bool* hasAlpha = nullptr);
    // 0..255, 0%..100%, 0.0..1.0
    static bool ParseOpacity(std::string_view s, uint8_t& out);
    // 150ms, 0.15s
    static bool ParseTime(std::string_view s, uint32_t& ms);
    // 45deg, 0.5turn, 1rad → tenths of a degree (LVGL rotation unit)
    static bool ParseAngle(std::string_view s, int32_t& decidegrees);
    // url(x), url("x"), "x", x
    static bool ParseUrl(std::string_view s, std::string& out);

    static std::string ColorToHex(uint32_t argb, bool withAlpha);
};
