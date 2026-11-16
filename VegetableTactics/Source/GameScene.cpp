#include "GameScene.h"
#include "axmol/audio/AudioEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace ax;

// ---------------------------------------------------------------------------
// Layout (design resolution 640 x 480, origin bottom-left)
// ---------------------------------------------------------------------------
static constexpr float W = 640.f, H = 480.f;
static constexpr float POT_L = 240.f, POT_R = 440.f, POT_B = 110.f, RIM = 330.f;
static constexpr float R = 14.f;           // vegetable radius
static constexpr float GRAVITY = 1100.f;
static constexpr Rect ADD_BTN{20.f, 150.f, 180.f, 56.f};
static constexpr Rect SHAKE_BTN{20.f, 80.f, 180.f, 56.f};

static Color col(uint32_t hex, float a = 1.f)
{
    return Color(((hex >> 16) & 0xFF) / 255.f, ((hex >> 8) & 0xFF) / 255.f, (hex & 0xFF) / 255.f, a);
}

// ---------------------------------------------------------------------------
// 5x7 pixel font, drawn with DrawNode rectangles (rows top -> bottom)
// ---------------------------------------------------------------------------
struct Glyph
{
    char c;
    const char* rows;  // 7 rows x 5 columns
};

static const Glyph kFont[] = {
    {'A', "01110" "10001" "10001" "11111" "10001" "10001" "10001"},
    {'B', "11110" "10001" "10001" "11110" "10001" "10001" "11110"},
    {'C', "01110" "10001" "10000" "10000" "10000" "10001" "01110"},
    {'D', "11110" "10001" "10001" "10001" "10001" "10001" "11110"},
    {'E', "11111" "10000" "10000" "11110" "10000" "10000" "11111"},
    {'F', "11111" "10000" "10000" "11110" "10000" "10000" "10000"},
    {'G', "01110" "10001" "10000" "10111" "10001" "10001" "01111"},
    {'H', "10001" "10001" "10001" "11111" "10001" "10001" "10001"},
    {'I', "01110" "00100" "00100" "00100" "00100" "00100" "01110"},
    {'J', "00111" "00010" "00010" "00010" "00010" "10010" "01100"},
    {'K', "10001" "10010" "10100" "11000" "10100" "10010" "10001"},
    {'L', "10000" "10000" "10000" "10000" "10000" "10000" "11111"},
    {'M', "10001" "11011" "10101" "10101" "10001" "10001" "10001"},
    {'N', "10001" "11001" "10101" "10011" "10001" "10001" "10001"},
    {'O', "01110" "10001" "10001" "10001" "10001" "10001" "01110"},
    {'P', "11110" "10001" "10001" "11110" "10000" "10000" "10000"},
    {'Q', "01110" "10001" "10001" "10001" "10101" "10010" "01101"},
    {'R', "11110" "10001" "10001" "11110" "10100" "10010" "10001"},
    {'S', "01111" "10000" "10000" "01110" "00001" "00001" "11110"},
    {'T', "11111" "00100" "00100" "00100" "00100" "00100" "00100"},
    {'U', "10001" "10001" "10001" "10001" "10001" "10001" "01110"},
    {'V', "10001" "10001" "10001" "10001" "10001" "01010" "00100"},
    {'W', "10001" "10001" "10001" "10101" "10101" "11011" "10001"},
    {'X', "10001" "10001" "01010" "00100" "01010" "10001" "10001"},
    {'Y', "10001" "10001" "01010" "00100" "00100" "00100" "00100"},
    {'Z', "11111" "00001" "00010" "00100" "01000" "10000" "11111"},
    {'0', "01110" "10001" "10011" "10101" "11001" "10001" "01110"},
    {'1', "00100" "01100" "00100" "00100" "00100" "00100" "01110"},
    {'2', "01110" "10001" "00001" "00010" "00100" "01000" "11111"},
    {'3', "11110" "00001" "00001" "01110" "00001" "00001" "11110"},
    {'4', "00010" "00110" "01010" "10010" "11111" "00010" "00010"},
    {'5', "11111" "10000" "11110" "00001" "00001" "10001" "01110"},
    {'6', "00110" "01000" "10000" "11110" "10001" "10001" "01110"},
    {'7', "11111" "00001" "00010" "00100" "01000" "01000" "01000"},
    {'8', "01110" "10001" "10001" "01110" "10001" "10001" "01110"},
    {'9', "01110" "10001" "10001" "01111" "00001" "00010" "01100"},
    {':', "00000" "00100" "00100" "00000" "00100" "00100" "00000"},
    {'/', "00001" "00010" "00010" "00100" "01000" "01000" "10000"},
    {'.', "00000" "00000" "00000" "00000" "00000" "01100" "01100"},
    {',', "00000" "00000" "00000" "00000" "01100" "00100" "01000"},
    {'!', "00100" "00100" "00100" "00100" "00100" "00000" "00100"},
    {'+', "00000" "00100" "00100" "11111" "00100" "00100" "00000"},
    {'-', "00000" "00000" "00000" "11111" "00000" "00000" "00000"},
};

