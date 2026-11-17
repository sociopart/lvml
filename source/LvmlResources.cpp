#include "LvmlResources.hpp"

bool CLvmlResources::Exists(const std::string& path)
{
    std::vector<uint8_t> tmp;
    return Read(path, tmp);
}

bool CLvmlResources::ReadText(const std::string& path, std::string& out)
{
    std::vector<uint8_t> data;
    if (!Read(path, data)) return false;
    out.assign(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

std::string CLvmlResources::NormalizePath(std::string_view path)
{
    std::vector<std::string> parts;
    std::string seg;
    auto push = [&]() {
        if (seg == "..") { if (!parts.empty()) parts.pop_back(); }
        else if (!seg.empty() && seg != ".") parts.push_back(seg);
        seg.clear();
    };
    for (char c : path) {
        if (c == '/' || c == '\\') push();
        else seg += c;
    }
    push();
    std::string out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) out += '/';
        out += parts[i];
    }
    return out;
}

std::string CLvmlResources::ResolvePath(std::string_view baseFile, std::string_view relative)
{
    if (!relative.empty() && (relative[0] == '/' || relative[0] == '\\')) return NormalizePath(relative);
    const size_t slash = baseFile.find_last_of("/\\");
    const std::string dir = slash == std::string_view::npos ? std::string() : std::string(baseFile.substr(0, slash + 1));
    return NormalizePath(dir + std::string(relative));
}
