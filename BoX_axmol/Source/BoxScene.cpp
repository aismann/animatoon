#include "BoxScene.h"
using namespace ax;

namespace
{
constexpr float kWin     = 640.f;
constexpr float kPPM     = 100.f;                // Pixel pro Meter (Box2D rechnet in Metern)
constexpr float kStep    = 1.f / 60.f / 3.f;     // 3 Substeps pro Frame wie im Original
constexpr float kBallR   = 20.f / kPPM;
constexpr int   kLevels  = 14;
constexpr float kWallR   = 3.f / kPPM;          // Wanddicke (Kapseln statt duenner Segmente -> kein Durchtunneln)
constexpr float kInner   = 200.f / kPPM - kWallR;  // Innenkante des Rahmens
constexpr float kMaxW    = 8.f;                 // max. Winkelgeschwindigkeit des Kastens (rad/s)
constexpr float kPartR   = 6.f / kPPM;
constexpr int   kFluidWin = 8;                  // Partikel im Ziel zum Gewinnen
constexpr float kWinDist = 23.f / kPPM;
const Color kBlack(0, 0, 0, 1), kBlue(0, 0, 1, 1), kRed(1, 0, 0, 1);

// PureBasic hat y nach unten, Axmol/Box2D nach oben -> y spiegeln. Eingabe in Pixeln, Ausgabe in Metern.
b2Vec2 V(float x, float y) { return {x / kPPM, -y / kPPM}; }
// b2Pos ist je nach Box2D-Build float oder double
template <class P> b2Vec2 toV(P p) { return {(float)p.x, (float)p.y}; }
Vec2 px(b2Vec2 v) { return Vec2(v.x * kPPM, v.y * kPPM); }

// 7-Segment: a(oben) b(o.r.) c(u.r.) d(unten) e(u.l.) f(o.l.) g(mitte)
const unsigned char kSeg[10] = {0b0111111, 0b0000110, 0b1011011, 0b1001111, 0b1100110,
                                0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1101111};
}  // namespace

BoxScene* BoxScene::create()
{
    auto* s = new BoxScene();
    if (s->init()) { s->autorelease(); return s; }
    delete s;
    return nullptr;
}

BoxScene::~BoxScene()
{
    if (B2_IS_NON_NULL(_world)) b2DestroyWorld(_world);
}

