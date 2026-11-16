// Animatoon - Axmol 3.0 port
#include "Tools.h"

#include "Util.h"

#include <cmath>
#include <cstring>
#include <vector>

namespace ani
{

static inline void plotCov(uint8_t* d, const Color& c, float cov, bool lockAlpha)
{
    if (cov <= 0)
        return;
    int sa = (int)(c.a * std::min(cov, 1.f) + 0.5f);
    if (sa <= 0)
        return;
    if (lockAlpha)
    {
        if (!d[3])
            return;
        d[0] = (uint8_t)(d[0] + (c.r - d[0]) * sa / 255);
        d[1] = (uint8_t)(d[1] + (c.g - d[1]) * sa / 255);
        d[2] = (uint8_t)(d[2] + (c.b - d[2]) * sa / 255);
    }
    else
        blendOver(d, c.r, c.g, c.b, sa);
}

void drawLine(Image& img, float x0, float y0, float x1, float y1, float width, const Color& c, const IRect& clip,
              bool lockAlpha, IRect* dirty)
{
    float hw = std::max(width, 1.f) * 0.5f;
    IRect r((int)std::floor(std::min(x0, x1) - hw - 1), (int)std::floor(std::min(y0, y1) - hw - 1), 0, 0);
    r.w = (int)std::ceil(std::max(x0, x1) + hw + 1) - r.x + 1;
    r.h = (int)std::ceil(std::max(y0, y1) + hw + 1) - r.y + 1;
    r   = r.intersect(clip).intersect(img.rect());
    if (r.empty())
        return;
    if (dirty)
        dirty->unite(r);
    float dx = x1 - x0, dy = y1 - y0, len2 = dx * dx + dy * dy;
    for (int y = r.y; y < r.bottom(); ++y)
    {
        uint8_t* d = img.at(r.x, y);
        for (int x = r.x; x < r.right(); ++x, d += 4)
        {
            float px = x + 0.5f - x0, py = y + 0.5f - y0;
            float t  = len2 > 0 ? std::clamp((px * dx + py * dy) / len2, 0.f, 1.f) : 0.f;
            float ex = px - dx * t, ey = py - dy * t;
            float dist = std::sqrt(ex * ex + ey * ey);
            plotCov(d, c, hw - dist + 0.5f, lockAlpha);
        }
    }
}

static inline float sdRoundBox(float px, float py, float hx, float hy, float rad)
{
    float qx = std::fabs(px) - hx + rad, qy = std::fabs(py) - hy + rad;
    float ox = std::max(qx, 0.f), oy = std::max(qy, 0.f);
    return std::sqrt(ox * ox + oy * oy) + std::min(std::max(qx, qy), 0.f) - rad;
}

void drawBox(Image& img, float x0, float y0, float x1, float y1, const Color& c, bool filled, float outline,
             float radius, const IRect& clip, bool lockAlpha, IRect* dirty)
{
    if (x1 < x0)
        std::swap(x0, x1);
    if (y1 < y0)
        std::swap(y0, y1);
    float hx = (x1 - x0) * 0.5f, hy = (y1 - y0) * 0.5f, cx = x0 + hx, cy = y0 + hy;
    radius   = std::clamp(radius, 0.f, std::min(hx, hy));
    float ext = filled ? 1.f : outline * 0.5f + 1.f;
    IRect r((int)std::floor(x0 - ext), (int)std::floor(y0 - ext), (int)std::ceil(x1 - x0 + ext * 2) + 1,
            (int)std::ceil(y1 - y0 + ext * 2) + 1);
    r = r.intersect(clip).intersect(img.rect());
    if (r.empty())
        return;
    if (dirty)
        dirty->unite(r);
    for (int y = r.y; y < r.bottom(); ++y)
    {
        uint8_t* d = img.at(r.x, y);
        for (int x = r.x; x < r.right(); ++x, d += 4)
        {
            float sd  = sdRoundBox(x + 0.5f - cx, y + 0.5f - cy, hx, hy, radius);
            float cov = filled ? 0.5f - sd : outline * 0.5f - std::fabs(sd) + 0.5f;
            plotCov(d, c, cov, lockAlpha);
        }
    }
}

void drawEllipse(Image& img, float x0, float y0, float x1, float y1, const Color& c, bool filled, float outline,
                 const IRect& clip, bool lockAlpha, IRect* dirty)
{
    if (x1 < x0)
        std::swap(x0, x1);
    if (y1 < y0)
        std::swap(y0, y1);
    float rx = std::max((x1 - x0) * 0.5f, 0.5f), ry = std::max((y1 - y0) * 0.5f, 0.5f);
    float cx = x0 + rx, cy = y0 + ry;
    float ext = filled ? 1.f : outline * 0.5f + 1.f;
    IRect r((int)std::floor(x0 - ext), (int)std::floor(y0 - ext), (int)std::ceil(x1 - x0 + ext * 2) + 1,
            (int)std::ceil(y1 - y0 + ext * 2) + 1);
    r = r.intersect(clip).intersect(img.rect());
    if (r.empty())
        return;
    if (dirty)
        dirty->unite(r);
    for (int y = r.y; y < r.bottom(); ++y)
    {
        uint8_t* d = img.at(r.x, y);
        for (int x = r.x; x < r.right(); ++x, d += 4)
        {
            // first order approximation of the distance to the ellipse : f / |grad f|
            float px = x + 0.5f - cx, py = y + 0.5f - cy;
            float nx = px / rx, ny = py / ry;
            float f  = nx * nx + ny * ny - 1.f;
            float gx = 2 * nx / rx, gy = 2 * ny / ry;
            float g  = std::sqrt(gx * gx + gy * gy);
            float sd = g > 1e-6f ? f / g : -std::min(rx, ry);
            float cov = filled ? 0.5f - sd : outline * 0.5f - std::fabs(sd) + 0.5f;
            plotCov(d, c, cov, lockAlpha);
        }
    }
}

void drawGradient(Image& img, float x0, float y0, float x1, float y1, const Color& a, const Color& b, int type,
                  const IRect& clip, bool lockAlpha)
{
    IRect r = clip.intersect(img.rect());
    float dx = x1 - x0, dy = y1 - y0;
    float len2 = std::max(dx * dx + dy * dy, 1e-4f), len = std::sqrt(len2);
    float base = std::atan2(dy, dx);
    for (int y = r.y; y < r.bottom(); ++y)
    {
        uint8_t* d = img.at(r.x, y);
        for (int x = r.x; x < r.right(); ++x, d += 4)
        {
            float px = x + 0.5f - x0, py = y + 0.5f - y0, t;
            switch (type)
            {
            case 1: // circular
                t = std::sqrt(px * px + py * py) / len;
                break;
            case 2: // elliptical
            {
                float ex = std::max(std::fabs(dx), 1.f), ey = std::max(std::fabs(dy), 1.f);
                t = std::sqrt((px / ex) * (px / ex) + (py / ey) * (py / ey));
                break;
            }
            case 3: // conical
            {
                float ang = std::atan2(py, px) - base;
                while (ang < 0)
                    ang += 6.2831853f;
                t = ang / 6.2831853f;
                break;
            }
            case 4: // boxed
                t = std::max(std::fabs(px) / std::max(std::fabs(dx), 1.f), std::fabs(py) / std::max(std::fabs(dy), 1.f));
                break;
            default: // linear
                t = (px * dx + py * dy) / len2;
                break;
            }
            t = std::clamp(t, 0.f, 1.f);
            Color c((int)(a.r + (b.r - a.r) * t), (int)(a.g + (b.g - a.g) * t), (int)(a.b + (b.b - a.b) * t),
                    (int)(a.a + (b.a - a.a) * t));
            plotCov(d, c, 1.f, lockAlpha);
        }
    }
}

IRect floodFill(Image& img, const Image& ref, int sx, int sy, const Color& c, int tolerance, const IRect& clipIn,
                bool lockAlpha)
{
    IRect clip = clipIn.intersect(img.rect()).intersect(ref.rect());
    if (!clip.contains(sx, sy))
        return {};
    const int w = img.width();
    Color target = ref.get(sx, sy);
    auto match   = [&](int x, int y) {
        const uint8_t* p = ref.at(x, y);
        // transparent pixels have no meaningful colour : compare the alpha only
        if (target.a == 0 || p[3] == 0)
            return std::abs(p[3] - target.a) <= tolerance;
        return std::abs(p[0] - target.r) <= tolerance && std::abs(p[1] - target.g) <= tolerance &&
               std::abs(p[2] - target.b) <= tolerance && std::abs(p[3] - target.a) <= tolerance;
    };
    std::vector<uint8_t> done((size_t)w * img.height(), 0);
    std::vector<std::pair<int, int>> stack;
    stack.emplace_back(sx, sy);
    IRect dirty;
    while (!stack.empty())
    {
        auto [x, y] = stack.back();
        stack.pop_back();
        if (done[(size_t)y * w + x] || !match(x, y))
            continue;
        // scanline : extend to the left and to the right
        int xl = x, xr = x;
        while (xl - 1 >= clip.x && !done[(size_t)y * w + xl - 1] && match(xl - 1, y))
            --xl;
        while (xr + 1 < clip.right() && !done[(size_t)y * w + xr + 1] && match(xr + 1, y))
            ++xr;
        uint8_t* d = img.at(xl, y);
        for (int i = xl; i <= xr; ++i, d += 4)
        {
            done[(size_t)y * w + i] = 1;
            plotCov(d, c, 1.f, lockAlpha);
        }
        dirty.unite(IRect(xl, y, xr - xl + 1, 1));
        for (int ny : {y - 1, y + 1})
        {
            if (ny < clip.y || ny >= clip.bottom())
                continue;
            bool run = false;
            for (int i = xl; i <= xr; ++i)
            {
                bool ok = !done[(size_t)ny * w + i] && match(i, ny);
                if (ok && !run)
                    stack.emplace_back(i, ny);
                run = ok;
            }
        }
    }
    return dirty;
}

// ------------------------------------------------------------------ adjustments

template <typename F>
static void forEachPixel(Image& img, const IRect& rc, F&& f)
{
    IRect r = rc.intersect(img.rect());
    for (int y = r.y; y < r.bottom(); ++y)
    {
        uint8_t* d = img.at(r.x, y);
        for (int x = r.x; x < r.right(); ++x, d += 4)
            if (d[3])
                f(d, x, y);
    }
}

static inline uint8_t c255(float v)
{
    return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
}

void fxInvert(Image& img, const IRect& r)
{
    forEachPixel(img, r, [](uint8_t* d, int, int) { d[0] = 255 - d[0], d[1] = 255 - d[1], d[2] = 255 - d[2]; });
}

void fxDesaturate(Image& img, const IRect& r)
{
    forEachPixel(img, r, [](uint8_t* d, int, int) {
        int g = (d[0] * 30 + d[1] * 59 + d[2] * 11) / 100;
        d[0] = d[1] = d[2] = (uint8_t)g;
    });
}

void fxBrightnessContrast(Image& img, const IRect& r, int brightness, int contrast)
{
    float f = (259.f * (contrast + 255)) / (255.f * (259 - contrast));
    uint8_t lut[256];
    for (int i = 0; i < 256; ++i)
        lut[i] = c255(f * (i - 128) + 128 + brightness);
    forEachPixel(img, r, [&](uint8_t* d, int, int) { d[0] = lut[d[0]], d[1] = lut[d[1]], d[2] = lut[d[2]]; });
}

void fxPosterize(Image& img, const IRect& r, int levels)
{
    levels = std::clamp(levels, 2, 255);
    uint8_t lut[256];
    for (int i = 0; i < 256; ++i)
        lut[i] = (uint8_t)(std::lround(i * (levels - 1) / 255.f) * 255 / (levels - 1));
    forEachPixel(img, r, [&](uint8_t* d, int, int) { d[0] = lut[d[0]], d[1] = lut[d[1]], d[2] = lut[d[2]]; });
}

void fxLevel(Image& img, const IRect& r, int mn, int mx)
{
    if (mx <= mn)
        mx = mn + 1;
    uint8_t lut[256];
    for (int i = 0; i < 256; ++i)
        lut[i] = c255((i - mn) * 255.f / (mx - mn));
    forEachPixel(img, r, [&](uint8_t* d, int, int) { d[0] = lut[d[0]], d[1] = lut[d[1]], d[2] = lut[d[2]]; });
}

void fxColorBalance(Image& img, const IRect& r, int dr, int dg, int db)
{
    forEachPixel(img, r, [&](uint8_t* d, int, int) {
        d[0] = c255((float)d[0] + dr), d[1] = c255((float)d[1] + dg), d[2] = c255((float)d[2] + db);
    });
}

void fxHueSaturation(Image& img, const IRect& r, int hue, int saturation, int value)
{
    forEachPixel(img, r, [&](uint8_t* d, int, int) {
        float h, s, v;
        rgbToHsv(Color(d[0], d[1], d[2]), h, s, v);
        h += hue;
        s = std::clamp(s * (1.f + saturation / 100.f), 0.f, 1.f);
        v = std::clamp(v * (1.f + value / 100.f), 0.f, 1.f);
        Color c = hsvToRgb(h, s, v);
        d[0] = c.r, d[1] = c.g, d[2] = c.b;
    });
}

// ------------------------------------------------------------------ filters

void fxNoise(Image& img, const IRect& r, int amount, bool mono)
{
    forEachPixel(img, r, [&](uint8_t* d, int, int) {
        int n = rndSym(amount);
        for (int i = 0; i < 3; ++i)
        {
            if (!mono && i)
                n = rndSym(amount);
            d[i] = c255((float)d[i] + n);
        }
    });
}

void fxClouds(Image& img, const IRect& rc, const Color& a, const Color& b, int scale)
{
    IRect r = rc.intersect(img.rect());
    if (r.empty())
        return;
    // value noise, a few octaves
    const int G = 64;
    std::vector<float> grid((size_t)G * G);
    for (auto& g : grid)
        g = rndf();
    auto noise = [&](float x, float y) {
        int x0 = (int)std::floor(x), y0 = (int)std::floor(y);
        float fx = x - x0, fy = y - y0;
        fx = fx * fx * (3 - 2 * fx), fy = fy * fy * (3 - 2 * fy);
        auto g = [&](int xx, int yy) { return grid[(size_t)(((yy % G) + G) % G) * G + (((xx % G) + G) % G)]; };
        return g(x0, y0) * (1 - fx) * (1 - fy) + g(x0 + 1, y0) * fx * (1 - fy) + g(x0, y0 + 1) * (1 - fx) * fy +
               g(x0 + 1, y0 + 1) * fx * fy;
    };
    float base = 1.f / std::max(scale, 4);
    for (int y = r.y; y < r.bottom(); ++y)
    {
        uint8_t* d = img.at(r.x, y);
        for (int x = r.x; x < r.right(); ++x, d += 4)
        {
            float v = 0, amp = 0.5f, f = base;
            for (int o = 0; o < 5; ++o, amp *= 0.5f, f *= 2)
                v += noise(x * f, y * f) * amp;
            v = std::clamp(v / 0.96875f, 0.f, 1.f);
            d[0] = c255(a.r + (b.r - a.r) * v), d[1] = c255(a.g + (b.g - a.g) * v), d[2] = c255(a.b + (b.b - a.b) * v);
            d[3] = 255;
        }
    }
}

void fxBlur(Image& img, const IRect& rc, int radius)
{
    IRect r = rc.intersect(img.rect());
    if (r.empty() || radius < 1)
        return;
    // separable box blur on premultiplied values, 2 passes ~ triangle filter
    std::vector<float> buf((size_t)r.w * r.h * 4), tmp(buf.size());
    for (int y = 0; y < r.h; ++y)
    {
        const uint8_t* s = img.at(r.x, r.y + y);
        float* d         = &buf[(size_t)y * r.w * 4];
        for (int x = 0; x < r.w; ++x, s += 4, d += 4)
            d[0] = s[0] * s[3] / 255.f, d[1] = s[1] * s[3] / 255.f, d[2] = s[2] * s[3] / 255.f, d[3] = s[3];
    }
    for (int pass = 0; pass < 2; ++pass)
    {
        for (int y = 0; y < r.h; ++y) // horizontal
            for (int x = 0; x < r.w; ++x)
            {
                float acc[4] = {0, 0, 0, 0};
                int n = 0;
                for (int k = -radius; k <= radius; ++k)
                {
                    int xx = std::clamp(x + k, 0, r.w - 1);
                    const float* s = &buf[((size_t)y * r.w + xx) * 4];
                    acc[0] += s[0], acc[1] += s[1], acc[2] += s[2], acc[3] += s[3];
                    ++n;
                }
                float* d = &tmp[((size_t)y * r.w + x) * 4];
                d[0] = acc[0] / n, d[1] = acc[1] / n, d[2] = acc[2] / n, d[3] = acc[3] / n;
            }
        for (int y = 0; y < r.h; ++y) // vertical
            for (int x = 0; x < r.w; ++x)
            {
                float acc[4] = {0, 0, 0, 0};
                int n = 0;
                for (int k = -radius; k <= radius; ++k)
                {
                    int yy = std::clamp(y + k, 0, r.h - 1);
                    const float* s = &tmp[((size_t)yy * r.w + x) * 4];
                    acc[0] += s[0], acc[1] += s[1], acc[2] += s[2], acc[3] += s[3];
                    ++n;
                }
                float* d = &buf[((size_t)y * r.w + x) * 4];
                d[0] = acc[0] / n, d[1] = acc[1] / n, d[2] = acc[2] / n, d[3] = acc[3] / n;
            }
    }
    for (int y = 0; y < r.h; ++y)
    {
        uint8_t* d     = img.at(r.x, r.y + y);
        const float* s = &buf[(size_t)y * r.w * 4];
        for (int x = 0; x < r.w; ++x, s += 4, d += 4)
        {
            float a = s[3];
            if (a < 0.5f)
            {
                d[3] = 0;
                continue;
            }
            d[0] = c255(s[0] * 255.f / a), d[1] = c255(s[1] * 255.f / a), d[2] = c255(s[2] * 255.f / a);
            d[3] = c255(a + 0.5f);
        }
    }
}

void fxSharpen(Image& img, const IRect& rc, float amount)
{
    IRect r = rc.intersect(img.rect());
    if (r.empty())
        return;
    Image blurred = img.cropped(r);
    fxBlur(blurred, blurred.rect(), 1);
    for (int y = 0; y < r.h; ++y)
    {
        uint8_t* d       = img.at(r.x, r.y + y);
        const uint8_t* b = blurred.at(0, y);
        for (int x = 0; x < r.w; ++x, d += 4, b += 4)
            if (d[3])
                for (int i = 0; i < 3; ++i)
                    d[i] = c255(d[i] + (d[i] - b[i]) * amount);
    }
}

void fxOffset(Image& img, int dx, int dy)
{
    if (!img.valid())
        return;
    int w = img.width(), h = img.height();
    Image out(w, h);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            std::memcpy(out.at(((x + dx) % w + w) % w, ((y + dy) % h + h) % h), img.at(x, y), 4);
    img = std::move(out);
}

void fxWhiteToAlpha(Image& img, const IRect& r)
{
    forEachPixel(img, r, [](uint8_t* d, int, int) {
        int lum = (d[0] * 30 + d[1] * 59 + d[2] * 11) / 100;
        d[3]    = (uint8_t)(d[3] * (255 - lum) / 255);
        d[0] = d[1] = d[2] = 0;
    });
}

} // namespace ani
