#include "LvmlFontManager.hpp"
#include "TextUtils.hpp"

CLvmlFontManager::CLvmlFontManager(CLvmlResources* resources) : m_resources(resources) {}

CLvmlFontManager::~CLvmlFontManager()
{
#if LV_USE_TINY_TTF
    for (auto& f : m_families)
        for (auto& size : f.second.ttf)
            if (size.second) lv_tiny_ttf_destroy(size.second);
#endif
}

std::string CLvmlFontManager::Key(const std::string& family)
{
    return CTextUtils::ToLower(CTextUtils::Unquote(family));
}

void CLvmlFontManager::RegisterFamily(const std::string& family, const std::string& path)
{
    const std::string key = Key(family);
    if (key.empty()) return;
    SFamily& f = m_families[key];
    if (f.loaded || !f.path.empty()) return;
    f.path = path;
    if (m_defaultFamily.empty()) m_defaultFamily = key;
}

void CLvmlFontManager::RegisterFamilyData(const std::string& family, std::vector<uint8_t> data)
{
    const std::string key = Key(family);
    if (key.empty()) return;
    SFamily& f = m_families[key];
    f.data   = std::move(data);
    f.loaded = true;
    if (m_defaultFamily.empty()) m_defaultFamily = key;
}

void CLvmlFontManager::RegisterStatic(const std::string& family, int32_t pixelSize, const lv_font_t* font)
{
    const std::string key = Key(family);
    m_families[key].statics[pixelSize] = font;
    m_sizes[font] = pixelSize;
    if (m_defaultFamily.empty()) m_defaultFamily = key;
}

bool CLvmlFontManager::HasFamily(const std::string& family) const
{
    return m_families.count(Key(family)) != 0;
}

void CLvmlFontManager::SetDefaultFamily(const std::string& family)
{
    m_defaultFamily = Key(family);
}

CLvmlFontManager::SFamily* CLvmlFontManager::Find(const std::string& family)
{
    // CSS font stacks: "Inter, sans-serif" — first known family wins.
    for (const std::string& item : CTextUtils::SplitTopLevel(family, ',')) {
        const auto it = m_families.find(Key(item));
        if (it != m_families.end()) return &it->second;
    }
    const auto it = m_families.find(m_defaultFamily);
    return it == m_families.end() ? nullptr : &it->second;
}

const lv_font_t* CLvmlFontManager::Create(SFamily& family, const std::string& key, int32_t pixelSize)
{
    (void)key;
#if LV_USE_TINY_TTF
    if (!family.loaded) {
        family.loaded = true;
        if (m_resources && !family.path.empty() && !m_resources->Read(family.path, family.data)) {
            LV_LOG_WARN("lvml: font file '%s' not found", family.path.c_str());
            family.data.clear();
        }
    }
    if (family.data.empty()) return nullptr;
    lv_font_t* font = lv_tiny_ttf_create_data_ex(family.data.data(), family.data.size(), pixelSize,
                                                  LV_FONT_KERNING_NORMAL, 256);
    if (!font) return nullptr;
    family.ttf[pixelSize] = font;
    m_sizes[font] = pixelSize;
    if (!m_fallbackFamily.empty() && !m_inFallback) {
        m_inFallback = true;
        const lv_font_t* fb = GetFont(m_fallbackFamily, pixelSize);
        if (fb != font && fb != LV_FONT_DEFAULT) font->fallback = fb;
        m_inFallback = false;
    }
    return font;
#else
    (void)family;
    (void)pixelSize;
    return nullptr;
#endif
}

const lv_font_t* CLvmlFontManager::GetFont(const std::string& family, int32_t pixelSize)
{
    if (pixelSize <= 0) pixelSize = m_defaultSize;
    SFamily* f = Find(family.empty() ? m_defaultFamily : family);
    if (!f) return LV_FONT_DEFAULT;

    if (const auto it = f->ttf.find(pixelSize); it != f->ttf.end()) return it->second;
    if (!f->statics.empty()) {
        // Nearest registered bitmap size.
        const lv_font_t* best = nullptr;
        int32_t bestDiff = INT32_MAX;
        for (const auto& s : f->statics) {
            const int32_t d = s.first > pixelSize ? s.first - pixelSize : pixelSize - s.first;
            if (d < bestDiff) { bestDiff = d; best = s.second; }
        }
        if (best && (bestDiff == 0 || (f->path.empty() && f->data.empty()))) return best;
    }
    if (const lv_font_t* created = Create(*f, family, pixelSize)) return created;
    return LV_FONT_DEFAULT;
}

int32_t CLvmlFontManager::GetPixelSize(const lv_font_t* font) const
{
    const auto it = m_sizes.find(font);
    return it == m_sizes.end() ? 0 : it->second;
}
