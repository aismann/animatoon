// Animatoon - Axmol 3.0 port
// CPU image buffer (RGBA8, straight / non-premultiplied alpha), the equivalent of the
// PureBasic 32 bit images used by the original program.
#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace ani
{

struct Color
{
    uint8_t r = 0, g = 0, b = 0, a = 255;
    Color() = default;
    Color(int r_, int g_, int b_, int a_ = 255)
        : r((uint8_t)std::clamp(r_, 0, 255))
        , g((uint8_t)std::clamp(g_, 0, 255))
        , b((uint8_t)std::clamp(b_, 0, 255))
        , a((uint8_t)std::clamp(a_, 0, 255))
    {}
    bool operator==(const Color& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
    /// PureBasic colour integer (R | G<<8 | B<<16), used in the .ini / .ani files
    int toPB() const { return r | (g << 8) | (b << 16); }
    static Color fromPB(int v, int alpha = 255) { return Color(v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF, alpha); }
};

inline Color mixColor(const Color& c1, const Color& c2, float scale)
{
    // ColorBlending() of the original : c1 * scale + c2 * (1 - scale)
    scale = std::clamp(scale, 0.f, 1.f);
    return Color((int)(c1.r * scale + c2.r * (1 - scale)), (int)(c1.g * scale + c2.g * (1 - scale)),
                 (int)(c1.b * scale + c2.b * (1 - scale)), 255);
}

void rgbToHsv(const Color& c, float& h, float& s, float& v);
Color hsvToRgb(float h, float s, float v);

struct IRect
{
    int x = 0, y = 0, w = 0, h = 0;
    IRect() = default;
    IRect(int x_, int y_, int w_, int h_) : x(x_), y(y_), w(w_), h(h_) {}
    bool empty() const { return w <= 0 || h <= 0; }
    int right() const { return x + w; }
    int bottom() const { return y + h; }
    bool contains(int px, int py) const { return px >= x && py >= y && px < x + w && py < y + h; }
    IRect intersect(const IRect& o) const
    {
        int x0 = std::max(x, o.x), y0 = std::max(y, o.y);
        int x1 = std::min(right(), o.right()), y1 = std::min(bottom(), o.bottom());
        if (x1 <= x0 || y1 <= y0)
            return {};
        return {x0, y0, x1 - x0, y1 - y0};
    }
    void unite(const IRect& o)
    {
        if (o.empty())
            return;
        if (empty())
        {
            *this = o;
            return;
        }
        int x0 = std::min(x, o.x), y0 = std::min(y, o.y);
        int x1 = std::max(right(), o.right()), y1 = std::max(bottom(), o.bottom());
        x = x0, y = y0, w = x1 - x0, h = y1 - y0;
    }
};

class Image
{
public:
    Image() = default;
    Image(int w, int h, Color fill = Color(0, 0, 0, 0)) { create(w, h, fill); }

    void create(int w, int h, Color fill = Color(0, 0, 0, 0));
    void clear(Color fill = Color(0, 0, 0, 0));
    bool valid() const { return _w > 0 && _h > 0; }
    int width() const { return _w; }
    int height() const { return _h; }
    IRect rect() const { return {0, 0, _w, _h}; }

    uint8_t* data() { return _px.data(); }
    const uint8_t* data() const { return _px.data(); }
    uint8_t* at(int x, int y) { return &_px[((size_t)y * _w + x) * 4]; }
    const uint8_t* at(int x, int y) const { return &_px[((size_t)y * _w + x) * 4]; }
    bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < _w && y < _h; }
    Color get(int x, int y) const
    {
        if (!inside(x, y))
            return Color(0, 0, 0, 0);
        const uint8_t* p = at(x, y);
        return Color(p[0], p[1], p[2], p[3]);
    }
    void set(int x, int y, const Color& c)
    {
        if (!inside(x, y))
            return;
        uint8_t* p = at(x, y);
        p[0] = c.r, p[1] = c.g, p[2] = c.b, p[3] = c.a;
    }
    /// Bilinear sample (clamped), returns straight alpha colour components as floats 0..255
    void sample(float x, float y, float out[4]) const;

    // ---- file IO (uses ax::Image) ----
    bool load(const std::string& path);
    bool savePNG(const std::string& path) const;
    /// Save without alpha (jpg or png depending on extension)
    bool saveFlattened(const std::string& path, Color background) const;
    bool loadFromMemory(const uint8_t* data, size_t size);
    /// Encodes the image as PNG into memory (through a temporary file)
    bool encodePNG(std::vector<uint8_t>& out) const;

    // ---- transforms (all return a new image) ----
    Image resized(int w, int h, bool smooth = true) const;
    Image rotated(float angleDeg, bool smooth = true) const; ///< free rotation, bounding box grows
    Image rotated90(int quarterTurns) const;                 ///< lossless 90/180/270
    Image mirrored(bool horizontal) const;
    Image cropped(const IRect& r) const;
    /// Bounding box of the non transparent pixels
    IRect opaqueBounds() const;

    /// Source-over copy of src into this at (dx,dy) with a global opacity 0..255
    void drawImage(const Image& src, int dx, int dy, int opacity = 255);
    /// Raw copy (replace) of src into this at (dx,dy)
    void copyFrom(const Image& src, int dx, int dy);
    void fillRect(const IRect& r, const Color& c, bool blend = false);

private:
    int _w = 0, _h = 0;
    std::vector<uint8_t> _px;
};

/// source-over of a straight alpha colour onto a straight alpha destination pixel
inline void blendOver(uint8_t* d, int r, int g, int b, int sa)
{
    if (sa <= 0)
        return;
    int da = d[3];
    if (sa >= 255 || da == 0)
    {
        if (da == 0)
        {
            d[0] = (uint8_t)r, d[1] = (uint8_t)g, d[2] = (uint8_t)b, d[3] = (uint8_t)sa;
            return;
        }
        if (sa >= 255)
        {
            d[0] = (uint8_t)r, d[1] = (uint8_t)g, d[2] = (uint8_t)b, d[3] = 255;
            return;
        }
    }
    int inv  = 255 - sa;
    int outA = sa * 255 + da * inv; // 0..65025
    d[0]     = (uint8_t)((r * sa * 255 + d[0] * da * inv) / outA);
    d[1]     = (uint8_t)((g * sa * 255 + d[1] * da * inv) / outA);
    d[2]     = (uint8_t)((b * sa * 255 + d[2] * da * inv) / outA);
    d[3]     = (uint8_t)((outA + 127) / 255);
}

} // namespace ani
