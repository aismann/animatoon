// Animatoon - Axmol 3.0 port
#include "AniImage.h"

#include "axmol/axmol.h"

#include <cmath>
#include <cstring>
#include <fstream>

namespace ani
{

void rgbToHsv(const Color& c, float& h, float& s, float& v)
{
    float r = c.r / 255.f, g = c.g / 255.f, b = c.b / 255.f;
    float mx = std::max({r, g, b}), mn = std::min({r, g, b});
    float d = mx - mn;
    v       = mx;
    s       = mx <= 0 ? 0 : d / mx;
    if (d <= 0)
        h = 0;
    else if (mx == r)
        h = 60.f * std::fmod((g - b) / d, 6.f);
    else if (mx == g)
        h = 60.f * ((b - r) / d + 2);
    else
        h = 60.f * ((r - g) / d + 4);
    if (h < 0)
        h += 360;
}

Color hsvToRgb(float h, float s, float v)
{
    h       = std::fmod(h, 360.f);
    if (h < 0)
        h += 360;
    float c = v * s;
    float x = c * (1 - std::fabs(std::fmod(h / 60.f, 2.f) - 1));
    float m = v - c;
    float r = 0, g = 0, b = 0;
    if (h < 60)
        r = c, g = x;
    else if (h < 120)
        r = x, g = c;
    else if (h < 180)
        g = c, b = x;
    else if (h < 240)
        g = x, b = c;
    else if (h < 300)
        r = x, b = c;
    else
        r = c, b = x;
    return Color((int)std::lround((r + m) * 255), (int)std::lround((g + m) * 255), (int)std::lround((b + m) * 255));
}

void Image::create(int w, int h, Color fill)
{
    _w = std::max(w, 0);
    _h = std::max(h, 0);
    _px.assign((size_t)_w * _h * 4, 0);
    if (fill.r || fill.g || fill.b || fill.a)
        clear(fill);
}

void Image::clear(Color fill)
{
    size_t n = (size_t)_w * _h;
    uint8_t* p = _px.data();
    for (size_t i = 0; i < n; ++i, p += 4)
        p[0] = fill.r, p[1] = fill.g, p[2] = fill.b, p[3] = fill.a;
}

void Image::sample(float x, float y, float out[4]) const
{
    if (!valid())
    {
        out[0] = out[1] = out[2] = out[3] = 0;
        return;
    }
    x = std::clamp(x, 0.f, (float)(_w - 1));
    y = std::clamp(y, 0.f, (float)(_h - 1));
    int x0 = (int)x, y0 = (int)y;
    int x1 = std::min(x0 + 1, _w - 1), y1 = std::min(y0 + 1, _h - 1);
    float fx = x - x0, fy = y - y0;
    const uint8_t *p00 = at(x0, y0), *p10 = at(x1, y0), *p01 = at(x0, y1), *p11 = at(x1, y1);
    float w00 = (1 - fx) * (1 - fy), w10 = fx * (1 - fy), w01 = (1 - fx) * fy, w11 = fx * fy;
    // alpha weighted so that transparent pixels do not bleed their colour
    float a   = p00[3] * w00 + p10[3] * w10 + p01[3] * w01 + p11[3] * w11;
    if (a <= 0.0001f)
    {
        out[0] = out[1] = out[2] = out[3] = 0;
        return;
    }
    for (int c = 0; c < 3; ++c)
        out[c] = (p00[c] * p00[3] * w00 + p10[c] * p10[3] * w10 + p01[c] * p01[3] * w01 + p11[c] * p11[3] * w11) / a;
    out[3] = a;
}

static bool fromAxImage(ax::Image* img, Image& dst)
{
    int w = img->getWidth(), h = img->getHeight();
    if (w <= 0 || h <= 0 || img->isCompressed())
        return false;
    dst.create(w, h);
    const uint8_t* s = img->getData();
    uint8_t* d       = dst.data();
    size_t n         = (size_t)w * h;
    // The pictures are loaded with Image::setPNGPremultipliedAlphaEnabled(false) : the pixels are
    // straight alpha even if the engine flags the image as premultiplied (it expects the shader
    // to do it), so nothing has to be converted here.
    const bool pma = false;
    switch (img->getPixelFormat())
    {
    case ax::rhi::PixelFormat::RGBA8:
        std::memcpy(d, s, n * 4);
        if (pma)
        {
            for (size_t i = 0; i < n; ++i, d += 4)
            {
                int a = d[3];
                if (a > 0 && a < 255)
                {
                    d[0] = (uint8_t)std::min(255, d[0] * 255 / a);
                    d[1] = (uint8_t)std::min(255, d[1] * 255 / a);
                    d[2] = (uint8_t)std::min(255, d[2] * 255 / a);
                }
            }
        }
        return true;
    case ax::rhi::PixelFormat::RGB8:
        for (size_t i = 0; i < n; ++i, d += 4, s += 3)
            d[0] = s[0], d[1] = s[1], d[2] = s[2], d[3] = 255;
        return true;
    case ax::rhi::PixelFormat::R8:
        for (size_t i = 0; i < n; ++i, d += 4, s += 1)
            d[0] = d[1] = d[2] = s[0], d[3] = 255;
        return true;
    case ax::rhi::PixelFormat::RG8:
        for (size_t i = 0; i < n; ++i, d += 4, s += 2)
            d[0] = d[1] = d[2] = s[0], d[3] = s[1];
        return true;
    default:
        break;
    }
    return false;
}

bool Image::load(const std::string& path)
{
    auto img = new ax::Image();
    ax::Image::setPNGPremultipliedAlphaEnabled(false);
    bool ok = img->initWithImageFile(path);
    ax::Image::setPNGPremultipliedAlphaEnabled(true);
    if (ok)
        ok = fromAxImage(img, *this);
    img->release();
    return ok;
}

bool Image::loadFromMemory(const uint8_t* data, size_t size)
{
    auto img = new ax::Image();
    ax::Image::setPNGPremultipliedAlphaEnabled(false);
    bool ok = img->initWithImageData(data, (ssize_t)size);
    ax::Image::setPNGPremultipliedAlphaEnabled(true);
    if (ok)
        ok = fromAxImage(img, *this);
    img->release();
    return ok;
}

bool Image::savePNG(const std::string& path) const
{
    if (!valid())
        return false;
    auto img = new ax::Image();
    bool ok  = img->initWithRawData(_px.data(), (ssize_t)_px.size(), _w, _h, 8, false);
    if (ok)
        ok = img->saveToFile(path, false);
    img->release();
    return ok;
}

bool Image::saveFlattened(const std::string& path, Color background) const
{
    if (!valid())
        return false;
    Image flat(_w, _h, Color(background.r, background.g, background.b, 255));
    flat.drawImage(*this, 0, 0);
    auto img = new ax::Image();
    bool ok  = img->initWithRawData(flat.data(), (ssize_t)flat._px.size(), _w, _h, 8, false);
    if (ok)
        ok = img->saveToFile(path, true);
    img->release();
    return ok;
}

bool Image::encodePNG(std::vector<uint8_t>& out) const
{
    std::string tmp = ax::FileUtils::getInstance()->getWritablePath() + "_animatoon_tmp.png";
    if (!savePNG(tmp))
        return false;
    std::ifstream f(tmp, std::ios::binary);
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    f.close();
    std::remove(tmp.c_str());
    return !out.empty();
}

Image Image::resized(int w, int h, bool smooth) const
{
    Image out(std::max(w, 1), std::max(h, 1));
    if (!valid())
        return out;
    w = out._w, h = out._h;
    float sx = (float)_w / w, sy = (float)_h / h;
    if (!smooth)
    {
        for (int y = 0; y < h; ++y)
        {
            int yy = std::min((int)(y * sy), _h - 1);
            for (int x = 0; x < w; ++x)
            {
                int xx = std::min((int)(x * sx), _w - 1);
                std::memcpy(out.at(x, y), at(xx, yy), 4);
            }
        }
        return out;
    }
    // Box filter when reducing (keeps brush tips smooth), bilinear when enlarging
    bool reduce = sx > 1.5f || sy > 1.5f;
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            uint8_t* d = out.at(x, y);
            if (reduce)
            {
                int x0 = (int)(x * sx), x1 = std::max(x0 + 1, std::min(_w, (int)std::ceil((x + 1) * sx)));
                int y0 = (int)(y * sy), y1 = std::max(y0 + 1, std::min(_h, (int)std::ceil((y + 1) * sy)));
                x1 = std::min(x1, _w), y1 = std::min(y1, _h);
                double r = 0, g = 0, b = 0, a = 0;
                int n = 0;
                for (int yy = y0; yy < y1; ++yy)
                    for (int xx = x0; xx < x1; ++xx)
                    {
                        const uint8_t* s = at(xx, yy);
                        r += s[0] * s[3], g += s[1] * s[3], b += s[2] * s[3], a += s[3];
                        ++n;
                    }
                if (a > 0 && n > 0)
                {
                    d[0] = (uint8_t)(r / a), d[1] = (uint8_t)(g / a), d[2] = (uint8_t)(b / a);
                    d[3] = (uint8_t)(a / n);
                }
            }
            else
            {
                float c[4];
                sample((x + 0.5f) * sx - 0.5f, (y + 0.5f) * sy - 0.5f, c);
                d[0] = (uint8_t)(c[0] + 0.5f), d[1] = (uint8_t)(c[1] + 0.5f), d[2] = (uint8_t)(c[2] + 0.5f);
                d[3] = (uint8_t)(c[3] + 0.5f);
            }
        }
    }
    return out;
}

