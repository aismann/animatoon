#include "BoxScene.h"
USING_NS_AX;

namespace
{
constexpr float kWin       = 640.f;
constexpr float kStep      = 1.f / 60.f / 3.f;   // 3 Substeps pro Frame wie im Original
constexpr float kBallR     = 20.f;
constexpr float kGoalR     = 10.f;
constexpr float kWinDist   = 23.f;
const Color kBlack(0, 0, 0, 1), kBlue(0, 0, 1, 1), kRed(1, 0, 0, 1);

// PureBasic hat y nach unten, Axmol nach oben -> y spiegeln (lokale Container-Koordinaten).
cpVect V(float x, float y) { return cpv(x, -y); }

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
    if (!_space) return;
    std::vector<cpShape*> shapes;
    std::vector<cpBody*> bodies;
    cpSpaceEachShape(_space, [](cpShape* s, void* v) { ((std::vector<cpShape*>*)v)->push_back(s); }, &shapes);
    cpSpaceEachBody(_space, [](cpBody* b, void* v) { ((std::vector<cpBody*>*)v)->push_back(b); }, &bodies);
    for (auto* s : shapes) { cpSpaceRemoveShape(_space, s); cpShapeFree(s); }
    for (auto* b : bodies) { cpSpaceRemoveBody(_space, b); cpBodyFree(b); }
    cpSpaceFree(_space);
}

bool BoxScene::init()
{
    if (!Scene::init()) return false;

    auto* bg = LayerColor::create(Color32(200, 200, 200, 255));
    addChild(bg);
    _draw = DrawNode::create();
    addChild(_draw);

    buildWorld();

    auto kb = EventListenerKeyboard::create();
    kb->onKeyPressed = [this](EventKeyboard::KeyCode k, Event*) {
        if (k == EventKeyboard::KeyCode::KEY_LEFT_ARROW) _left = true;
        if (k == EventKeyboard::KeyCode::KEY_RIGHT_ARROW) _right = true;
        if (k == EventKeyboard::KeyCode::KEY_ESCAPE) Director::getInstance()->end();
    };
    kb->onKeyReleased = [this](EventKeyboard::KeyCode k, Event*) {
        if (k == EventKeyboard::KeyCode::KEY_LEFT_ARROW) _left = false;
        if (k == EventKeyboard::KeyCode::KEY_RIGHT_ARROW) _right = false;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(kb, this);

    scheduleUpdate();
    return true;
}

void BoxScene::buildWorld()
{
    _space = cpSpaceNew();
    cpSpaceSetGravity(_space, cpv(0, -2600));
    cpSpaceSetIterations(_space, 50);

    // Container: kinematischer Body, dreht sich um die Fenstermitte
    _boxBody = cpBodyNewKinematic();
    cpBodySetPosition(_boxBody, cpv(320, 320));
    cpSpaceAddBody(_space, _boxBody);

    auto seg = [&](cpVect a, cpVect b) {
        cpShape* s = cpSegmentShapeNew(_boxBody, a, b, 0.f);
        cpShapeSetElasticity(s, 1.f);
        cpShapeSetFriction(s, 1.f);
        cpSpaceAddShape(_space, s);
    };

    // Aussenrahmen
    cpVect A = V(-200, -200), B = V(-200, 200), C = V(200, 200), D = V(200, -200);
    seg(A, B); seg(B, C); seg(C, D); seg(D, A);

    // Innere Waende (Labyrinth)
    for (int n = 0; n <= 3; ++n)
    {
        seg(V(-200, -100 + n * 100), V(100, -100 + n * 100));
        seg(V(-100, -150 + n * 100), V(200, -150 + n * 100));
    }

    // Ziel: kleiner Kreis, nur Sensor (im Original reine Zielmarke)
    _goalLocal = V(175, 175);
    _goal      = cpCircleShapeNew(_boxBody, kGoalR, _goalLocal);
    cpShapeSetSensor(_goal, cpTrue);
    cpSpaceAddShape(_space, _goal);

    // Ball (Original: Startposition 500/150 im Fenster, y nach unten)
    cpFloat mass = 1.0;
    _ball        = cpBodyNew(mass, cpMomentForCircle(mass, 0, kBallR, cpvzero));
    cpBodySetPosition(_ball, cpv(500, kWin - 150));
    cpSpaceAddBody(_space, _ball);
    cpShape* bs = cpCircleShapeNew(_ball, kBallR, cpvzero);
    cpShapeSetElasticity(bs, 0.f);
    cpShapeSetFriction(bs, 1.f);
    cpSpaceAddShape(_space, bs);
}

void BoxScene::update(float dt)
{
    if (_won) { draw(); return; }

    _elapsed += dt;
    if (_left)  _w += 0.2f;
    if (_right) _w -= 0.2f;   // y-Achse gespiegelt: rechts = im Uhrzeigersinn = negativ
    cpBodySetAngularVelocity(_boxBody, _w);

    for (int i = 0; i < 3; ++i) cpSpaceStep(_space, kStep);

    cpVect g = cpBodyLocalToWorld(_boxBody, _goalLocal);
    cpVect b = cpBodyGetPosition(_ball);
    if (std::abs(b.x - g.x) < kWinDist && std::abs(b.y - g.y) < kWinDist)
        _won = true;

    draw();
}

void BoxScene::draw()
{
    _draw->clear();

    // Segmente + Ziel
    cpSpaceEachShape(_space, [](cpShape* s, void* ud) {
        auto* self = (BoxScene*)ud;
        cpBody* body = cpShapeGetBody(s);
        if (body != self->_boxBody) return;
        if (s == self->_goal) {
            cpVect p = cpBodyLocalToWorld(body, self->_goalLocal);
            self->_draw->drawCircle(Vec2(p.x, p.y), kGoalR, kBlack, 1.f);
        } else {
            cpVect a = cpBodyLocalToWorld(body, cpSegmentShapeGetA(s));
            cpVect b = cpBodyLocalToWorld(body, cpSegmentShapeGetB(s));
            self->_draw->drawLine(Vec2(a.x, a.y), Vec2(b.x, b.y), kBlack, 1.f);
        }
    }, this);

    // Ball mit Winkelstrich
    cpVect p = cpBodyGetPosition(_ball);
    float a  = (float)cpBodyGetAngle(_ball);
    _draw->drawCircle(Vec2(p.x, p.y), kBallR, kBlack, 1.f);
    _draw->drawLine(Vec2(p.x, p.y), Vec2(p.x + cosf(a) * kBallR, p.y + sinf(a) * kBallR), kBlack, 1.f);

    // Kontaktpunkte (rot)
    cpBodyEachArbiter(_ball, [](cpBody*, cpArbiter* arb, void* ud) {
        auto* self = (BoxScene*)ud;
        cpContactPointSet set = cpArbiterGetContactPointSet(arb);
        for (int i = 0; i < set.count; ++i)
            self->_draw->drawDot(Vec2(set.points[i].pointA.x, set.points[i].pointA.y), 2.f, kRed);
    }, this);

    // Zeit als 7-Segment (statt Label, da nur DrawNode erlaubt)
    drawTime((long)(_elapsed * 1000.0), Vec2(10, kWin - 40));
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
