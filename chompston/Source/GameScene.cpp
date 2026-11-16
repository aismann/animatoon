#include "GameScene.h"

#include "Actors.h"
#include "Backdrop.h"
#include "Config.h"
#include "Logo.h"
#include "Sound.h"
#include "TextDraw.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace ax;

namespace
{
constexpr float kW = 512.f, kH = 384.f;
constexpr float kPi = 3.14159265f;
constexpr float kWallH = 0.28f;  // chase view: half height of a wall slab

// flat view layout: 25x19 tiles of 18 px, HUD strip on top
constexpr float kT  = 18.f;
constexpr float kOX = (kW - kCols * kT) * 0.5f;
constexpr float kOY = 3.f;

const Color kYellow(1.0f, 0.86f, 0.05f);
const Color kGray(0.7f, 0.7f, 0.85f);

Color hueColor(float h, float s = 0.85f, float v = 1.f)
{
    h = h - std::floor(h);
    float f = h * 6.f;
    int i   = (int)f;
    float q = f - i;
    float p = v * (1 - s), qq = v * (1 - s * q), t = v * (1 - s * (1 - q));
    switch (i % 6)
    {
    case 0: return Color(v, t, p);
    case 1: return Color(qq, v, p);
    case 2: return Color(p, v, t);
    case 3: return Color(p, qq, v);
    case 4: return Color(t, p, v);
    default: return Color(v, p, qq);
    }
}

std::string num(int v, int width)
{
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%0*d", width, v);
    return buf;
}

float wrapPi(float a)
{
    while (a > kPi)
        a -= 2 * kPi;
    while (a < -kPi)
        a += 2 * kPi;
    return a;
}

Color ghostColor(int id)
{
    static const Color body[3] = {Color(0.84f, 0.78f, 1.0f), Color(0.30f, 0.80f, 0.35f), Color(0.98f, 0.50f, 0.15f)};
    return body[id % 3];
}
}  // namespace

GameScene::GameScene() {}
GameScene::~GameScene() {}

