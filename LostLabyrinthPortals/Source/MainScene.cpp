#include "MainScene.h"

#include "GameHost.h"
#include "axmol/audio/AudioEngine.h"

using namespace ax;

namespace
{
constexpr float kW = 640.f, kH = 480.f;

// font id -> TTF file / pixel size / line height (the original used Verdana/Arial; DejaVu Sans ships with the port)
struct FontDef
{
    const char* file;
    float size;
    float line;
};
const FontDef kFonts[pb::FONT_COUNT] = {
    {"fonts/DejaVuSans.ttf", 12.f, 16.f},       // FONT_DEFAULT
    {"fonts/DejaVuSans.ttf", 10.f, 13.f},       // FONT_SMALL
    {"fonts/DejaVuSans.ttf", 14.f, 18.f},       // FONT_HARRINGTON
    {"fonts/DejaVuSans.ttf", 12.f, 16.f},       // FONT_ARIAL_NORMAL
    {"fonts/DejaVuSans.ttf", 12.f, 16.f},       // FONT_VERDANA
    {"fonts/DejaVuSans-Bold.ttf", 12.f, 16.f},  // FONT_VERDANA_BOLD
};

int mapKey(EventKeyboard::KeyCode k)
{
    using K = EventKeyboard::KeyCode;
    switch (k)
    {
    case K::KEY_ESCAPE: return pb::K_ESCAPE;
    case K::KEY_ENTER:
    case K::KEY_KP_ENTER: return pb::K_RETURN;
    case K::KEY_SPACE: return pb::K_SPACE;
    case K::KEY_UP_ARROW: return pb::K_UP;
    case K::KEY_DOWN_ARROW: return pb::K_DOWN;
    case K::KEY_LEFT_ARROW: return pb::K_LEFT;
    case K::KEY_RIGHT_ARROW: return pb::K_RIGHT;
    case K::KEY_ALT: return pb::K_LEFT_ALT;
    case K::KEY_PG_UP: return pb::K_PAGE_UP;
    case K::KEY_PG_DOWN: return pb::K_PAGE_DOWN;
    case K::KEY_F1: return pb::K_F1;
    case K::KEY_BACKSPACE: return pb::K_BACKSPACE;
    case K::KEY_DELETE: return pb::K_DELETE;
    case K::KEY_TAB: return pb::K_TAB;
    default: break;
    }
    int v = (int)k, a = (int)K::KEY_A, z = (int)K::KEY_Z, n0 = (int)K::KEY_0, n9 = (int)K::KEY_9;
    if (v >= a && v <= z)
        return pb::K_A + (v - a);
    if (v >= n0 && v <= n9)
        return pb::K_0 + (v - n0);
    return 0;
}

Color32 toColor(uint32_t c, uint8_t alpha = 255)
{
    return Color32((uint8_t)(c & 255), (uint8_t)((c >> 8) & 255), (uint8_t)((c >> 16) & 255), alpha);
}
}  // namespace

MainScene::~MainScene() {}