bool BoxScene::init()
{
    if (!Scene::init()) return false;

    auto* bg = LayerColor::create(Color32(200, 200, 200, 255));
    addChild(bg);
    _draw = DrawNode::create();
    addChild(_draw);

    auto kb = KeyboardEventListener::create();
    kb->onKeyPressed = [this](KeyboardEvent* e) {
        using K = KeyboardEvent::KeyCode;
        K k = e->getKeyCode();
        if (_state == State::Menu)
        {
            switch (k)
            {
            case K::KEY_LEFT_ARROW:
            case K::KEY_RIGHT_ARROW: _mode = (_mode == Mode::Ball) ? Mode::Fluid : Mode::Ball; break;
            case K::KEY_1: _mode = Mode::Ball; break;
            case K::KEY_2: _mode = Mode::Fluid; break;
            case K::KEY_UP_ARROW:   _menuLevel = _menuLevel % kLevels + 1; break;
            case K::KEY_DOWN_ARROW: _menuLevel = (_menuLevel + kLevels - 2) % kLevels + 1; break;
            case K::KEY_ENTER:
            case K::KEY_KP_ENTER:
            case K::KEY_RETURN:
            case K::KEY_SPACE: startGame(); break;
            case K::KEY_ESCAPE: Director::getInstance()->end(); break;
            default: break;
            }
            return;
        }
        switch (k)
        {
        case K::KEY_LEFT_ARROW:  _left = true; break;
        case K::KEY_RIGHT_ARROW: _right = true; break;
        case K::KEY_ESCAPE:      _state = State::Menu; break;   // zurueck ins Menue
        default: break;
        }
    };
    kb->onKeyReleased = [this](KeyboardEvent* e) {
        if (e->getKeyCode() == KeyboardEvent::KeyCode::KEY_LEFT_ARROW) _left = false;
        if (e->getKeyCode() == KeyboardEvent::KeyCode::KEY_RIGHT_ARROW) _right = false;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(kb, this);

    scheduleUpdate();
    return true;
}

void BoxScene::startLevel(int level)
{
    if (B2_IS_NON_NULL(_world)) b2DestroyWorld(_world);
    _segs.clear();
    _parts.clear();
    _ball = b2_nullBodyId;
    _level = level; _won = false; _w = 0.f; _winTimer = 0.f; _elapsed = 0.0;
    buildWorld(level);
}

void BoxScene::startGame()
{
    _left = _right = false;
    _finished = false;
    _finishTimer = 0.f;
    _state = State::Play;
    startLevel(_menuLevel);
}

void BoxScene::buildWorld(int level)
{
    b2WorldDef wd = b2DefaultWorldDef();
    wd.gravity    = {0.f, -2600.f / kPPM};
    _world        = b2CreateWorld(&wd);

    // Container: kinematischer Body, dreht sich um die Fenstermitte
    b2BodyDef bd = b2DefaultBodyDef();
    bd.type      = b2_kinematicBody;
    bd.position  = {320.f / kPPM, 320.f / kPPM};
    _boxBody     = b2CreateBody(_world, &bd);

    // Box2D mischt Restitution mit max(), Chipmunk multiplizierte (1*0 = 0) -> hier 0 fuer gleiches Verhalten.
    b2ShapeDef sd             = b2DefaultShapeDef();
    sd.material.friction      = 1.f;
    sd.material.restitution   = 0.f;

    auto seg = [&](b2Vec2 a, b2Vec2 b) {
        b2Capsule s{a, b, kWallR};
        b2CreateCapsuleShape(_boxBody, &sd, &s);
        _segs.push_back({a, b});
    };

    // Aussenrahmen
    b2Vec2 A = V(-200, -200), B = V(-200, 200), C = V(200, 200), D = V(200, -200);
    seg(A, B); seg(B, C); seg(C, D); seg(D, A);

    // Level-Daten (Pixel, y nach unten wie im Original): Ziel + innere Waende
    float goalX = 0, goalY = 0, goalR = 16;
    auto line = [&](float x1, float y1, float x2, float y2) { seg(V(x1, y1), V(x2, y2)); };

    auto hline = [&](float y, float x1, float x2) { line(x1, y, x2, y); };
    auto vline = [&](float x, float y1, float y2) { line(x, y1, x, y2); };

    switch (level)
    {
    case 1:
        goalX = 175; goalY = 175;
        for (int n = 0; n <= 3; ++n)
        {
            line(-200, -100 + n * 100, 100, -100 + n * 100);
            line(-100, -150 + n * 100, 200, -150 + n * 100);
        }
        break;
    case 2:
        line(-150, -150, 200, -150);
        line(-150, -150, -150, 150);
        line(150, 150, -150, 150);
        line(150, 150, 150, -100);
        line(150, -100, -100, -100);
        line(-100, -100, -100, 100);
        line(-100, 100, 50, 100);
        break;
    case 3:
    case 4:   // laut Vorlage: nur Rahmen + Ziel in der Mitte
        break;

    // ---- neue Levels 5-14 (Start: Ball/Fluid oben rechts, Streifen y < -150 bleibt frei) ----
    case 5:   // Zickzack
        hline(-100, -200, 110); hline(0, -110, 200); hline(100, -200, 110);
        goalX = 160; goalY = 165;
        break;
    case 6:   // Trichter
        line(-200, -100, -50, 0); line(200, -100, 50, 0);
        line(-200, 60, -30, 120); line(200, 60, 30, 120);
        goalX = 0; goalY = 170;
        break;
    case 7:   // Pegs
        for (int r = 0; r < 4; ++r)
            for (int k = 0; k < 4; ++k)
            {
                float x = -150 + k * 100 + (r % 2) * 50, y = -90 + r * 60;
                hline(y, x - 20, x + 20);
            }
        goalX = -170; goalY = 170;
        break;
    case 8:   // Trennwand
        vline(0, -150, 120);
        hline(-60, -200, -70);
        goalX = -150; goalY = -110;
        break;
    case 9:   // S-Waende
        vline(80, -50, 200); vline(-80, -150, 100);
        goalX = -150; goalY = 150;
        break;
    case 10:  // Tresor: Ziel in kleinem Kasten mit Oeffnung rechts
        hline(-60, -60, 60); hline(60, -60, 60); vline(-60, -60, 60);
        vline(60, -60, -30); vline(60, 30, 60);
        goalX = 0; goalY = 0;
        break;
    case 11:  // Tunnel: 4 Regale, Luecken wechseln
        hline(-110, -200, 140); hline(-45, -140, 200); hline(20, -200, 140); hline(85, -140, 200);
        goalX = -160; goalY = 165;
        break;
    case 12:  // Ring: Achteck mit Luecke rechts unten, Ziel in der Mitte
        {
            const float Rr = 100.f, k = 3.14159265f / 4.f;
            for (int i = 0; i < 8; ++i)
            {
                if (i == 7) continue;   // Luecke
                float a0 = k * (i + 0.5f), a1 = k * (i + 1.5f);
                line(Rr * cosf(a0), Rr * sinf(a0), Rr * cosf(a1), Rr * sinf(a1));
            }
        }
        goalX = 0; goalY = 0;
        break;
    case 13:  // Slalom
        vline(120, -150, 100); vline(40, -100, 200); vline(-40, -150, 100); vline(-120, -100, 200);
        goalX = -160; goalY = 0;
        break;
    case 14:  // Finale: Regale + Tresor unten rechts
        hline(-100, -200, 110); hline(0, -110, 200);
        hline(100, 100, 200); vline(100, 100, 140);
        goalX = 160; goalY = 160;
        break;
    }

    // Ziel: Sensor-Kreis
    _goalLocal         = V(goalX, goalY);
    _goalR             = goalR / kPPM;
    b2ShapeDef gd      = b2DefaultShapeDef();
    gd.isSensor        = true;
    b2Circle goal{_goalLocal, _goalR};
    b2CreateCircleShape(_boxBody, &gd, &goal);

    if (_mode == Mode::Ball)
    {
        // Ball (Original: Startposition 500/150 im Fenster, y nach unten)
        b2BodyDef ballDef = b2DefaultBodyDef();
        ballDef.type      = b2_dynamicBody;
        ballDef.isBullet  = true;
        ballDef.position  = {500.f / kPPM, (kWin - 150.f) / kPPM};
        _ball             = b2CreateBody(_world, &ballDef);
        b2ShapeDef bs     = sd;
        bs.density        = 1.f;
        b2Circle circle{{0.f, 0.f}, kBallR};
        b2CreateCircleShape(_ball, &bs, &circle);
    }
    else
    {
        // Fluid: 3 x 30 kleine Kreise im oberen Streifen des Kastens (Container steht bei Start still)
        b2ShapeDef ps           = b2DefaultShapeDef();
        ps.material.friction    = 0.05f;
        ps.material.restitution = 0.f;
        ps.density              = 1.f;
        b2Circle pc{{0.f, 0.f}, kPartR};
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 30; ++c)
            {
                b2Vec2 l          = V(190.f - c * 12.5f, -188.f + r * 14.f);
                b2BodyDef pd      = b2DefaultBodyDef();
                pd.type           = b2_dynamicBody;
                pd.isBullet       = true;
                pd.linearDamping  = 0.05f;
                pd.position       = {320.f / kPPM + l.x, 320.f / kPPM + l.y};
                b2BodyId id       = b2CreateBody(_world, &pd);
                b2CreateCircleShape(id, &ps, &pc);
                _parts.push_back(id);
            }
    }
}

