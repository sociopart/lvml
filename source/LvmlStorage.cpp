#include "LvmlStorage.hpp"

CLvmlStorage::~CLvmlStorage()
{
    for (lv_style_t& s : m_styles) lv_style_reset(&s);
}

lv_style_t* CLvmlStorage::NewStyle()
{
    m_styles.emplace_back();
    lv_style_init(&m_styles.back());
    return &m_styles.back();
}

const char* CLvmlStorage::Intern(const std::string& text)
{
    for (const std::string& s : m_strings)
        if (s == text) return s.c_str();
    m_strings.push_back(text);
    return m_strings.back().c_str();
}

const lv_style_transition_dsc_t* CLvmlStorage::NewTransition(std::vector<lv_style_prop_t> props, lv_anim_path_cb_t path,
                                                              uint32_t timeMs, uint32_t delayMs)
{
    if (props.empty() || props.back() != LV_STYLE_PROP_INV) props.push_back(LV_STYLE_PROP_INV);
    m_propLists.push_back(std::move(props));
    m_transitions.emplace_back();
    lv_style_transition_dsc_init(&m_transitions.back(), m_propLists.back().data(), path, timeMs, delayMs, nullptr);
    return &m_transitions.back();
}
