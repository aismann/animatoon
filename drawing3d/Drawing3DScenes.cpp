#include "Drawing3DScenes.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace d3d
{

namespace
{
constexpr float kPi = 3.14159265358979323846f;
inline float radian(float deg) { return deg * (kPi / 180.0f); }

constexpr ColorPB RGBA(int r, int g, int b, int a) { return rgba(r, g, b, a); }

// PureBasic's RGBA() just masks the channels; clamp instead so out-of-range values saturate.
int clamp8(float v) { return std::clamp(static_cast<int>(v), 0, 255); }

const char* const kNames[kDemoSceneCount] = {
    "Drei sich durchdringende Fl\xC3\xA4""chen",
    "W\xC3\xBCrfel aus Linien",
    "Belichteter W\xC3\xBCrfel",
    "Zwei durchdringende Quader",
    "Tetraeder mit verschiedenen Eckfarben",
    "Texturfl\xC3\xA4""che",
    "Punkte",
    "Zylinder",
    "Anwendungsbeispiel: Magnet & Feldlinien",
    "Anwendungsbeispiel: 3D-Funktion",
    "Anwendungsbeispiel: Ikosaeder",
    "Anwendungsbeispiel: M\xC3\xB6" "bius",
    "Anwendungsbeispiel: Text3D",
};

// The 20 faces of the icosahedron in Case 10 (each row: three x,y,z vertices).
const float kIcosahedron[20][9] = {
    {162, 0, 100, 0, -100, 162, 0, 100, 162},     {162, 0, 100, 0, 100, 162, 100, 162, 0},
    {162, 0, 100, 100, 162, 0, 162, 0, -100},     {162, 0, 100, 162, 0, -100, 100, -162, 0},
    {162, 0, 100, 100, -162, 0, 0, -100, 162},

    {-162, 0, -100, 0, -100, -162, 0, 100, -162}, {-162, 0, -100, 0, 100, -162, -100, 162, 0},
    {-162, 0, -100, -100, 162, 0, -162, 0, 100},  {-162, 0, -100, -162, 0, 100, -100, -162, 0},
    {-162, 0, -100, -100, -162, 0, 0, -100, -162},

    {0, -100, 162, -162, 0, 100, 0, 100, 162},    {0, 100, 162, -100, 162, 0, 100, 162, 0},
    {100, 162, 0, 0, 100, -162, 162, 0, -100},    {162, 0, -100, 0, -100, -162, 100, -162, 0},
    {100, -162, 0, -100, -162, 0, 0, -100, 162},

    {0, -100, -162, 162, 0, -100, 0, 100, -162},  {0, 100, -162, 100, 162, 0, -100, 162, 0},
    {-100, 162, 0, 0, 100, 162, -162, 0, 100},    {-162, 0, 100, 0, -100, 162, -100, -162, 0},
    {-100, -162, 0, 100, -162, 0, 0, -100, -162},
};
}  // namespace

const char* demoSceneName(int index) { return (index >= 0 && index < kDemoSceneCount) ? kNames[index] : ""; }

void drawDemoScene(Drawing3D& g, int scene, const Image3DPtr& image)
{
    // Scenes in which surfaces penetrate each other need splitting (see setSubdivision).
    g.setSubdivision((scene == 0 || scene == 3) ? 10.0f : (scene == 7 ? 16.0f : (scene == 8 ? 12.0f : 0.0f)));

    switch (scene)
    {
    case 0:
        g.drawPlane3D(0, 0, 0, 340, 200, 0, 0, 0, 0x80FF0000);
        g.drawPlane3D(0, 0, 0, 340, 200, 90, 90, 0, 0x800000FF);
        g.drawPlane3D(0, 0, 0, 340, 200, 90, 0, 90, 0x8000FF00);
        break;

    case 1:
        g.setMode(Mode::Outline);
        g.drawBox3D(0, 0, 0, 200, 200, 200, 0, 0, 0, 0xFF000000);
        g.setMode(Mode::Default);
        break;

    case 2:
        g.addLight(1, 0, 0, 0xFFFFFF00);
        g.addLight(-1, 0, 0, 0xFF00FFFF);
        g.drawBox3D(0, 0, 0, 200, 200, 200, 0, 0, 0, 0xFF000000);
        break;

    case 3:
        g.addLight(0.5f, 1, 1, 0xFFFFFFFF);
        g.drawBox3D(0, 0, 0, 100, 200, 300, 0, 0, 0, 0xC000C060);
        g.drawBox3D(0, 0, 0, 100, 200, 300, 30, 45, 15, 0xC08000C0);
        break;

    case 4:
        g.addLight(0.5f, 1, -1, 0xFFFFFFFF);
        g.drawTriangle3D(-100, 100, 100, 100, 100, -100, 100, -100, 100, 0xFF00C000, 0xFFC00000, 0xFF0000C0);
        g.drawTriangle3D(-100, 100, 100, 100, -100, 100, -100, -100, -100, 0xFF00C000, 0xFF0000C0, 0xFF00C0C0);
        g.drawTriangle3D(-100, 100, 100, -100, -100, -100, 100, 100, -100, 0xFF00C000, 0xFF00C0C0, 0xFFC00000);
        g.drawTriangle3D(100, -100, 100, 100, 100, -100, -100, -100, -100, 0xFF0000C0, 0xFFC00000, 0xFF00C0C0);
        g.drawLine3D(90, -90, -90, -90, -90, 90, 0xFF00C000, 0xFFC00000);
        g.drawLine3D(-90, -90, 90, -90, 90, -90, 0xFFC00000, 0xFF0000C0);
        g.drawLine3D(-90, 90, -90, 90, -90, -90, 0xFF0000C0, 0xFF00C000);
        g.drawLine3D(90, -90, -90, 90, 90, 90, 0xFF00C000, 0xFF00C0C0);
        g.drawLine3D(-90, -90, 90, 90, 90, 90, 0xFFC00000, 0xFF00C0C0);
        g.drawLine3D(-90, 90, -90, 90, 90, 90, 0xFF0000C0, 0xFF00C0C0);
        break;

    case 5:
        g.drawImage3D(image, 0, 0, 0, 400, 400, 0, 0, 0);
        break;

    case 6:
    {
        std::mt19937 rng(1);
        std::uniform_int_distribution<int> z100(0, 100);
        g.drawPlane3D(0, 0, 0, 200, 200, 90, 0, 0, 0xFF808080);
        for (int y = -200; y <= 200; ++y)
            for (int x = -200; x <= 200; ++x)
                g.drawPoint3D(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z100(rng) - 50),
                              0xFF000000);
        break;
    }

    case 7:
        g.addLight(0.5f, 1, 1, 0x80FFFFFF);
        g.setMode(Mode::Default);
        g.drawCylinder3D(0, 0, 0, 20, 200, 0, 0, 0, 0xFF800000, 0xFF008080);
        g.drawCylinder3D(0, 0, 0, 20, 200, 0, 0, 90, 0xFF008000, 0xFF800080);
        g.drawCylinder3D(0, 0, 0, 20, 200, 90, 0, 0, 0xFF000080, 0xFF808000);
        g.setMode(Mode::Outline);
        g.drawBox3D(0, 0, 0, 200, 200, 200, 0, 0, 0, 0x80000000);
        g.setMode(Mode::Default);
        break;

    case 8:  // magnet & field lines
    {
        std::mt19937 rng(5);
        g.addLight(0.5f, 1, 1, 0x40FFFFFF);
        g.drawBox3D(0, 0, 0, 100, 30, 30, 0, 0, 0, 0xFF404040);
        g.drawBox3D(-75, 0, 0, 50, 30, 30, 0, 0, 0, 0xFF808000);
        g.drawBox3D(75, 0, 0, 50, 30, 30, 0, 0, 0, 0xFF000080);
        for (int n = 1; n <= 50; ++n)
        {
            const int phi = std::uniform_int_distribution<int>(0, 359)(rng);
            const int r   = std::uniform_int_distribution<int>(0, 100)(rng) + 30;
            for (int w = 0; w <= 359; w += 5)
            {
                const float r1 = (1 - std::cos(radian(static_cast<float>(w)))) * r;
                const float r2 = (1 - std::cos(radian(static_cast<float>(w + 5)))) * r;
                g.drawLine3D(-std::sin(radian(static_cast<float>(w))) * r * 2, r1 * std::sin(radian(static_cast<float>(phi))),
                             r1 * std::cos(radian(static_cast<float>(phi))),
                             -std::sin(radian(static_cast<float>(w + 5))) * r * 2,
                             r2 * std::sin(radian(static_cast<float>(phi))), r2 * std::cos(radian(static_cast<float>(phi))),
                             RGBA(255 * w / 360, 255 * (360 - w) / 360, 255 * (360 - w) / 360, 192));
            }
        }
        break;
    }

    case 9:  // 3D function
    {
        g.addLight(0.5f, 1, 1, 0x20FFFFFF);
        float Y[41][41];
        ColorPB C[41][41];
        for (int z = -20; z <= 20; ++z)
        {
            for (int x = -20; x <= 20; ++x)
            {
                // NB: in the PureBasic original X and Z are integers, so "-X*X/100" and "Z/4" are
                // integer divisions and the surface comes out stepped. Float maths gives the
                // smooth Gaussian that was obviously intended.
                const float fx = static_cast<float>(x), fz = static_cast<float>(z);
                float h = std::exp(-fx * fx / 100.0f - fz * fz / 100.0f) * (fx + fz / 4.0f) / 5.0f;
                ColorPB c;
                if (h < 0)
                    c = RGBA(0, clamp8(128 + 128 * h), clamp8(-255 * h), 255);
                else
                    c = RGBA(clamp8(255 * h), clamp8(128 + 128 * h), 0, 255);
                C[x + 20][z + 20] = c;
                Y[x + 20][z + 20] = h * 200;
            }
        }
        for (int z = -20; z <= 19; ++z)
        {
            for (int x = -20; x <= 19; ++x)
            {
                const int i = x + 20, j = z + 20;
                g.drawTriangle3D(x * 10.f, Y[i][j], z * 10.f, (x + 1) * 10.f, Y[i + 1][j], z * 10.f, x * 10.f,
                                 Y[i][j + 1], (z + 1) * 10.f, C[i][j], C[i + 1][j], C[i][j + 1]);
                g.drawTriangle3D((x + 1) * 10.f, Y[i + 1][j + 1], (z + 1) * 10.f, x * 10.f, Y[i][j + 1],
                                 (z + 1) * 10.f, (x + 1) * 10.f, Y[i + 1][j], z * 10.f, C[i + 1][j + 1], C[i][j + 1],
                                 C[i + 1][j]);
            }
        }
        break;
    }

    case 10:  // icosahedron
    case 11:  // moebius strip
        g.addLight(std::cos(radian(120)), std::sin(radian(120)), 1, 0xFFC08000);
        g.addLight(1, 0, 1, 0xFF00C080);
        g.addLight(std::cos(radian(240)), std::sin(radian(240)), 1, 0xFF8000C0);
        if (scene == 10)
        {
            for (const auto& f : kIcosahedron)
                g.drawTriangle3D(f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], 0xC0404040);
            g.setMode(Mode::Outline);
            for (const auto& f : kIcosahedron)
                g.drawTriangle3D(f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], 0xFF000000);
            g.setMode(Mode::Default);
        }
        else
        {
            const ColorPB color = 0xFF000000;
            for (int phi = 0; phi <= 359; phi += 5)
            {
                for (int r = -50; r <= 49; r += 5)
                {
                    const float phi1 = radian(static_cast<float>(phi)), phi2 = radian(static_cast<float>(phi + 5));
                    const float r1 = static_cast<float>(r), r2 = static_cast<float>(r + 5);
                    auto P = [](float ph, float rr, float out[3]) {
                        out[0] = std::cos(ph) * (100 + rr * std::cos(ph / 2));
                        out[1] = std::sin(ph) * (100 + rr * std::cos(ph / 2));
                        out[2] = std::sin(ph / 2) * rr;
                    };
                    float a[3], b[3], c[3], d[3];
                    P(phi1, r1, a);
                    P(phi2, r1, b);
                    P(phi2, r2, c);
                    P(phi1, r2, d);
                    g.drawTriangle3D(a[0], a[1], a[2], b[0], b[1], b[2], c[0], c[1], c[2], color);
                    g.drawTriangle3D(a[0], a[1], a[2], d[0], d[1], d[2], c[0], c[1], c[2], color);
                }
            }
        }
        break;

    case 12:  // text
        g.drawBox3D(0, 0, 0, 400, 5, 5, 0, 0, 0, 0x40FF8000);
        g.drawBox3D(0, 0, 0, 5, 400, 5, 0, 0, 0, 0x4000E070);
        g.drawBox3D(0, 0, 0, 5, 5, 400, 0, 0, 0, 0x408000FF);
        g.drawText3D(100, 20, 0, 0, 0, 0, "X-Achse", 0x80FF8000);
        g.drawText3D(-100, -20, 0, 0, 0, 180, "X-Achse", 0x80FF8000);
        g.drawText3D(0, 100, 20, 90, 90, 0, "Y-Achse", 0x8000E070);
        g.drawText3D(0, -100, -20, 90, 90, 180, "Y-Achse", 0x8000E070);
        g.drawText3D(20, 0, 100, -90, 0, -90, "Z-Achse", 0x808000FF);
        g.drawText3D(-20, 0, -100, -90, 0, 90, "Z-Achse", 0x808000FF);
        break;
    }
}