Image Image::rotated(float angleDeg, bool smooth) const
{
    if (!valid())
        return Image();
    float a  = angleDeg * 3.14159265f / 180.f;
    float cs = std::cos(a), sn = std::sin(a);
    int nw = (int)std::ceil(std::fabs(_w * cs) + std::fabs(_h * sn));
    int nh = (int)std::ceil(std::fabs(_w * sn) + std::fabs(_h * cs));
    Image out(std::max(nw, 1), std::max(nh, 1));
    float cx = _w * 0.5f, cy = _h * 0.5f, ncx = out._w * 0.5f, ncy = out._h * 0.5f;
    for (int y = 0; y < out._h; ++y)
    {
        for (int x = 0; x < out._w; ++x)
        {
            float dx = x + 0.5f - ncx, dy = y + 0.5f - ncy;
            float sx = dx * cs + dy * sn + cx - 0.5f;
            float sy = -dx * sn + dy * cs + cy - 0.5f;
            if (sx < -0.5f || sy < -0.5f || sx > _w - 0.5f || sy > _h - 0.5f)
                continue;
            uint8_t* d = out.at(x, y);
            if (smooth)
            {
                float c[4];
                sample(sx, sy, c);
                d[0] = (uint8_t)(c[0] + 0.5f), d[1] = (uint8_t)(c[1] + 0.5f), d[2] = (uint8_t)(c[2] + 0.5f);
                d[3] = (uint8_t)(c[3] + 0.5f);
            }
            else
                std::memcpy(d, at(std::clamp((int)std::lround(sx), 0, _w - 1), std::clamp((int)std::lround(sy), 0, _h - 1)), 4);
        }
    }
    return out;
}

