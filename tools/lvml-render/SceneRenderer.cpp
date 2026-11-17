#include "SceneRenderer.hpp"
#include "LvmlContext.hpp"
#include "LvmlDirectoryResources.hpp"
#include "LvmlScene.hpp"
#include <cstdio>
#include <vector>

void CSceneRenderer::FlushNoop(lv_display_t* display, const lv_area_t*, uint8_t*)
{
    lv_display_flush_ready(display);
}

bool CSceneRenderer::WriteBmp(const std::string& path, const void* argb, int width, int height, int stride)
{
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    const uint32_t imageSize = uint32_t(width) * uint32_t(height) * 4;
    uint8_t header[54] = { 'B', 'M' };
    auto put32 = [&](int at, uint32_t v) { for (int i = 0; i < 4; ++i) header[at + i] = uint8_t(v >> (8 * i)); };
    put32(2, 54 + imageSize);
    put32(10, 54);
    put32(14, 40);
    put32(18, uint32_t(width));
    put32(22, uint32_t(-height)); // top-down
    header[26] = 1;
    header[28] = 32;
    put32(34, imageSize);
    std::fwrite(header, 1, sizeof(header), f);
    const uint8_t* row = static_cast<const uint8_t*>(argb);
    for (int y = 0; y < height; ++y, row += stride) std::fwrite(row, 1, size_t(width) * 4, f);
    std::fclose(f);
    return true;
}

void CSceneRenderer::DumpTree(CLvmlScene& scene, lv_obj_t* obj, int depth, int maxDepth)
{
    if (depth > maxDepth) return;
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    const CLvmlNode* node = scene.GetNode(obj);
    std::string what = node ? "<" + node->GetTag() + ">" : std::string("(anon)");
    if (node && node->HasAttribute("id")) what += " #" + node->GetId();
    if (node && node->HasAttribute("class")) what += " ." + node->GetAttribute("class");
    std::printf("%*s%s  [%d,%d %dx%d]%s%s%s\n", depth * 2, "", what.c_str(), int(a.x1), int(a.y1), int(lv_area_get_width(&a)),
                int(lv_area_get_height(&a)), lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN) ? " hidden" : "",
                lv_obj_get_style_layout(obj, LV_PART_MAIN) == LV_LAYOUT_FLEX ? " flex" : "",
                lv_obj_has_flag(obj, LV_OBJ_FLAG_IGNORE_LAYOUT) ? " out-of-flow" : "");
    const uint32_t n = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < n; ++i) DumpTree(scene, lv_obj_get_child(obj, int32_t(i)), depth + 1, maxDepth);
}

int CSceneRenderer::Run(const std::string& root, const std::string& scenePath, const std::string& outPath, int width,
                        int height, float scale, const std::string& clickId, int treeDepth)
{
    lv_init();
    lv_display_t* display = lv_display_create(width, height);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_XRGB8888);
    std::vector<uint8_t> buffer(size_t(width) * size_t(height) * 4);
    lv_display_set_buffers(display, buffer.data(), nullptr, uint32_t(buffer.size()), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(display, FlushNoop);

    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFF00FF), 0); // magenta shows uncovered areas
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    int warnings = 0;
    CLvmlContext context(std::make_shared<CLvmlDirectoryResources>(root));
    context.SetScale(scale);
    context.SetLogger([&](const std::string& m) { std::printf("warning: %s\n", m.c_str()); ++warnings; });

    std::unique_ptr<CLvmlScene> scene = context.LoadScene(scenePath);
    if (!scene) { std::printf("error: scene failed to load\n"); return 2; }
    scene->OnAny([](CLvmlEvent& e) { std::printf("event: action='%s' id='%s' href='%s'\n", e.GetAction().c_str(),
                                                e.GetId().c_str(), e.GetHref().c_str()); });

    if (!clickId.empty()) {
        if (lv_obj_t* target = scene->Find(clickId)) lv_obj_send_event(target, LV_EVENT_CLICKED, nullptr);
        else std::printf("warning: no element #%s\n", clickId.c_str());
    }

    lv_obj_update_layout(screen);
    if (treeDepth > 0) DumpTree(*scene, scene->GetRoot(), 0, treeDepth);
    lv_refr_now(display);
    lv_draw_buf_t* snap = lv_snapshot_take(screen, LV_COLOR_FORMAT_ARGB8888);
    if (!snap) { std::printf("error: snapshot failed\n"); return 3; }
    const bool ok = WriteBmp(outPath, snap->data, int(snap->header.w), int(snap->header.h), int(snap->header.stride));
    lv_draw_buf_destroy(snap);
    std::printf("%s %s (%d warnings)\n", ok ? "wrote" : "failed to write", outPath.c_str(), warnings);

    scene.reset();
    return ok ? 0 : 4;
}