// ---------------------------------------------------------------------------
// Examples
// ---------------------------------------------------------------------------

void drawExample01(Drawing3D& g, int width, int height)
{
    g.start(width, height);
    g.setBackground(0xFFFFFFFF);
    g.setMode(Mode::Outline);
    g.drawBox3D(0, 0, 0, 200, 200, 200, 30, 30, 0, 0xFF000000);
    g.stop();
}

void drawExample02(Drawing3D& g, int width, int height)
{
    g.start(width, height);
    g.setBackground(0xFFFFFFFF);
    g.setMode(Mode::Default);
    g.addLight(1, 1, 1, 0x40FFFFFF);
    g.drawBox3D(0, 0, 0, 200, 200, 200, 30, 30, 0, 0xFF804000);
    g.stop();
}

void drawExample03(Drawing3D& g, int width, int height)
{
    g.start(width, height);
    g.setBackground(0xFFFFFFFF);
    g.setMode(Mode::Default);
    g.addLight(1, 1, 1, 0x40FFFFFF);
    g.drawBox3D(-70, 0, 0, 200, 200, 200, 30, 30, 0, 0xC0C04000);
    g.drawBox3D(70, 0, 0, 200, 200, 200, -20, 30, 0, 0xC0400080);
    g.stop();
}

}  // namespace d3d
