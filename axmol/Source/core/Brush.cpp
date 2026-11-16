// Animatoon - Axmol 3.0 port
#include "Brush.h"

#include "Tools.h"
#include "Util.h"

#include <cmath>
#include <cstring>

namespace ani
{

const char* actionName(int a)
{
    static const char* names[] = {"Pen",    "Brush",  "Spray",   "Clone stamp", "Particles", "Line",   "Box",
                                  "Circle", "Shape",  "Gradient", "Fill",       "Text",      "Eraser", "Clear",
                                  "Pipette", "Select", "Move",    "Transform",  "Rotate",    "Hand",   "Zoom"};
    return (a < 0 || a >= Action_Count) ? "" : names[a];
}

const char* toolTypeName(int t)
{
    static const char* names[] = {"Brush", "Pen",   "Eraser", "Light", "Dark", "Color", "Shape", "Water",
                                  "Smudge", "Glass", "Noise", "Pixel", "Blur", "Solarize", "Line", "Clone stamp"};
    return (t < 0 || t >= ToolType_Count) ? "" : names[t];
}

// ------------------------------------------------------------------ presets

bool BrushParams::load(const std::string& file)
{
    IniFile ini;
    if (!ini.load(file))
        return false;
    name         = ini.getString("name", pathStem(file));
    version      = ini.getInt("version", 0);
    size         = std::max(1, ini.getInt("size", 32));
    sizeW        = ini.getInt("sizeX", 100);
    sizeH        = ini.getInt("sizeY", 100);
    sizeRand     = ini.getInt("randSiz", 0);
    sizeMin      = ini.getInt("size_min", 1);
    sizeRndFactor = ini.getInt("brushsizeRndfactor", 0);
    sizePressure = ini.getInt("sizepressure", 0);

    rotate        = ini.getInt("rotate", 0);
    randRot       = ini.getInt("randRot", 0);
    rotateByAngle = ini.getInt("rotateangle", 0);
    rotateMin     = ini.getInt("RotateMin", 0);

    alpha             = ini.getInt("alpha", 255);
    alphaFactorVsTime = ini.getInt("alphaFactorVsTime", 0);
    alphaVsTime       = ini.getInt("alphaVsTime", 0);
    alphaPressure     = ini.getInt("alphapressure", 0);
    alphaRand         = ini.getInt("alphaRand", 0);
    alphaMin          = ini.getInt("alphaMin", 0);

    addNoiseOnImgMax      = ini.getInt("AddNoiseOnImgMax", 0);
    addNoiseOnImgMin      = ini.getInt("AddNoiseOnImgMin", 0);
    addNoiseRandomOnImage = ini.getInt("AddNoiseRandomOnImagebrush", 0);

    type    = std::clamp(ini.getInt("tool", 0), 0, ToolType_Count - 1);
    scatter = ini.getInt("scatter", 10);

    if (version < 6)
    {
        // old presets (see OpenPreset() of the PureBasic version)
        pas = ini.getInt("pas", 5) + (1 - version) * 20;
        if (rotate == 1)
            randRot = 360;
        alphaRand = ini.getInt("randTra", 0);
        alphaMin  = ini.getInt("transpmini", 0);
    }
    else
        pas = ini.getInt("pas", 5);
    if (pas < 0)
        pas = 5;

    trait     = ini.getInt("paintline", 0);
    hardness  = ini.getInt("hardness", 0);
    softness  = ini.getInt("softness", 0);
    intensity = ini.getInt("intensity", 0);
    smooth    = ini.getInt("smooth", 0);
    trim      = ini.getInt("trim", 0);

    mix     = ini.getInt("mixing", 0);
    visco   = ini.getInt("visco", 0);
    mixType = ini.getInt("mixtyp", 2);
    water   = ini.getInt("water", 0);
    wash    = ini.getInt("wash", 1);
    symetry = ini.getInt("symetry", 0);
    filter  = ini.getInt("filter", 20);

    image    = ini.getString("image", "");
    brushDir = ini.getString("brushdir", "blendman");
    id       = ini.getInt("brush", 5);
    if (image.empty())
        image = "brush" + std::to_string(id) + ".png";
    return true;
}

bool BrushParams::save(const std::string& file) const
{
    IniFile ini;
    ini.set("name", name);
    ini.set("size", size);
    ini.set("sizeX", sizeW);
    ini.set("sizeY", sizeH);
    ini.set("randSiz", sizeRand);
    ini.set("size_min", sizeMin);
    ini.set("sizepressure", sizePressure);
    ini.set("brushsizeRndfactor", sizeRndFactor);
    ini.set("rotate", rotate);
    ini.set("randRot", randRot);
    ini.set("rotateangle", rotateByAngle);
    ini.set("RotateMin", rotateMin);
    ini.set("scatter", scatter);
    ini.set("alpha", alpha);
    ini.set("alphaFactorVsTime", alphaFactorVsTime);
    ini.set("alphaVsTime", alphaVsTime);
    ini.set("alphapressure", alphaPressure);
    ini.set("alphaRand", alphaRand);
    ini.set("alphaMin", alphaMin);
    ini.set("AddNoiseRandomOnImagebrush", addNoiseRandomOnImage);
    ini.set("AddNoiseOnImgMin", addNoiseOnImgMin);
    ini.set("AddNoiseOnImgMax", addNoiseOnImgMax);
    ini.set("image", image);
    ini.set("brushdir", brushDir);
    ini.set("brush", id);
    ini.set("pas", pas);
    ini.set("mixing", mix);
    ini.set("visco", visco);
    ini.set("mixtyp", mixType);
    ini.set("water", water);
    ini.set("wash", wash);
    ini.set("symetry", symetry);
    ini.set("filter", filter);
    ini.set("tool", type);
    ini.set("version", 6);
    ini.set("paintline", trait);
    ini.set("hardness", hardness);
    ini.set("softness", softness);
    ini.set("intensity", intensity);
    ini.set("smooth", smooth);
    ini.set("trim", trim);
    return ini.save(file, "//Animatoon brush Preset - Blendman 2015");
}

// ------------------------------------------------------------------ tip

static void finishMask(BrushTip& tip, const BrushParams& p)
{
    for (auto& m : tip.mask)
    {
        int a = m;
        if (a <= 0)
            continue;
        if (p.intensity >= 0)
            a = std::max(a, std::min(p.intensity, 255));
        else
            a = std::min(a, 255 + p.intensity);
        if (a < p.hardness)
            a = 0;
        if (a > 0 && p.addNoiseRandomOnImage > 0)
        {
            int lo = a * p.addNoiseOnImgMin / 10, hi = a * p.addNoiseOnImgMax / 10;
            a -= rndRange(lo, hi);
        }
        m = (uint8_t)std::clamp(a, 0, 255);
    }
}

void BrushTip::build(const Image& source, const BrushParams& p)
{
    if (!source.valid())
    {
        buildRound(p);
        return;
    }
    // 1) the picture becomes an alpha mask. Pictures without alpha (jpg) : dark = opaque
    Image img = source;
    {
        bool hasAlpha = false;
        size_t n      = (size_t)img.width() * img.height();
        const uint8_t* s = img.data();
        for (size_t i = 0; i < n && !hasAlpha; ++i)
            hasAlpha = s[i * 4 + 3] < 255;
        if (!hasAlpha)
        {
            uint8_t* d = img.data();
            for (size_t i = 0; i < n; ++i, d += 4)
                d[3] = (uint8_t)(255 - (d[0] * 30 + d[1] * 59 + d[2] * 11) / 100);
        }
    }
    if (p.trim)
    {
        IRect b = img.opaqueBounds();
        if (!b.empty())
            img = img.cropped(b);
    }
    // 2) size
    int sw = std::max(2, p.sizeW * p.size / 100);
    int sh = std::max(2, p.sizeH * p.size / 100);
    img    = img.resized(sw, sh, true);
    // 3) rotation
    if (p.rotate % 360 != 0)
        img = img.rotated((float)p.rotate, true);
    w = img.width(), h = img.height();
    mask.resize((size_t)w * h);
    const uint8_t* s = img.data();
    for (size_t i = 0; i < mask.size(); ++i)
        mask[i] = s[i * 4 + 3];
    finishMask(*this, p);
}

void BrushTip::buildRound(const BrushParams& p)
{
    w = std::max(2, p.sizeW * p.size / 100);
    h = std::max(2, p.sizeH * p.size / 100);
    mask.assign((size_t)w * h, 0);
    float soft = std::clamp(1.f - p.hardness / 255.f, 0.05f, 1.f);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            float dx = (x + 0.5f - w * 0.5f) / (w * 0.5f), dy = (y + 0.5f - h * 0.5f) / (h * 0.5f);
            float d  = std::sqrt(dx * dx + dy * dy);
            float a  = std::clamp((1.f - d) / soft, 0.f, 1.f);
            mask[(size_t)y * w + x] = (uint8_t)(a * 255);
        }
}

