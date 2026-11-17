#include "SceneRenderer.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>

// lvml-render <root-dir> <scene.lvml> <out.bmp> [width height [scale [click-id|- [tree-depth]]]]
int main(int argc, char** argv)
{
    if (argc < 4) {
        std::printf("usage: lvml-render <root-dir> <scene.lvml> <out.bmp> [width height [scale [click-id]]]\n");
        return 1;
    }
    const int   width  = argc > 5 ? std::atoi(argv[4]) : 1000;
    const int   height = argc > 5 ? std::atoi(argv[5]) : 600;
    const float scale  = argc > 6 ? float(std::atof(argv[6])) : 1.0f;
    const std::string click = argc > 7 && std::string(argv[7]) != "-" ? argv[7] : "";
    const int tree = argc > 8 ? std::atoi(argv[8]) : 0;
    return CSceneRenderer::Run(argv[1], argv[2], argv[3], width, height, scale, click, tree);
}