// Box2D 3.x hat kein eingebautes Fluid (LiquidFun entfaellt): Partikel-Fluid aus Kreisen
// plus Viskositaet (Geschwindigkeits-Angleichung) und schwache Kohaesion zwischen Nachbarn.
void BoxScene::stepFluid()
{
    const size_t n = _parts.size();
    std::vector<b2Vec2> pos(n), vel(n);
    for (size_t i = 0; i < n; ++i)
    {
        pos[i] = toV(b2Body_GetPosition(_parts[i]));
        vel[i] = b2Body_GetLinearVelocity(_parts[i]);
    }
    const float h = 0.20f, dt = 1.f / 60.f;
    for (size_t i = 0; i < n; ++i)
    {
        b2Vec2 dv{0.f, 0.f}, coh{0.f, 0.f};
        float wsum = 0.f;
        for (size_t j = 0; j < n; ++j)
        {
            if (i == j) continue;
            b2Vec2 d = b2Sub(pos[j], pos[i]);
            float dist = std::sqrt(d.x * d.x + d.y * d.y);
            if (dist >= h || dist < 1e-5f) continue;
            float w = 1.f - dist / h;
            dv   = b2Add(dv, b2MulSV(w, b2Sub(vel[j], vel[i])));
            wsum += w;
            if (dist > 2.f * kPartR)   // nur anziehen, wenn nicht im Kontakt
                coh = b2Add(coh, b2MulSV(w / dist, d));
        }
        if (wsum <= 0.f) continue;
        b2Vec2 acc = b2Add(b2MulSV(0.3f / wsum / dt, dv), b2MulSV(3.f, coh));   // Viskositaet + Kohaesion
        float m    = b2Body_GetMass(_parts[i]);
        b2Body_ApplyLinearImpulseToCenter(_parts[i], b2MulSV(m * dt, acc), true);
    }
}