Image BrushTip::toImage(const Color& c) const
{
    Image img(std::max(w, 1), std::max(h, 1));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            img.set(x, y, Color(c.r, c.g, c.b, mask[(size_t)y * w + x]));
    return img;
}

// ------------------------------------------------------------------ engine

void BrushEngine::updateTip(const BrushParams& p)
{
    std::string key = p.brushDir + "/" + p.image;
    if (key != _sourceKey)
    {
        _sourceKey = key;
        _source    = Image();
        std::string path = resolvePath("data/Presets/Brush/" + p.brushDir + "/" + p.image);
        if (!fileExists(path))
            path = resolvePath("data/Presets/Brush/" + p.brushDir + "/brush" + std::to_string(p.id) + ".png");
        if (fileExists(path))
            _source.load(path);
    }
    _tip.build(_source, p);
}

static inline void lerpPixel(uint8_t* d, const uint8_t* s, int t /*0..255*/)
{
    if (t <= 0)
        return;
    int it = 255 - t;
    int pa = d[3] * it + s[3] * t; // alpha * 255
    if (pa <= 0)
    {
        d[3] = 0;
        return;
    }
    for (int c = 0; c < 3; ++c)
        d[c] = (uint8_t)((d[c] * d[3] * it + s[c] * s[3] * t) / pa);
    d[3] = (uint8_t)((pa + 127) / 255);
}

