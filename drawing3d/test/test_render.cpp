// Headless test: renders every demo scene / example with the CPU reference rasteriser
// and writes raw RGBA files (convert with test/to_png.py).
//   g++ -std=c++17 -O2 -I. test/test_render.cpp Drawing3D.cpp Drawing3DScenes.cpp -o test_render
//   ./test_render <outdir> <Image.rgba 256x256>
#include <cstdio>
#include <chrono>
#include <fstream>
#include <string>
#include <vector>

#include "Drawing3DScenes.h"

using namespace d3d;

static void save(const std::string& path, const std::vector<uint8_t>& px)
{
    std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(px.data()), px.size());
}

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    Image3DPtr tex;
    if (argc > 2)
    {
        std::ifstream f(argv[2], std::ios::binary);
        std::vector<uint8_t> raw((std::istreambuf_iterator<char>(f)), {});
        if (raw.size() == 256 * 256 * 4)
            tex = Image3D::fromRGBA8(raw.data(), 256, 256, false);
    }

    Drawing3D g;
    // Fake text rasteriser: a hollow rectangle sized like "X-Achse" at Verdana 24 bold.
    g.setTextRasterizer([](const std::string& text) {
        auto img = Image3D::create(static_cast<int>(text.size()) * 17, 30);
        for (int y = 0; y < img->height; ++y)
            for (int x = 0; x < img->width; ++x)
                if (x < 3 || y < 3 || x >= img->width - 3 || y >= img->height - 3 || (x == y * 2))
                    img->at(x, y) = {1, 1, 1, 1};
        return img;
    });

    std::vector<uint8_t> px;
    for (int s = 0; s < kDemoSceneCount; ++s)
    {
        const auto t0 = std::chrono::steady_clock::now();
        g.start(580, 580);
        g.setBackground(0xFFFFFFFF);
        g.setPosition(0, 0, -700);
        g.setRotation(30, 30, 90);
        drawDemoScene(g, s, tex);
        g.stop();
        g.rasterize(px);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
        std::printf("scene %2d: %7zu triangles, %4lld ms  %s\n", s, g.triangles().size(), static_cast<long long>(ms), demoSceneName(s));
        save(out + "/scene" + std::to_string(s) + ".rgba", px);
    }
    void (*ex[3])(Drawing3D&, int, int) = {drawExample01, drawExample02, drawExample03};
    for (int i = 0; i < 3; ++i)
    {
        ex[i](g, 500, 500);
        g.rasterize(px);
        std::printf("example %d: %zu triangles\n", i + 1, g.triangles().size());
        save(out + "/example" + std::to_string(i + 1) + ".rgba", px);
    }
    return 0;
}
