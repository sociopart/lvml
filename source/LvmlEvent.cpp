#include "LvmlEvent.hpp"
#include "LvmlScene.hpp"

CLvmlEvent::CLvmlEvent(CLvmlScene& scene, lv_event_t* event, lv_obj_t* object, CLvmlNode* node, std::string action,
                       std::string href)
    : m_scene(scene), m_event(event), m_object(object), m_node(node), m_action(std::move(action)), m_href(std::move(href))
{
}

lv_event_code_t CLvmlEvent::GetCode() const
{
    return m_event ? lv_event_get_code(m_event) : LV_EVENT_ALL;
}

lv_obj_t* CLvmlEvent::GetTarget() const
{
    return m_event ? static_cast<lv_obj_t*>(lv_event_get_target(m_event)) : m_object;
}

std::string CLvmlEvent::GetId() const
{
    return m_node ? m_node->GetId() : std::string();
}

std::string CLvmlEvent::GetAttribute(const std::string& name, const std::string& fallback) const
{
    return m_node ? m_node->GetAttribute(name, fallback) : fallback;
}

bool CLvmlEvent::IsChecked() const
{
    return m_object && lv_obj_has_state(m_object, LV_STATE_CHECKED);
}

int32_t CLvmlEvent::GetValue() const
{
    return m_scene.GetValue(m_object);
}

std::string CLvmlEvent::GetText() const
{
    return m_scene.GetText(m_object);
}
