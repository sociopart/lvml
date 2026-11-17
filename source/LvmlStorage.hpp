// lvml — LVGL markup & style sheets.
#pragma once
#include <lvgl.h>
#include <deque>
#include <string>
#include <vector>

// Owns everything LVGL styles point to: lv_style_t objects, interned strings (image paths),
// transition descriptors. Must outlive every object that uses these styles.
class CLvmlStorage
{
public:
    CLvmlStorage() = default;
    ~CLvmlStorage();
    CLvmlStorage(const CLvmlStorage&)            = delete;
    CLvmlStorage& operator=(const CLvmlStorage&) = delete;

    lv_style_t*                      NewStyle();
    const char*                      Intern(const std::string& text);
    const lv_style_transition_dsc_t* NewTransition(std::vector<lv_style_prop_t> props, lv_anim_path_cb_t path,
                                                   uint32_t timeMs, uint32_t delayMs);

private:
    std::deque<lv_style_t>                   m_styles;
    std::deque<std::string>                  m_strings;
    std::deque<std::vector<lv_style_prop_t>> m_propLists;
    std::deque<lv_style_transition_dsc_t>    m_transitions;
};
