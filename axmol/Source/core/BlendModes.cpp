// Animatoon - Axmol 3.0 port
#include "BlendModes.h"

#include "AniImage.h"

#include <cmath>

namespace ani
{

const char* blendModeName(int bm)
{
    static const char* names[] = {"Normal",      "Darken",     "Multiply",   "Color burn", "Linear burn", "Add",
                                  "Screen",      "Lighten",    "Hard light", "Soft light", "Color light", "Overlay",
                                  "Linear light", "Inverse",   "Overlay 2",  "Dissolve",   "Difference",  "Exclusion",
                                  "Saturation",  "Color",      "Vivid light", "Color dodge", "Custom"};
    if (bm < 0 || bm >= Bm_Count)
        return "Normal";
    return names[bm];
}

static inline int clamp255(int v)
{
    return v < 0 ? 0 : (v > 255 ? 255 : v);
}

static inline int chOverlay(int d, int s)
{
    return d < 128 ? (2 * d * s) / 255 : 255 - (2 * (255 - d) * (255 - s)) / 255;
}

static inline int chSoft(int d, int s)
{
    float fd = d / 255.f, fs = s / 255.f;
    float r = fs < 0.5f ? fd - (1 - 2 * fs) * fd * (1 - fd)
                        : fd + (2 * fs - 1) * ((fd < 0.25f ? ((16 * fd - 12) * fd + 4) * fd : std::sqrt(fd)) - fd);
    return clamp255((int)(r * 255 + 0.5f));
}

static inline int chBurn(int d, int s)
{
    return s == 0 ? 0 : clamp255(255 - ((255 - d) * 255) / s);
}

static inline int chDodge(int d, int s)
{
    return s >= 255 ? 255 : clamp255((d * 255) / (255 - s));
}

void blendChannels(int bm, const uint8_t d[3], const uint8_t s[3], int, int, uint8_t out[3])
{
    switch (bm)
    {
    case Bm_Saturation:
    case Bm_Color:
    {
        float dh, ds, dv, sh, ss, sv;
        rgbToHsv(Color(d[0], d[1], d[2]), dh, ds, dv);
        rgbToHsv(Color(s[0], s[1], s[2]), sh, ss, sv);
        Color c = bm == Bm_Saturation ? hsvToRgb(dh, ss, dv) : hsvToRgb(sh, ss, dv);
        out[0] = c.r, out[1] = c.g, out[2] = c.b;
        return;
    }
    default:
        break;
    }
    for (int i = 0; i < 3; ++i)
    {
        int a = d[i], b = s[i], r;
        switch (bm)
        {
        case Bm_Darken:
            r = std::min(a, b);
            break;
        case Bm_Multiply:
            r = a * b / 255;
            break;
        case Bm_ColorBurn:
            r = chBurn(a, b);
            break;
        case Bm_LinearBurn:
            r = clamp255(a + b - 255);
            break;
        case Bm_Add:
            r = clamp255(a + b);
            break;
        case Bm_Screen:
            r = 255 - (255 - a) * (255 - b) / 255;
            break;
        case Bm_Lighten:
            r = std::max(a, b);
            break;
        case Bm_Hardlight:
            r = chOverlay(b, a);
            break;
        case Bm_Clearlight:
            r = chSoft(a, b);
            break;
        case Bm_Colorlight:
        case Bm_ColorDodge:
            r = chDodge(a, b);
            break;
        case Bm_Overlay:
        case Bm_Overlay2:
            r = chOverlay(a, b);
            break;
        case Bm_LinearLight:
            r = clamp255(a + 2 * b - 255);
            break;
        case Bm_Inverse:
            r = 255 - b;
            break;
        case Bm_Difference:
            r = std::abs(a - b);
            break;
        case Bm_Exclusion:
            r = a + b - 2 * a * b / 255;
            break;
        case Bm_VividLight:
            r = b < 128 ? chBurn(a, 2 * b) : chDodge(a, 2 * (b - 128));
            break;
        default:
            r = b;
            break;
        }
        out[i] = (uint8_t)r;
    }
}

void compositeAlpha(int bm, uint8_t* d, const uint8_t* s, int opacity, int x, int y)
{
    int sa = s[3] * opacity / 255;
    if (sa <= 0)
        return;
    int da = d[3];
    if (bm == Bm_Normal || da == 0)
    {
        blendOver(d, s[0], s[1], s[2], sa);
        return;
    }
    if (bm == Bm_Dissolve)
    {
        unsigned h = (unsigned)x * 374761393u + (unsigned)y * 668265263u;
        h          = (h ^ (h >> 13)) * 1274126177u;
        if ((int)((h >> 16) & 0xFF) < sa)
            d[0] = s[0], d[1] = s[1], d[2] = s[2], d[3] = 255;
        return;
    }
    uint8_t o[3];
    blendChannels(bm, d, s, x, y, o);
    // W3C compositing : co = (1-sa)*da*cd + (1-da)*sa*cs + sa*da*B(cd,cs)
    float fsa = sa / 255.f, fda = da / 255.f;
    float ao  = fsa + fda - fsa * fda;
    for (int i = 0; i < 3; ++i)
    {
        float co = (1 - fsa) * fda * d[i] + (1 - fda) * fsa * s[i] + fsa * fda * o[i];
        d[i]     = (uint8_t)clamp255((int)(co / ao + 0.5f));
    }
    d[3] = (uint8_t)clamp255((int)(ao * 255 + 0.5f));
}

} // namespace ani