int BoxScene::countParticlesInGoal() const
{
    b2Vec2 g = toV(b2Body_GetWorldPoint(_boxBody, _goalLocal));
    int cnt  = 0;
    for (b2BodyId id : _parts)
    {
        b2Vec2 p = toV(b2Body_GetPosition(id));
        b2Vec2 d = b2Sub(p, g);
        if (d.x * d.x + d.y * d.y < (_goalR + kPartR) * (_goalR + kPartR)) ++cnt;
    }
    return cnt;
}

void BoxScene::update(float dt)
{
    if (_state == State::Menu)
    {
        _menuT += dt;
        _won = false;
        drawMenu();
        return;
    }
    if (_won)
    {
        _winTimer += dt;
        if (_winTimer > 1.5f && !_finished)
        {
            if (_level < kLevels) startLevel(_level + 1);
            else _finished = true;
        }
        if (_finished && (_finishTimer += dt) > 3.f) _state = State::Menu;
        drawAll();
        return;
    }

    _elapsed += dt;
    if (_left)  _w += 0.2f;
    if (_right) _w -= 0.2f;   // y-Achse gespiegelt: rechts = im Uhrzeigersinn = negativ
    _w = std::max(-kMaxW, std::min(kMaxW, _w));
    b2Body_SetAngularVelocity(_boxBody, _w);

    if (_mode == Mode::Fluid) stepFluid();
    for (int i = 0; i < 3; ++i)
    {
        b2World_Step(_world, kStep, 4);
        enforceBounds();
    }

    b2Vec2 g = toV(b2Body_GetWorldPoint(_boxBody, _goalLocal));
    if (_mode == Mode::Ball)
    {
        b2Vec2 b = toV(b2Body_GetPosition(_ball));
        if (std::abs(b.x - g.x) < kWinDist && std::abs(b.y - g.y) < kWinDist)
            _won = true;
    }
    else if (countParticlesInGoal() >= kFluidWin)
        _won = true;

    drawAll();
}