static const char* findGlyph(char c)
{
    if (c >= 'a' && c <= 'z')
        c = static_cast<char>(c - 'a' + 'A');
    for (const auto& g : kFont)
        if (g.c == c)
            return g.rows;
    return nullptr;
}

float GameScene::textWidth(std::string_view s, float scale) const
{
    return s.empty() ? 0.f : (6.f * s.size() - 1.f) * scale;
}

void GameScene::drawText(DrawNode* n, std::string_view s, float x, float y, float scale, const Color& c,
                         const Color& shadow, bool centered)
{
    if (centered)
        x -= textWidth(s, scale) * 0.5f;
    for (int pass = 0; pass < 2; ++pass)
    {
        const Vec2 off = pass == 0 ? Vec2(scale, -scale) : Vec2(0, 0);
        const Color& cc = pass == 0 ? shadow : c;
        if (pass == 0 && shadow.a <= 0.f)
            continue;
        float cx = x;
        for (char ch : s)
        {
            if (const char* g = findGlyph(ch))
            {
                for (int row = 0; row < 7; ++row)
                {
                    int col0 = 0;
                    while (col0 < 5)
                    {
                        if (g[row * 5 + col0] != '1')
                        {
                            ++col0;
                            continue;
                        }
                        int col1 = col0;
                        while (col1 < 5 && g[row * 5 + col1] == '1')
                            ++col1;
                        const float px = cx + col0 * scale + off.x;
                        const float py = y + (6 - row) * scale + off.y;
                        n->drawSolidRect(Vec2(px, py), Vec2(px + (col1 - col0) * scale, py + scale), cc);
                        col0 = col1;
                    }
                }
            }
            cx += 6.f * scale;
        }
    }
}

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------
static std::vector<Vec2> ellipse(float cx, float cy, float a, float b, int n = 18)
{
    std::vector<Vec2> p;
    p.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        const float t = 2.f * static_cast<float>(M_PI) * i / n;
        p.emplace_back(cx + a * std::cos(t), cy + b * std::sin(t));
    }
    return p;
}

float GameScene::rnd()
{
    _rng ^= _rng << 13;
    _rng ^= _rng >> 17;
    _rng ^= _rng << 5;
    return (_rng & 0xFFFFFF) / static_cast<float>(0x1000000);
}