bool MainScene::init()
{
    if (!Scene::init())
        return false;

    _background = LayerColor::create(Color32(0, 0, 0, 255), kW, kH);
    addChild(_background, -1);
    _layer = Node::create();
    addChild(_layer, 0);

    measureFonts();

    _white = _director->getTextureCache()->addImage("white.png");
    if (_white)
        _white->setAliasTexParameters();

    _keyboard = EventListenerKeyboard::create();
    _keyboard->onKeyPressed = [this](KeyboardEvent* ev)
    {
        if (ev->getKeyCode() == EventKeyboard::KeyCode::KEY_ALT)
            _altDown = true;
        if (_altDown && ev->getKeyCode() == EventKeyboard::KeyCode::KEY_ENTER)
        {
            // ALT-ENTER: fullscreen toggle of the host window is not wired up in the port (windowed only)
        }
        if (int k = mapKey(ev->getKeyCode()))
            pb::Platform::get().onKey(k, true);
    };
    _keyboard->onKeyReleased = [this](KeyboardEvent* ev)
    {
        if (ev->getKeyCode() == EventKeyboard::KeyCode::KEY_ALT)
            _altDown = false;
        if (int k = mapKey(ev->getKeyCode()))
            pb::Platform::get().onKey(k, false);
    };
    _eventDispatcher->addEventListenerWithFixedPriority(_keyboard, 11);

    _pointer = PointerEventListener::create();
    auto track = [](PointerEvent* ev, bool down, bool move)
    {
        auto loc = ev->getPoint();
        auto& p = pb::Platform::get();
        bool left = move ? ev->isPrimaryPressed() : down;
        p.onMouse((int)loc.x, (int)(kH - loc.y), left);
    };
    _pointer->onPointerDown = [track](PointerEvent* ev)
    {
        track(ev, true, false);
        return true;
    };
    _pointer->onPointerMove = [track](PointerEvent* ev) { track(ev, false, true); };
    _pointer->onPointerUp = [track](PointerEvent* ev) { track(ev, false, false); };
    _pointer->onPointerScroll = [](PointerEvent* ev)
    {
        float s = ev->getScrollY();
        if (s != 0)
            pb::Platform::get().onWheel(s > 0 ? 1 : -1);
        ev->stopPropagation();
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(_pointer, this);

    scheduleUpdate();
    return true;
}

void MainScene::measureFonts()
{
    auto& plat = pb::Platform::get();
    for (int f = 0; f < pb::FONT_COUNT; ++f)
    {
        // identical configs share the measurement
        int same = -1;
        for (int g = 0; g < f; ++g)
            if (std::string(kFonts[g].file) == kFonts[f].file && kFonts[g].size == kFonts[f].size)
                same = g;
        std::vector<float> widths(256, 7.f);
        if (same >= 0)
        {
            // reuse is handled below by copying
        }
        auto* label = Label::createWithTTF("||", kFonts[f].file, kFonts[f].size);
        if (!label)
            continue;
        float bars = label->getContentSize().width;
        for (int c = 32; c < 256; ++c)
        {
            if (c >= 127 && c < 160)
                continue;
            // Latin-1 code point -> UTF-8
            std::string ch;
            if (c < 128)
                ch = std::string(1, (char)c);
            else
            {
                ch += (char)(0xC0 | (c >> 6));
                ch += (char)(0x80 | (c & 0x3F));
            }
            label->setString("|" + ch + "|");
            widths[c] = std::max(1.f, label->getContentSize().width - bars);
        }
        plat.setGlyphWidths(f, widths, kFonts[f].line);
    }
}

Sprite* MainScene::nextSprite()
{
    if (_spriteUsed == _sprites.size())
    {
        auto* s = Sprite::create();
        s->setAnchorPoint(Vec2::zero);
        _layer->addChild(s);
        _sprites.push_back(s);
    }
    auto* s = _sprites[_spriteUsed++];
    s->setVisible(true);
    s->setLocalZOrder(_z++);
    return s;
}

Label* MainScene::nextLabel(int font)
{
    auto& pool = _labels[font];
    if (_labelUsed[font] == pool.size())
    {
        auto* l = Label::createWithTTF("", kFonts[font].file, kFonts[font].size);
        l->setAnchorPoint(Vec2(0.f, 1.f));
        _layer->addChild(l);
        pool.push_back(l);
    }
    auto* l = pool[_labelUsed[font]++];
    l->setVisible(true);
    l->setLocalZOrder(_z++);
    return l;
}

// PureBasic treats pure black as transparent for DisplayTransparentSprite() on images without an alpha
// channel (colour key). Bake that into an alpha channel so the same blit code works in Axmol.
ax::Texture2D* MainScene::loadSheetTexture(const std::string& relPath)
{
    auto* cache = _director->getTextureCache();
    bool isPng = relPath.size() > 4 && relPath.compare(relPath.size() - 4, 4, ".png") == 0;
    if (!isPng)
        return cache->addImage(relPath);
    std::string full = FileUtils::getInstance()->fullPathForFilename(relPath);
    auto* img = new Image();
    Texture2D* result = nullptr;
    const bool keyBlack = relPath.find("light") == std::string::npos;  // light overlays use real alpha gradients
    if (keyBlack && img->initWithImageFile(full))
    {
        const auto fmt = img->getPixelFormat();
        if (fmt == rhi::PixelFormat::RGB8 || fmt == rhi::PixelFormat::RGBA8)
        {
            const int w = img->getWidth(), h = img->getHeight();
            const int bpp = fmt == rhi::PixelFormat::RGB8 ? 3 : 4;
            std::vector<uint8_t> rgba((size_t)w * h * 4);
            const uint8_t* src = img->getData();
            for (size_t i = 0, n = (size_t)w * h; i < n; ++i)
            {
                uint8_t r = src[i * bpp], g = src[i * bpp + 1], b = src[i * bpp + 2];
                uint8_t a = bpp == 4 ? src[i * bpp + 3] : 255;
                rgba[i * 4] = r;
                rgba[i * 4 + 1] = g;
                rgba[i * 4 + 2] = b;
                rgba[i * 4 + 3] = (r | g | b) == 0 ? 0 : a;
            }
            auto* conv = new Image();
            if (conv->initWithRawData(rgba.data(), (ssize_t)rgba.size(), w, h, 8))
                result = cache->addImage(conv, relPath);
            conv->release();
        }
    }
    img->release();
    return result ? result : cache->addImage(relPath);
}

void MainScene::present(const std::vector<pb::Cmd>& cmds, uint32_t clearColor)
{
    auto& plat = pb::Platform::get();
    _background->setColor(Color32(clearColor & 255, (clearColor >> 8) & 255, (clearColor >> 16) & 255));
    _spriteUsed = 0;
    _labelUsed.fill(0);
    _z = 0;
    int sheetCount = plat.sheetCount();
    if ((int)_textures.size() < sheetCount)
        _textures.resize((size_t)sheetCount, nullptr);

    for (const auto& c : cmds)
    {
        switch (c.type)
        {
        case pb::Cmd::Blit:
        {
            if (c.sheet < 0 || c.sheet >= sheetCount)
                break;
            if (!_textures[(size_t)c.sheet])
            {
                auto* t = loadSheetTexture(plat.sheet(c.sheet).path);
                if (t)
                    t->setAliasTexParameters();
                _textures[(size_t)c.sheet] = t;
            }
            auto* tex = _textures[(size_t)c.sheet];
            if (!tex)
                break;
            auto* s = nextSprite();
            s->setTexture(tex);
            s->setTextureRect(Rect((float)c.sx, (float)c.sy, (float)c.sw, (float)c.sh));
            s->setBlendFunc(c.opaque ? BlendFunc::DISABLE
                                     : (tex->hasPremultipliedAlpha() ? BlendFunc::ALPHA_PREMULTIPLIED
                                                                     : BlendFunc::ALPHA_NON_PREMULTIPLIED));
            float dw = c.w > 0 ? c.w : (float)c.sw, dh = c.h > 0 ? c.h : (float)c.sh;
            s->setScale(dw / c.sw, dh / c.sh);
            s->setColor(Color32::white);
            s->setOpacity(c.alpha);
            s->setPosition(Vec2(c.x, kH - c.y - dh));
            break;
        }
        case pb::Cmd::Box:
        case pb::Cmd::Line:
        {
            float x = c.x, y = c.y, w = c.w, h = c.h;
            if (c.type == pb::Cmd::Line)
            {
                // axis-aligned 1px line from (x,y) to (w,h)
                float x2 = c.w, y2 = c.h;
                x = std::min(c.x, x2);
                y = std::min(c.y, y2);
                w = std::max(1.f, std::abs(x2 - c.x));
                h = std::max(1.f, std::abs(y2 - c.y));
                if (c.x == x2)
                    w = 1;
                if (c.y == y2)
                    h = 1;
            }
            if (w <= 0 || h <= 0)
                break;
            auto* s = nextSprite();
            s->setTexture(_white);
            s->setTextureRect(Rect(0, 0, 1, 1));
            s->setBlendFunc(c.alpha == 255 ? BlendFunc::DISABLE : BlendFunc::ALPHA_NON_PREMULTIPLIED);
            s->setScale(w, h);
            s->setColor(Color32(c.color & 255, (c.color >> 8) & 255, (c.color >> 16) & 255));
            s->setOpacity(c.alpha);
            s->setPosition(Vec2(x, kH - y - h));
            break;
        }
        case pb::Cmd::Text:
        {
            auto* l = nextLabel(c.font);
            if (l->getString() != c.text)
                l->setString(c.text);
            l->setTextColor(toColor(c.color, c.alpha));
            l->setPosition(Vec2(c.x, kH - c.y));
            break;
        }
        }
    }
    for (size_t i = _spriteUsed; i < _sprites.size(); ++i)
        _sprites[i]->setVisible(false);
    for (int f = 0; f < pb::FONT_COUNT; ++f)
        for (size_t i = _labelUsed[f]; i < _labels[f].size(); ++i)
            _labels[f][i]->setVisible(false);
}

void MainScene::update(float)
{
    auto& plat = pb::Platform::get();
    if (!_started)
    {
        _started = true;
        std::string res = FileUtils::getInstance()->fullPathForFilename("data/items.xml");
        auto cut = res.rfind("/data/items.xml");
        std::string root = cut == std::string::npos ? "." : res.substr(0, cut);
        std::string writable = FileUtils::getInstance()->getWritablePath() + "LostLabyrinthPortals";
        GameHost::start(root.c_str(), writable.c_str());
    }
    for (auto& s : plat.takeSounds())
    {
        std::string path = FileUtils::getInstance()->fullPathForFilename(s.first);
        if (!path.empty())
            AudioEngine::play2d(path, false, s.second);
    }
    std::vector<pb::Cmd> frame;
    uint32_t clear = 0;
    if (plat.takeFrame(frame, clear))
        present(frame, clear);
    plat.signalFrameDone();
    if (plat.quitting())
    {
        GameHost::stop();
        _director->end();
    }
}
