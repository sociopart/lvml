#include "HtmlConverter.hpp"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// html2lvml <page.html> <out-dir> [--name scene] [--no-flow] [--title text]
//   writes <out-dir>/<name>.lvml, <out-dir>/<name>.lvss and prints the conversion report.
int main(int argc, char** argv)
{
    if (argc < 3) {
        std::printf("usage: html2lvml <page.html> <out-dir> [--name scene] [--no-flow] [--title text]\n");
        return 1;
    }
    const std::filesystem::path input = std::filesystem::u8path(argv[1]);
    const std::filesystem::path outDir = std::filesystem::u8path(argv[2]);
    std::string name = "scene";
    CHtmlConverter::SOptions options;
    for (int i = 3; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--name") && i + 1 < argc) name = argv[++i];
        else if (!std::strcmp(argv[i], "--title") && i + 1 < argc) options.title = argv[++i];
        else if (!std::strcmp(argv[i], "--no-flow")) options.flowLayout = false;
    }
    options.styleSheetName = name + ".lvss";

    std::ifstream f(input, std::ios::binary);
    if (!f) { std::printf("error: cannot read %s\n", argv[1]); return 2; }
    std::stringstream ss;
    ss << f.rdbuf();

    CHtmlConverter::SResult result;
    CHtmlConverter::Convert(ss.str(), input.parent_path().u8string(), options, result);

    std::filesystem::create_directories(outDir);
    std::ofstream(outDir / (name + ".lvml"), std::ios::binary) << result.lvml;
    std::ofstream(outDir / (name + ".lvss"), std::ios::binary) << result.lvss;
    std::printf("wrote %s and %s\n", (outDir / (name + ".lvml")).u8string().c_str(), (outDir / (name + ".lvss")).u8string().c_str());
    std::printf("%s", result.report.ToString().c_str());
    return 0;
}
