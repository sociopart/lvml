// lvml — LVGL markup & style sheets.
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// Where .lvml / .lvss / images / fonts are read from. Paths are '/'-separated and relative
// to the resource root ("01_eula/scene.lvml").
class CLvmlResources
{
public:
    virtual ~CLvmlResources() = default;

    virtual bool Read(const std::string& path, std::vector<uint8_t>& out) = 0;
    virtual bool Exists(const std::string& path);

    bool ReadText(const std::string& path, std::string& out);

    // "a\\b/./c/../d" → "a/b/d"; leading '/' dropped.
    static std::string NormalizePath(std::string_view path);
    // Resolves `relative` against the directory of `baseFile`. "/x" is resolved from the root.
    static std::string ResolvePath(std::string_view baseFile, std::string_view relative);
};