void BrushEngine::dab(Image& img, const BrushParams& p, int type, float cx, float cy, float scale, float angleDeg,
                      const Color& c, int alpha, const IRect& clip, bool lockAlpha, IRect* dirty)
{
    if (!_tip.valid() || alpha <= 0 || scale <= 0.001f)
        return;
    float a  = angleDeg * 3.14159265f / 180.f;
    float cs = std::cos(a), sn = std::sin(a);
    float hw = _tip.w * scale * 0.5f, hh = _tip.h * scale * 0.5f;
    float bw = std::fabs(hw * cs) + std::fabs(hh * sn), bh = std::fabs(hw * sn) + std::fabs(hh * cs);
    int x0 = (int)std::floor(cx - bw), y0 = (int)std::floor(cy - bh);
    int x1 = (int)std::ceil(cx + bw), y1 = (int)std::ceil(cy + bh);
    IRect r = IRect(x0, y0, x1 - x0 + 1, y1 - y0 + 1).intersect(clip).intersect(img.rect());
    if (r.empty())
        return;
    if (dirty)
        dirty->unite(r);

    const bool needCopy = type == ToolType_Blur || type == ToolType_Pixel || type == ToolType_Glass;
    if (needCopy)
    {
        _scratch.resize((size_t)r.w * r.h * 4);
        for (int y = 0; y < r.h; ++y)
            std::memcpy(&_scratch[(size_t)y * r.w * 4], img.at(r.x, r.y + y), (size_t)r.w * 4);
    }
    auto scratchAt = [&](int x, int y) -> const uint8_t* {
        x = std::clamp(x, r.x, r.right() - 1) - r.x;
        y = std::clamp(y, r.y, r.bottom() - 1) - r.y;
        return &_scratch[((size_t)y * r.w + x) * 4];
    };

    const float inv   = 1.f / scale;
    const float tcx   = _tip.w * 0.5f, tcy = _tip.h * 0.5f;
    const uint8_t* mk = _tip.mask.data();
    const int tw = _tip.w, th = _tip.h;
    const float k = (0.02f + p.filter * 0.002f);

    for (int y = r.y; y < r.bottom(); ++y)
    {
        uint8_t* d = img.at(r.x, y);
        float dy   = y + 0.5f - cy;
        for (int x = r.x; x < r.right(); ++x, d += 4)
        {
            float dx = x + 0.5f - cx;
            float u  = (dx * cs + dy * sn) * inv + tcx - 0.5f;
            float v  = (-dx * sn + dy * cs) * inv + tcy - 0.5f;
            if (u <= -1 || v <= -1 || u >= tw || v >= th)
                continue;
            // bilinear sample of the mask (0 outside)
            int u0 = (int)std::floor(u), v0 = (int)std::floor(v);
            float fu = u - u0, fv = v - v0;
            auto M = [&](int xx, int yy) -> float {
                return (xx < 0 || yy < 0 || xx >= tw || yy >= th) ? 0.f : (float)mk[(size_t)yy * tw + xx];
            };
            float m = M(u0, v0) * (1 - fu) * (1 - fv) + M(u0 + 1, v0) * fu * (1 - fv) + M(u0, v0 + 1) * (1 - fu) * fv +
                      M(u0 + 1, v0 + 1) * fu * fv;
            int sa = (int)(m * alpha / 255.f + 0.5f);
            if (sa <= 0)
                continue;

            switch (type)
            {
            case ToolType_Eraser:
                d[3] = (uint8_t)std::max(0, d[3] - sa);
                break;
            case ToolType_Water:
                d[3] = (uint8_t)std::max(0, d[3] - sa / 2);
                break;
            case ToolType_Light:
                if (d[3])
                    for (int i = 0; i < 3; ++i)
                        d[i] = (uint8_t)std::min(255, (int)(d[i] + (d[i] * k + 2) * sa / 255.f));
                break;
            case ToolType_Dark:
                if (d[3])
                    for (int i = 0; i < 3; ++i)
                        d[i] = (uint8_t)std::max(0, (int)(d[i] - (d[i] * k + 2) * sa / 255.f));
                break;
            case ToolType_Color:
                if (d[3])
                {
                    d[0] = (uint8_t)(d[0] + (c.r - d[0]) * sa / 255);
                    d[1] = (uint8_t)(d[1] + (c.g - d[1]) * sa / 255);
                    d[2] = (uint8_t)(d[2] + (c.b - d[2]) * sa / 255);
                }
                break;
            case ToolType_Noise:
                if (d[3])
                {
                    int n = rndSym(10 + p.filter) * sa / 255;
                    for (int i = 0; i < 3; ++i)
                        d[i] = (uint8_t)std::clamp(d[i] + n, 0, 255);
                }
                break;
            case ToolType_Sol:
                if (d[3])
                    for (int i = 0; i < 3; ++i)
                    {
                        int t = d[i] > 128 ? 255 - d[i] : d[i];
                        d[i]  = (uint8_t)(d[i] + (t - d[i]) * sa / 255);
                    }
                break;
            case ToolType_Blur:
            {
                int pr = 0, pg = 0, pb = 0, pa = 0;
                for (int j = -1; j <= 1; ++j)
                    for (int i = -1; i <= 1; ++i)
                    {
                        const uint8_t* s = scratchAt(x + i, y + j);
                        pr += s[0] * s[3], pg += s[1] * s[3], pb += s[2] * s[3], pa += s[3];
                    }
                uint8_t avg[4] = {0, 0, 0, (uint8_t)(pa / 9)};
                if (pa > 0)
                    avg[0] = (uint8_t)(pr / pa), avg[1] = (uint8_t)(pg / pa), avg[2] = (uint8_t)(pb / pa);
                lerpPixel(d, avg, sa);
                break;
            }
            case ToolType_Pixel:
            {
                int block = 2 + p.filter / 5;
                lerpPixel(d, scratchAt(x / block * block, y / block * block), sa);
                break;
            }
            case ToolType_Glass:
            {
                int j = 2 + p.filter / 10;
                lerpPixel(d, scratchAt(x + rndSym(j), y + rndSym(j)), sa);
                break;
            }
            case ToolType_CloneStamp:
                if (_pattern && _pattern->valid())
                {
                    const uint8_t* s = _pattern->at(((x % _pattern->width()) + _pattern->width()) % _pattern->width(),
                                                    ((y % _pattern->height()) + _pattern->height()) % _pattern->height());
                    int da = d[3];
                    blendOver(d, s[0], s[1], s[2], sa * s[3] / 255);
                    if (lockAlpha)
                        d[3] = (uint8_t)da;
                    break;
                }
                [[fallthrough]];
            default:
            {
                if (lockAlpha)
                {
                    if (d[3])
                    {
                        d[0] = (uint8_t)(d[0] + (c.r - d[0]) * sa / 255);
                        d[1] = (uint8_t)(d[1] + (c.g - d[1]) * sa / 255);
                        d[2] = (uint8_t)(d[2] + (c.b - d[2]) * sa / 255);
                    }
                }
                else if (p.blendmode == Bm_Multiply && d[3])
                {
                    int da = d[3];
                    blendOver(d, d[0] * c.r / 255, d[1] * c.g / 255, d[2] * c.b / 255, sa);
                    d[3] = (uint8_t)std::max(da, (int)d[3]);
                }
                else if (p.blendmode == Bm_Dissolve)
                {
                    if (rnd(255) < sa)
                        d[0] = c.r, d[1] = c.g, d[2] = c.b, d[3] = (uint8_t)std::max((int)d[3], std::min(255, sa * 2));
                }
                else
                    blendOver(d, c.r, c.g, c.b, sa);
                break;
            }
            }
        }
    }
}

