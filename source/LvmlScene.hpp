// lvml — LVGL markup & style sheets.
#pragma once
#include "LvmlDocument.hpp"
#include "LvmlEvent.hpp"
#include "LvmlStorage.hpp"
#include "LvmlStyleSet.hpp"
#include "LvmlWidgetFactory.hpp"
#include <lvgl.h>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class CLvmlContext;

// A built .lvml document: the LVGL object tree plus everything its styles point to.
// Destroying the scene deletes its objects. Do not destroy a scene from inside one of its
// own handlers — defer it (e.g. load the next scene on the next loop iteration).
class CLvmlScene
{
public:
    using Handler = std::function<void(CLvmlEvent&)>;

    ~CLvmlScene();
    CLvmlScene(const CLvmlScene&)            = delete;
    CLvmlScene& operator=(const CLvmlScene&) = delete;

    lv_obj_t*      GetRoot() const { return m_root; }
    CLvmlDocument& GetDocument() { return *m_document; }
    CLvmlContext&  GetContext() { return m_context; }

    lv_obj_t*  Find(std::string_view id) const;
    CLvmlNode* FindNode(std::string_view id) const;
    CLvmlNode* GetNode(lv_obj_t* obj) const;
    lv_obj_t*  GetObject(const CLvmlNode* node) const;

    // Handlers by action name (the value of onclick="…", onchange="…", …; "link" for href).
    void On(const std::string& action, Handler handler);
    // Called for actions without a dedicated handler.
    void OnAny(Handler handler) { m_fallback = std::move(handler); }

    // ---- runtime helpers (by id) ----
    bool        SetText(std::string_view id, const std::string& text);
    std::string GetText(std::string_view id) const;
    bool        SetVisible(std::string_view id, bool visible);
    bool        IsVisible(std::string_view id) const;
    bool        SetChecked(std::string_view id, bool checked);
    bool        IsChecked(std::string_view id) const;
    bool        SetEnabled(std::string_view id, bool enabled);
    bool        SetValue(std::string_view id, int32_t value, bool animate = false);
    int32_t     GetValue(std::string_view id) const;
    bool        SetImage(std::string_view id, const std::string& path); // relative to the document
    bool        AddClass(std::string_view id, std::string_view cls);
    bool        RemoveClass(std::string_view id, std::string_view cls);
    bool        ToggleClass(std::string_view id, std::string_view cls, bool on);
    bool        HasClass(std::string_view id, std::string_view cls) const;
    bool        SetStyle(std::string_view id, const std::string& declarations); // replaces style=""
    bool        SetAttribute(std::string_view id, const std::string& name, const std::string& value);

    // ---- same, by object ----
    bool        SetText(lv_obj_t* obj, const std::string& text);
    std::string GetText(lv_obj_t* obj) const;
    int32_t     GetValue(lv_obj_t* obj) const;

    // Re-applies styles (after class/attribute changes done directly on nodes).
    void Restyle(lv_obj_t* obj, bool recursive = true);

private:
    friend class CLvmlBuilder;

    struct SObject {
        lv_obj_t*                          obj    = nullptr;
        CLvmlNode*                         node   = nullptr;
        const CLvmlWidgetFactory::SWidget* widget = nullptr;
        bool                               keepTheme      = false;
        bool                               hasHandler     = false;
        bool                               dependsOnState = false; // matched by "ancestor:state …" rules
        std::string                        fontFamily;
        int32_t                            fontPx = 0;
        std::vector<CLvssDeclaration>      inlineDecls;
    };

    struct SBinding {
        CLvmlScene*     scene = nullptr;
        CLvmlNode*      node  = nullptr;
        std::string     action;
        std::string     href;
        bool            spans = false; // a richtext group: resolve the clicked span
    };

    struct SEventName {
        const char*     name;
        lv_event_code_t code;
    };
    static const SEventName kEventNames[];

    struct SSpanLink {
        CLvmlNode*  node = nullptr;
        std::string action;
        std::string href;
    };

    CLvmlScene(CLvmlContext& context, std::unique_ptr<CLvmlDocument> document, CLvssStyleSheet sheet);

    SObject*       Register(lv_obj_t* obj, CLvmlNode* node, const CLvmlWidgetFactory::SWidget* widget);
    SObject*       Info(lv_obj_t* obj);
    const SObject* Info(lv_obj_t* obj) const;
    void           Bind(SObject& o);
    void           ApplyStyle(SObject& o);
    void           ApplyFont(SObject& o, const std::map<std::string, std::string>& decls);
    void           ApplyObjectProperties(SObject& o, const std::map<std::string, std::string>& decls, bool needsInput);
    void           ApplyLineHeight(SObject& o, const std::string& value);
    void           CollectStyle(const CLvmlNode& node, CLvssProperty::CSink& sink, std::string& family, std::string& size);
    void           RestyleDependents(lv_obj_t* obj);
    void           Dispatch(CLvmlEvent& event);
    lv_style_t*    SpanStyle(CLvmlNode& node, const SObject& group);

    static void OnLvEvent(lv_event_t* e);
    static void OnDeleted(lv_event_t* e);
    static void OnStateChanged(lv_event_t* e);

    CLvmlContext&                             m_context;
    std::unique_ptr<CLvmlDocument>            m_document;
    std::unique_ptr<CLvmlStorage>             m_storage;
    std::unique_ptr<CLvmlStyleSet>            m_styles;
    lv_obj_t*                                 m_root     = nullptr;
    lv_style_t*                               m_rootBase = nullptr;
    lv_style_t*                               m_containerBase = nullptr; // content × content (lv_obj defaults to 96 × 96)
    std::unordered_map<lv_obj_t*, SObject>    m_objects;
    std::unordered_map<const CLvmlNode*, lv_obj_t*> m_nodeObjects;
    std::deque<SBinding>                      m_bindings;
    std::unordered_map<const lv_span_t*, SSpanLink> m_spanLinks;
    std::map<std::string, Handler>            m_handlers;
    Handler                                   m_fallback;
    bool                                      m_destroying = false;
};
