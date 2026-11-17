#include "LvmlDirectoryResources.hpp"
#include <filesystem>
#include <fstream>

CLvmlDirectoryResources::CLvmlDirectoryResources(std::string root) : m_root(std::move(root)) {}

bool CLvmlDirectoryResources::Read(const std::string& path, std::vector<uint8_t>& out)
{
    const std::filesystem::path full = std::filesystem::u8path(m_root) / std::filesystem::u8path(NormalizePath(path));
    std::ifstream f(full, std::ios::binary);
    if (!f) return false;
    f.seekg(0, std::ios::end);
    const std::streamoff size = f.tellg();
    if (size < 0) return false;
    f.seekg(0, std::ios::beg);
    out.resize(size_t(size));
    if (size > 0) f.read(reinterpret_cast<char*>(out.data()), size);
    return bool(f) || f.eof();
}