void BrushEngine::dabWithSymetry(Document& doc, const BrushParams& p, float x, float y, float scale, float angle,
                                 const Color& c, int alpha)
{
    Layer& l   = doc.cur();
    Image& img = l.image;
    IRect dirty;
    auto one = [&](float px, float py, float ang) {
        if (_type == ToolType_Smudge)
        {
            // drag the picture taken under the previous dot
            int S  = (int)std::ceil(std::max(_tip.w, _tip.h) * scale * 1.5f) + 2;
            int ox = (int)std::lround(px) - S / 2, oy = (int)std::lround(py) - S / 2;
            IRect r = IRect(ox, oy, S, S).intersect(_clip).intersect(img.rect());
            if (_hasSmudge && _smudge.width() == S && !r.empty())
            {
                float strength = (p.mix > 0 ? p.mix : 60) / 100.f;
                float inv      = 1.f / scale;
                for (int yy = r.y; yy < r.bottom(); ++yy)
                {
                    uint8_t* d = img.at(r.x, yy);
                    for (int xx = r.x; xx < r.right(); ++xx, d += 4)
                    {
                        float u = (xx + 0.5f - px) * inv + _tip.w * 0.5f, v = (yy + 0.5f - py) * inv + _tip.h * 0.5f;
                        if (u < 0 || v < 0 || u >= _tip.w || v >= _tip.h)
                            continue;
                        int m = _tip.mask[(size_t)(int)v * _tip.w + (int)u];
                        int t = (int)(m * alpha / 255 * strength);
                        if (t > 0)
                            lerpPixel(d, _smudge.at(xx - ox, yy - oy), std::min(t, 255));
                    }
                }
                dirty.unite(r);
            }
            _smudge    = img.cropped(IRect(ox, oy, S, S));
            _hasSmudge = true;
            return;
        }
        dab(img, p, _type, px, py, scale, ang, c, alpha, _clip, l.lockAlpha, &dirty);
        // water : the brush also removes a little of the paint (Paint_EraseAlpha)
        if (p.water > 0 && _type != ToolType_Eraser)
        {
            int w1 = std::min((int)(p.water * 0.3f + 40 - alpha * 0.5f), 255 - alpha);
            if (w1 > 0)
                dab(img, p, ToolType_Eraser, px, py, scale, ang, c, w1, _clip, false, &dirty);
        }
    };
    one(x, y, angle);
    if (_type != ToolType_Smudge)
    {
        float sx = doc.width() - x, sy = doc.height() - y;
        switch (p.symetry)
        {
        case 1:
            one(sx, y, -angle);
            break;
        case 2:
            one(x, sy, -angle);
            break;
        case 3:
            one(sx, sy, angle);
            break;
        case 4:
            one(sx, y, -angle);
            one(x, sy, -angle);
            one(sx, sy, angle);
            break;
        default:
            break;
        }
    }
    doc.invalidate(dirty);
}