Image Image::rotated90(int q) const
{
    q = ((q % 4) + 4) % 4;
    if (q == 0 || !valid())
        return *this;
    Image out((q % 2) ? _h : _w, (q % 2) ? _w : _h);
    for (int y = 0; y < _h; ++y)
        for (int x = 0; x < _w; ++x)
        {
            int nx, ny;
            if (q == 1) // 90 clockwise
                nx = _h - 1 - y, ny = x;
            else if (q == 2)
                nx = _w - 1 - x, ny = _h - 1 - y;
            else
                nx = y, ny = _w - 1 - x;
            std::memcpy(out.at(nx, ny), at(x, y), 4);
        }
    return out;
}

Image Image::mirrored(bool horizontal) const
{
    Image out(_w, _h);
    for (int y = 0; y < _h; ++y)
        for (int x = 0; x < _w; ++x)
            std::memcpy(out.at(horizontal ? _w - 1 - x : x, horizontal ? y : _h - 1 - y), at(x, y), 4);
    return out;
}

Image Image::cropped(const IRect& r) const
{
    Image out(std::max(r.w, 1), std::max(r.h, 1));
    out.copyFrom(*this, -r.x, -r.y);
    return out;
}

IRect Image::opaqueBounds() const
{
    int x0 = _w, y0 = _h, x1 = -1, y1 = -1;
    for (int y = 0; y < _h; ++y)
    {
        const uint8_t* p = at(0, y);
        for (int x = 0; x < _w; ++x, p += 4)
            if (p[3])
            {
                x0 = std::min(x0, x), x1 = std::max(x1, x);
                y0 = std::min(y0, y), y1 = std::max(y1, y);
            }
    }
    if (x1 < 0)
        return {};
    return {x0, y0, x1 - x0 + 1, y1 - y0 + 1};
}

void Image::drawImage(const Image& src, int dx, int dy, int opacity)
{
    IRect r = IRect(dx, dy, src._w, src._h).intersect(rect());
    if (r.empty() || opacity <= 0)
        return;
    for (int y = r.y; y < r.bottom(); ++y)
    {
        const uint8_t* s = src.at(r.x - dx, y - dy);
        uint8_t* d       = at(r.x, y);
        for (int x = 0; x < r.w; ++x, s += 4, d += 4)
            blendOver(d, s[0], s[1], s[2], opacity >= 255 ? s[3] : s[3] * opacity / 255);
    }
}

void Image::copyFrom(const Image& src, int dx, int dy)
{
    IRect r = IRect(dx, dy, src._w, src._h).intersect(rect());
    if (r.empty())
        return;
    for (int y = r.y; y < r.bottom(); ++y)
        std::memcpy(at(r.x, y), src.at(r.x - dx, y - dy), (size_t)r.w * 4);
}

void Image::fillRect(const IRect& rc, const Color& c, bool blend)
{
    IRect r = rc.intersect(rect());
    for (int y = r.y; y < r.bottom(); ++y)
    {
        uint8_t* d = at(r.x, y);
        for (int x = 0; x < r.w; ++x, d += 4)
        {
            if (blend)
                blendOver(d, c.r, c.g, c.b, c.a);
            else
                d[0] = c.r, d[1] = c.g, d[2] = c.b, d[3] = c.a;
        }
    }
}

} // namespace ani
