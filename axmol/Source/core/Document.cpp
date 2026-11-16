// Animatoon - Axmol 3.0 port
#include "Document.h"

#include <cstring>

namespace ani
{

// ------------------------------------------------------------------ Paper

void Paper::rebuildTile()
{
    tile = Image();
    if (!texture.valid())
        return;
    float s = std::max(scale, 1) / 10.f;
    int w   = std::clamp((int)(texture.width() * s), 1, 5000);
    int h   = std::clamp((int)(texture.height() * s), 1, 5000);
    tile    = (w == texture.width() && h == texture.height()) ? texture : texture.resized(w, h, true);
    // same formula as PaperUpdate() of the PureBasic version
    if (intensity > 1 || brightness > 1)
    {
        float e  = (400.f + intensity) / 400.f;
        float eb = brightness > 1 ? brightness / 100.f : 1.f;
        size_t n = (size_t)tile.width() * tile.height();
        uint8_t* p = tile.data();
        for (size_t i = 0; i < n; ++i, p += 4)
            for (int c = 0; c < 3; ++c)
                p[c] = (uint8_t)std::clamp((int)((p[c] * e + 127 * (1 - e)) * eb), 0, 255);
    }
}

// ------------------------------------------------------------------ Document

Document::Document()
{
    create(1024, 768);
}

void Document::create(int w, int h, bool)
{
    _w = std::clamp(w, 1, 16384);
    _h = std::clamp(h, 1, 16384);
    layers.clear();
    layerId          = 0;
    _nextUid         = 1;
    selection.active = false;
    _composite.create(_w, _h, Color(255, 255, 255, 255));
    clearHistory();
    addLayer();
    changed = false;
    invalidateAll();
}

bool Document::canPaint() const
{
    if (layers.empty())
        return false;
    const Layer& l = layers[std::clamp(layerId, 0, (int)layers.size() - 1)];
    return l.view && !l.locked && !l.lockPaint;
}

int Document::addLayer(const std::string& nm, int index)
{
    Layer l;
    l.uid  = _nextUid++;
    l.name = nm.empty() ? "Layer" + std::to_string(l.uid) : nm;
    l.image.create(_w, _h);
    if (index < 0 || index > (int)layers.size())
        index = layers.empty() ? 0 : std::min(layerId + 1, (int)layers.size());
    layers.insert(layers.begin() + index, std::move(l));
    layerId = index;
    changed = true;
    return index;
}

void Document::deleteLayer(int i)
{
    if (layers.size() <= 1 || i < 0 || i >= (int)layers.size())
        return;
    layers.erase(layers.begin() + i);
    layerId = std::clamp(layerId > i ? layerId - 1 : layerId, 0, (int)layers.size() - 1);
    if (layerId >= (int)layers.size())
        layerId = (int)layers.size() - 1;
    changed = true;
    invalidateAll();
}

void Document::duplicateLayer(int i)
{
    if (i < 0 || i >= (int)layers.size())
        return;
    Layer l = layers[i];
    l.uid   = _nextUid++;
    l.name += " copy";
    layers.insert(layers.begin() + i + 1, std::move(l));
    layerId = i + 1;
    changed = true;
    invalidateAll();
}

void Document::moveLayer(int i, int dir)
{
    int j = i + dir;
    if (i < 0 || i >= (int)layers.size() || j < 0 || j >= (int)layers.size())
        return;
    std::swap(layers[i], layers[j]);
    if (layerId == i)
        layerId = j;
    else if (layerId == j)
        layerId = i;
    changed = true;
    invalidateAll();
}

static void mergeInto(Layer& dst, const Layer& src)
{
    int w = dst.image.width(), h = dst.image.height();
    for (int y = 0; y < h; ++y)
    {
        uint8_t* d       = dst.image.at(0, y);
        const uint8_t* s = src.image.at(0, y);
        for (int x = 0; x < w; ++x, d += 4, s += 4)
            compositeAlpha(src.bm, d, s, src.alpha, x, y);
    }
}

// Bake the opacity of a layer into its pixels (needed before it receives other layers)
static void bakeOpacity(Layer& l)
{
    if (l.alpha >= 255)
        return;
    size_t n = (size_t)l.image.width() * l.image.height();
    uint8_t* p = l.image.data();
    for (size_t i = 0; i < n; ++i, p += 4)
        p[3] = (uint8_t)(p[3] * l.alpha / 255);
    l.alpha = 255;
}

void Document::mergeLayers(int mode)
{
    if (layers.size() < 2)
        return;
    if (mode == 0)
    {
        if (layerId <= 0)
            return;
        Layer& below = layers[layerId - 1];
        bakeOpacity(below);
        mergeInto(below, layers[layerId]);
        layers.erase(layers.begin() + layerId);
        layerId--;
    }
    else
    {
        // find the layers to merge, the lowest one receives the others
        std::vector<int> ids;
        for (int i = 0; i < (int)layers.size(); ++i)
        {
            bool take = mode == 1 || (mode == 2 && layers[i].view) || (mode == 3 && layers[i].link);
            if (take)
                ids.push_back(i);
        }
        if (ids.size() < 2)
            return;
        Layer& base = layers[ids[0]];
        bakeOpacity(base);
        for (size_t k = 1; k < ids.size(); ++k)
            mergeInto(base, layers[ids[k]]);
        for (size_t k = ids.size() - 1; k >= 1; --k)
            layers.erase(layers.begin() + ids[k]);
        layerId = ids[0];
    }
    changed = true;
    invalidateAll();
}

void Document::clearLayer(int i)
{
    if (i < 0 || i >= (int)layers.size())
        return;
    if (selection.active)
        layers[i].image.fillRect(selection.rect, Color(0, 0, 0, 0));
    else
        layers[i].image.clear();
    changed = true;
    invalidateAll();
}

void Document::fillLayer(int i, const Color& c)
{
    if (i < 0 || i >= (int)layers.size())
        return;
    Layer& l = layers[i];
    IRect r  = selection.active ? selection.rect.intersect(rect()) : rect();
    for (int y = r.y; y < r.bottom(); ++y)
    {
        uint8_t* d = l.image.at(r.x, y);
        for (int x = 0; x < r.w; ++x, d += 4)
        {
            if (l.lockAlpha)
            {
                int a = d[3];
                blendOver(d, c.r, c.g, c.b, c.a);
                d[3] = (uint8_t)a;
            }
            else
                blendOver(d, c.r, c.g, c.b, c.a);
        }
    }
    changed = true;
    invalidate(r);
}

void Document::fillLayerPattern(int i, const Image& pattern)
{
    if (i < 0 || i >= (int)layers.size() || !pattern.valid())
        return;
    Layer& l = layers[i];
    IRect r  = selection.active ? selection.rect.intersect(rect()) : rect();
    for (int y = r.y; y < r.bottom(); ++y)
    {
        uint8_t* d = l.image.at(r.x, y);
        for (int x = r.x; x < r.right(); ++x, d += 4)
        {
            const uint8_t* s = pattern.at(x % pattern.width(), y % pattern.height());
            int a            = d[3];
            blendOver(d, s[0], s[1], s[2], s[3]);
            if (l.lockAlpha)
                d[3] = (uint8_t)a;
        }
    }
    changed = true;
    invalidate(r);
}

void Document::resizeDocument(int w, int h, bool smooth)
{
    w = std::clamp(w, 1, 16384), h = std::clamp(h, 1, 16384);
    for (auto& l : layers)
        l.image = l.image.resized(w, h, smooth);
    _w = w, _h = h;
    selection.active = false;
    _composite.create(_w, _h, Color(255, 255, 255, 255));
    changed = true;
    invalidateAll();
}

void Document::resizeCanvas(int w, int h, int ax, int ay)
{
    w = std::clamp(w, 1, 16384), h = std::clamp(h, 1, 16384);
    int dx = ax < 0 ? 0 : (ax == 0 ? (w - _w) / 2 : w - _w);
    int dy = ay < 0 ? 0 : (ay == 0 ? (h - _h) / 2 : h - _h);
    for (auto& l : layers)
    {
        Image n(w, h);
        n.copyFrom(l.image, dx, dy);
        l.image = std::move(n);
    }
    _w = w, _h = h;
    selection.active = false;
    _composite.create(_w, _h, Color(255, 255, 255, 255));
    changed = true;
    invalidateAll();
}

void Document::crop(const IRect& rc)
{
    IRect r = rc.intersect(rect());
    if (r.empty())
        return;
    for (auto& l : layers)
        l.image = l.image.cropped(r);
    _w = r.w, _h = r.h;
    selection.active = false;
    _composite.create(_w, _h, Color(255, 255, 255, 255));
    changed = true;
    invalidateAll();
}

void Document::rotate90(int q)
{
    for (auto& l : layers)
        l.image = l.image.rotated90(q);
    if (q % 2)
        std::swap(_w, _h);
    selection.active = false;
    _composite.create(_w, _h, Color(255, 255, 255, 255));
    changed = true;
    invalidateAll();
}

void Document::mirror(bool horizontal, bool all)
{
    for (int i = 0; i < (int)layers.size(); ++i)
        if (all || i == layerId)
            layers[i].image = layers[i].image.mirrored(horizontal);
    changed = true;
    invalidateAll();
}

// ------------------------------------------------------------------ rendering

void Document::invalidate(const IRect& r)
{
    _dirty.unite(r.intersect(rect()));
}

void Document::compositeRect(const IRect& r, Image& dst, bool withPaper, bool onlyVisible, int maxLayer) const
{
    // 1) the paper : colour, then the texture (multiplied), faded with its alpha
    for (int y = r.y; y < r.bottom(); ++y)
    {
        uint8_t* d = dst.at(r.x, y);
        if (!withPaper)
        {
            std::memset(d, 0, (size_t)r.w * 4);
            continue;
        }
        const bool tex = paper.tile.valid() && paper.alpha > 0;
        int tw = tex ? paper.tile.width() : 1, th = tex ? paper.tile.height() : 1;
        const uint8_t* trow = tex ? paper.tile.at(0, y % th) : nullptr;
        for (int x = r.x; x < r.right(); ++x, d += 4)
        {
            int cr = paper.color.r, cg = paper.color.g, cb = paper.color.b;
            if (tex)
            {
                const uint8_t* t = trow + (size_t)(x % tw) * 4;
                // white where the texture is faded out
                int tr = 255 + (t[0] - 255) * paper.alpha / 255;
                int tg = 255 + (t[1] - 255) * paper.alpha / 255;
                int tb = 255 + (t[2] - 255) * paper.alpha / 255;
                cr = cr * tr / 255, cg = cg * tg / 255, cb = cb * tb / 255;
            }
            d[0] = (uint8_t)cr, d[1] = (uint8_t)cg, d[2] = (uint8_t)cb, d[3] = 255;
        }
    }
    // 2) the layers, bottom to top
    int last = maxLayer < 0 ? (int)layers.size() - 1 : std::min(maxLayer, (int)layers.size() - 1);
    for (int i = 0; i <= last; ++i)
    {
        const Layer& l = layers[i];
        if ((onlyVisible && !l.view) || l.alpha <= 0 || !l.image.valid())
            continue;
        for (int y = r.y; y < r.bottom(); ++y)
        {
            int sy = y - l.offY;
            if (sy < 0 || sy >= l.image.height())
                continue;
            uint8_t* d = dst.at(r.x, y);
            for (int x = r.x; x < r.right(); ++x, d += 4)
            {
                int sx = x - l.offX;
                if (sx < 0 || sx >= l.image.width())
                    continue;
                const uint8_t* s = l.image.at(sx, sy);
                if (!s[3])
                    continue;
                if (withPaper)
                    compositeOpaque(l.bm, d, s, l.alpha, x, y);
                else
                    compositeAlpha(l.bm, d, s, l.alpha, x, y);
            }
        }
    }
}

IRect Document::updateComposite()
{
    IRect r = _dirty.intersect(rect());
    _dirty  = IRect();
    if (r.empty())
        return r;
    if (_composite.width() != _w || _composite.height() != _h)
    {
        _composite.create(_w, _h, Color(255, 255, 255, 255));
        r = rect();
    }
    compositeRect(r, _composite, true, true, -1);
    return r;
}

Image Document::flatten(bool withPaper, bool onlyVisible) const
{
    Image out(_w, _h);
    compositeRect(rect(), out, withPaper, onlyVisible, -1);
    return out;
}

Color Document::pickColor(int x, int y, int mode) const
{
    if (x < 0 || y < 0 || x >= _w || y >= _h || layers.empty())
        return Color(0, 0, 0, 0);
    int id = std::clamp(layerId, 0, (int)layers.size() - 1);
    if (mode == 1)
        return layers[id].image.get(x, y);
    int last = mode == 0 ? (int)layers.size() - 1 : id;
    // the layers (all, or the current one + the ones below), over the paper
    uint8_t d[4] = {paper.color.r, paper.color.g, paper.color.b, 255};
    if (paper.tile.valid() && paper.alpha > 0)
    {
        const uint8_t* t = paper.tile.at(x % paper.tile.width(), y % paper.tile.height());
        for (int c = 0; c < 3; ++c)
            d[c] = (uint8_t)(d[c] * (255 + (t[c] - 255) * paper.alpha / 255) / 255);
    }
    for (int i = 0; i <= last; ++i)
    {
        const Layer& l = layers[i];
        if (!l.view || !l.image.inside(x, y))
            continue;
        compositeOpaque(l.bm, d, l.image.at(x, y), l.alpha, x, y);
    }
    return Color(d[0], d[1], d[2], 255);
}

Color Document::pickPaint(int x, int y, int mode) const
{
    if (x < 0 || y < 0 || x >= _w || y >= _h || layers.empty())
        return Color(0, 0, 0, 0);
    int id = std::clamp(layerId, 0, (int)layers.size() - 1);
    if (mode == 1)
        return layers[id].image.get(x, y);
    int last     = mode == 0 ? (int)layers.size() - 1 : id;
    uint8_t d[4] = {0, 0, 0, 0};
    for (int i = 0; i <= last; ++i)
    {
        const Layer& l = layers[i];
        if (l.view && l.image.inside(x, y))
            compositeAlpha(l.bm, d, l.image.at(x, y), l.alpha, x, y);
    }
    return Color(d[0], d[1], d[2], d[3]);
}

// ------------------------------------------------------------------ history

int Document::layerIndexFromUid(int uid) const
{
    for (int i = 0; i < (int)layers.size(); ++i)
        if (layers[i].uid == uid)
            return i;
    return -1;
}

void Document::pushUndoLayer(int i, const char* label)
{
    if (i < 0 || i >= (int)layers.size())
        return;
    auto s      = std::make_unique<State>();
    s->label    = label;
    s->layerUid = layers[i].uid;
    s->image    = layers[i].image;
    _undo.push_back(std::move(s));
    while ((int)_undo.size() > std::max(maxUndo, 1))
        _undo.pop_front();
    _redo.clear();
    changed = true;
}

void Document::pushUndoDocument(const char* label)
{
    auto s     = std::make_unique<State>();
    s->label   = label;
    s->fullDoc = true;
    s->w = _w, s->h = _h, s->layerId = layerId;
    s->layers  = layers;
    _undo.push_back(std::move(s));
    while ((int)_undo.size() > std::max(maxUndo, 1))
        _undo.pop_front();
    _redo.clear();
    changed = true;
}

std::unique_ptr<Document::State> Document::captureLike(const State& s) const
{
    auto c     = std::make_unique<State>();
    c->label   = s.label;
    c->fullDoc = s.fullDoc;
    if (s.fullDoc)
    {
        c->w = _w, c->h = _h, c->layerId = layerId;
        c->layers = layers;
    }
    else
    {
        c->layerUid = s.layerUid;
        int i       = layerIndexFromUid(s.layerUid);
        if (i >= 0)
            c->image = layers[i].image;
    }
    return c;
}

void Document::apply(const State& s)
{
    if (s.fullDoc)
    {
        layers  = s.layers;
        layerId = std::clamp(s.layerId, 0, std::max(0, (int)layers.size() - 1));
        if (s.w != _w || s.h != _h)
        {
            _w = s.w, _h = s.h;
            _composite.create(_w, _h, Color(255, 255, 255, 255));
            selection.active = false;
        }
    }
    else
    {
        int i = layerIndexFromUid(s.layerUid);
        if (i >= 0 && s.image.valid())
            layers[i].image = s.image;
    }
    changed = true;
    invalidateAll();
}

bool Document::undo()
{
    if (_undo.empty())
        return false;
    auto s = std::move(_undo.back());
    _undo.pop_back();
    _redo.push_back(captureLike(*s));
    apply(*s);
    return true;
}

bool Document::redo()
{
    if (_redo.empty())
        return false;
    auto s = std::move(_redo.back());
    _redo.pop_back();
    _undo.push_back(captureLike(*s));
    apply(*s);
    return true;
}

void Document::clearHistory()
{
    _undo.clear();
    _redo.clear();
}

} // namespace ani