bool GameScene::init()
{
    if (!Scene::init())
        return false;

    _bg = DrawNode::create();
    addChild(_bg, 0);
    _world = DrawNode::create();
    _world->setPreserveDrawOrder(true);  // painter's algorithm: keep submission order
    addChild(_world, 1);
    _logo = Node::create();
    addChild(_logo, 2);
    _hud = DrawNode::create();
    addChild(_hud, 3);
    _overlay = DrawNode::create();
    _overlay->setPreserveDrawOrder(true);
    addChild(_overlay, 4);

    // every letter is drawn once into its own node; update() only moves the nodes
    {
        const std::string text = "Chompston";
        const float unit = 46.f, radius = 6.4f, baseY = 246.f;
        const float total = logo::width(text, unit);
        float x           = kW * 0.5f - total * 0.5f;
        const float x0    = x;
        for (char c : text)
        {
            auto dn = DrawNode::create();
            dn->setPreserveDrawOrder(true);
            logo::drawChar(dn, c, x, baseY, unit, radius, x0, total);
            _logo->addChild(dn);
            _logoLetters.push_back(dn);
            x += logo::make(c).adv * unit;
        }
    }

    auto kb          = EventListenerKeyboard::create();
    kb->onKeyPressed = AX_CALLBACK_1(GameScene::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(kb, this);

    auto pointer           = PointerEventListener::create();  // mouse + touch
    pointer->onPointerDown = AX_CALLBACK_1(GameScene::onPointerDown, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(pointer, this);

    _gfx.smooth = Settings::get().smoothing != 0;
    Sound::init();

    scheduleUpdate();
    return true;
}

// ------------------------------------------------------------------ input / menu

void GameScene::applyAndSave()
{
    _gfx.smooth = Settings::get().smoothing != 0;
    Sound::applySettings();
    Settings::get().saveUserData();
}

void GameScene::toggleFullscreen()
{
#if (AX_TARGET_PLATFORM == AX_PLATFORM_WIN32) || (AX_TARGET_PLATFORM == AX_PLATFORM_MAC) || \
    (AX_TARGET_PLATFORM == AX_PLATFORM_LINUX)
    auto& st    = Settings::get();
    st.windowed = !st.windowed;
    if (auto view = dynamic_cast<RenderView*>(_director->getRenderView()))
    {
        if (st.windowed)
            view->setWindowed(st.windowWidth, st.windowWidth * 3 / 4);
        else
            view->setFullscreen();
    }
    applyAndSave();
#endif
}

void GameScene::confirm()
{
    if (_game.state == State::GameOver)
        _game.toTitle();
}

namespace
{
constexpr int kMainItems = 3;     // START, OPTIONS, EXIT
constexpr int kOptionItems = 5;   // MUSIC, VOLUME, WINDOW, SMOOTHING, BACK
}

void GameScene::menuMove(int d)
{
    int n = _menu == Menu::Main ? kMainItems : kOptionItems;
    _sel  = (_sel + d + n) % n;
    Sound::play(Sfx::Turn);
}

void GameScene::menuAdjust(int d)
{
    if (_menu != Menu::Options)
        return;
    auto& st = Settings::get();
    switch (_sel)
    {
    case 0: st.music = !st.music; break;
    case 1: st.audioVolume = (st.audioVolume + d + Settings::kMaxVolume + 1) % (Settings::kMaxVolume + 1); break;
    case 2: toggleFullscreen(); return;
    case 3: st.smoothing = !st.smoothing; break;
    default: return;
    }
    applyAndSave();
}

void GameScene::menuActivate()
{
    if (_menu == Menu::Main)
    {
        switch (_sel)
        {
        case 0: _game.startGame(); break;
        case 1:
            _menu = Menu::Options;
            _sel  = 0;
            break;
        default: _director->end(); break;
        }
    }
    else
    {
        if (_sel == 4)
        {
            _menu = Menu::Main;
            _sel  = 1;
        }
        else
            menuAdjust(+1);
    }
}

void GameScene::onKeyPressed(KeyboardEvent* ev)
{
    using K        = KeyboardEvent::KeyCode;
    const auto code = ev->getKeyCode();
    auto& st       = Settings::get();

    if (_game.state == State::Title)
    {
        switch (code)
        {
        case K::KEY_UP_ARROW:
        case K::KEY_W: menuMove(-1); return;
        case K::KEY_DOWN_ARROW:
        case K::KEY_S: menuMove(+1); return;
        case K::KEY_LEFT_ARROW:
        case K::KEY_A: menuAdjust(-1); return;
        case K::KEY_RIGHT_ARROW:
        case K::KEY_D: menuAdjust(+1); return;
        case K::KEY_RETURN:
        case K::KEY_KP_ENTER:
        case K::KEY_SPACE: menuActivate(); return;
        case K::KEY_ESCAPE:
            if (_menu == Menu::Options)
            {
                _menu = Menu::Main;
                _sel  = 1;
            }
            else
                _director->end();
            return;
        default: break;
        }
    }

    switch (code)
    {
    // flat view: arrows are absolute directions (map is fixed); chase view: relative turns
    case K::KEY_LEFT_ARROW:
    case K::KEY_A: _flat ? _game.steer(3) : _game.turnLeft(); break;
    case K::KEY_RIGHT_ARROW:
    case K::KEY_D: _flat ? _game.steer(1) : _game.turnRight(); break;
    case K::KEY_UP_ARROW:
    case K::KEY_W: _flat ? _game.steer(0) : _game.reverse(); break;
    case K::KEY_DOWN_ARROW:
    case K::KEY_S: _flat ? _game.steer(2) : _game.reverse(); break;
    case K::KEY_SPACE:
    case K::KEY_CTRL: _game.dropBomb(); break;
    case K::KEY_P:
    case K::KEY_PAUSE: _game.togglePause(); break;
    case K::KEY_RETURN:
    case K::KEY_KP_ENTER: confirm(); break;
    case K::KEY_ESCAPE:
        if (_game.state == State::Playing || _game.state == State::Paused)
            _game.togglePause();
        else
            confirm();
        break;
    case K::KEY_C: _flat = !_flat; break;
    case K::KEY_M:
        st.music = !st.music;
        applyAndSave();
        break;
    case K::KEY_V:
        st.audioVolume = (st.audioVolume + 1) % (Settings::kMaxVolume + 1);
        applyAndSave();
        break;
    case K::KEY_G:
        st.smoothing = !st.smoothing;
        applyAndSave();
        break;
    case K::KEY_F: toggleFullscreen(); break;
    default: break;
    }
}

bool GameScene::onPointerDown(PointerEvent* ev)
{
    Vec2 p = ev->getWorldPoint();
    if (_game.state == State::Title)
    {
        // menu rows are centred at x=256, 24 px apart
        const float rowsY[5] = {168.f, 144.f, 120.f, 96.f, 72.f};
        int n                = _menu == Menu::Main ? kMainItems : kOptionItems;
        for (int i = 0; i < n; ++i)
            if (std::fabs(p.y - (rowsY[i] + 7.f)) < 12.f && std::fabs(p.x - 256.f) < 110.f)
            {
                _sel = i;
                menuActivate();
                return true;
            }
        return true;
    }
    if (_game.state == State::GameOver)
    {
        confirm();
        return true;
    }
    if (_game.state == State::Paused)
    {
        _game.togglePause();
        return true;
    }
    if (_flat)
    {
        // steer towards the tap, relative to the player on screen
        Vec2 d = p - tilePos(_game.px, _game.pz);
        if (std::fabs(d.x) > std::fabs(d.y))
            _game.steer(d.x > 0 ? 1 : 3);
        else
            _game.steer(d.y > 0 ? 0 : 2);
    }
    else if (p.y < 70.f && p.x > 180.f && p.x < 332.f)
        _game.dropBomb();
    else if (p.x < kW * 0.5f)
        _game.turnLeft();
    else
        _game.turnRight();
    return true;
}

// ------------------------------------------------------------------ main loop

void GameScene::update(float dt)
{
    auto& st = Settings::get();
    _time += dt;
    if (st.frameSkip == 0)
        _game.update(std::min(dt, 0.05f));
    else
    {
        // fixed 60 Hz simulation; FrameSkip = how many missed steps may be caught up per rendered frame
        _acc += std::min(dt, 0.25f);
        int steps = 0;
        while (_acc >= 1.f / 60.f && steps <= st.frameSkip)
        {
            _game.update(1.f / 60.f);
            _acc -= 1.f / 60.f;
            ++steps;
        }
        if (steps > st.frameSkip)
            _acc = 0.f;
    }

    const bool title = _game.state == State::Title;
    _logo->setVisible(title);
    if (title)
        for (size_t i = 0; i < _logoLetters.size(); ++i)
        {
            // travelling sine wave: each letter is delayed a bit more than its left neighbour
            float ph = _time * 3.2f - i * 0.65f;
            _logoLetters[i]->setPosition(Vec2(0.f, std::sin(ph) * 9.f));
        }
    _bg->setVisible(!title && _flat);
    if (title)
        renderTitle();
    else if (_flat)
        renderFlat();
    else
        renderChase();
    renderHud();
    renderOverlay();
}

// ------------------------------------------------------------------ title screen

void GameScene::renderTitle()
{
    _world->clear();
    _gfx.smooth = Settings::get().smoothing != 0;
    backdrop::draw(_gfx, _world, _time, kW, kH);
}

// ------------------------------------------------------------------ flat top view (default)

Vec2 GameScene::tilePos(float x, float z) const { return Vec2(kOX + (x + 0.5f) * kT, kOY + (z + 0.5f) * kT); }

void GameScene::rebuildMaze()
{
    // Corridors are the raised part (lit purple lip on the side where a wall begins), walls are black pits.
    _bg->clear();
    const Color floorA(0.24f, 0.20f, 0.29f), floorB(0.21f, 0.17f, 0.26f);
    const Color lip(0.33f, 0.28f, 0.72f), lipHi(0.45f, 0.40f, 0.88f), edge(0.10f, 0.08f, 0.15f);
    for (int r = 0; r < kRows; ++r)
        for (int c = 0; c < kCols; ++c)
        {
            if (_game.wall[r][c])
                continue;
            float x = kOX + c * kT, y = kOY + r * kT;
            _bg->drawSolidRect(Vec2(x, y), Vec2(x + kT, y + kT), ((r + c) & 1) ? floorA : floorB);
            if (!_game.open(c, r - 1))  // south edge: dark rim
                _bg->drawSolidRect(Vec2(x, y), Vec2(x + kT, y + 2.f), edge);
            if (!_game.open(c - 1, r))
                _bg->drawSolidRect(Vec2(x, y), Vec2(x + 2.f, y + kT), edge);
            if (!_game.open(c + 1, r))
                _bg->drawSolidRect(Vec2(x + kT - 2.f, y), Vec2(x + kT, y + kT), edge);
            if (!_game.open(c, r + 1))  // north edge: the lit lip
            {
                _bg->drawSolidRect(Vec2(x, y + kT - 5.f), Vec2(x + kT, y + kT), lip);
                _bg->drawSolidRect(Vec2(x, y + kT - 2.f), Vec2(x + kT, y + kT), lipHi);
            }
        }
    _mazeId = _game.mazeId;
    _bgFlat = true;
}

void GameScene::renderFlat()
{
    if (_mazeId != _game.mazeId || !_bgFlat)
        rebuildMaze();
    _world->clear();
    const float S = kT;

    // pellets: rainbow dots
    for (auto& p : _game.pickups)
    {
        if (!p.alive)
            continue;
        Vec2 c = tilePos((float)p.c, (float)p.r);
        switch (p.kind)
        {
        case Pickup::Kind::Pellet:
        {
            Color col = hueColor((p.c * 5 + p.r * 3) / 29.f, 0.8f, 1.f);
            _world->drawSolidCircle(c, 4.2f, 0.f, 12, shade(col, 0.55f));
            _world->drawSolidCircle(c + Vec2(-0.4f, 0.4f), 3.2f, 0.f, 12, col);
            _world->drawSolidCircle(c + Vec2(-1.1f, 1.1f), 1.2f, 0.f, 8, Color(1.f, 1.f, 1.f, 0.8f));
            break;
        }
        case Pickup::Kind::Power:
        {
            float k = 7.f + 1.2f * std::sin(_time * 8.f);
            _world->drawSolidCircle(c, k + 1.5f, 0.f, 16, Color(1.f, 1.f, 1.f, 0.9f));
            _world->drawSolidCircle(c, k, 0.f, 16, Color(1.f, 0.5f, 0.1f));
            _world->drawSolidCircle(c + Vec2(-1.5f, 1.5f), k * 0.4f, 0.f, 10, Color(1.f, 0.9f, 0.5f));
            break;
        }
        case Pickup::Kind::Heart:
            drawHeart(_world, c + Vec2(-7.f, -6.f), 2.f, Color(0.95f, 0.12f, 0.25f));
            break;
        case Pickup::Kind::Bomb:
            _world->drawSolidCircle(c + Vec2(0, -1), 6.f, 0.f, 14, Color(0.10f, 0.10f, 0.14f));
            _world->drawSolidCircle(c + Vec2(-2, 1), 1.8f, 0.f, 8, Color(0.55f, 0.55f, 0.65f));
            _world->drawSolidRect(c + Vec2(3, 4), c + Vec2(6, 8), (std::fmod(_time, 0.3f) < 0.15f) ? Color(1.f, 0.85f, 0.1f) : Color(1.f, 0.4f, 0.05f));
            break;
        }
    }

    // explosions
    for (auto& e : _game.explosions)
    {
        float t = e.t / 0.9f, a = 1.f - t;
        Vec2 c = tilePos(e.x, e.z);
        float r = (0.4f + t * 3.3f) * S;
        _world->drawSolidCircle(c, r, 0.f, 28, Color(1.f, 0.35f, 0.05f, 0.55f * a));
        _world->drawSolidCircle(c, r * 0.72f, 0.f, 24, Color(1.f, 0.65f, 0.1f, 0.65f * a));
        _world->drawSolidCircle(c, r * 0.42f, 0.f, 20, Color(1.f, 0.95f, 0.5f, 0.8f * a));
    }

    // ghosts: lilac-white blocks with eyes and a ragged hem
    for (auto& g : _game.ghosts)
    {
        if (g.mode == Ghost::Mode::Dead)
            continue;
        bool scared   = g.mode == Ghost::Mode::Scared;
        bool flashing = scared && _game.scaredT < 1.6f && std::fmod(_time, 0.3f) < 0.15f;
        Color col     = ghostColor(g.id);
        if (scared)
            col = flashing ? Color(0.95f, 0.95f, 1.f) : Color(0.20f, 0.28f, 0.95f);
        Vec2 c  = tilePos(g.x, g.z) + Vec2(0, std::fabs(std::sin(g.bob)) * 1.5f);
        float w = S * 0.46f, h = S * 0.62f;
        _world->drawSolidRect(c + Vec2(-w - 1, -h - 1), c + Vec2(w + 1, h + 1), Color(0.f, 0.f, 0.f, 0.35f));
        _world->drawSolidRect(c + Vec2(-w, -h), c + Vec2(w, h), col);
        _world->drawSolidRect(c + Vec2(-w, h - 3), c + Vec2(w, h), shade(col, 1.1f));       // lit top edge
        _world->drawSolidRect(c + Vec2(w - 3, -h), c + Vec2(w, h), shade(col, 0.78f));      // shaded side
        for (int k = 0; k < 3; ++k)                                                           // hem notches
            _world->drawSolidRect(c + Vec2(-w + 2 + k * (w * 0.66f), -h), c + Vec2(-w + 4.5f + k * (w * 0.66f), -h + 3), Color(0.f, 0.f, 0.f, 0.8f));
        Color eye = scared ? Color(1.f, 1.f, 1.f) : Color(0.05f, 0.04f, 0.1f);
        _world->drawSolidRect(c + Vec2(-w + 3, 0), c + Vec2(-w + 6, 6), eye);
        _world->drawSolidRect(c + Vec2(w - 7, 0), c + Vec2(w - 4, 6), eye);
    }

    // Chompston
    bool blink = _game.state == State::Playing && _game.invuln > 0.f && std::fmod(_time, 0.2f) < 0.1f;
    if (!blink)
    {
        float deathT = _game.state == State::Dying ? _game.stateT : 0.f;
        float k      = deathT > 0.f ? std::max(0.f, 1.f - deathT / 1.6f) : 1.f;
        if (k > 0.f)
        {
            Vec2 c  = tilePos(_game.px, _game.pz);
            float h = S * 0.52f * k;
            _world->drawSolidRect(c + Vec2(-h - 1, -h - 1), c + Vec2(h + 1, h + 1), Color(0.f, 0.f, 0.f, 0.4f));
            _world->drawSolidRect(c + Vec2(-h, -h), c + Vec2(h, h), shade(kYellow, 0.9f));
            _world->drawSolidRect(c + Vec2(-h, h - 3 * k), c + Vec2(h, h), shade(kYellow, 1.1f));
            _world->drawSolidRect(c + Vec2(h - 3 * k, -h), c + Vec2(h, h), shade(kYellow, 0.72f));
            const Color black(0.05f, 0.04f, 0.02f);
            float open = _game.pMoving ? 0.5f + 0.5f * std::sin(_game.chompAnim * 1.4f) : 0.15f;
            _world->drawSolidRect(c + Vec2(-h * 0.60f, h * 0.10f), c + Vec2(-h * 0.28f, h * 0.62f), black);  // eyes
            _world->drawSolidRect(c + Vec2(h * 0.26f, h * 0.10f), c + Vec2(h * 0.58f, h * 0.62f), black);
            float my = -h * 0.72f + h * 0.15f + h * 0.34f * open;  // mouth top
            _world->drawSolidRect(c + Vec2(-h * 0.70f, -h * 0.72f), c + Vec2(h * 0.70f, my), black);
            _world->drawSolidRect(c + Vec2(-h * 0.62f, my - h * 0.14f), c + Vec2(h * 0.62f, my - h * 0.02f), Color(0.85f, 0.85f, 0.9f));
        }
    }

    for (auto& p : _game.popups)
    {
        Vec2 s = tilePos(p.x, p.z) + Vec2(0, 8.f + p.t * 14.f);
        pixfont::drawShadow(_world, std::to_string(p.value), s, 2.f, Color(0.3f, 1.f, 1.f), 0.5f);
    }

    if (_game.hitFlash > 0.f)
        _world->drawSolidRect(Vec2(0, 0), Vec2(kW, kH), Color(1.f, 0.15f, 0.1f, std::min(0.6f, _game.hitFlash * 1.6f)));
}

// ------------------------------------------------------------------ 3D chase view (C)

void GameScene::updateCamera(float dt)
{
    const float target = _game.heading * kPi * 0.5f;
    float d            = wrapPi(target - _yawVis);
    _yawVis += d * std::min(1.f, dt * 9.f);

    _gfx.pitch    = 0.22f;
    _gfx.fogStart = 4.f;
    _gfx.fogEnd   = 13.f;
    _gfx.focal    = 380.f;
    _gfx.center   = Vec2(256.f, 120.f);
    _gfx.yaw      = _yawVis;
    _gfx.fogColor = Color(0.16f, 0.13f, 0.30f, 1.f);
    float sy = std::sin(_yawVis), cy = std::cos(_yawVis);
    float shake = (_game.hitFlash > 0.f) ? std::sin(_time * 90.f) * 0.08f * _game.hitFlash : 0.f;
    _gfx.pos    = V3(_game.px - sy * 3.4f + shake, 1.6f, _game.pz - cy * 3.4f);
}

void GameScene::renderChase()
{
    _bgFlat = false;
    updateCamera(1.f / 60.f);
    _world->clear();
    _gfx.begin(_world);
    drawSky();
    drawFloor();
    drawEntities();
    drawEffects();
    if (_game.hitFlash > 0.f)
        _world->drawSolidRect(Vec2(0, 0), Vec2(kW, kH), Color(1.f, 0.15f, 0.1f, std::min(0.6f, _game.hitFlash * 1.6f)));
}

void GameScene::drawSky()
{
    const Color sky0(0.03f, 0.03f, 0.09f), sky1(0.20f, 0.15f, 0.38f);
    float horizon = std::clamp(_gfx.center.y + _gfx.focal * std::tan(_gfx.pitch), 0.f, kH);
    _world->drawSolidRect(Vec2(0, 0), Vec2(kW, horizon), _gfx.fogColor);
    const int bands = 14;
    for (int i = 0; i < bands; ++i)
    {
        float t0 = float(i) / bands, t1 = float(i + 1) / bands;
        float y0 = horizon + (kH - horizon) * t0;
        float y1 = i == bands - 1 ? kH : horizon + (kH - horizon) * t1;
        _world->drawSolidRect(Vec2(0, y0), Vec2(kW, y1 + 1.f), mix(sky1, sky0, std::pow(t0, 0.6f)));
    }
}

void GameScene::drawFloor()
{
    const Color a(0.17f, 0.18f, 0.25f), b(0.14f, 0.15f, 0.21f);
    for (int r = 0; r < kRows; ++r)
        for (int c = 0; c < kCols; ++c)
        {
            if (_game.wall[r][c])
                continue;
            V3 cc = _gfx.toCam(V3((float)c, 0.f, (float)r));
            if (cc.z < -1.2f || cc.z > 13.f || std::fabs(cc.x) > cc.z * 1.0f + 2.5f)
                continue;
            const float h = 0.5f;
            _gfx.quad(V3(c - h, 0, r - h), V3(c + h, 0, r - h), V3(c + h, 0, r + h), V3(c - h, 0, r + h),
                      ((r + c) & 1) ? a : b);
        }
}

void GameScene::drawEntities()
{
    _drawList.clear();
    auto visible = [&](float x, float z, float margin) {
        V3 cc = _gfx.toCam(V3(x, 0.4f, z));
        if (cc.z < -1.0f - margin || cc.z > 13.f)
            return false;
        return std::fabs(cc.x) <= std::max(0.f, cc.z) * 1.0f + 2.5f + margin;
    };
    auto distKey = [&](float x, float z) {
        float dx = x - _gfx.pos.x, dz = z - _gfx.pos.z;
        return dx * dx + dz * dz;
    };

    for (int r = 0; r < kRows; ++r)
        for (int c = 0; c < kCols; ++c)
        {
            if (!_game.wall[r][c] || !visible((float)c, (float)r, 0.8f))
                continue;
            int skip = 0;
            auto w   = [&](int cc, int rr) { return cc < 0 || rr < 0 || cc >= kCols || rr >= kRows || _game.wall[rr][cc]; };
            if (w(c + 1, r)) skip |= 1;
            if (w(c - 1, r)) skip |= 2;
            if (w(c, r + 1)) skip |= 4;
            if (w(c, r - 1)) skip |= 8;
            _drawList.emplace_back(distKey((float)c, (float)r), [this, c, r, skip] {
                Gfx3D::BoxStyle st;
                st.color       = Color(0.33f, 0.28f, 0.74f);
                st.top         = Color(0.06f, 0.05f, 0.12f);
                st.useTopColor = true;
                st.ambient     = 0.42f;
                _gfx.box(V3((float)c, kWallH, (float)r), V3(0.5f, kWallH, 0.5f), 0.f, st, skip);
            });
        }

    for (auto& p : _game.pickups)
        if (p.alive && visible((float)p.c, (float)p.r, 0.3f))
            _drawList.emplace_back(distKey((float)p.c, (float)p.r), [this, &p] { drawPickup3D(p); });

    for (auto& g : _game.ghosts)
        if (g.mode != Ghost::Mode::Dead && visible(g.x, g.z, 0.5f))
            _drawList.emplace_back(distKey(g.x, g.z), [this, &g] {
                bool scared   = g.mode == Ghost::Mode::Scared;
                bool flashing = scared && _game.scaredT < 1.6f && std::fmod(_time, 0.3f) < 0.15f;
                Color col     = ghostColor(g.id);
                if (scared)
                    col = flashing ? Color(0.95f, 0.95f, 1.f) : Color(0.18f, 0.25f, 0.95f);
                float hop = std::fabs(std::sin(g.bob)) * 0.07f;
                float rot = (g.dx || g.dz) ? std::atan2((float)g.dx, (float)g.dz) : 0.f;
                _gfx.disc(g.x, 0.01f, g.z, 0.42f, Color(0, 0, 0, 0.35f), 14);
                actors::ghostCube(_gfx, V3(g.x, 0.30f + hop, g.z), 0.30f, rot, col, scared);
            });

    if (visible(_game.px, _game.pz, 0.5f))
        _drawList.emplace_back(distKey(_game.px, _game.pz) - 0.05f, [this] {
            if (_game.state == State::Playing && _game.invuln > 0.f && std::fmod(_time, 0.2f) < 0.1f)
                return;
            float deathT  = _game.state == State::Dying ? _game.stateT : 0.f;
            float rot     = _yawVis + deathT * 9.f;
            float k       = deathT > 0.f ? std::max(0.f, 1.f - deathT / 1.6f) : 1.f;
            float hop     = _game.pMoving ? std::fabs(std::sin(_game.chompAnim)) * 0.07f : 0.f;
            const float h = 0.36f * k;
            float open    = _game.pMoving ? 0.5f + 0.5f * std::sin(_game.chompAnim * 1.4f) : 0.15f;
            _gfx.disc(_game.px, 0.01f, _game.pz, 0.5f * k, Color(0, 0, 0, 0.4f), 16);
            actors::chompCube(_gfx, V3(_game.px, h + hop + (deathT > 0.f ? deathT * 0.25f : 0.f), _game.pz), h, rot, open,
                              false);
        });

    std::sort(_drawList.begin(), _drawList.end(), [](auto& a, auto& b) { return a.first > b.first; });
    for (auto& d : _drawList)
        d.second();
}

void GameScene::drawPickup3D(const Pickup& p)
{
    float x = (float)p.c, z = (float)p.r;
    float spin = _time * 2.f + p.c * 0.7f;
    Gfx3D::BoxStyle st;
    switch (p.kind)
    {
    case Pickup::Kind::Pellet:
        st.color = hueColor((p.c * 5 + p.r * 3) / 29.f, 0.7f, 1.f);
        _gfx.box(V3(x, 0.28f, z), V3(0.08f, 0.08f, 0.08f), spin, st);
        break;
    case Pickup::Kind::Power:
    {
        float k  = 0.20f + 0.03f * std::sin(_time * 8.f);
        st.color = Color(1.f, 0.55f, 0.1f);
        _gfx.box(V3(x, 0.38f, z), V3(k, k, k), spin, st);
        break;
    }
    case Pickup::Kind::Heart:
        st.color = Color(0.95f, 0.12f, 0.25f);
        _gfx.box(V3(x, 0.40f + 0.05f * std::sin(_time * 4.f), z), V3(0.2f, 0.2f, 0.12f), spin, st);
        break;
    case Pickup::Kind::Bomb:
        st.color = Color(0.18f, 0.18f, 0.22f);
        _gfx.box(V3(x, 0.38f, z), V3(0.2f, 0.2f, 0.2f), spin, st);
        break;
    }
}

void GameScene::drawEffects()
{
    for (auto& e : _game.explosions)
    {
        float t = e.t / 0.9f, r = 0.4f + t * 3.3f, a = 1.f - t;
        _gfx.disc(e.x, 0.04f, e.z, r, Color(1.f, 0.35f, 0.05f, 0.55f * a), 24, false);
        _gfx.disc(e.x, 0.06f, e.z, r * 0.75f, Color(1.f, 0.65f, 0.1f, 0.65f * a), 24, false);
        _gfx.disc(e.x, 0.08f, e.z, r * 0.45f, Color(1.f, 0.95f, 0.5f, 0.8f * a), 20, false);
    }
    for (auto& p : _game.popups)
    {
        V3 cc = _gfx.toCam(V3(p.x, 0.9f + p.t * 0.8f, p.z));
        if (cc.z < 0.3f)
            continue;
        pixfont::drawShadow(_world, std::to_string(p.value), _gfx.project(cc), 2.f, Color(0.3f, 1.f, 1.f), 0.5f);
    }
}

// ------------------------------------------------------------------ HUD / overlay

void GameScene::drawHeart(DrawNode* dn, Vec2 pos, float px, const Color& c)
{
    static const char* rows[6] = {".##.##.", "#######", "#######", ".#####.", "..###..", "...#..."};
    for (int r = 0; r < 6; ++r)
        for (int col = 0; col < 7;)
        {
            if (rows[r][col] != '#')
            {
                ++col;
                continue;
            }
            int run = col;
            while (run < 7 && rows[r][run] == '#')
                ++run;
            dn->drawSolidRect(Vec2(pos.x + col * px, pos.y + (5 - r) * px), Vec2(pos.x + run * px, pos.y + (6 - r) * px), c);
            col = run;
        }
}

void GameScene::renderHud()
{
    if (_game.state == State::Title)
    {
        if (!_hudKey.empty())
        {
            _hud->clear();
            _hudKey.clear();
        }
        return;
    }
    char key[96];
    std::snprintf(key, sizeof(key), "%d/%d/%d/%d", _game.score, _game.lives, _game.bombs, _game.level);
    if (_hudKey == key)
        return;
    _hudKey = key;
    _hud->clear();

    const float top = kH - 28.f;
    // hearts: three slots (grey = lost), extra lives show as additional red hearts
    int slots = std::max(3, _game.lives);
    for (int i = 0; i < slots; ++i)
        drawHeart(_hud, Vec2(8.f + i * 22.f, top + 3.f), 2.f,
                  i < _game.lives ? Color(0.9f, 0.1f, 0.15f) : Color(0.28f, 0.28f, 0.30f));

    pixfont::drawShadow(_hud, "SCORE " + num(_game.score, 6), Vec2(150.f, top + 4.f), 2.f, kYellow);

    Vec2 bp(318.f, top + 11.f);
    _hud->drawSolidCircle(bp, 7.f, 0.f, 14, Color(0.12f, 0.12f, 0.16f));
    _hud->drawSolidCircle(bp + Vec2(-2, 2), 2.f, 0.f, 8, Color(0.5f, 0.5f, 0.6f));
    _hud->drawSolidRect(bp + Vec2(4, 5), bp + Vec2(7, 10), Color(1.f, 0.7f, 0.1f));
    pixfont::drawShadow(_hud, "x" + std::to_string(_game.bombs), Vec2(330.f, top + 4.f), 2.f, Color(1.f, 1.f, 1.f));

    pixfont::drawShadow(_hud, "LEVEL " + std::to_string(_game.level), Vec2(504.f, top + 2.f), 3.f, kYellow, 1.f);
}

void GameScene::renderOverlay()
{
    _overlay->clear();
    auto& st = Settings::get();

    auto centered = [&](const std::string& s, float y, float scale, Color c) {
        pixfont::drawShadow(_overlay, s, Vec2(256.f, y), scale, c, 0.5f);
    };

    switch (_game.state)
    {
    case State::Title:
    {
        centered("HI SCORE: " + std::to_string(std::max(st.hiScore, _game.hiScore)), kH - 22.f, 2.f, kYellow);

        char buf[64];
        std::vector<std::string> items;
        if (_menu == Menu::Main)
            items = {"START", "OPTIONS", "EXIT"};
        else
        {
            std::snprintf(buf, sizeof(buf), "MUSIC: %s", st.music ? "ON" : "OFF");
            items.push_back(buf);
            std::snprintf(buf, sizeof(buf), "VOLUME: %d/%d", st.audioVolume, Settings::kMaxVolume);
            items.push_back(buf);
            items.push_back(std::string("WINDOW: ") + (st.windowed ? "WINDOWED" : "FULLSCREEN"));
            items.push_back(std::string("SMOOTHING: ") + (st.smoothing ? "ON" : "OFF"));
            items.push_back("BACK");
        }
        const float rowsY[5] = {168.f, 144.f, 120.f, 96.f, 72.f};
        for (int i = 0; i < (int)items.size(); ++i)
        {
            bool sel = i == _sel;
            Color c  = sel ? Color(1.f, 1.f, 0.55f) : kYellow;
            if (sel && std::fmod(_time, 0.8f) < 0.5f)
                c = Color(1.f, 1.f, 1.f);
            centered(items[i], rowsY[i], 2.f, c);
            if (sel)
            {
                float hw = pixfont::textWidth(items[i], 2.f) * 0.5f;
                pixfont::drawShadow(_overlay, ">", Vec2(256.f - hw - 18.f, rowsY[i]), 2.f, Color(1.f, 0.5f, 0.2f));
                pixfont::drawShadow(_overlay, "<", Vec2(256.f + hw + 8.f, rowsY[i]), 2.f, Color(1.f, 0.5f, 0.2f));
            }
        }
        pixfont::drawShadow(_overlay, _menu == Menu::Main ? "UP/DOWN + ENTER" : "LEFT/RIGHT CHANGE  ESC BACK",
                            Vec2(256.f, 8.f), 1.f, Color(0.6f, 0.6f, 0.8f), 0.5f);
        break;
    }
    case State::Ready:
        centered("READY!", kH * 0.5f + 20.f, 5.f, kYellow);
        centered("LEVEL " + std::to_string(_game.level), kH * 0.5f - 24.f, 2.f, Color(1.f, 1.f, 1.f));
        break;
    case State::LevelClear: centered("LEVEL CLEAR!", kH * 0.5f, 4.f, Color(0.4f, 1.f, 0.5f)); break;
    case State::GameOver:
        centered("GAME OVER", kH * 0.5f + 40.f, 6.f, Color(1.f, 0.25f, 0.25f));
        centered("SCORE " + num(_game.score, 6), kH * 0.5f - 10.f, 3.f, kYellow);
        if (_game.newHiScore && std::fmod(_time, 0.6f) < 0.35f)
            centered("NEW HI SCORE!", kH * 0.5f - 44.f, 2.f, Color(0.4f, 1.f, 1.f));
        centered("PRESS ENTER", kH * 0.5f - 80.f, 2.f, Color(1.f, 1.f, 1.f));
        break;
    case State::Paused:
        centered("PAUSED", kH * 0.5f + 10.f, 5.f, Color(1.f, 1.f, 1.f));
        centered("P TO CONTINUE", kH * 0.5f - 30.f, 2.f, Color(0.8f, 0.8f, 1.f));
        break;
    default: break;
    }
}
