// lvml — LVGL markup & style sheets.
#pragma once
#include "LvmlDocument.hpp"
#include "LvmlScene.hpp"
#include "LvssStyleSheet.hpp"
#include <lvgl.h>
#include <memory>

class CLvmlContext;

// Turns a parsed document + style sheet into LVGL objects.
class CLvmlBuilder
{
public:
    CLvmlBuilder() = delete;

    static std::unique_ptr<CLvmlScene> Build(CLvmlContext& context, std::unique_ptr<CLvmlDocument> document,
                                             CLvssStyleSheet sheet, lv_obj_t* parent);

private:
    struct SSpanState {
        bool atLineStart = true;
    };

    static lv_obj_t* BuildElement(CLvmlScene& scene, CLvmlNode& node, lv_obj_t* parent);
    static void      ApplyAttributes(CLvmlScene& scene, lv_obj_t* obj, CLvmlNode& node);
    static void      BuildChildren(CLvmlScene& scene, CLvmlScene::SObject& o);
    static void      BuildSpans(CLvmlScene& scene, CLvmlScene::SObject& group, CLvmlNode& node, lv_style_t* style,
                                const CLvmlScene::SSpanLink* link, SSpanState& state);
    static void      AddAnonymousLabel(lv_obj_t* parent, const std::string& text);
    static bool      IsInlineContent(const CLvmlNode& node);
};
