// lvml — LVGL markup & style sheets.
#pragma once
#include "LvmlResources.hpp"
#include <unordered_map>

// Resources from an in-memory store (e.g. an unpacked resource archive). Keys are normalised.
class CLvmlMemoryResources : public CLvmlResources
{
public:
    using Store = std::unordered_map<std::string, std::vector<uint8_t>>;

    CLvmlMemoryResources() = default;
    explicit CLvmlMemoryResources(Store store);

    void Add(const std::string& path, std::vector<uint8_t> data);
    bool Empty() const { return m_store.empty(); }

    // Zero-copy access; the pointer stays valid until the entry is replaced.
    const std::vector<uint8_t>* Find(const std::string& path) const;

    bool Read(const std::string& path, std::vector<uint8_t>& out) override;
    bool Exists(const std::string& path) override { return Find(path) != nullptr; }

private:
    Store m_store;
};
