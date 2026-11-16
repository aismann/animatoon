#include "GameScene.h"

#include "Config.h"
#include "Sound.h"
#include "TextDraw.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace ax;

namespace
{
constexpr float kW = 512.f, kH = 512.f;
constexpr float kPi = 3.14159265f;
constexpr float kWallH = 0.28f;  // half height of a wall slab (the cube is taller than the walls)

const Color kYellow(1.0f, 0.86f, 0.05f);
const Color kSky0(0.03f, 0.03f, 0.09f);   // zenith
const Color kSky1(0.20f, 0.15f, 0.38f);   // horizon

Color hueColor(float h, float s = 0.85f, float v = 1.f)
{
    h = h - std::floor(h);
    float r = 0, g = 0, b = 0;
    float f = h * 6.f;
    int i   = (int)f;
    float q = f - i;
    float p = v * (1 - s), qq = v * (1 - s * q), t = v * (1 - s * (1 - q));
    switch (i % 6)
    {
    case 0: r = v; g = t; b = p; break;
    case 1: r = qq; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = qq; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = qq; break;
    }
    return Color(r, g, b);
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
}  // namespace

GameScene::GameScene() {}
GameScene::~GameScene() {}

bool GameScene::init()
{
    if (!Scene::init())
        return false;

    _world = DrawNode::create();
    _world->setPreserveDrawOrder(true);  // painter's algorithm: keep submission order
    addChild(_world, 0);
    _hud = DrawNode::create();
    addChild(_hud, 1);
    _overlay = DrawNode::create();
    addChild(_overlay, 2);

    auto kb        = EventListenerKeyboard::create();
    kb->onKeyPressed = AX_CALLBACK_2(GameScene::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(kb, this);

    auto touch            = EventListenerTouchAllAtOnce::create();
    touch->onTouchesBegan = AX_CALLBACK_2(GameScene::onTouchesBegan, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);

    _gfx.smooth = Settings::get().smoothing != 0;
    Sound::init();

    scheduleUpdate();
    return true;
}

// ------------------------------------------------------------------ input

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
    auto& st = Settings::get();
    st.windowed = !st.windowed;
    if (auto view = dynamic_cast<RenderViewImpl*>(_director->getRenderView()))
    {
        if (st.windowed)
            view->setWindowed(st.windowWidth, st.windowWidth);
        else
            view->setFullscreen();
    }
    applyAndSave();
#endif
}

void GameScene::confirm()
{
    switch (_game.state)
    {
    case State::Title: _game.startGame(); break;
    case State::GameOver: _game.toTitle(); break;
    default: break;
    }
}

void GameScene::onKeyPressed(EventKeyboard::KeyCode code, Event*)
{
    using K = EventKeyboard::KeyCode;
    auto& st = Settings::get();
    switch (code)
    {
    case K::KEY_LEFT_ARROW:
    case K::KEY_A: _game.turnLeft(); break;
    case K::KEY_RIGHT_ARROW:
    case K::KEY_D: _game.turnRight(); break;
    case K::KEY_DOWN_ARROW:
    case K::KEY_S: _game.reverse(); break;
    case K::KEY_SPACE:
    case K::KEY_CTRL:
    case K::KEY_UP_ARROW:
    case K::KEY_W: _game.dropBomb(); break;
    case K::KEY_P:
    case K::KEY_PAUSE: _game.togglePause(); break;
    case K::KEY_RETURN:
    case K::KEY_KP_ENTER: confirm(); break;
    case K::KEY_ESCAPE:
        if (_game.state == State::Playing || _game.state == State::Paused)
            _game.togglePause();
        else if (_game.state == State::Title)
            _director->end();
        else
            confirm();
        break;
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

void GameScene::onTouchesBegan(const std::vector<Touch*>& touches, Event*)
{
    if (touches.empty())
        return;
    auto p = _director->convertToGL(touches[0]->getLocationInView());
    if (_game.state == State::Title || _game.state == State::GameOver)
    {
        confirm();
        return;
    }
    if (_game.state == State::Paused)
    {
        _game.togglePause();
        return;
    }
    if (p.y < 90.f && p.x > 180.f && p.x < 332.f)
        _game.dropBomb();
    else if (p.x < kW * 0.5f)
        _game.turnLeft();
    else
        _game.turnRight();
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

    updateCamera(dt);
    renderWorld();
    renderHud();
    renderOverlay();
}

void GameScene::updateCamera(float dt)
{
    const float target = _game.heading * kPi * 0.5f;
    float d            = wrapPi(target - _yawVis);
    _yawVis += d * std::min(1.f, dt * 9.f);

    if (_game.state == State::Title)
    {
        // slow fly-around over the demo maze
        float a   = _time * 0.25f;
        float R   = 11.5f;
        float cx  = (kGrid - 1) * 0.5f, cz = (kGrid - 1) * 0.5f;
        _gfx.pos   = V3(cx + std::sin(a) * R, 7.0f, cz + std::cos(a) * R);
        _gfx.yaw   = a + kPi;
        _gfx.pitch = 0.52f;
        _gfx.fogStart = 14.f;
        _gfx.fogEnd   = 30.f;
        _gfx.center   = Vec2(256.f, 215.f);
        return;
    }

    _gfx.pitch    = 0.26f;
    _gfx.fogStart = 4.f;
    _gfx.fogEnd   = 13.f;
    _gfx.center   = Vec2(256.f, 140.f);
    _gfx.yaw      = _yawVis;
    float sy = std::sin(_yawVis), cy = std::cos(_yawVis);
    float shake = (_game.hitFlash > 0.f) ? std::sin(_time * 90.f) * 0.08f * _game.hitFlash : 0.f;
    _gfx.pos    = V3(_game.px - sy * 3.2f + shake, 1.9f, _game.pz - cy * 3.2f);
}

// ------------------------------------------------------------------ world rendering

void GameScene::renderWorld()
{
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
    // horizon line on screen: direction (0,0,inf) projects to centre.y + focal * tan(pitch)
    float horizon = _gfx.center.y + _gfx.focal * std::tan(_gfx.pitch);
    horizon       = std::clamp(horizon, 0.f, kH);
    // ground colour below the horizon (fogged floor colour)
    _world->drawSolidRect(Vec2(0, 0), Vec2(kW, horizon), _gfx.fogColor);
    // banded sky gradient above it
    const int bands = 14;
    for (int i = 0; i < bands; ++i)
    {
        float t0 = float(i) / bands, t1 = float(i + 1) / bands;
        float y0 = horizon + (kH - horizon) * t0;
        float y1 = i == bands - 1 ? kH : horizon + (kH - horizon) * t1;
        _world->drawSolidRect(Vec2(0, y0), Vec2(kW, y1 + 1.f), mix(kSky1, kSky0, std::pow(t0, 0.6f)));
    }
}

void GameScene::drawFloor()
{
    const bool title = _game.state == State::Title;
    const float maxD = title ? 40.f : 13.f;
    const Color a(0.17f, 0.18f, 0.25f), b(0.14f, 0.15f, 0.21f);
    for (int r = 0; r < kGrid; ++r)
        for (int c = 0; c < kGrid; ++c)
        {
            if (_game.wall[r][c])
                continue;
            V3 centre((float)c, 0.f, (float)r);
            V3 cc = _gfx.toCam(centre);
            if (cc.z < -1.2f || cc.z > maxD)
                continue;
            if (!title && std::fabs(cc.x) > cc.z * 0.9f + 2.2f)
                continue;
            const float h = 0.5f;
            _gfx.quad(V3(c - h, 0, r - h), V3(c + h, 0, r - h), V3(c + h, 0, r + h), V3(c - h, 0, r + h),
                      ((r + c) & 1) ? a : b);
        }
}

void GameScene::drawEntities()
{
    _drawList.clear();
    const bool title = _game.state == State::Title;
    const float maxD = title ? 40.f : 13.f;

    auto visible = [&](float x, float z, float margin) {
        V3 cc = _gfx.toCam(V3(x, 0.4f, z));
        if (cc.z < -1.0f - margin || cc.z > maxD)
            return false;
        return title || std::fabs(cc.x) <= std::max(0.f, cc.z) * 0.9f + 2.2f + margin;
    };
    auto distKey = [&](float x, float z) {
        float dx = x - _gfx.pos.x, dz = z - _gfx.pos.z;
        return dx * dx + dz * dz;
    };

    // walls
    for (int r = 0; r < kGrid; ++r)
        for (int c = 0; c < kGrid; ++c)
        {
            if (!_game.wall[r][c] || !visible((float)c, (float)r, 0.8f))
                continue;
            int skip = 0;
            auto w   = [&](int cc, int rr) { return cc < 0 || rr < 0 || cc >= kGrid || rr >= kGrid || _game.wall[rr][cc]; };
            if (w(c + 1, r)) skip |= 1;
            if (w(c - 1, r)) skip |= 2;
            if (w(c, r + 1)) skip |= 4;
            if (w(c, r - 1)) skip |= 8;
            const bool border = c == 0 || r == 0 || c == kGrid - 1 || r == kGrid - 1;
            _drawList.emplace_back(distKey((float)c, (float)r), [this, c, r, skip, border] {
                Gfx3D::BoxStyle st;
                st.color       = border ? Color(0.24f, 0.20f, 0.55f) : Color(0.33f, 0.28f, 0.74f);
                st.top         = Color(0.06f, 0.05f, 0.12f);
                st.useTopColor = true;
                st.ambient     = 0.42f;
                _gfx.box(V3((float)c, kWallH, (float)r), V3(0.5f, kWallH, 0.5f), 0.f, st, skip);
            });
        }

    for (auto& p : _game.pickups)
        if (p.alive && visible((float)p.c, (float)p.r, 0.3f))
            _drawList.emplace_back(distKey((float)p.c, (float)p.r), [this, &p] { drawPickup(p); });

    for (auto& g : _game.ghosts)
        if (g.mode != Ghost::Mode::Dead && visible(g.x, g.z, 0.5f))
            _drawList.emplace_back(distKey(g.x, g.z), [this, &g] { drawGhost(g); });

    if (!title && visible(_game.px, _game.pz, 0.5f))
        _drawList.emplace_back(distKey(_game.px, _game.pz) - 0.05f, [this] { drawPlayer(); });

    std::sort(_drawList.begin(), _drawList.end(), [](auto& a, auto& b) { return a.first > b.first; });
    for (auto& d : _drawList)
        d.second();
}

void GameScene::drawPickup(const Pickup& p)
{
    float x = (float)p.c, z = (float)p.r;
    float spin = _time * 2.f + p.c * 0.7f;
    Gfx3D::BoxStyle st;
    switch (p.kind)
    {
    case Pickup::Kind::Pellet:
        st.color = Color(1.f, 0.93f, 0.55f);
        _gfx.box(V3(x, 0.28f, z), V3(0.07f, 0.07f, 0.07f), spin, st);
        break;
    case Pickup::Kind::Power:
    {
        float k  = 0.20f + 0.03f * std::sin(_time * 8.f);
        st.color = Color(1.f, 0.55f, 0.1f);
        _gfx.disc(x, 0.01f, z, 0.32f, Color(0, 0, 0, 0.35f), 14);
        _gfx.box(V3(x, 0.38f, z), V3(k, k, k), spin, st);
        break;
    }
    case Pickup::Kind::Heart:
    {
        float k  = 0.20f + 0.03f * std::sin(_time * 6.f);
        st.color = Color(0.95f, 0.12f, 0.25f);
        _gfx.disc(x, 0.01f, z, 0.32f, Color(0, 0, 0, 0.35f), 14);
        _gfx.box(V3(x, 0.40f + 0.05f * std::sin(_time * 4.f), z), V3(k, k, k * 0.6f), spin, st);
        break;
    }
    case Pickup::Kind::Bomb:
    {
        st.color = Color(0.18f, 0.18f, 0.22f);
        _gfx.disc(x, 0.01f, z, 0.32f, Color(0, 0, 0, 0.35f), 14);
        float by = 0.38f + 0.04f * std::sin(_time * 5.f);
        _gfx.box(V3(x, by, z), V3(0.2f, 0.2f, 0.2f), spin, st);
        Gfx3D::BoxStyle fuse;
        fuse.color = (std::fmod(_time, 0.3f) < 0.15f) ? Color(1.f, 0.85f, 0.1f) : Color(1.f, 0.35f, 0.05f);
        _gfx.box(V3(x, by + 0.26f, z), V3(0.05f, 0.07f, 0.05f), spin, fuse);
        break;
    }
    }
}

void GameScene::drawGhost(const Ghost& g)
{
    static const Color body[3] = {Color(0.70f, 0.64f, 0.92f), Color(0.20f, 0.72f, 0.25f), Color(0.98f, 0.45f, 0.12f)};
    Color col                  = body[g.id % 3];
    bool scared                = g.mode == Ghost::Mode::Scared;
    bool flashing              = scared && _game.scaredT < 1.6f && std::fmod(_time, 0.3f) < 0.15f;
    if (scared)
        col = flashing ? Color(0.95f, 0.95f, 1.f) : Color(0.18f, 0.25f, 0.95f);

    float hop = std::fabs(std::sin(g.bob)) * 0.07f;
    float rot = (g.dx || g.dz) ? std::atan2((float)g.dx, (float)g.dz) : 0.f;
    const float h = 0.30f;
    _gfx.disc(g.x, 0.01f, g.z, 0.42f, Color(0, 0, 0, 0.35f), 14);

    Gfx3D::BoxStyle st;
    st.color = col;
    st.ambient = 0.6f;
    V3 c(g.x, h + hop, g.z);
    int mask = _gfx.box(c, V3(h, h, h), rot, st);

    // eyes on the +z (front) face
    if (mask & 4)
    {
        float cs = std::cos(rot), sn = std::sin(rot);
        V3 axv(cs, 0, -sn), azv(sn, 0, cs);
        V3 origin = c + axv * h + azv * (h + 0.003f) + V3(0, -h, 0);
        V3 u      = axv * (-2 * h);
        V3 v(0, 2 * h, 0);
        Color eye = scared ? Color(1.f, 1.f, 1.f) : Color(0.05f, 0.04f, 0.1f);
        _gfx.decal(origin, u, v, 0.18f, 0.50f, 0.34f, 0.82f, eye);
        _gfx.decal(origin, u, v, 0.66f, 0.50f, 0.82f, 0.82f, eye);
        if (scared)
            _gfx.decal(origin, u, v, 0.20f, 0.20f, 0.80f, 0.30f, Color(1.f, 1.f, 1.f));
    }
}

void GameScene::drawPlayer()
{
    // Chompston: yellow cube. Body follows the camera yaw so we always look at his back,
    // therefore the face (eyes + chomping mouth) is painted on both front and back.
    if (_game.state == State::Playing && _game.invuln > 0.f && std::fmod(_time, 0.2f) < 0.1f)
        return;

    float deathT = _game.state == State::Dying ? _game.stateT : 0.f;
    float rot    = _yawVis + deathT * 9.f;
    float k      = deathT > 0.f ? std::max(0.f, 1.f - deathT / 1.6f) : 1.f;
    float hop    = _game.pMoving ? std::fabs(std::sin(_game.chompAnim)) * 0.07f : 0.f;
    const float h = 0.36f * k;

    V3 c(_game.px, h + hop + (deathT > 0.f ? deathT * 0.25f : 0.f), _game.pz);
    _gfx.disc(_game.px, 0.01f, _game.pz, 0.5f * k, Color(0, 0, 0, 0.4f), 16);

    Gfx3D::BoxStyle st;
    st.color   = kYellow;
    st.ambient = 0.62f;
    int mask   = _gfx.box(c, V3(h, h, h), rot, st);
    if (k <= 0.f)
        return;

    float cs = std::cos(rot), sn = std::sin(rot);
    V3 axv(cs, 0, -sn), azv(sn, 0, cs);
    float open = _game.pMoving ? 0.5f + 0.5f * std::sin(_game.chompAnim * 1.4f) : 0.15f;

    auto face = [&](V3 origin, V3 u) {
        V3 v(0, 2 * h, 0);
        const Color black(0.04f, 0.03f, 0.02f);
        _gfx.decal(origin, u, v, 0.20f, 0.52f, 0.32f, 0.86f, black);  // eyes
        _gfx.decal(origin, u, v, 0.68f, 0.52f, 0.80f, 0.86f, black);
        float top = 0.16f + 0.10f + 0.20f * open;
        _gfx.decal(origin, u, v, 0.12f, 0.16f, 0.88f, top, black);     // mouth
        _gfx.decal(origin, u, v, 0.16f, top - 0.07f, 0.84f, top - 0.01f, Color(0.82f, 0.82f, 0.86f));  // teeth
    };
    if (mask & 8)  // back face, seen from behind: screen right == local +x
        face(c + axv * (-h) + azv * (-h - 0.003f) + V3(0, -h, 0), axv * (2 * h));
    if (mask & 4)  // front face
        face(c + axv * h + azv * (h + 0.003f) + V3(0, -h, 0), axv * (-2 * h));
}

void GameScene::drawEffects()
{
    // bomb explosions: expanding fire rings on the floor + rising glow columns
    for (auto& e : _game.explosions)
    {
        float t = e.t / 0.9f;
        float r = 0.4f + t * 3.3f;
        float a = 1.f - t;
        _gfx.disc(e.x, 0.04f, e.z, r, Color(1.f, 0.35f, 0.05f, 0.55f * a), 24, false);
        _gfx.disc(e.x, 0.06f, e.z, r * 0.75f, Color(1.f, 0.65f, 0.1f, 0.65f * a), 24, false);
        _gfx.disc(e.x, 0.08f, e.z, r * 0.45f, Color(1.f, 0.95f, 0.5f, 0.8f * a), 20, false);
        for (int i = 0; i < 3; ++i)
            _gfx.disc(e.x, 0.3f + i * 0.35f + t * 0.8f, e.z, r * (0.30f - i * 0.07f), Color(1.f, 0.85f, 0.2f, 0.5f * a),
                      14, false);
    }
    // score popups
    for (auto& p : _game.popups)
    {
        V3 cc = _gfx.toCam(V3(p.x, 0.9f + p.t * 0.8f, p.z));
        if (cc.z < 0.3f)
            continue;
        Vec2 s = _gfx.project(cc);
        pixfont::drawShadow(_world, std::to_string(p.value), s, 2.f, Color(0.3f, 1.f, 1.f), 0.5f);
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
    std::snprintf(key, sizeof(key), "%d/%d/%d/%d/%d", _game.score, _game.hiScore, _game.lives, _game.bombs,
                  _game.level);
    if (_hudKey == key)
        return;
    _hudKey = key;
    _hud->clear();

    const Color gray(0.7f, 0.7f, 0.85f);
    pixfont::drawShadow(_hud, "SCORE", Vec2(12, 494), 2.f, gray);
    pixfont::drawShadow(_hud, num(_game.score, 6), Vec2(12, 474), 3.f, kYellow);
    pixfont::drawShadow(_hud, "HI", Vec2(500, 494), 2.f, gray, 1.f);
    pixfont::drawShadow(_hud, num(_game.hiScore, 6), Vec2(500, 474), 3.f, Color(1.f, 0.55f, 0.2f), 1.f);

    for (int i = 0; i < _game.lives; ++i)
        drawHeart(_hud, Vec2(12.f + i * 28.f, 14.f), 3.f, Color(0.95f, 0.12f, 0.25f));

    // bombs
    Vec2 bp(180.f, 24.f);
    _hud->drawSolidCircle(bp, 10.f, 0.f, 16, Color(0.12f, 0.12f, 0.16f));
    _hud->drawSolidCircle(bp + Vec2(-3, 3), 3.f, 0.f, 8, Color(0.5f, 0.5f, 0.6f));
    _hud->drawSolidRect(bp + Vec2(6, 8), bp + Vec2(10, 16), Color(1.f, 0.7f, 0.1f));
    pixfont::drawShadow(_hud, "x" + std::to_string(_game.bombs), Vec2(196, 17), 2.f, Color(1.f, 1.f, 1.f));

    pixfont::drawShadow(_hud, "LEVEL " + std::to_string(_game.level), Vec2(500, 14), 2.f, gray, 1.f);
}

void GameScene::renderOverlay()
{
    _overlay->clear();
    auto& st = Settings::get();
    const Vec2 mid(256.f, 0.f);

    auto centered = [&](const std::string& s, float y, float scale, Color c) {
        pixfont::drawShadow(_overlay, s, Vec2(256.f, y), scale, c, 0.5f);
    };

    switch (_game.state)
    {
    case State::Title:
    {
        const std::string title = "CHOMPSTON";
        float scale             = 8.f;
        float x0                = 256.f - pixfont::textWidth(title, scale) * 0.5f;
        // shadow, then rainbow letters (like the logo of the original)
        pixfont::draw(_overlay, title, Vec2(x0 + 4, 396 - 4), scale, Color(0, 0, 0, 0.85f));
        pixfont::draw(_overlay, title, Vec2(x0, 396), scale,
                      [&](size_t i) { return hueColor(float(i) / 9.f + _time * 0.15f, 0.9f, 1.f); });

        if (std::fmod(_time, 1.f) < 0.65f)
            centered("PRESS ENTER TO START", 120, 3.f, Color(1.f, 1.f, 1.f));
        centered("HI SCORE " + num(std::max(st.hiScore, _game.hiScore), 6), 330, 2.f, Color(1.f, 0.55f, 0.2f));

        char buf[64];
        std::snprintf(buf, sizeof(buf), "M MUSIC: %s", st.music ? "ON" : "OFF");
        pixfont::drawShadow(_overlay, buf, Vec2(14, 66), 2.f, Color(0.8f, 0.8f, 1.f));
        std::snprintf(buf, sizeof(buf), "V VOLUME: %d/%d", st.audioVolume, Settings::kMaxVolume);
        pixfont::drawShadow(_overlay, buf, Vec2(14, 46), 2.f, Color(0.8f, 0.8f, 1.f));
        std::snprintf(buf, sizeof(buf), "F WINDOWED: %s", st.windowed ? "YES" : "NO");
        pixfont::drawShadow(_overlay, buf, Vec2(14, 26), 2.f, Color(0.8f, 0.8f, 1.f));
        std::snprintf(buf, sizeof(buf), "G SMOOTHING: %s", st.smoothing ? "ON" : "OFF");
        pixfont::drawShadow(_overlay, buf, Vec2(14, 6), 2.f, Color(0.8f, 0.8f, 1.f));

        pixfont::drawShadow(_overlay, "LEFT/RIGHT TURN", Vec2(498, 66), 2.f, Color(0.8f, 1.f, 0.8f), 1.f);
        pixfont::drawShadow(_overlay, "DOWN REVERSE", Vec2(498, 46), 2.f, Color(0.8f, 1.f, 0.8f), 1.f);
        pixfont::drawShadow(_overlay, "SPACE BOMB", Vec2(498, 26), 2.f, Color(0.8f, 1.f, 0.8f), 1.f);
        pixfont::drawShadow(_overlay, "P PAUSE", Vec2(498, 6), 2.f, Color(0.8f, 1.f, 0.8f), 1.f);
        break;
    }
    case State::Ready:
        centered("READY!", 300, 6.f, kYellow);
        centered("LEVEL " + std::to_string(_game.level), 250, 3.f, Color(1.f, 1.f, 1.f));
        break;
    case State::LevelClear:
        centered("LEVEL CLEAR!", 300, 5.f, Color(0.4f, 1.f, 0.5f));
        break;
    case State::GameOver:
        centered("GAME OVER", 320, 7.f, Color(1.f, 0.25f, 0.25f));
        centered("SCORE " + num(_game.score, 6), 260, 3.f, kYellow);
        if (_game.newHiScore && std::fmod(_time, 0.6f) < 0.35f)
            centered("NEW HI SCORE!", 220, 3.f, Color(0.4f, 1.f, 1.f));
        centered("PRESS ENTER", 150, 2.f, Color(1.f, 1.f, 1.f));
        break;
    case State::Paused:
        centered("PAUSED", 300, 6.f, Color(1.f, 1.f, 1.f));
        centered("P TO CONTINUE", 250, 2.f, Color(0.8f, 0.8f, 1.f));
        break;
    default: break;
    }
}