void BoxScene::drawAll()
{
    _draw->clear();

    // Segmente
    for (const Seg& s : _segs)
    {
        b2Vec2 a = toV(b2Body_GetWorldPoint(_boxBody, s.a));
        b2Vec2 b = toV(b2Body_GetWorldPoint(_boxBody, s.b));
        _draw->drawLine(px(a), px(b), kBlack, 1.f);
    }
    // Ziel
    _draw->drawCircle(px(toV(b2Body_GetWorldPoint(_boxBody, _goalLocal))), _goalR * kPPM, kBlack, 1.f);

    if (_mode == Mode::Ball)
    {
        // Ball mit Winkelstrich
        Vec2 p  = px(toV(b2Body_GetPosition(_ball)));
        float a = b2Rot_GetAngle(b2Body_GetRotation(_ball));
        _draw->drawCircle(p, kBallR * kPPM, kBlack, 1.f);
        _draw->drawLine(p, Vec2(p.x + cosf(a) * kBallR * kPPM, p.y + sinf(a) * kBallR * kPPM), kBlack, 1.f);

        // Kontaktpunkte (rot). anchorA ist relativ zum Body von Shape A.
        int cap = b2Body_GetContactCapacity(_ball);
        if (cap > 0)
        {
            std::vector<b2ContactData> cd(cap);
            int n = b2Body_GetContactData(_ball, cd.data(), cap);
            for (int i = 0; i < n; ++i)
            {
                b2Vec2 origin = toV(b2Body_GetPosition(b2Shape_GetBody(cd[i].shapeIdA)));
                for (int k = 0; k < cd[i].manifold.pointCount; ++k)
                {
                    b2Vec2 w = b2Add(origin, cd[i].manifold.points[k].anchorA);
                    _draw->drawDot(px(w), 2.f, kRed);
                }
            }
        }
    }
    else
    {
        drawMetaballs();
    }

    // Zeit als 7-Segment (statt Label, da nur DrawNode erlaubt)
    drawTime((long)(_elapsed * 1000.0), Vec2(10, kWin - 40));

    // Level-Anzeige: Zahl + Punkte fuer alle Levels (gefuellt = erreicht)
    drawNumber(_level, Vec2(kWin - 40, kWin - 40), 8.f);
    for (int i = 1; i <= kLevels; ++i)
    {
        Vec2 d(kWin - 235 + i * 12, kWin - 50);
        if (i <= _level) _draw->drawDot(d, 3.f, _finished ? kBlue : kBlack);
        else _draw->drawCircle(d, 3.f, kBlack, 1.f);
    }
}

void BoxScene::drawDigit(int d, Vec2 o, float s)
{
    // o = links unten, Hoehe 2*s, Breite s
    const Vec2 p[6] = {{0, 0}, {s, 0}, {s, s}, {s, 2 * s}, {0, 2 * s}, {0, s}};
    // Segmente a..g als Punktpaare
    const int seg[7][2] = {{4, 3}, {3, 2}, {2, 1}, {0, 1}, {0, 5}, {5, 4}, {5, 2}};
    for (int i = 0; i < 7; ++i)
        if (kSeg[d] & (1 << i))
            _draw->drawLine(o + p[seg[i][0]], o + p[seg[i][1]], _won ? kBlue : kBlack, 2.f);
}

void BoxScene::drawTime(long ms, Vec2 pos)
{
    std::string s = std::to_string(ms);
    if (s.size() < 4) s.insert(0, 4 - s.size(), '0');
    float x = pos.x, size = 8.f;
    for (char c : s) { drawDigit(c - '0', Vec2(x, pos.y), size); x += size + 8; }
}