Color BrushEngine::mixedColor(Document& doc, BrushParams& p, int x, int y)
{
    if (p.mix <= 0 || _type != ToolType_Brush)
        return _baseColor;
    float mix = std::min(p.mix, 100) * 0.01f;
    // colour of the picture under the brush (BrushGetMixColor)
    Color canvas = doc.pickColor(x, y, p.mixLayer == 1 ? 1 : (p.mixLayer == 0 ? 0 : 2));
    bool nothing = p.mixLayer == 1 && canvas.a == 0;
    switch (p.mixType)
    {
    case 0: // classic : fades to the picked colour, the viscosity is the number of dots of the fade
    {
        float tx = p.mix / (float)(p.visco + 1);
        if (_viscoCur > 0)
            _viscoCur--;
        else
        {
            _viscoCur = p.visco;
            _oldCol   = _col;
            _newCol   = nothing ? _col : canvas;
        }
        float v = (p.mix - _viscoCur * tx) * 0.01f, u = 1.f - v;
        _col    = Color((int)(_newCol.r * v + _oldCol.r * u), (int)(_newCol.g * v + _oldCol.g * u),
                        (int)(_newCol.b * v + _oldCol.b * u), 255);
        return _col;
    }
    case 1: // inverse : the brush keeps the colour it picks (smear)
        if (_viscoCur > 0)
            _viscoCur--;
        else
        {
            _viscoCur = p.visco;
            if (!nothing)
                _col = mixColor(canvas, _col, mix);
        }
        return _col;
    case 3: // new : like "old" but the colour is only picked every (viscosity) dots
        if (_viscoCur > 0)
            _viscoCur--;
        else
        {
            _viscoCur = p.visco;
            _col      = nothing ? _baseColor : mixColor(canvas, _baseColor, mix);
        }
        return _col;
    default: // old : simple mix of the picture and the colour of the brush
        return nothing ? _baseColor : mixColor(canvas, _baseColor, mix);
    }
}

