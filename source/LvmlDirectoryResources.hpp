// lvml — LVGL markup & style sheets.
#pragma once
#include "LvmlResources.hpp"

// Resources from a directory on disk (UTF-8 root path).
class CLvmlDirectoryResources : public CLvmlResources
{
public:
    explicit CLvmlDirectoryResources(std::string root);

    bool Read(const std::string& path, std::vector<uint8_t>& out) override;

private:
    std::string m_root;
};