void BoxScene::drawMenu()
{
    _draw->clear();
    const float cx = kWin / 2;

    // Titel: drehender Kasten mit Ball und Ziel (Logo)
    float a = _menuT * 0.8f;
    auto rot = [&](float x, float y) {
        return Vec2(cx + x * cosf(a) - y * sinf(a), 520 + x * sinf(a) + y * cosf(a));
    };
    const float s = 50.f;
    Vec2 q[4] = {rot(-s, -s), rot(-s, s), rot(s, s), rot(s, -s)};
    for (int i = 0; i < 4; ++i) _draw->drawLine(q[i], q[(i + 1) % 4], kBlack, 2.f);
    _draw->drawLine(rot(-s, 0), rot(s * 0.5f, 0), kBlack, 2.f);
    _draw->drawCircle(rot(s * 0.6f, s * 0.6f), 8.f, kBlack, 1.f);
    _draw->drawDot(rot(0, -s * 0.5f), 7.f, kBlack);

    // Zwei Felder: 1 = ein Ball, 2 = Fluid
    for (int m = 0; m < 2; ++m)
    {
        bool sel   = (int)_mode == m;
        float px0  = (m == 0 ? 175.f : 465.f);
        Vec2 lo(px0 - 110, 190), hi(px0 + 110, 420);
        float th   = sel ? 4.f : 1.f;
        Color col  = sel ? kBlue : kBlack;
        _draw->drawLine(lo, Vec2(hi.x, lo.y), col, th);
        _draw->drawLine(Vec2(hi.x, lo.y), hi, col, th);
        _draw->drawLine(hi, Vec2(lo.x, hi.y), col, th);
        _draw->drawLine(Vec2(lo.x, hi.y), lo, col, th);

        Vec2 c(px0, 330);
        if (m == 0)
        {
            float b = sel ? sinf(_menuT * 6.f) * 10.f : 0.f;   // Ball wippt, wenn gewaehlt
            _draw->drawCircle(Vec2(c.x, c.y + b), 40.f, kBlack, 2.f);
            _draw->drawLine(Vec2(c.x, c.y + b), Vec2(c.x + 40.f * cosf(_menuT * 3.f), c.y + b + 40.f * sinf(_menuT * 3.f)),
                            kBlack, 2.f);
        }
        else
        {
            const Color water(0.1f, 0.35f, 0.95f, 0.85f);
            for (int r = 0; r < 4; ++r)
                for (int k = 0; k < 8; ++k)
                {
                    float wave = sel ? sinf(_menuT * 5.f + k * 0.8f) * 4.f : 0.f;
                    _draw->drawDot(Vec2(c.x - 70 + k * 20.f, c.y - 30 + r * 16.f + wave), 7.f, water);
                }
        }
        drawDigit(m + 1, Vec2(px0 - 8, 215), 8.f);
    }

    // Start-Level (Hoch/Runter): Zahl zwischen zwei Pfeilen + Punkte fuer alle Levels
    drawNumber(_menuLevel, Vec2(cx - (_menuLevel >= 10 ? 16.f : 8.f), 100), 8.f);
    _draw->drawSolidTriangle(Vec2(cx - 8, 140), Vec2(cx + 8, 140), Vec2(cx, 154), kBlack, kBlack, 1.f);
    _draw->drawSolidTriangle(Vec2(cx - 8, 90), Vec2(cx + 8, 90), Vec2(cx, 76), kBlack, kBlack, 1.f);
    for (int i = 1; i <= kLevels; ++i)
    {
        Vec2 d(cx - 7 * kLevels + i * 14 - 7, 55);
        if (i == _menuLevel) _draw->drawDot(d, 4.f, kBlue);
        else _draw->drawCircle(d, 3.f, kBlack, 1.f);
    }
}

