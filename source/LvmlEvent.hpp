// lvml — LVGL markup & style sheets.
#pragma once
#include "LvmlNode.hpp"
#include <lvgl.h>
#include <string>

class CLvmlScene;

// Passed to scene handlers. `action` is the attribute value: <button onclick="submit"> → "submit";
// links (<a href> inside <richtext>, or any element with href and no handler) report "link".
class CLvmlEvent
{
public:
    CLvmlEvent(CLvmlScene& scene, lv_event_t* event, lv_obj_t* object, CLvmlNode* node, std::string action,
               std::string href);

    const std::string& GetAction() const { return m_action; }
    const std::string& GetHref() const { return m_href; }
    lv_event_code_t    GetCode() const;
    lv_obj_t*          GetObject() const { return m_object; } // element that declared the handler
    lv_obj_t*          GetTarget() const;                     // element that was actually hit
    CLvmlNode*         GetNode() const { return m_node; }
    CLvmlScene&        GetScene() const { return m_scene; }
    lv_event_t*        GetLvEvent() const { return m_event; }

    std::string GetId() const;
    std::string GetAttribute(const std::string& name, const std::string& fallback = {}) const;
    bool        IsChecked() const;
    int32_t     GetValue() const;
    std::string GetText() const;

    // Stops the action from reaching the scene's fallback handler.
    void SetHandled() { m_handled = true; }
    bool IsHandled() const { return m_handled; }

private:
    CLvmlScene& m_scene;
    lv_event_t* m_event;
    lv_obj_t*   m_object;
    CLvmlNode*  m_node;
    std::string m_action;
    std::string m_href;
    bool        m_handled = false;
};
