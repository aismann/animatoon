#include "BoxScene.h"
using namespace ax;

namespace
{
constexpr float kWin     = 640.f;
constexpr float kPPM     = 100.f;                // Pixel pro Meter (Box2D rechnet in Metern)
constexpr float kStep    = 1.f / 60.f / 3.f;     // 3 Substeps pro Frame wie im Original
constexpr float kBallR   = 20.f / kPPM;
constexpr int   kLevels  = 4;
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

    startLevel(1);

    auto kb = KeyboardEventListener::create();
    kb->onKeyPressed = [this](KeyboardEvent* e) {
        switch (e->getKeyCode())
        {
        case KeyboardEvent::KeyCode::KEY_LEFT_ARROW:  _left = true; break;
        case KeyboardEvent::KeyCode::KEY_RIGHT_ARROW: _right = true; break;
        case KeyboardEvent::KeyCode::KEY_ESCAPE:      Director::getInstance()->end(); break;
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
    _level = level; _won = false; _w = 0.f; _winTimer = 0.f; _elapsed = 0.0;
    buildWorld(level);
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
        b2Segment s{a, b};
        b2CreateSegmentShape(_boxBody, &sd, &s);
        _segs.push_back({a, b});
    };

    // Aussenrahmen
    b2Vec2 A = V(-200, -200), B = V(-200, 200), C = V(200, 200), D = V(200, -200);
    seg(A, B); seg(B, C); seg(C, D); seg(D, A);

    // Level-Daten (Pixel, y nach unten wie im Original): Ziel + innere Waende
    float goalX = 0, goalY = 0, goalR = 16;
    auto line = [&](float x1, float y1, float x2, float y2) { seg(V(x1, y1), V(x2, y2)); };

    if (level == 1)
    {
        goalX = 175; goalY = 175;
        for (int n = 0; n <= 3; ++n)
        {
            line(-200, -100 + n * 100, 100, -100 + n * 100);
            line(-100, -150 + n * 100, 200, -150 + n * 100);
        }
    }
    else if (level == 2)
    {
        line(-150, -150, 200, -150);
        line(-150, -150, -150, 150);
        line(150, 150, -150, 150);
        line(150, 150, 150, -100);
        line(150, -100, -100, -100);
        line(-100, -100, -100, 100);
        line(-100, 100, 50, 100);
    }
    // Level 3 und 4: nur Rahmen + Ziel in der Mitte

    // Ziel: Sensor-Kreis
    _goalLocal         = V(goalX, goalY);
    _goalR             = goalR / kPPM;
    b2ShapeDef gd      = b2DefaultShapeDef();
    gd.isSensor        = true;
    b2Circle goal{_goalLocal, _goalR};
    b2CreateCircleShape(_boxBody, &gd, &goal);

    // Ball (Original: Startposition 500/150 im Fenster, y nach unten)
    b2BodyDef ballDef = b2DefaultBodyDef();
    ballDef.type      = b2_dynamicBody;
    ballDef.position  = {500.f / kPPM, (kWin - 150.f) / kPPM};
    _ball             = b2CreateBody(_world, &ballDef);
    b2ShapeDef bs     = sd;
    bs.density        = 1.f;
    b2Circle circle{{0.f, 0.f}, kBallR};
    b2CreateCircleShape(_ball, &bs, &circle);
}

void BoxScene::update(float dt)
{
    if (_won)
    {
        _winTimer += dt;
        if (_winTimer > 1.5f && !_finished)
        {
            if (_level < kLevels) startLevel(_level + 1);
            else _finished = true;
        }
        draw();
        return;
    }

    _elapsed += dt;
    if (_left)  _w += 0.2f;
    if (_right) _w -= 0.2f;   // y-Achse gespiegelt: rechts = im Uhrzeigersinn = negativ
    b2Body_SetAngularVelocity(_boxBody, _w);

    for (int i = 0; i < 3; ++i) b2World_Step(_world, kStep, 4);

    b2Vec2 g = toV(b2Body_GetWorldPoint(_boxBody, _goalLocal));
    b2Vec2 b = toV(b2Body_GetPosition(_ball));
    if (std::abs(b.x - g.x) < kWinDist && std::abs(b.y - g.y) < kWinDist)
        _won = true;

    draw();
}

void BoxScene::draw()
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

    // Zeit als 7-Segment (statt Label, da nur DrawNode erlaubt)
    drawTime((long)(_elapsed * 1000.0), Vec2(10, kWin - 40));

    // Level-Anzeige: Ziffer + Punkte fuer alle Levels (gefuellt = erreicht)
    drawDigit(_level, Vec2(kWin - 30, kWin - 40), 8.f);
    for (int i = 1; i <= kLevels; ++i)
    {
        Vec2 d(kWin - 100 + i * 14, kWin - 50);
        if (i <= _level) _draw->drawDot(d, 4.f, _finished ? kBlue : kBlack);
        else _draw->drawCircle(d, 4.f, kBlack, 1.f);
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