// Metaballs nur mit DrawNode: Skalarfeld aus Partikel-Kernen auf einem Raster, Iso-Kontur per Marching Squares,
// Innenflaeche als Dreiecksfaecher, Kontur als Linien.
void BoxScene::drawMetaballs()
{
    constexpr float cell = 6.f;                 // Rasterweite in Pixeln
    constexpr float R    = 22.f;                // Kernradius
    constexpr float T    = 0.6f;                // Iso-Wert
    constexpr int N      = (int)(kWin / cell) + 2;
    static std::vector<float> f;
    f.assign(N * N, 0.f);

    for (b2BodyId id : _parts)
    {
        Vec2 p = px(toV(b2Body_GetPosition(id)));
        int x0 = std::max(0, (int)std::floor((p.x - R) / cell)), x1 = std::min(N - 1, (int)std::ceil((p.x + R) / cell));
        int y0 = std::max(0, (int)std::floor((p.y - R) / cell)), y1 = std::min(N - 1, (int)std::ceil((p.y + R) / cell));
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
            {
                float dx = x * cell - p.x, dy = y * cell - p.y;
                float q  = 1.f - (dx * dx + dy * dy) / (R * R);
                if (q > 0.f) f[y * N + x] += q * q;
            }
    }

    const Color fill(0.15f, 0.4f, 1.f, 1.f), edge(0.05f, 0.2f, 0.7f, 1.f);
    for (int y = 0; y < N - 1; ++y)
        for (int x = 0; x < N - 1; ++x)
        {
            // Ecken gegen den Uhrzeigersinn: unten-links, unten-rechts, oben-rechts, oben-links
            const int cx[4] = {x, x + 1, x + 1, x}, cy[4] = {y, y, y + 1, y + 1};
            float v[4]; Vec2 c[4];
            int inside = 0;
            for (int i = 0; i < 4; ++i)
            {
                v[i] = f[cy[i] * N + cx[i]];
                c[i] = Vec2(cx[i] * cell, cy[i] * cell);
                inside += v[i] >= T;
            }
            if (inside == 0) continue;

            Vec2 poly[8]; int n = 0;
            Vec2 cross[4]; int nc = 0;
            for (int i = 0; i < 4; ++i)
            {
                int j = (i + 1) & 3;
                if (v[i] >= T) poly[n++] = c[i];
                if ((v[i] >= T) != (v[j] >= T))
                {
                    float t = (T - v[i]) / (v[j] - v[i]);
                    Vec2 pt = c[i] + (c[j] - c[i]) * t;
                    poly[n++] = pt;
                    cross[nc++] = pt;
                }
            }
            for (int i = 1; i + 1 < n; ++i)
                _draw->drawSolidTriangle(poly[0], poly[i], poly[i + 1], fill, fill, 0.5f);
            for (int i = 0; i + 1 < nc; i += 2)
                _draw->drawLine(cross[i], cross[i + 1], edge, 1.5f, DrawNode::EndStyle::Butt, DrawNode::EndStyle::Butt);
        }
}

// Sicherheitsnetz: Ball/Partikel duerfen den Kasten nie verlassen. Liegt ein Koerper ausserhalb des Rahmens
// (z.B. durch extreme Drehung), wird er in Container-Koordinaten zurueck in den Kasten gesetzt.
void BoxScene::enforceBounds()
{
    auto fix = [&](b2BodyId id, float r) {
        b2Vec2 l = toV(b2Body_GetLocalPoint(_boxBody, b2Body_GetPosition(id)));
        const float lim = kInner - r;
        if (std::abs(l.x) <= lim && std::abs(l.y) <= lim) return;
        b2Vec2 cl{std::max(-lim, std::min(lim, l.x)), std::max(-lim, std::min(lim, l.y))};
        b2Vec2 w = toV(b2Body_GetWorldPoint(_boxBody, cl));
        b2Body_SetTransform(id, {w.x, w.y}, b2Body_GetRotation(id));
        b2Body_SetLinearVelocity(id, b2Body_GetWorldPointVelocity(_boxBody, {w.x, w.y}));
    };
    if (B2_IS_NON_NULL(_ball)) fix(_ball, kBallR);
    for (b2BodyId id : _parts) fix(id, kPartR);
}

void BoxScene::drawNumber(int n, Vec2 pos, float size)
{
    std::string s = std::to_string(n);
    for (char ch : s) { drawDigit(ch - '0', pos, size); pos.x += size + 8; }
}