void BrushEngine::placeDot(Document& doc, BrushParams& p, float x, float y, float pressure, float dirAngle)
{
    // ---- size (Brush_SetSizeWithPression)
    float final = p.sizePressure ? pressure : 1.f;
    if (p.sizeRand > 0)
        final *= rndRange(std::min(p.sizeMin, p.size), p.size) / (float)std::max(p.size, 1);
    if (final * p.size < p.sizeMin)
        final = p.sizeMin / (float)std::max(p.size, 1);
    if (final <= 0)
        return;

    // ---- alpha (CheckAlpha)
    float alpha = p.alphaRand > 0 ? (float)rndRange(std::min(p.alphaMin, p.alpha), p.alpha) : (float)p.alpha;
    if (p.alphaPressure)
        alpha *= pressure;
    if (p.water > 0 && _type != ToolType_Eraser)
        alpha *= std::max(0.f, 1.f - p.water / 255.f);
    int ia = std::clamp((int)alpha, 0, 255);
    if (ia <= 0 && p.water <= 0)
        return;

    // ---- scatter
    float sx = rndSym(p.scatter) * p.size * 0.01f, sy = rndSym(p.scatter) * p.size * 0.01f;
    float px = x + sx, py = y + sy;

    // ---- rotation
    float angle = 0;
    if (p.rotateByAngle)
        angle = dirAngle * p.rotateByAngle + rndSym(p.randRot);
    else if (p.randRot > 0)
        angle = (float)rnd(p.randRot);

    Color c = mixedColor(doc, p, (int)px, (int)py);
    dabWithSymetry(doc, p, px, py, final, angle, c, std::max(ia, 1));
}

void BrushEngine::beginStroke(Document& doc, const BrushParams& p, int action, float x, float y, float pressure)
{
    _active = true;
    _action = action;
    _type   = p.type;
    if (action == Action_Eraser)
        _type = ToolType_Eraser;
    else if (action == Action_CloneStamp)
        _type = ToolType_CloneStamp;
    else if (action == Action_Pen)
        _type = ToolType_Pen;
    _lastX = x, _lastY = y;
    _carry     = 0;
    _first     = true;
    _hasSmudge = false;
    _clip      = doc.selection.active ? doc.selection.rect.intersect(doc.rect()) : doc.rect();
    if (p.wash || !(_baseColor == p.color))
    {
        _col = _oldCol = _newCol = p.color;
        _viscoCur = 0;
    }
    _baseColor = p.color;
    BrushParams tmp = p;
    strokeTo(doc, tmp, x, y, pressure);
}

