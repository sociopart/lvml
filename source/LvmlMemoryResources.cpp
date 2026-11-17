#include "LvmlMemoryResources.hpp"

CLvmlMemoryResources::CLvmlMemoryResources(Store store)
{
    for (auto& entry : store) m_store[NormalizePath(entry.first)] = std::move(entry.second);
}

void CLvmlMemoryResources::Add(const std::string& path, std::vector<uint8_t> data)
{
    m_store[NormalizePath(path)] = std::move(data);
}

const std::vector<uint8_t>* CLvmlMemoryResources::Find(const std::string& path) const
{
    const auto it = m_store.find(NormalizePath(path));
    return it == m_store.end() ? nullptr : &it->second;
}

bool CLvmlMemoryResources::Read(const std::string& path, std::vector<uint8_t>& out)
{
    const std::vector<uint8_t>* data = Find(path);
    if (!data) return false;
    out = *data;
    return true;
}
