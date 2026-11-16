// Animatoon - Axmol 3.0 port
// Layer blend modes. The enum order is the one of the PureBasic version (#Bm_xxx)
// because the index is written in the .ani documents.
#pragma once

#include <algorithm>
#include <cstdint>

namespace ani
{

enum BlendMode : int
{
    Bm_Normal = 0,
    Bm_Darken,
    Bm_Multiply,
    Bm_ColorBurn,
    Bm_LinearBurn,
    Bm_Add,
    Bm_Screen,
    Bm_Lighten,
    Bm_Hardlight,
    Bm_Clearlight, // soft light
    Bm_Colorlight, // colour dodge (old name)
    Bm_Overlay,
    Bm_LinearLight,
    Bm_Inverse,
    Bm_Overlay2,
    Bm_Dissolve,
    Bm_Difference,
    Bm_Exclusion,
    Bm_Saturation,
    Bm_Color,
    Bm_VividLight,
    Bm_ColorDodge,
    Bm_Custom,
    Bm_Count
};

const char* blendModeName(int bm);

/// Blend the three colour channels of src (s) over dst (d), result in out (all 0..255)
void blendChannels(int bm, const uint8_t d[3], const uint8_t s[3], int x, int y, uint8_t out[3]);

/// Composite a straight alpha source pixel on an OPAQUE destination (the screen / flattened picture)
inline void compositeOpaque(int bm, uint8_t* d, const uint8_t* s, int opacity, int x, int y)
{
    int sa = s[3] * opacity / 255;
    if (sa <= 0)
        return;
    if (bm == Bm_Normal)
    {
        d[0] = (uint8_t)(d[0] + (s[0] - d[0]) * sa / 255);
        d[1] = (uint8_t)(d[1] + (s[1] - d[1]) * sa / 255);
        d[2] = (uint8_t)(d[2] + (s[2] - d[2]) * sa / 255);
        return;
    }
    if (bm == Bm_Dissolve)
    {
        // pseudo random threshold, stable for a pixel position
        unsigned h = (unsigned)x * 374761393u + (unsigned)y * 668265263u;
        h          = (h ^ (h >> 13)) * 1274126177u;
        if ((int)((h >> 16) & 0xFF) < sa)
            d[0] = s[0], d[1] = s[1], d[2] = s[2];
        return;
    }
    uint8_t o[3];
    blendChannels(bm, d, s, x, y, o);
    d[0] = (uint8_t)(d[0] + (o[0] - d[0]) * sa / 255);
    d[1] = (uint8_t)(d[1] + (o[1] - d[1]) * sa / 255);
    d[2] = (uint8_t)(d[2] + (o[2] - d[2]) * sa / 255);
}

/// Composite a straight alpha source pixel on a straight alpha destination (merge of two layers)
void compositeAlpha(int bm, uint8_t* d, const uint8_t* s, int opacity, int x, int y);

} // namespace ani