void BrushEngine::strokeTo(Document& doc, BrushParams& p, float x, float y, float pressure)
{
    if (!_active)
        return;
    pressure = std::clamp(pressure, 0.f, 1.f);
    float dx = x - _lastX, dy = y - _lastY;
    float dist = std::sqrt(dx * dx + dy * dy);
    float dirAngle = dist > 0.001f ? std::atan2(dy, dx) * 180.f / 3.14159265f : 0.f;

    if (_action == Action_Spray)
    {
        for (int i = 0; i < std::max(1, p.nbSpray); ++i)
        {
            float a = rndf() * 6.2831853f, r = std::sqrt(rndf()) * p.spray;
            placeDot(doc, p, x + std::cos(a) * r, y + std::sin(a) * r, pressure, dirAngle);
        }
        _lastX = x, _lastY = y;
        _first = false;
        return;
    }

    if (_type == ToolType_Pen)
    {
        // hard anti-aliased line, the width follows the pressure
        float final = p.sizePressure ? pressure : 1.f;
        float w     = std::max((float)p.sizeMin, p.size * final);
        int alpha   = p.alphaPressure ? (int)(p.alpha * pressure) : p.alpha;
        if (!_first || dist < 0.5f)
        {
            Layer& l = doc.cur();
            IRect dirty;
            Color c(p.color.r, p.color.g, p.color.b, alpha);
            auto seg = [&](float ax, float ay, float bx, float by) {
                drawLine(l.image, ax, ay, bx, by, w, c, _clip, l.lockAlpha, &dirty);
            };
            float W = (float)doc.width(), H = (float)doc.height();
            seg(_lastX, _lastY, x, y);
            if (p.symetry == 1 || p.symetry == 4)
                seg(W - _lastX, _lastY, W - x, y);
            if (p.symetry == 2 || p.symetry == 4)
                seg(_lastX, H - _lastY, x, H - y);
            if (p.symetry == 3 || p.symetry == 4)
                seg(W - _lastX, H - _lastY, W - x, H - y);
            doc.invalidate(dirty);
        }
        _lastX = x, _lastY = y;
        _first = false;
        return;
    }

    if (_first)
    {
        placeDot(doc, p, x, y, pressure, dirAngle);
        _first = false;
        _lastX = x, _lastY = y;
        _carry = 0;
        return;
    }
    if (dist < 0.01f)
        return;

    if (!p.trait)
    {
        // no line between the points : one dot per position of the pointer
        if (dist >= 1)
        {
            placeDot(doc, p, x, y, pressure, dirAngle);
            _lastX = x, _lastY = y;
        }
        return;
    }

    float final = p.sizePressure ? std::max(pressure, 0.02f) : 1.f;
    float step  = std::max(1.f, p.size * final * p.pas * 0.01f * p.sizeW * 0.01f);
    float pos   = step - _carry;
    float ux = dx / dist, uy = dy / dist;
    while (pos <= dist)
    {
        placeDot(doc, p, _lastX + ux * pos, _lastY + uy * pos, pressure, dirAngle);
        pos += step;
    }
    _carry = dist - (pos - step);
    _lastX = x, _lastY = y;
}

void BrushEngine::endStroke(Document&, BrushParams&)
{
    _active    = false;
    _hasSmudge = false;
    _smudge    = Image();
}

void BrushEngine::drawPreview(Image& img, const BrushParams& p)
{
    Document doc;
    doc.create(img.width(), img.height());
    BrushParams q = p;
    q.mix         = 0;
    q.symetry     = 0;
    int w = img.width(), h = img.height();
    const int N = 40;
    for (int i = 0; i <= N; ++i)
    {
        float t  = i / (float)N;
        float x  = 12 + t * (w - 24);
        float y  = h * 0.5f + std::sin(t * 6.2831853f) * h * 0.25f;
        float pr = std::sin(t * 3.14159265f) * 0.9f + 0.1f;
        if (i == 0)
            beginStroke(doc, q, Action_Brush, x, y, pr);
        else
            strokeTo(doc, q, x, y, pr);
    }
    endStroke(doc, q);
    img = doc.layers[0].image;
}

} // namespace ani