void GameScene::play(const char* file, float vol)
{
    AudioEngine::play2d(file, false, vol);
}

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------
bool GameScene::init()
{
    if (!Scene::init())
        return false;

    _rng = static_cast<unsigned>(time(nullptr)) | 1u;

    _root = Node::create();
    addChild(_root);
    _back  = DrawNode::create();
    _dyn   = DrawNode::create();
    _front = DrawNode::create();
    _root->addChild(_back, 0);
    _root->addChild(_dyn, 1);
    _root->addChild(_front, 2);
    buildStatic();

    _pointerListener                = PointerEventListener::create();
    _pointerListener->onPointerDown = AX_CALLBACK_1(GameScene::onPointerDown, this);
    _pointerListener->onPointerMove = AX_CALLBACK_1(GameScene::onPointerMove, this);
    _pointerListener->onPointerUp   = AX_CALLBACK_1(GameScene::onPointerUp, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(_pointerListener, this);

    _keyboardListener               = EventListenerKeyboard::create();
    _keyboardListener->onKeyPressed = AX_CALLBACK_1(GameScene::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithFixedPriority(_keyboardListener, 11);

    AudioEngine::play2d("Audio/Music.wav", true, 0.35f);

    restart();
    scheduleUpdate();
    return true;
}

// ---------------------------------------------------------------------------
// static scenery: tiled kitchen wall, chef, pot, button frames, labels
// ---------------------------------------------------------------------------
void GameScene::buildStatic()
{
    auto* b = _back;

    // wall tiles
    b->drawSolidRect(Vec2(0, 0), Vec2(W, H), col(0xD9DBDF));
    for (int ty = 0; ty < 4; ++ty)
        for (int tx = 0; tx < 5; ++tx)
        {
            const float x0 = tx * 128.f + 3.f, y0 = H - (ty + 1) * 128.f + 3.f, s = 122.f;
            const bool alt = ((tx + ty) & 1) != 0;
            b->drawSolidRect(Vec2(x0, y0), Vec2(x0 + s, y0 + s), col(alt ? 0xEEEFF1 : 0xF8F8F9));
            Vec2 tri[3] = {Vec2(x0, y0 + s), Vec2(x0 + s * 0.55f, y0 + s), Vec2(x0, y0 + s * 0.45f)};
            b->drawSolidPolygon(tri, 3, col(0xFFFFFF, 0.85f), 0, Color(0.f, 0.f, 0.f, 0.f), true);
            Vec2 tri2[3] = {Vec2(x0 + s, y0), Vec2(x0 + s, y0 + s * 0.4f), Vec2(x0 + s * 0.6f, y0)};
            b->drawSolidPolygon(tri2, 3, col(0xDCDEE2, 0.8f), 0, Color(0.f, 0.f, 0.f, 0.f), true);
        }

    // chef: torso, arms, head, hat
    const Color skin = col(0xE9B58C), skinDark = col(0xB98A63), white = col(0xF1ECDD), shade = col(0xCFC8B0);
    {
        Vec2 torso[] = {Vec2(250, 345), Vec2(260, 395), Vec2(380, 395), Vec2(390, 345)};
        b->drawSolidPolygon(torso, 4, white, 2, shade, true);
        Vec2 armL[] = {Vec2(262, 390), Vec2(232, 352), Vec2(214, 322), Vec2(246, 316), Vec2(270, 350), Vec2(284, 372)};
        Vec2 armR[] = {Vec2(378, 390), Vec2(408, 352), Vec2(426, 322), Vec2(394, 316), Vec2(370, 350), Vec2(356, 372)};
        b->drawSolidPolygon(armL, 6, white, 2, shade);
        b->drawSolidPolygon(armR, 6, white, 2, shade);
        b->drawSolidCircle(Vec2(320, 360), 3, 0, 8, col(0x6C7040));  // coat button
        // head
        auto head = ellipse(320, 408, 40, 36, 28);
        b->drawSolidPolygon(head.data(), static_cast<int>(head.size()), skin, 2, skinDark, true);
        b->drawSolidCircle(Vec2(296, 398), 9, 0, 12, col(0xF2A58A, 0.8f));
        b->drawSolidCircle(Vec2(344, 398), 9, 0, 12, col(0xF2A58A, 0.8f));
        b->drawSolidCircle(Vec2(306, 414), 3, 0, 8, col(0x3A2A20));
        b->drawSolidCircle(Vec2(334, 414), 3, 0, 8, col(0x3A2A20));
        auto mouth = ellipse(320, 390, 4, 5, 10);
        b->drawSolidPolygon(mouth.data(), static_cast<int>(mouth.size()), col(0xC0392B), 1, col(0x6B1E16), true);
        // ears
        b->drawSolidCircle(Vec2(281, 408), 6, 0, 10, skin);
        b->drawSolidCircle(Vec2(359, 408), 6, 0, 10, skin);
        // hat
        b->drawSolidRect(Vec2(294, 430), Vec2(346, 446), white, 2, shade);
        for (float x : {298.f, 320.f, 342.f})
            b->drawSolidCircle(Vec2(x, 458), 20, 0, 20, white);
        b->drawSolidCircle(Vec2(320, 466), 22, 0, 22, white);
        b->drawCircle(Vec2(320, 466), 22, col(0xCFC8B0), 1.5f);
    }

    // pot interior (back wall) with vertical shading
    for (int i = 0; i < 20; ++i)
    {
        const float x0 = POT_L + i * (POT_R - POT_L) / 20.f, x1 = POT_L + (i + 1) * (POT_R - POT_L) / 20.f;
        const float t = std::abs(i - 9.5f) / 9.5f;  // 0 centre .. 1 edge
        const float v = 0.30f + 0.12f * (1.f - t);
        b->drawSolidRect(Vec2(x0, POT_B), Vec2(x1 + 0.5f, RIM), Color(v, v + 0.01f, v + 0.03f, 1.f));
    }

    // pot front: walls drawn over vegetables
    auto* f = _front;
    for (int i = 0; i < 18; ++i)  // outer shell, left & right bands with metallic gradient
    {
        const float t = i / 17.f;
        const float v = 0.40f + 0.25f * std::sin(t * 3.14159f);
        const Color c(v, v + 0.01f, v + 0.04f, 1.f);
        const float xl = POT_L - 16.f + t * 16.f, xr = POT_R + t * 16.f;
        f->drawSolidRect(Vec2(xl, POT_B - 16.f), Vec2(xl + 1.f, RIM + 6.f), c);
        f->drawSolidRect(Vec2(xr, POT_B - 16.f), Vec2(xr + 1.f, RIM + 6.f), c);
    }
    f->drawSolidRect(Vec2(POT_L - 16.f, POT_B - 16.f), Vec2(POT_R + 16.f, POT_B), col(0x6A6D73), 2, col(0x25272B));
    f->drawRect(Vec2(POT_L - 16.f, POT_B - 16.f), Vec2(POT_L, RIM + 6.f), col(0x1F2125), 2.f);
    f->drawRect(Vec2(POT_R, POT_B - 16.f), Vec2(POT_R + 16.f, RIM + 6.f), col(0x1F2125), 2.f);
    f->drawSolidRect(Vec2(POT_L - 18.f, RIM), Vec2(POT_L + 2.f, RIM + 8.f), col(0xA8ABB1), 1.5f, col(0x25272B));
    f->drawSolidRect(Vec2(POT_R - 2.f, RIM), Vec2(POT_R + 18.f, RIM + 8.f), col(0xA8ABB1), 1.5f, col(0x25272B));
    // chef mitts gripping the rim
    for (float cx : {POT_L - 22.f, POT_R + 22.f})
    {
        auto mitt = ellipse(cx, RIM + 2.f, 20.f, 17.f, 20);
        f->drawSolidPolygon(mitt.data(), static_cast<int>(mitt.size()), skin, 2, skinDark, true);
        f->drawSolidCircle(Vec2(cx + (cx < 320 ? 10.f : -10.f), RIM + 6.f), 6.f, 0, 10, col(0xF6CBA8));
    }

    // buttons
    auto button = [&](const Rect& r, std::string_view label) {
        const Vec2 o = r.origin, e = r.origin + r.size;
        f->drawSolidRect(o, e, col(0xBDBDBD));
        f->drawSolidRect(Vec2(o.x, e.y - 3), Vec2(e.x, e.y), col(0xF4F4F4));
        f->drawSolidRect(Vec2(o.x, o.y), Vec2(o.x + 3, e.y), col(0xF4F4F4));
        f->drawSolidRect(Vec2(o.x, o.y), Vec2(e.x, o.y + 3), col(0x6E6E6E));
        f->drawSolidRect(Vec2(e.x - 3, o.y), Vec2(e.x, e.y), col(0x6E6E6E));
        drawText(f, label, o.x + r.size.width * 0.5f, o.y + 14.f, 4.f, col(0x7CFC1A), col(0x1D4A00), true);
    };
    button(ADD_BTN, "ADD VEG");
    button(SHAKE_BTN, "SHAKE");

    // HUD labels
    const Color lab = col(0xC4666B), labS = col(0x6B2B30);
    drawText(f, "LEVEL", 18, 444, 4.f, lab, labS);
    drawText(f, "SCORE", 18, 404, 4.f, lab, labS);
    drawText(f, "CHANCE", W - 18.f - textWidth("CHANCE", 4.f), 444, 4.f, lab, labS);
    drawText(f, "SPACE ADD  S SHAKE  R RESTART", 320, 6, 1.5f, col(0x5C5F66), col(0xFFFFFF, 0.7f), true);
}

// ---------------------------------------------------------------------------
// vegetables (all shapes are DrawNode primitives)
// ---------------------------------------------------------------------------
static const uint32_t kVegColor[] = {0xB0235F, 0xF2800F, 0x5B3482, 0xEBDDAA, 0x5CBA3C, 0xDB2020, 0xFFD82A};

void GameScene::drawVeg(DrawNode* n, const Veg& v, bool glow)
{
    const float c = std::cos(v.angle), s = std::sin(v.angle);
    auto T = [&](float x, float y) { return Vec2(v.pos.x + x * c - y * s, v.pos.y + x * s + y * c); };
    auto poly = [&](std::initializer_list<Vec2> pts, const Color& fill, const Color& line) {
        std::vector<Vec2> p;
        p.reserve(pts.size());
        for (auto& q : pts)
            p.push_back(T(q.x, q.y));
        n->drawPolygon(p.data(), static_cast<int>(p.size()), fill, 1.f, line, false);
    };
    auto ell = [&](float cx, float cy, float a, float b, const Color& fill, const Color& line) {
        auto e = ellipse(cx, cy, a, b, 16);
        for (auto& q : e)
            q = T(q.x, q.y);
        n->drawPolygon(e.data(), static_cast<int>(e.size()), fill, 1.f, line, true);
    };
    auto dot = [&](float cx, float cy, float r, const Color& fill) {
        n->drawSolidCircle(T(cx, cy), r, 0, 10, fill);
    };

    if (glow)
    {
        const float pulse = 0.5f + 0.5f * std::sin(_time * 12.f);
        n->drawSolidCircle(v.pos, R + 5.f + pulse * 1.5f, 0, 20, col(0xFFFFFF, 0.55f));
        n->drawCircle(v.pos, R + 5.f, col(0xFFF36A), 2.f);
    }

    const Color base = col(kVegColor[v.type]);
    const Color line = col(0x2B1A12);
    const Color leaf = col(0x2F9E2F);
    switch (v.type)
    {
    case Beetroot:
        poly({{-3, 9}, {-5, 17}, {0, 14}, {5, 17}, {3, 9}}, leaf, col(0x14591A));
        ell(0, -1, 12.5f, 12.f, base, line);
        dot(-4, 3, 3.5f, col(0xE0629A, 0.8f));
        break;
    case Carrot:
        poly({{-4, 10}, {-8, 17}, {-1, 13}}, leaf, col(0x14591A));
        poly({{1, 10}, {2, 18}, {6, 12}}, leaf, col(0x14591A));
        poly({{-9, 11}, {9, 11}, {0, -15}}, base, line);
        n->drawLine(T(-4, 4), T(2, 4), col(0xB05A00), 1.5f);
        n->drawLine(T(-2, -3), T(3, -3), col(0xB05A00), 1.5f);
        n->drawLine(T(-1, -9), T(1, -9), col(0xB05A00), 1.5f);
        break;
    case EggPlant:
        ell(0, -1, 9.f, 13.f, base, line);
        poly({{-7, 10}, {-3, 14}, {0, 17}, {3, 14}, {7, 10}, {0, 8}}, leaf, col(0x14591A));
        dot(-3.5f, 2, 2.8f, col(0x9A6CC9, 0.8f));
        break;
    case Onion:
        poly({{-5, 7}, {0, 17}, {5, 7}}, col(0xC9A24F), line);
        ell(0, -2, 11.5f, 10.5f, base, col(0x8A6A2A));
        n->drawLine(T(0, -12), T(0, 7), col(0xC9A24F), 1.f);
        n->drawLine(T(-5, -11), T(-4, 5), col(0xC9A24F), 1.f);
        n->drawLine(T(5, -11), T(4, 5), col(0xC9A24F), 1.f);
        break;
    case Pea:
        ell(0, 0, 13.f, 7.5f, base, col(0x2C6B1A));
        dot(-7, 0, 4.3f, col(0xA6EA7A));
        dot(0, 0, 4.3f, col(0xA6EA7A));
        dot(7, 0, 4.3f, col(0xA6EA7A));
        break;
    case RedPepper:
        poly({{-2, 9}, {-3, 16}, {3, 16}, {2, 9}}, leaf, col(0x14591A));
        ell(-4.5f, -2, 7.5f, 10.5f, base, col(0x5E0A0A));
        ell(4.5f, -2, 7.5f, 10.5f, base, col(0x5E0A0A));
        dot(-6, 2, 2.6f, col(0xFF8A80, 0.85f));
        break;
    case Sweetcorn:
        poly({{-8, -13}, {0, 0}, {8, -13}}, col(0x6DB13A), col(0x2F6B14));
        ell(0, 2, 8.f, 12.f, base, col(0x9A7A00));
        for (int r = 0; r < 4; ++r)
            for (int q = -1; q <= 1; q += 2)
                dot(q * 3.2f, -3.f + r * 4.2f, 1.6f, col(0xE0A800));
        break;
    default: break;
    }
}

// ---------------------------------------------------------------------------
// rendering of everything that changes
// ---------------------------------------------------------------------------
void GameScene::render()
{
    auto* d = _dyn;
    d->clear();

    // vegetables (non-selected first so glow never hides neighbours)
    for (const auto& v : _veg)
        if (!v.selected)
            drawVeg(d, v, false);
    for (const auto& v : _veg)
        if (v.selected)
            drawVeg(d, v, true);

    // particles
    for (const auto& p : _parts)
    {
        const float k = std::max(0.f, p.life / p.maxLife);
        Color c = p.color;
        c.a     = k;
        d->drawSolidCircle(p.pos, p.size * (0.4f + 0.6f * k), 0, 8, c);
    }

    // floating score text
    const Color gold = col(0xFFE23A), goldS = col(0x5A4200);
    for (const auto& f : _floats)
    {
        char buf[16];
        snprintf(buf, sizeof buf, "+%d", f.value);
        const float k = f.life / 1.2f;
        drawText(d, buf, f.pos.x, f.pos.y + (1.f - k) * 40.f, 3.f, gold.a > 0 ? Color(gold.r, gold.g, gold.b, std::min(1.f, k * 2)) : gold,
                 Color(goldS.r, goldS.g, goldS.b, std::min(1.f, k * 2)), true);
    }

    // HUD numbers
    const Color num = col(0xFFFFFF), numS = col(0x333333);
    char buf[48];
    snprintf(buf, sizeof buf, "%d", _level);
    drawText(d, buf, 140, 444, 4.f, num, numS);
    snprintf(buf, sizeof buf, "%d", _score);
    drawText(d, buf, 140, 404, 4.f, num, numS);

    // chances: little carrots
    for (int i = 0; i < _chance; ++i)
    {
        Veg icon;
        icon.type  = Carrot;
        icon.pos   = Vec2(W - 30.f - i * 34.f, 418.f);
        icon.angle = 0.4f;
        drawVeg(d, icon, false);
    }

    // goal progress
    const int target = 10 + 5 * _level;
    drawText(d, "GOAL", W - 18.f - textWidth("GOAL", 3.f), 370, 3.f, col(0xC4666B), col(0x6B2B30));
    snprintf(buf, sizeof buf, "%d/%d", _cleared, target);
    drawText(d, buf, W - 18.f - textWidth(buf, 4.f), 338, 4.f, num, numS);

    // multiplier preview / last multiplier
    if (_selected >= 0)
    {
        auto grp = groupAt(_selected);
        const int n = static_cast<int>(grp.size());
        const int m = std::clamp(n, 3, 6);
        if (n >= 3)
        {
            snprintf(buf, sizeof buf, "X%d", m);
            drawText(d, buf, 545, 262, 7.f, col(0x7CFC1A), col(0x1D4A00), true);
            snprintf(buf, sizeof buf, "+%d", n * 10 * m);
            drawText(d, buf, 545, 232, 3.f, gold, goldS, true);
        }
        else
            drawText(d, "NEED 3+", 545, 262, 3.f, col(0xFF6A5A), col(0x5A1A12), true);
    }
    else if (_multTimer > 0.f)
    {
        snprintf(buf, sizeof buf, "X%d", _lastMult);
        drawText(d, buf, 545, 262, 7.f, Color(0.49f, 0.99f, 0.1f, std::min(1.f, _multTimer)), Color(0.1f, 0.3f, 0, std::min(1.f, _multTimer)), true);
    }

    // auto-add countdown  m:ss
    if (_state == State::Playing)
    {
        const int secs = std::max(0, static_cast<int>(std::ceil(_autoTimer)));
        snprintf(buf, sizeof buf, "%d:%02d", secs / 60, secs % 60);
        drawText(d, "NEXT VEG", 545, 190, 2.5f, col(0xC4666B), col(0x6B2B30), true);
        drawText(d, buf, 545, 162, 4.f, secs <= 3 ? col(0xFF5A4A) : num, numS, true);
    }

    // disabled buttons
    if (!addEnabled())
        d->drawSolidRect(ADD_BTN.origin, ADD_BTN.origin + ADD_BTN.size, col(0xFFFFFF, 0.6f));
    if (!shakeEnabled())
        d->drawSolidRect(SHAKE_BTN.origin, SHAKE_BTN.origin + SHAKE_BTN.size, col(0xFFFFFF, 0.6f));
    if (_shakeCooldown > 0.f)
        d->drawSolidRect(Vec2(SHAKE_BTN.origin.x, SHAKE_BTN.origin.y - 8.f),
                         Vec2(SHAKE_BTN.origin.x + SHAKE_BTN.size.width * (1.f - _shakeCooldown / 4.f), SHAKE_BTN.origin.y - 4.f),
                         col(0x7CFC1A));

    // banners
    const Vec2 mid(320.f, 235.f);
    auto bannerText = [&](std::string_view s, float sc, const Color& c, const Color& cs, float dy = 0) {
        drawText(d, s, mid.x, mid.y + dy, sc, c, cs, true);
    };
    if (_state == State::Ready)
        bannerText("READY", 10.f, col(0xFF8A2A), col(0x5A2A00));
    else if (_state == State::Go)
        bannerText("GO!", 12.f, col(0x7CFC1A), col(0x1D4A00));
    else if (_state == State::GameOver)
    {
        d->drawSolidRect(Vec2(0, 0), Vec2(W, H), col(0x000000, 0.45f));
        bannerText("GAME OVER", 9.f, col(0xFF5A4A), col(0x400000), 20.f);
        snprintf(buf, sizeof buf, "SCORE %d", _score);
        bannerText(buf, 4.f, num, numS, -30.f);
        bannerText("PRESS R OR CLICK", 3.f, col(0xFFE23A), goldS, -70.f);
    }
    else if (_banner > 0.f)
    {
        const float a = std::min(1.f, _banner * 2.f);
        auto A        = [&](Color c) { c.a = a; return c; };
        if (_bannerKind == 1)
            bannerText("POT OUT", 9.f, A(col(0xFF5A4A)), A(col(0x400000)));
        else if (_bannerKind == 2)
            bannerText("LEVEL UP", 8.f, A(col(0x7CFC1A)), A(col(0x1D4A00)));
        else if (_bannerKind == 3)
            bannerText("SHOW OFF", 7.f, A(col(0xFFE23A)), A(col(0x5A4200)), 60.f);
    }
}

// ---------------------------------------------------------------------------
// game logic
// ---------------------------------------------------------------------------
int GameScene::typesThisLevel() const
{
    return std::min<int>(VegCount, 3 + _level);
}

bool GameScene::addEnabled() const
{
    return _state == State::Playing && _addCooldown <= 0.f && _pendingSpawn == 0;
}

bool GameScene::shakeEnabled() const
{
    return _state == State::Playing && _shakeCooldown <= 0.f && !_veg.empty();
}

void GameScene::restart()
{
    _veg.clear();
    _parts.clear();
    _floats.clear();
    _pendingSpawn = 0;
    _score        = 0;
    _level        = 1;
    _chance       = 3;
    _cleared      = 0;
    _selected     = -1;
    _addCooldown = _shakeCooldown = _shakeFx = _banner = _multTimer = 0.f;
    _state     = State::Ready;
    _stateTime = 0.f;
    _autoTimer = 12.f;
}

void GameScene::queueVeg(int count)
{
    _pendingSpawn += count;
}

void GameScene::spawnOne()
{
    Veg v;
    v.pos   = Vec2(rnd(POT_L + R, POT_R - R), RIM + 35.f);
    v.vel   = Vec2(rnd(-40.f, 40.f), -250.f);
    v.type  = static_cast<VegType>(static_cast<int>(rnd() * typesThisLevel()) % typesThisLevel());
    v.angle = rnd(0.f, 6.28f);
    _veg.push_back(v);
}

void GameScene::addVegPressed()
{
    if (!addEnabled())
        return;
    queueVeg(3);
    _addCooldown = 1.0f;
}

void GameScene::shakePressed()
{
    if (!shakeEnabled())
        return;
    for (auto& v : _veg)
    {
        v.vel.y += rnd(380.f, 620.f);
        v.vel.x += rnd(-220.f, 220.f);
    }
    _shakeCooldown = 4.f;
    _shakeFx       = 0.5f;
}

void GameScene::stepPhysics(float dt)
{
    constexpr int SUB = 6;
    const float h     = dt / SUB;
    const float minD  = 2.f * R;
    for (int s = 0; s < SUB; ++s)
    {
        for (auto& v : _veg)
        {
            v.vel.y -= GRAVITY * h;
            v.vel *= (1.f - 0.4f * h);
            v.pos += v.vel * h;
            v.angle += v.vel.x * h / R * 0.6f;
        }
        for (int it = 0; it < 2; ++it)
        {
            for (size_t i = 0; i < _veg.size(); ++i)
                for (size_t j = i + 1; j < _veg.size(); ++j)
                {
                    Veg &a = _veg[i], &b = _veg[j];
                    Vec2 d     = b.pos - a.pos;
                    const float d2 = d.x * d.x + d.y * d.y;
                    if (d2 >= minD * minD || d2 < 1e-6f)
                        continue;
                    const float dist = std::sqrt(d2);
                    const Vec2 nrm   = d / dist;
                    const float ov   = minD - dist;
                    a.pos -= nrm * (ov * 0.5f);
                    b.pos += nrm * (ov * 0.5f);
                    const float rv = (b.vel - a.vel).dot(nrm);
                    if (rv < 0.f)
                    {
                        const float e = rv < -120.f ? 0.15f : 0.f;
                        const float jn = -(1.f + e) * rv * 0.5f;
                        a.vel -= nrm * jn;
                        b.vel += nrm * jn;
                        // a bit of tangential friction so the pile settles
                        const Vec2 tng(-nrm.y, nrm.x);
                        const float tv = (b.vel - a.vel).dot(tng) * 0.02f;
                        a.vel += tng * tv;
                        b.vel -= tng * tv;
                    }
                }
            for (auto& v : _veg)
            {
                if (v.pos.x < POT_L + R)
                {
                    v.pos.x = POT_L + R;
                    if (v.vel.x < 0) v.vel.x *= -0.2f;
                }
                if (v.pos.x > POT_R - R)
                {
                    v.pos.x = POT_R - R;
                    if (v.vel.x > 0) v.vel.x *= -0.2f;
                }
                if (v.pos.y < POT_B + R)
                {
                    v.pos.y = POT_B + R;
                    if (v.vel.y < 0) v.vel.y *= -0.1f;
                    v.vel.x *= 0.96f;
                }
            }
        }
    }
}

int GameScene::vegAt(const Vec2& p) const
{
    int best = -1;
    float bd = (R + 4.f) * (R + 4.f);
    for (size_t i = 0; i < _veg.size(); ++i)
    {
        const float d = (_veg[i].pos - p).lengthSquared();
        if (d < bd)
        {
            bd   = d;
            best = static_cast<int>(i);
        }
    }
    return best;
}

std::vector<int> GameScene::groupAt(int index) const
{
    std::vector<int> out;
    if (index < 0 || index >= static_cast<int>(_veg.size()))
        return out;
    std::vector<char> seen(_veg.size(), 0);
    out.push_back(index);
    seen[index] = 1;
    const float lim = (2.f * R + 4.f) * (2.f * R + 4.f);
    for (size_t k = 0; k < out.size(); ++k)
    {
        const Veg& a = _veg[out[k]];
        for (size_t j = 0; j < _veg.size(); ++j)
            if (!seen[j] && _veg[j].type == a.type && (_veg[j].pos - a.pos).lengthSquared() < lim)
            {
                seen[j] = 1;
                out.push_back(static_cast<int>(j));
            }
    }
    return out;
}

void GameScene::burst(const Vec2& p, const Color& c, int n)
{
    for (int i = 0; i < n; ++i)
    {
        Particle q;
        const float a = rnd(0.f, 6.28f), sp = rnd(60.f, 220.f);
        q.pos         = p;
        q.vel         = Vec2(std::cos(a), std::sin(a)) * sp;
        q.maxLife = q.life = rnd(0.3f, 0.7f);
        q.size            = rnd(2.f, 5.f);
        q.color           = c;
        _parts.push_back(q);
    }
}

void GameScene::removeGroup(const std::vector<int>& idx)
{
    const int n = static_cast<int>(idx.size());
    const int m = std::clamp(n, 3, 6);
    Vec2 centre;
    for (int i : idx)
    {
        centre += _veg[i].pos;
        burst(_veg[i].pos, col(kVegColor[_veg[i].type]), 6);
    }
    centre = centre / static_cast<float>(n);

    std::vector<int> sorted = idx;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());
    for (int i : sorted)
        _veg.erase(_veg.begin() + i);

    const int gained = n * 10 * m;
    _score += gained;
    _cleared += n;
    _lastMult  = m;
    _multTimer = 1.2f;
    _floats.push_back({centre, 1.2f, gained});
    play("Audio/TokenRemoved.wav");
    // let the rest of the pile move
    for (auto& v : _veg)
        v.vel.y += 20.f;

    if (n >= 6)
    {
        _banner     = 1.2f;
        _bannerKind = 3;
        play("Audio/ShowOff.wav");
    }
    if (_cleared >= 10 + 5 * _level)
        levelUp();
}

void GameScene::levelUp()
{
    ++_level;
    _cleared = 0;
    _banner     = 1.6f;
    _bannerKind = 2;
    if (_chance < 5)
        ++_chance;
    play("Audio/ShowOff.wav");
}

void GameScene::potOut()
{
    for (const auto& v : _veg)
        burst(v.pos, col(kVegColor[v.type]), 3);
    _veg.clear();
    _pendingSpawn = 0;
    _selected     = -1;
    --_chance;
    _banner     = 1.6f;
    _bannerKind = 1;
    play("Audio/LostChance.wav");
    if (_chance <= 0)
    {
        _state     = State::GameOver;
        _stateTime = 0.f;
    }
    else
        _autoTimer = std::max(4.f, 13.f - _level);
}

// ---------------------------------------------------------------------------
// update loop
// ---------------------------------------------------------------------------
void GameScene::update(float dt)
{
    dt = std::min(dt, 1.f / 20.f);
    _time += dt;
    _stateTime += dt;

    if (_state == State::Ready && _stateTime > 1.2f)
    {
        _state     = State::Go;
        _stateTime = 0.f;
        queueVeg(14);
    }
    else if (_state == State::Go && _stateTime > 0.9f)
    {
        _state     = State::Playing;
        _stateTime = 0.f;
        _autoTimer = 12.f;
    }

    if (_state != State::GameOver)
    {
        // staggered spawning
        if (_pendingSpawn > 0)
        {
            _spawnTimer -= dt;
            if (_spawnTimer <= 0.f)
            {
                spawnOne();
                --_pendingSpawn;
                _spawnTimer = 0.07f;
            }
        }
        stepPhysics(dt);
    }

    if (_state == State::Playing)
    {
        _addCooldown   = std::max(0.f, _addCooldown - dt);
        _shakeCooldown = std::max(0.f, _shakeCooldown - dt);
        _autoTimer -= dt;
        if (_autoTimer <= 0.f)
        {
            queueVeg(4 + _level / 2);
            _autoTimer = std::max(5.f, 13.f - _level);
        }

        // overflow detection: veg resting above the rim
        bool over = false;
        for (auto& v : _veg)
        {
            if (v.pos.y > RIM + 4.f && v.vel.lengthSquared() < 50.f * 50.f && _pendingSpawn == 0)
            {
                v.still += dt;
                if (v.still > 1.2f)
                    over = true;
            }
            else
                v.still = 0.f;
        }
        if (over)
            potOut();
    }

    // effects
    _shakeFx = std::max(0.f, _shakeFx - dt);
    _banner  = std::max(0.f, _banner - dt);
    _multTimer = std::max(0.f, _multTimer - dt);
    _root->setPosition(_shakeFx > 0.f ? Vec2(rnd(-5.f, 5.f), rnd(-5.f, 5.f)) * (_shakeFx * 2.f) : Vec2(0.f, 0.f));
    for (auto& p : _parts)
    {
        p.life -= dt;
        p.vel.y -= 400.f * dt;
        p.pos += p.vel * dt;
    }
    _parts.erase(std::remove_if(_parts.begin(), _parts.end(), [](const Particle& p) { return p.life <= 0.f; }), _parts.end());
    for (auto& f : _floats)
        f.life -= dt;
    _floats.erase(std::remove_if(_floats.begin(), _floats.end(), [](const Floater& f) { return f.life <= 0.f; }), _floats.end());

    render();
}

// ---------------------------------------------------------------------------
// input
// ---------------------------------------------------------------------------
bool GameScene::onPointerDown(PointerEvent* ev)
{
    _pointer = ev->getWorldPoint();
    if (_state == State::GameOver)
    {
        if (_stateTime > 0.6f)
            restart();
        return true;
    }
    if (_state != State::Playing)
        return true;

    if (ADD_BTN.containsPoint(_pointer))
    {
        addVegPressed();
        return true;
    }
    if (SHAKE_BTN.containsPoint(_pointer))
    {
        shakePressed();
        return true;
    }

    // select the group of touching same-type vegetables under the pointer
    for (auto& v : _veg)
        v.selected = false;
    _selected = vegAt(_pointer);
    if (_selected >= 0)
        for (int i : groupAt(_selected))
            _veg[i].selected = true;
    return true;
}

void GameScene::onPointerMove(PointerEvent* ev)
{
    _pointer = ev->getWorldPoint();
}

void GameScene::onPointerUp(PointerEvent* ev)
{
    _pointer = ev->getWorldPoint();
    if (_selected >= 0 && _selected < static_cast<int>(_veg.size()))
    {
        auto grp = groupAt(_selected);
        const int under = vegAt(_pointer);
        const bool hit  = std::find(grp.begin(), grp.end(), under) != grp.end();
        const int n     = static_cast<int>(grp.size());
        for (auto& v : _veg)
            v.selected = false;
        _selected = -1;
        if (hit && n >= 3)
            removeGroup(grp);
    }
    else
        _selected = -1;
}

void GameScene::onKeyPressed(KeyboardEvent* ev)
{
    switch (ev->getKeyCode())
    {
    case KeyboardEvent::KeyCode::KEY_SPACE: addVegPressed(); break;
    case KeyboardEvent::KeyCode::KEY_S: shakePressed(); break;
    case KeyboardEvent::KeyCode::KEY_R: restart(); break;
    case KeyboardEvent::KeyCode::KEY_ESCAPE: Director::getInstance()->end(); break;
    default: break;
    }
}
