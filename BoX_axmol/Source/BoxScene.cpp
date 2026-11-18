#include "BoxScene.h"
#include "MetaballMesh.h"
#include <sstream>
using namespace ax;

namespace
{
constexpr float kWin     = 640.f;
constexpr float kPPM     = 100.f;                // Pixel pro Meter (Box2D rechnet in Metern)
constexpr float kStep    = 1.f / 60.f / 3.f;     // 3 Substeps pro Frame wie im Original
constexpr float kBallR   = 20.f / kPPM;
constexpr int   kLevels  = 14;
constexpr float kWallR   = 2.5f / kPPM;          // Wanddicke 5 px (Kapseln statt duenner Segmente -> kein Durchtunneln)
constexpr float kFrame   = 200.f / kPPM;          // Mitte der Rahmenwand
constexpr float kInner   = kFrame - kWallR;       // Innenkante des Rahmens
constexpr float kFriction = 0.25f;               // Reibung Wand/Ball ("Klebrigkeit"); 1 = sehr klebrig, 0 = rutschig
constexpr float kTiltGain = 12.f;                // rad/s pro g Neigung (0,5 g ~ 6 rad/s)
constexpr float kTiltDead = 0.05f;               // Totzone in g
constexpr float kTiltSign = 1.f;                 // -1, falls das Geraet die Neigung verkehrt herum meldet
constexpr float kLineW   = 9.f;                  // GEZEICHNETE Liniendicke in px (nur Optik; Physik bleibt bei kWallR = 5 px)
constexpr float kMetaBaseR  = 20.f;              // Basis-Kernradius der Metaballs in px (Skalierung 1)
constexpr float kFluidDrawR = 14.f;              // GLOBAL: Kernradius des gezeichneten Schleims im Spiel in px (kleiner = kleinere Kreise)
constexpr float kMaxW    = 8.f;                 // max. Winkelgeschwindigkeit des Kastens (rad/s)
constexpr float kPartR   = 2.f / kPPM;
constexpr int   kFluidWin = 1;                  // Partikel im Ziel zum Gewinnen
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
    setAccelerometer(false);
    if (B2_IS_NON_NULL(_world)) b2DestroyWorld(_world);
}

bool BoxScene::init()
{
    if (!Scene::init()) return false;

    Director::getInstance()->setStatsDisplay(false);   // FPS-/Statistik-Anzeige aus (auch in AppDelegate moeglich)

    auto* bg = LayerColor::create(Color32(200, 200, 200, 255));
    addChild(bg);
    _draw = DrawNode::create();
    addChild(_draw);

    _levels.resize(kLevels + 1);
    for (int i = 1; i <= kLevels; ++i) _levels[i] = builtinLevel(i);
    loadLevels();   // gespeicherte Editor-Levels ueberschreiben die eingebauten

    auto kb = KeyboardEventListener::create();
    kb->onKeyPressed = [this](KeyboardEvent* e) {
        using K = KeyboardEvent::KeyCode;
        K k = e->getKeyCode();
        if (k == K::KEY_M) { _sfxOn = !_sfxOn; sfx(Sfx::Click); return; }   // Ton an/aus
        if (k == K::KEY_C && _state == State::Menu) { _control = _control == Control::Buttons ? Control::Tilt : Control::Buttons; sfx(Sfx::Select); return; }   // Steuerung
        if (k == K::KEY_G && _state != State::Editor) { _shineOn = !_shineOn; sfx(Sfx::Select); return; }   // Glanz an/aus
        if (_state == State::Menu)
        {
            switch (k)
            {
            case K::KEY_LEFT_ARROW:
            case K::KEY_RIGHT_ARROW: _mode = (_mode == Mode::Ball) ? Mode::Fluid : Mode::Ball; sfx(Sfx::Select); break;
            case K::KEY_1: _mode = Mode::Ball; sfx(Sfx::Select); break;
            case K::KEY_2: _mode = Mode::Fluid; sfx(Sfx::Select); break;
            case K::KEY_UP_ARROW:   _menuLevel = _menuLevel % kLevels + 1; sfx(Sfx::Click); break;
            case K::KEY_DOWN_ARROW: _menuLevel = (_menuLevel + kLevels - 2) % kLevels + 1; sfx(Sfx::Click); break;
            case K::KEY_E: _editorOn = !_editorOn; sfx(Sfx::Select); break;   // Editor-Schalter
            case K::KEY_ENTER:
            case K::KEY_KP_ENTER:
            case K::KEY_RETURN:
            case K::KEY_SPACE:
                if (_editorOn) startEditor();
                else startGame();
                break;
            case K::KEY_ESCAPE: Director::getInstance()->end(); break;
            default: break;
            }
            return;
        }
        if (_state == State::Editor)
        {
            editorKey((int)k);
            return;
        }
        switch (k)
        {
        case K::KEY_LEFT_ARROW:  _left = true; break;
        case K::KEY_RIGHT_ARROW: _right = true; break;
        case K::KEY_ESCAPE:      _state = _fromEditor ? State::Editor : State::Menu; break;   // zurueck
        default: break;
        }
    };
    kb->onKeyReleased = [this](KeyboardEvent* e) {
        if (e->getKeyCode() == KeyboardEvent::KeyCode::KEY_LEFT_ARROW) _left = false;
        if (e->getKeyCode() == KeyboardEvent::KeyCode::KEY_RIGHT_ARROW) _right = false;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(kb, this);

    // Maus/Touch: Editor (Waende ziehen, Ziel setzen, radieren) und Klicks im Menue
    auto ptr = PointerEventListener::create();
    ptr->onPointerDown = [this](PointerEvent* e) {
        Vec2 p = e->getWorldPoint();
        if (_state == State::Editor) { editorPointer(0, p, e->getButton()); return true; }
        if (_state == State::Play) { playTouch(0, e->getPointerId(), p); return true; }
        if (_state == State::Menu)
        {
            auto begin = [this]() { if (_editorOn) startEditor(); else startGame(); };
            const float cx = kWin / 2;
            if (p.x > 585 && p.x < 635 && p.y > 585 && p.y < 635) { _sfxOn = !_sfxOn; sfx(Sfx::Click); }   // Lautsprecher
            else if (p.x > 20 && p.x < 140 && p.y > 560 && p.y < 622) { _control = p.x < 80 ? Control::Buttons : Control::Tilt; sfx(Sfx::Select); }   // Steuerung
            else if (p.x > 30 && p.x < 130 && p.y > 85 && p.y < 160) { _shineOn = !_shineOn; sfx(Sfx::Select); }   // Glanz-Schalter
            else if (p.x > 520 && p.x < 600 && p.y > 85 && p.y < 135) { _editorOn = !_editorOn; sfx(Sfx::Select); }      // Editor-Schalter
            else if (p.y > 200 && p.y < 440 && p.x > 75 && p.x < 275) { _mode = Mode::Ball; begin(); }    // Karte: Ball -> Start
            else if (p.y > 200 && p.y < 440 && p.x > 365 && p.x < 565) { _mode = Mode::Fluid; begin(); }  // Karte: Fluid -> Start
            else if (std::abs(p.x - cx) < 40 && p.y > 125 && p.y < 165) { _menuLevel = _menuLevel % kLevels + 1; sfx(Sfx::Click); }            // Pfeil hoch
            else if (std::abs(p.x - cx) < 40 && p.y > 78 && p.y < 100) { _menuLevel = (_menuLevel + kLevels - 2) % kLevels + 1; sfx(Sfx::Click); }   // Pfeil runter
            else if (std::abs(p.y - 66.f) < 9.f)   // Punkte: Level direkt waehlen
            {
                int i = (int)std::lround((p.x - (cx - 7.f * kLevels - 7.f)) / 14.f);
                if (i >= 1 && i <= kLevels) { _menuLevel = i; sfx(Sfx::Click); }
            }
        }
        return true;
    };
    ptr->onPointerMove = [this](PointerEvent* e) { if (_state == State::Editor) editorPointer(1, e->getWorldPoint(), 0); };
    ptr->onPointerUp   = [this](PointerEvent* e) {
        if (_state == State::Editor) editorPointer(2, e->getWorldPoint(), 0);
        playTouch(2, e->getPointerId(), e->getWorldPoint());   // Touch-Tasten loslassen (auch nach Zustandswechsel)
    };
    ptr->onPointerCancel = [this](PointerEvent* e) {
        if (_state == State::Editor) editorPointer(2, e->getWorldPoint(), 0);
        playTouch(2, e->getPointerId(), e->getWorldPoint());
    };
    // Fixed-Priority-Listener bekommen in Axmol keinen Kamera-Strahl -> getWorldPoint() waere immer (0,0).
    // Darum Scene-Graph-Listener mit eigenem Hit-Test, der jeden Punkt der Szene trifft.
    ptr->onPointerHitTest = [](PointerEvent* e, Vec3* hit) {
        Vec2 w = e->getWorldPoint();   // der Strahl ist hier bereits gesetzt
        if (hit) *hit = Vec3(w.x, w.y, 0.f);
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(ptr, this);

    // Beschleunigungssensor (nur aktiv, wenn im Spiel die Neigungssteuerung gewaehlt ist)
    Device::setAccelerometerInterval(1.f / 60.f);
    auto acc = AccelerationEventListener::create([this](AccelerationEvent* e) { _tilt = (float)e->getAcceleration().x * kTiltSign; });
    _eventDispatcher->addEventListenerWithSceneGraphPriority(acc, this);

    scheduleUpdate();
    return true;
}

BoxScene::LevelData BoxScene::builtinLevel(int level)
{
    LevelData L;
    float goalX = 0, goalY = 0;
    auto line  = [&](float x1, float y1, float x2, float y2) { L.segs.push_back({x1, y1, x2, y2}); };
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

    L.gx = goalX;
    L.gy = goalY;
    return L;
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
    _tLeft = _tRight = false;
    _tLId = _tRId = -1;
    _finished = false;
    _fromEditor = false;
    _finishTimer = 0.f;
    _state = State::Play;
    sfx(Sfx::Start);
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
    sd.material.friction      = kFriction;
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
    const LevelData& L = _levels[level];
    const float goalX = L.gx, goalY = L.gy, goalR = 16;
    for (const auto& s : L.segs) seg(V(s[0], s[1]), V(s[2], s[3]));

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
        // Fluid: 3 x 5 kleine Kreise im oberen Streifen des Kastens (Container steht bei Start still)
        b2ShapeDef ps           = b2DefaultShapeDef();
        ps.material.friction    = 0.05f;
        ps.material.restitution = 0.f;
        ps.density              = 1.f;
        b2Circle pc{{0.f, 0.f}, kPartR};
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 5; ++c)
            {
                b2Vec2 l          = V(190.f - c * 14.f, -188.f + r * 14.f);
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

// Gewonnen, sobald der Ball dem Ziel nahe kommt: Abstand der Mitten < Zielradius + 2 x Ballradius (16 + 40 = 56 px). Beim Schleim reicht
// kFluidWin Partikel im Ziel. Frueher musste die Ballmitte in einem 23-px-Quadrat liegen - das Beruehren reichte nicht.
bool BoxScene::goalTouched() const
{
    if (_mode == Mode::Fluid) return countParticlesInGoal() >= kFluidWin;
    b2Vec2 g = toV(b2Body_GetWorldPoint(_boxBody, _goalLocal));
    b2Vec2 d = b2Sub(toV(b2Body_GetPosition(_ball)), g);
    const float rr = _goalR + kBallR * 2.f;
    return d.x * d.x + d.y * d.y < rr * rr;
}

void BoxScene::update(float dt)
{
    setAccelerometer(_state == State::Play && _control == Control::Tilt);
    if (_state == State::Editor)
    {
        _menuT += dt;
        drawEditor();
        return;
    }
    if (_state == State::Menu)
    {
        _menuT += dt;
        _swAnim += ((_editorOn ? 1.f : 0.f) - _swAnim) * std::min(1.f, dt * 12.f);
        _shAnim += ((_shineOn ? 1.f : 0.f) - _shAnim) * std::min(1.f, dt * 12.f);
        _won = false;
        drawMenu();
        return;
    }
    if (_won)
    {
        _menuT += dt;
        _winTimer += dt;
        if (_fromEditor && _winTimer > 1.5f)
        {
            _state = State::Editor;   // Testspiel gewonnen -> zurueck in den Editor
            return;
        }
        if (_winTimer > 1.5f && !_finished)
        {
            if (_level < kLevels) { startLevel(_level + 1); sfx(Sfx::Level); }
            else { _finished = true; sfx(Sfx::Win); }
        }
        if (_finished && (_finishTimer += dt) > 3.f) _state = State::Menu;
        drawAll();
        return;
    }

    _menuT += dt;
    _elapsed += dt;
    if (_control == Control::Tilt)   // Neigung -> Drehgeschwindigkeit (weich nachgefuehrt); rechts neigen = im Uhrzeigersinn
    {
        float t = std::abs(_tilt) < kTiltDead ? 0.f : _tilt;
        _w += (-t * kTiltGain - _w) * 0.15f;
    }
    if (_left || _tLeft)   _w += 0.2f;   // Tastatur geht immer, Touch-Tasten nur im Tasten-Modus (siehe playTouch)
    if (_right || _tRight) _w -= 0.2f;   // y-Achse gespiegelt: rechts = im Uhrzeigersinn = negativ
    _w = std::max(-kMaxW, std::min(kMaxW, _w));
    b2Body_SetAngularVelocity(_boxBody, _w);

    // Geschwindigkeiten vor dem Schritt merken: ein ploetzlicher Sprung = Aufprall (Gravitation allein macht nur ~0,4 m/s)
    b2Vec2 ballV0{0.f, 0.f};
    if (_mode == Mode::Ball) ballV0 = b2Body_GetLinearVelocity(_ball);

    if (_mode == Mode::Fluid) stepFluid();
    for (int i = 0; i < 3; ++i)
    {
        b2World_Step(_world, kStep, 4);
        enforceBounds();
        if (goalTouched()) { _won = true; break; }   // nach jedem Substep pruefen: schnelle Koerper rutschen nicht am Ziel vorbei
    }

    if (_mode == Mode::Ball)
    {
        b2Vec2 d = b2Sub(b2Body_GetLinearVelocity(_ball), ballV0);
        float dv = std::sqrt(d.x * d.x + d.y * d.y);
        if (dv > 3.f) sfx(Sfx::HitHard, std::min(1.f, dv / 8.f));
        else if (dv > 1.2f) sfx(Sfx::HitSoft, std::min(1.f, dv / 4.f));
    }

    if (_won)
    {
        sfx(Sfx::Win);
        sfx(Sfx::Yeah);   // "yeaahhh" - Ziel erreicht
    }

    drawAll();
}

void BoxScene::drawAll()
{
    _draw->clear();
    drawBackground();

    // Startpunkt (blendet in den ersten Sekunden aus): gestrichelter Ring + Pfeil nach unten
    {
        float fade = std::max(0.f, 1.f - (float)_elapsed / 2.5f);
        if (fade > 0.f)
        {
            Vec2 s(500.f, kWin - 150.f);
            Color oc(0.85f, 0.45f, 0.2f, fade);
            for (int i = 0; i < 16; i += 2)
            {
                float a0 = i * 3.14159265f / 8.f, a1 = (i + 1) * 3.14159265f / 8.f;
                _draw->drawLine(Vec2(s.x + cosf(a0) * 30.f, s.y + sinf(a0) * 30.f),
                                Vec2(s.x + cosf(a1) * 30.f, s.y + sinf(a1) * 30.f), oc, 2.f);
            }
            _draw->drawSolidTriangle(Vec2(s.x - 8.f, s.y - 38.f), Vec2(s.x + 8.f, s.y - 38.f), Vec2(s.x, s.y - 50.f), oc, oc, 1.f);
        }
    }

    // Kasten: halbtransparente Flaeche (dreht sich mit), darauf die Waende. Nur Optik - die Physik nutzt weiter dieselben
    // 5-px-Kapseln; Schatten und Glanzlinie liegen innerhalb bzw. daneben und aendern nichts an der Kollision.
    {
        Vec2 f[4];
        for (int i = 0; i < 4; ++i) f[i] = px(toV(b2Body_GetWorldPoint(_boxBody, _segs[i].a)));   // Rahmenecken A, B, C, D
        _draw->drawSolidPoly(f, 4, Color(1.f, 1.f, 1.f, 0.28f), 0.f, Color(0, 0, 0, 0));
    }
    const Color ink(0.08f, 0.14f, 0.35f, 1.f);
    for (int pass = 0; pass < 2; ++pass)   // erst alle Schatten, dann alle Waende (keine Schatten ueber Waenden)
        for (const Seg& s : _segs)
        {
            Vec2 a = px(toV(b2Body_GetWorldPoint(_boxBody, s.a)));
            Vec2 b = px(toV(b2Body_GetWorldPoint(_boxBody, s.b)));
            if (pass == 0)
            {
                _draw->drawLine(Vec2(a.x + 2.5f, a.y - 3.5f), Vec2(b.x + 2.5f, b.y - 3.5f), Color(0.f, 0.f, 0.25f, 0.20f), kLineW + 2.f);
                continue;
            }
            _draw->drawLine(a, b, ink, kLineW);
            _draw->drawLine(Vec2(a.x - 1.2f, a.y + 1.2f), Vec2(b.x - 1.2f, b.y + 1.2f), Color(0.55f, 0.70f, 1.f, 0.65f), 2.f);   // Glanzlinie
            for (const Vec2& p : {a, b})   // runde Endkappen mit Glanz
            {
                _draw->drawDot(p, kLineW * 0.5f, ink);
                _draw->drawDot(Vec2(p.x - 1.2f, p.y + 1.2f), kLineW * 0.25f, Color(0.60f, 0.75f, 1.f, 1.f));
            }
        }

    // Ziel: Zielscheibe mit Ringen, Mittelpunkt und pulsierendem Aussenring (gruen, wie im Editor)
    {
        Vec2 g   = px(toV(b2Body_GetWorldPoint(_boxBody, _goalLocal)));
        float r  = _goalR * kPPM;
        float pu = 0.5f + 0.5f * sinf(_menuT * 4.f);
        const Color gr(0.1f, 0.55f, 0.25f, 1.f);
        _draw->drawDot(g, r, Color(0.2f, 0.75f, 0.35f, 0.35f));
        _draw->drawCircle(g, r, gr, 3.f);
        _draw->drawCircle(g, r * 0.55f, gr, 2.f);
        _draw->drawDot(g, 3.5f, gr);
        _draw->drawCircle(g, r + 4.f + 5.f * pu, Color(0.1f, 0.55f, 0.25f, 0.5f * (1.f - pu)), 2.f);
    }

    if (_mode == Mode::Ball)
    {
        // Ball wie im Menue: schattiert, Licht von oben links (bleibt beim Rollen fest), dazu Rollmarke
        Vec2 p  = px(toV(b2Body_GetPosition(_ball)));
        float a = b2Rot_GetAngle(b2Body_GetRotation(_ball));
        const float r = kBallR * kPPM;
        _draw->drawDot(Vec2(p.x + 2.f, p.y - 3.f), r, Color(0.f, 0.f, 0.f, 0.12f));                  // weicher Schatten
        _draw->drawDot(p, r, Color(0.08f, 0.14f, 0.35f, 1.f));
        _draw->drawDot(Vec2(p.x - r * 0.10f, p.y + r * 0.10f), r * 0.84f, Color(0.12f, 0.30f, 0.75f, 1.f));
        Vec2 spot(p.x + cosf(a) * r * 0.62f, p.y + sinf(a) * r * 0.62f);                              // dreht sich mit
        _draw->drawDot(spot, r * 0.17f, Color(0.45f, 0.70f, 1.f, 0.8f));
        _draw->drawDot(Vec2(p.x - r * 0.26f, p.y + r * 0.29f), r * 0.42f, Color(0.45f, 0.70f, 1.f, 0.85f));
        _draw->drawDot(Vec2(p.x - r * 0.34f, p.y + r * 0.37f), r * 0.16f, Color(1.f, 1.f, 1.f, 0.9f));

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

    drawSpeaker(Vec2(kWin - 30.f, 545.f), _sfxOn);   // Ton an/aus: M oder Tippen
    drawTouchButtons();

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

namespace
{
// Schleim (grün, wie die Vorlage): Rand kraeftig und fast deckend, nach innen zunehmend durchsichtig,
// dazu heller Glanz. Verlauf per Eckenfarben (drawColoredTriangle).
const Color kWater[4] = {Color(0.33f, 0.62f, 0.03f, 0.95f), Color(0.50f, 0.82f, 0.06f, 0.80f),
                         Color(0.58f, 0.90f, 0.12f, 0.55f), Color(0.64f, 0.93f, 0.20f, 0.38f)};
const Color kSlimeSoft(0.86f, 1.00f, 0.55f, 0.55f), kSlimeCore(1.00f, 1.00f, 0.90f, 0.90f);
}

void BoxScene::fillRect(Vec2 lo, Vec2 hi, const Color& col)
{
    _draw->drawSolidTriangle(lo, Vec2(hi.x, lo.y), hi, col, col, 0.f);
    _draw->drawSolidTriangle(lo, hi, Vec2(lo.x, hi.y), col, col, 0.f);
}

void BoxScene::roundedRect(Vec2 c, Vec2 half, float r, const Color& fill, const Color& border, float thick)
{
    std::vector<Vec2> poly;
    const Vec2 ctr[4] = {Vec2(c.x + half.x - r, c.y - half.y + r), Vec2(c.x + half.x - r, c.y + half.y - r),
                         Vec2(c.x - half.x + r, c.y + half.y - r), Vec2(c.x - half.x + r, c.y - half.y + r)};
    for (int k = 0; k < 4; ++k)                      // gegen den Uhrzeigersinn, je 6 Punkte pro Ecke
        for (int i = 0; i <= 6; ++i)
        {
            float a = (-90.f + k * 90.f + i * 15.f) * 3.14159265f / 180.f;
            poly.push_back(Vec2(ctr[k].x + cosf(a) * r, ctr[k].y + sinf(a) * r));
        }
    _draw->drawSolidPoly(poly.data(), (unsigned)poly.size(), fill, thick, border);
}

// Buchstaben B, O, X als dicke Linienzuege (nur DrawNode)
void BoxScene::strokeLetter(char ch, Vec2 o, float w, float h, const Color& col, float thick)
{
    auto L = [&](float x1, float y1, float x2, float y2) {
        _draw->drawLine(Vec2(o.x + x1 * w, o.y + y1 * h), Vec2(o.x + x2 * w, o.y + y2 * h), col, thick);
    };
    if (ch == 'B')
    {
        L(0, 0, 0, 1);
        L(0, 1, .7f, 1); L(.7f, 1, 1, .88f); L(1, .88f, 1, .62f); L(1, .62f, .7f, .5f); L(.7f, .5f, 0, .5f);
        L(.7f, .5f, 1, .38f); L(1, .38f, 1, .12f); L(1, .12f, .7f, 0); L(.7f, 0, 0, 0);
    }
    else if (ch == 'X')
    {
        L(0, 0, 1, 1);
        L(0, 1, 1, 0);
    }
}

void BoxScene::drawBackground()
{
    // Hintergrund: vertikaler Verlauf aus Baendern (derselbe wie im Menue)
    for (int i = 0; i < 16; ++i)
    {
        float k = i / 15.f;
        fillRect(Vec2(0, i * kWin / 16), Vec2(kWin, (i + 1) * kWin / 16 + 1),
                 Color(0.80f + 0.14f * (1 - k), 0.88f + 0.09f * (1 - k), 1.f - 0.02f * k, 1.f));
    }
}

void BoxScene::drawMenu()
{
    _draw->clear();
    const float cx = kWin / 2, t = _menuT;

    drawBackground();   // blauer Verlauf

    // Weiche Hintergrund-Blobs (Metaballs, blasse Palette), driften langsam
    {
        std::vector<MBPoint> bg;
        for (int i = 0; i < 6; ++i)
            bg.push_back({Vec2(cx + sinf(t * 0.3f + i * 1.7f) * 260.f, 320 + cosf(t * 0.23f + i * 2.3f) * 280.f),
                          2.2f + 0.5f * sinf(t * 0.5f + i)});
        MBPalette pal{{Color(0.70f, 0.82f, 0.98f, 1.f), Color(0.74f, 0.85f, 0.99f, 1.f), Color(0.79f, 0.89f, 1.f, 1.f),
                       Color(0.84f, 0.92f, 1.f, 1.f)},
                      {0.30f, 0.50f, 0.85f, 1.40f}, Color(), Color(), false, 8.f, 3.f, {0.f, 6.f, 14.f, 24.f}};
        renderMetaballs(bg, pal);
    }

    // Titel "BOX": B und X als Linienbuchstaben, das O ist der drehende Kasten mit Ball und Ziel
    const Color ink(0.08f, 0.14f, 0.35f, 1.f);
    const float lw = 56.f, lh = 84.f;
    for (int i = 0; i < 3; ++i)
    {
        float bob = sinf(t * 2.f + i * 0.9f) * 5.f;
        float x   = cx + (i - 1) * 110.f - lw / 2;
        float y   = 505.f + bob;
        if (i == 1)
        {
            float a = t * 0.9f;
            auto rot = [&](float px_, float py_) {
                return Vec2(cx + px_ * cosf(a) - py_ * sinf(a), y + lh / 2 + px_ * sinf(a) + py_ * cosf(a));
            };
            const float s = 42.f;
            Vec2 q[4] = {rot(-s, -s), rot(-s, s), rot(s, s), rot(s, -s)};
            for (int k = 0; k < 4; ++k) _draw->drawLine(q[k], q[(k + 1) & 3], ink, 8.f);
            _draw->drawLine(rot(-s, 0), rot(s * 0.3f, 0), ink, 5.f);   // Innenwand
            Vec2 ballP = rot(s * 0.1f + sinf(t * 2.f) * 4.f, -s * 0.55f);
            _draw->drawDot(ballP, 9.f, Color(0.15f, 0.45f, 1.f, 1.f));
            _draw->drawDot(Vec2(ballP.x - 2.5f, ballP.y + 3.f), 3.f, Color(1.f, 1.f, 1.f, 0.9f));
            _draw->drawCircle(rot(s * 0.55f, s * 0.55f), 9.f, ink, 3.f);
        }
        else
        {
            strokeLetter(i == 0 ? 'B' : 'X', Vec2(x, y), lw, lh, Color(0.f, 0.f, 0.f, 0.12f), 14.f);   // Schatten
            strokeLetter(i == 0 ? 'B' : 'X', Vec2(x, y + 3.f), lw, lh, ink, 10.f);
        }
    }

    // Karten: links ein Ball, rechts Fluid
    for (int m = 0; m < 2; ++m)
    {
        bool sel    = (int)_mode == m;
        float pulse = sel ? 1.f + 0.03f * sinf(t * 4.f) : 1.f;
        Vec2 c(m == 0 ? 175.f : 465.f, 320.f + (sel ? 6.f : 0.f));
        Vec2 half(100.f * pulse, 120.f * pulse);
        roundedRect(Vec2(c.x + 4, c.y - 6), half, 22.f, Color(0.f, 0.f, 0.2f, 0.12f), Color(0, 0, 0, 0), 0.f);   // Schatten
        roundedRect(c, half, 22.f, sel ? Color(1.f, 1.f, 1.f, 1.f) : Color(0.90f, 0.93f, 0.98f, 1.f),
                    sel ? Color(0.15f, 0.45f, 1.f, 1.f) : Color(0.62f, 0.70f, 0.85f, 1.f), sel ? 4.f : 2.f);
        if (m == 0)
        {
            float b = sel ? fabsf(sinf(t * 3.5f)) * 22.f : 0.f;                       // Ball huepft, wenn gewaehlt
            Vec2 p(c.x, c.y + 10.f + b);
            _draw->drawDot(Vec2(c.x, c.y - 34.f), 28.f - b * 0.35f, Color(0.f, 0.f, 0.f, 0.10f));   // Schatten am Boden
            _draw->drawDot(p, 38.f, Color(0.08f, 0.14f, 0.35f, 1.f));
            _draw->drawDot(Vec2(p.x - 4.f, p.y + 4.f), 32.f, Color(0.12f, 0.30f, 0.75f, 1.f));
            _draw->drawDot(Vec2(p.x - 10.f, p.y + 11.f), 16.f, Color(0.45f, 0.70f, 1.f, 1.f));
            _draw->drawDot(Vec2(p.x - 13.f, p.y + 14.f), 6.f, Color(1.f, 1.f, 1.f, 0.9f));
        }
        drawDigit(m + 1, Vec2(c.x - 8.f, c.y - 98.f), 8.f);
    }

    // Fluid-Icon: wabbelnder Metaball-Tropfen (gleiche Technik wie im Spiel)
    {
        const bool sel = _mode == Mode::Fluid;
        Vec2 c(465.f, 326.f + (sel ? 6.f : 0.f));
        std::vector<MBPoint> pts;
        for (int r = 0; r < 3; ++r)
            for (int k = 0; k < 5; ++k)
            {
                float w = sel ? 1.f : 0.25f;
                pts.push_back({Vec2(c.x - 48.f + k * 24.f + sinf(t * 2.5f + k * 1.1f + r) * 5.f * w,
                                    c.y - 24.f + r * 22.f + sinf(t * 3.1f + k * 0.9f + r * 2.f) * 6.f * w),
                               1.f});
            }
        // ein Tropfen loest sich und faellt
        float ph = fmodf(t * 0.7f, 1.f);
        if (sel) pts.push_back({Vec2(c.x + 10.f, c.y + 46.f - ph * ph * 90.f), 0.55f + 0.2f * (1.f - ph)});
        MBPalette pal{{kWater[0], kWater[1], kWater[2], kWater[3]}, {0.28f, 0.45f, 0.80f, 1.35f},
                      kSlimeSoft, kSlimeCore, _shineOn, 4.f, 1.5f, {0.f, 3.f, 7.f, 12.f}};
        renderMetaballs(pts, pal);
    }

    // Level-Auswahl (Hoch/Runter): Karte mit Zahl, Pfeilen und Fortschrittspunkten
    roundedRect(Vec2(cx, 105.f), Vec2(190.f, 62.f), 18.f, Color(1.f, 1.f, 1.f, 1.f), Color(0.62f, 0.70f, 0.85f, 1.f), 2.f);
    drawNumber(_menuLevel, Vec2(cx - (_menuLevel >= 10 ? 16.f : 8.f), 100.f), 8.f);
    float bob = sinf(t * 4.f) * 2.f;
    _draw->drawSolidTriangle(Vec2(cx - 9, 138 + bob), Vec2(cx + 9, 138 + bob), Vec2(cx, 152 + bob), ink, ink, 1.f);
    _draw->drawSolidTriangle(Vec2(cx - 9, 92 - bob), Vec2(cx + 9, 92 - bob), Vec2(cx, 78 - bob), ink, ink, 1.f);
    for (int i = 1; i <= kLevels; ++i)
    {
        Vec2 d(cx - 7 * kLevels + i * 14 - 7, 66);
        if (i == _menuLevel) _draw->drawDot(d, 4.f, Color(0.15f, 0.45f, 1.f, 1.f));
        else _draw->drawCircle(d, 3.f, ink, 1.f);
    }

    drawSpeaker(Vec2(610.f, 610.f), _sfxOn);
    drawControlSwitch(Vec2(80.f, 591.f));

    // Glanz-Schalter (G oder Klick): an = Schleim-Gruen, aus = grau. Steuert den Glanz auf dem Schleim.
    {
        const Color on(0.50f, 0.80f, 0.10f, 1.f), off(0.62f, 0.68f, 0.80f, 1.f);
        Vec2 c(80.f, 105.f);
        Color sc = _shineOn ? on : off;
        Vec2 s(c.x, c.y + 40.f);                                   // Funkel-Symbol ueber dem Schalter
        float r = 12.f + (_shineOn ? 2.f * sinf(t * 5.f) : 0.f);
        _draw->drawLine(Vec2(s.x, s.y - r), Vec2(s.x, s.y + r), sc, 4.f);
        _draw->drawLine(Vec2(s.x - r, s.y), Vec2(s.x + r, s.y), sc, 4.f);
        _draw->drawLine(Vec2(s.x - r * 0.5f, s.y - r * 0.5f), Vec2(s.x + r * 0.5f, s.y + r * 0.5f), sc, 2.f);
        _draw->drawLine(Vec2(s.x - r * 0.5f, s.y + r * 0.5f), Vec2(s.x + r * 0.5f, s.y - r * 0.5f), sc, 2.f);
        roundedRect(c, Vec2(32.f, 16.f), 16.f,
                    Color(off.r + (on.r - off.r) * _shAnim, off.g + (on.g - off.g) * _shAnim,
                          off.b + (on.b - off.b) * _shAnim, 1.f),
                    ink, 2.f);
        float kx = c.x - 16.f + 32.f * _shAnim;
        _draw->drawDot(Vec2(kx, c.y), 12.f, Color(1.f, 1.f, 1.f, 1.f));
        _draw->drawCircle(Vec2(kx, c.y), 12.f, ink, 1.5f);
    }

    // Editor-Schalter (E oder Klick): aus = grau, an = orange. Enter startet dann den Editor statt des Spiels.
    {
        const Color orange(1.f, 0.6f, 0.1f, 1.f), off(0.62f, 0.68f, 0.80f, 1.f);
        Vec2 c(560.f, 105.f);
        // Stift-Symbol ueber dem Schalter
        Color pc = _editorOn ? orange : off;
        _draw->drawLine(Vec2(c.x - 12, c.y + 28), Vec2(c.x + 10, c.y + 50), pc, 6.f);
        _draw->drawSolidTriangle(Vec2(c.x - 12, c.y + 28), Vec2(c.x - 18, c.y + 22), Vec2(c.x - 6, c.y + 24), ink, ink, 1.f);
        roundedRect(c, Vec2(32.f, 16.f), 16.f,
                    Color(off.r + (orange.r - off.r) * _swAnim, off.g + (orange.g - off.g) * _swAnim,
                          off.b + (orange.b - off.b) * _swAnim, 1.f),
                    ink, 2.f);
        float kx = c.x - 16.f + 32.f * _swAnim;
        _draw->drawDot(Vec2(kx, c.y), 12.f, Color(1.f, 1.f, 1.f, 1.f));
        _draw->drawCircle(Vec2(kx, c.y), 12.f, ink, 1.5f);
    }
}

// Metaballs nur mit DrawNode: Skalarfelder aus Kernen auf einem Raster, Iso-Konturen per Marching Squares.
// Koerper: 4 Schichten fuer Tiefe. Glanz (optional): eigenes, kleineres, nach oben-links versetztes Metaball-Feld.
void BoxScene::renderMetaballs(const std::vector<MBPoint>& pts, const MBPalette& pal)
{
    const float cell = pal.cell;
    const int N      = (int)(kWin / cell) + 2;
    static std::vector<float> body, shine;
    body.assign((size_t)N * N, 0.f);
    if (pal.shine) shine.assign((size_t)N * N, 0.f);

    int minX = N, minY = N, maxX = 0, maxY = 0;
    auto splat = [&](std::vector<float>& f, Vec2 p, float R) {
        int x0 = std::max(0, (int)std::floor((p.x - R) / cell)), x1 = std::min(N - 1, (int)std::ceil((p.x + R) / cell));
        int y0 = std::max(0, (int)std::floor((p.y - R) / cell)), y1 = std::min(N - 1, (int)std::ceil((p.y + R) / cell));
        minX = std::min(minX, x0); maxX = std::max(maxX, x1);
        minY = std::min(minY, y0); maxY = std::max(maxY, y1);
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
            {
                float dx = x * cell - p.x, dy = y * cell - p.y;
                float q  = 1.f - (dx * dx + dy * dy) / (R * R);
                if (q > 0.f) f[(size_t)y * N + x] += q * q;
            }
    };

    for (const MBPoint& m : pts)
    {
        splat(body, m.p, kMetaBaseR * m.s);
        if (pal.shine) splat(shine, Vec2(m.p.x - 3.f * m.s, m.p.y + 4.f * m.s), 9.f * m.s);   // Licht oben links
    }
    if (minX > maxX) return;

    // Clipper2: Zellen -> Union -> Closing -> Ringe (Offset nach innen) -> Baender (Difference) -> Delaunay-Dreiecke.
    // Jede Dreiecksecke kennt ihren Ring und bekommt dessen Farbe: drawColoredTriangle zeichnet den Verlauf
    // Rand -> Kern, die Dreiecke ueberlappen nicht (halbtransparent ohne Doppelblending).
    auto shaded = [&](const std::vector<float>& f, float T, const std::vector<double>& insets, const Color* cols) {
        static std::vector<mb::ColTri> tris;
        if (!mb::buildShaded(f, N, cell, minX, minY, maxX, maxY, T, pal.smooth, 0, insets, tris)) return;
        for (const mb::ColTri& t : tris)
        {
            const Vec2 v[3] = {Vec2((float)t.p[0].x, (float)t.p[0].y), Vec2((float)t.p[1].x, (float)t.p[1].y),
                               Vec2((float)t.p[2].x, (float)t.p[2].y)};
            const Color c3[3] = {cols[t.ring[0]], cols[t.ring[1]], cols[t.ring[2]]};
            _draw->drawColoredTriangle(v, c3);
        }
    };

    shaded(body, pal.t[0], {pal.inset[0], pal.inset[1], pal.inset[2], pal.inset[3]}, pal.c);
    if (pal.shine)
    {
        const Color sc[2] = {pal.soft, pal.core};   // weicher Schein -> heller Kern
        shaded(shine, 0.40f, {0.0, 3.0}, sc);
    }
}

void BoxScene::drawMetaballs()
{
    const float s = kFluidDrawR / kMetaBaseR;   // Groesse relativ zum Basis-Kernradius
    std::vector<MBPoint> pts;
    pts.reserve(_parts.size());
    for (b2BodyId id : _parts)
        pts.push_back({px(toV(b2Body_GetPosition(id))), s});
    // Ringabstaende (Rand -> Kern) mitskalieren, sonst verschwinden die inneren Ringe bei kleinen Kreisen
    MBPalette pal{{kWater[0], kWater[1], kWater[2], kWater[3]}, {0.28f, 0.45f, 0.80f, 1.35f},
                  kSlimeSoft, kSlimeCore, _shineOn, 4.f, 1.5f, {0.f, 3.f * s, 7.f * s, 12.f * s}};
    renderMetaballs(pts, pal);
}

// Sicherheitsnetz: Ball/Partikel duerfen den Kasten nie verlassen. Liegt ein Koerper ausserhalb des Rahmens
// (z.B. durch extreme Drehung), wird er in Container-Koordinaten zurueck in den Kasten gesetzt.
void BoxScene::enforceBounds()
{
    auto fix = [&](b2BodyId id, float r) {
        b2Vec2 l = toV(b2Body_GetLocalPoint(_boxBody, b2Body_GetPosition(id)));
        // Nur eingreifen, wenn der Koerper wirklich ueber die Rahmenmitte hinaus ist (= ausgebrochen). Frueher wurde schon bei
        // jeder kleinen Eindringtiefe am Rand zurueckgesetzt und die Geschwindigkeit auf die des Kastens gesetzt - das hat
        // Ball und Schleim am Rand "kleben" lassen, an den inneren Waenden aber nicht.
        const float lim = kInner - r;
        if (std::abs(l.x) <= kFrame && std::abs(l.y) <= kFrame) return;
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

// ------------------------------------------------------------------------------------------------
// Level-Editor (nur DrawNode). Koordinaten: lokal zum Kasten (Pixel, y nach unten wie im Original),
// Raster 10 px. Werkzeuge: 1 Wand ziehen, 2 Ziel setzen, 3 Radierer (auch: rechte Maustaste = loeschen).
// Z = letzte Wand zurueck, Entf/Backspace = alles loeschen, R = Original-Level, T / Enter = testen,
// S = speichern, Esc = speichern + zurueck ins Menue.
// ------------------------------------------------------------------------------------------------
namespace
{
std::string levelsPath()
{
    return FileUtils::getInstance()->getWritablePath() + "box_levels.txt";
}

float distToSeg(float px_, float py_, const std::array<float, 4>& s)
{
    float dx = s[2] - s[0], dy = s[3] - s[1];
    float l2 = dx * dx + dy * dy;
    float t  = l2 > 0.f ? std::max(0.f, std::min(1.f, ((px_ - s[0]) * dx + (py_ - s[1]) * dy) / l2)) : 0.f;
    float qx = s[0] + t * dx - px_, qy = s[1] + t * dy - py_;
    return std::sqrt(qx * qx + qy * qy);
}
}  // namespace

Vec2 BoxScene::toLocal(Vec2 s) const
{
    auto snap = [](float v) { return std::max(-200.f, std::min(200.f, std::round(v / 10.f) * 10.f)); };
    return Vec2(snap(s.x - 320.f), snap(320.f - s.y));   // Bildschirm y nach oben -> Level y nach unten
}

void BoxScene::loadLevels()
{
    auto* fu = FileUtils::getInstance();
    std::string path = levelsPath();
    if (!fu->isFileExist(path)) return;
    std::istringstream in(fu->getStringFromFile(path));
    std::string tag;
    while (in >> tag)
    {
        int n = 0, count = 0;
        LevelData L;
        if (tag != "L" || !(in >> n >> L.gx >> L.gy >> count) || n < 1 || n >= (int)_levels.size() || count < 0 || count > 500)
            return;   // beschaedigte Datei: eingebaute Levels behalten
        for (int i = 0; i < count; ++i)
        {
            std::array<float, 4> s{};
            if (!(in >> s[0] >> s[1] >> s[2] >> s[3])) return;
            L.segs.push_back(s);
        }
        _levels[n] = std::move(L);
    }
}

void BoxScene::saveLevels() const
{
    std::ostringstream out;
    for (int i = 1; i < (int)_levels.size(); ++i)
    {
        const LevelData& L = _levels[i];
        out << "L " << i << ' ' << L.gx << ' ' << L.gy << ' ' << L.segs.size() << '\n';
        for (const auto& s : L.segs) out << s[0] << ' ' << s[1] << ' ' << s[2] << ' ' << s[3] << '\n';
    }
    FileUtils::getInstance()->writeStringToFile(out.str(), levelsPath());
}

void BoxScene::startEditor()
{
    _state      = State::Editor;
    sfx(Sfx::Start);
    _fromEditor = false;
    _level      = _menuLevel;
    _tool       = 0;
    _drag       = false;
    _left = _right = false;
}

void BoxScene::editorPointer(int phase, Vec2 world, int button)
{
    LevelData& L = _levels[_level];
    Vec2 p       = toLocal(world);
    _cursor      = p;

    auto eraseAt = [&](Vec2 at) {
        int best = -1;
        float bd = 14.f;
        for (int i = 0; i < (int)L.segs.size(); ++i)
        {
            float d = distToSeg(at.x, at.y, L.segs[i]);
            if (d < bd) { bd = d; best = i; }
        }
        if (best >= 0) { L.segs.erase(L.segs.begin() + best); saveLevels(); sfx(Sfx::Erase); }
    };

    if (phase == 0)
    {
        if (editorButton(world)) return;   // Touch-Leisten (Werkzeuge oben, Aktionen unten)
        if (button == InputButton::Right) { eraseAt(Vec2(world.x - 320.f, 320.f - world.y)); return; }
        if (_tool == 0) { _drag = true; _dragA = _dragB = p; }
        else if (_tool == 1) { L.gx = p.x; L.gy = p.y; saveLevels(); sfx(Sfx::Goal); }
        else eraseAt(Vec2(world.x - 320.f, 320.f - world.y));
    }
    else if (phase == 1)
    {
        if (_drag) _dragB = p;
    }
    else if (_drag)
    {
        _drag = false;
        if (std::abs(_dragB.x - _dragA.x) + std::abs(_dragB.y - _dragA.y) >= 10.f)
        {
            L.segs.push_back({_dragA.x, _dragA.y, _dragB.x, _dragB.y});
            saveLevels();
            sfx(Sfx::Pop);
        }
    }
}

void BoxScene::editorKey(int key)
{
    using K = KeyboardEvent::KeyCode;
    LevelData& L = _levels[_level];
    switch ((K)key)
    {
    case K::KEY_1: _tool = 0; break;
    case K::KEY_2: _tool = 1; break;
    case K::KEY_3: _tool = 2; break;
    case K::KEY_Z:
        if (!L.segs.empty()) { L.segs.pop_back(); saveLevels(); sfx(Sfx::Erase); }
        break;
    case K::KEY_DELETE:
    case K::KEY_BACKSPACE: L.segs.clear(); saveLevels(); sfx(Sfx::Erase); break;
    case K::KEY_R: L = builtinLevel(_level); saveLevels(); break;
    case K::KEY_S: saveLevels(); break;
    case K::KEY_T:
    case K::KEY_ENTER:
    case K::KEY_KP_ENTER:
    case K::KEY_RETURN:
        saveLevels();
        _fromEditor = true;
        _finished   = false;
        _state      = State::Play;
        startLevel(_level);
        break;
    case K::KEY_UP_ARROW:   _level = _level % kLevels + 1; break;
    case K::KEY_DOWN_ARROW: _level = (_level + kLevels - 2) % kLevels + 1; break;
    case K::KEY_ESCAPE:
        saveLevels();
        _menuLevel = _level;
        _state     = State::Menu;
        break;
    default: break;
    }
}

void BoxScene::drawEditor()
{
    _draw->clear();
    const Color ink(0.08f, 0.14f, 0.35f, 1.f), accent(0.15f, 0.45f, 1.f, 1.f), orange(1.f, 0.6f, 0.1f, 1.f);
    auto S = [](float lx, float ly) { return Vec2(320.f + lx, 320.f - ly); };   // Level -> Bildschirm

    fillRect(Vec2(0, 0), Vec2(kWin, kWin), Color(0.90f, 0.93f, 0.98f, 1.f));
    fillRect(S(-200, 200), S(200, -200), Color(1.f, 1.f, 1.f, 1.f));

    // Startbereich von Ball/Fluid: obere Leiste (Waende hier vermeiden)
    fillRect(S(-200, -150), S(200, -200), Color(1.f, 0.9f, 0.85f, 1.f));
    {   // Startpunkt: kleiner Ball mit Glanz, gestrichelter Ring und Pfeil nach unten
        Vec2 s = S(180, -170);
        for (int i = 0; i < 16; i += 2)
        {
            float a0 = i * 3.14159265f / 8.f, a1 = (i + 1) * 3.14159265f / 8.f;
            _draw->drawLine(Vec2(s.x + cosf(a0) * 24.f, s.y + sinf(a0) * 24.f),
                            Vec2(s.x + cosf(a1) * 24.f, s.y + sinf(a1) * 24.f), Color(0.85f, 0.45f, 0.2f, 1.f), 2.f);
        }
        _draw->drawDot(s, 16.f, Color(0.08f, 0.14f, 0.35f, 1.f));
        _draw->drawDot(Vec2(s.x - 2.f, s.y + 2.f), 13.f, Color(0.12f, 0.30f, 0.75f, 1.f));
        _draw->drawDot(Vec2(s.x - 5.f, s.y + 5.f), 6.f, Color(0.45f, 0.70f, 1.f, 1.f));
        _draw->drawDot(Vec2(s.x - 6.f, s.y + 6.f), 2.5f, Color(1.f, 1.f, 1.f, 0.9f));
        _draw->drawSolidTriangle(Vec2(s.x - 7.f, s.y - 28.f), Vec2(s.x + 7.f, s.y - 28.f), Vec2(s.x, s.y - 38.f),
                                 Color(0.85f, 0.45f, 0.2f, 1.f), Color(0.85f, 0.45f, 0.2f, 1.f), 1.f);
    }

    for (int g = -200; g <= 200; g += 20)   // Raster
    {
        Color gc(0.80f, 0.85f, 0.93f, 1.f);
        _draw->drawLine(S((float)g, -200), S((float)g, 200), gc, 1.f);
        _draw->drawLine(S(-200, (float)g), S(200, (float)g), gc, 1.f);
    }

    const LevelData& L = _levels[_level];
    const Vec2 q[4]    = {S(-200, -200), S(-200, 200), S(200, 200), S(200, -200)};
    for (int i = 0; i < 4; ++i) _draw->drawLine(q[i], q[(i + 1) & 3], ink, 5.f);
    for (const auto& s : L.segs)
    {
        _draw->drawLine(S(s[0], s[1]), S(s[2], s[3]), ink, 5.f);
        for (int e = 0; e < 2; ++e)   // Endpunkte: runde Kappe mit hellem Kern
        {
            Vec2 p = S(s[e * 2], s[e * 2 + 1]);
            _draw->drawDot(p, 5.f, ink);
            _draw->drawDot(p, 2.2f, Color(0.75f, 0.85f, 1.f, 1.f));
        }
    }

    {   // Ziel: pulsierende Zielscheibe (Ringe) mit Mittelpunkt
        Vec2 g   = S(L.gx, L.gy);
        float pu = 0.5f + 0.5f * sinf(_menuT * 4.f);
        const Color gr(0.1f, 0.55f, 0.25f, 1.f);
        _draw->drawDot(g, 16.f, Color(0.2f, 0.75f, 0.35f, 0.35f));
        _draw->drawCircle(g, 16.f, gr, 3.f);
        _draw->drawCircle(g, 9.f, gr, 2.f);
        _draw->drawDot(g, 3.5f, gr);
        _draw->drawCircle(g, 20.f + 5.f * pu, Color(0.1f, 0.55f, 0.25f, 0.5f * (1.f - pu)), 2.f);
    }

    if (_drag)   // Vorschau der gezogenen Wand: Start gruen, Ende blau (pulsierend), Laenge im Raster
    {
        float pu = 0.5f + 0.5f * sinf(_menuT * 8.f);
        _draw->drawLine(S(_dragA.x, _dragA.y), S(_dragB.x, _dragB.y), Color(0.15f, 0.45f, 1.f, 0.55f), 5.f);
        Vec2 a = S(_dragA.x, _dragA.y), b = S(_dragB.x, _dragB.y);
        _draw->drawDot(a, 7.f, Color(0.2f, 0.75f, 0.35f, 1.f));
        _draw->drawCircle(a, 10.f, Color(1.f, 1.f, 1.f, 1.f), 2.f);
        _draw->drawDot(b, 7.f, accent);
        _draw->drawCircle(b, 10.f + 3.f * pu, Color(0.15f, 0.45f, 1.f, 1.f - 0.6f * pu), 2.f);
    }
    else
    {   // Fadenkreuz am Raster-Cursor
        Vec2 p = S(_cursor.x, _cursor.y);
        Color cc = _tool == 2 ? Color(0.9f, 0.2f, 0.2f, 1.f) : accent;
        _draw->drawCircle(p, 7.f, cc, 2.f);
        _draw->drawLine(Vec2(p.x - 12.f, p.y), Vec2(p.x - 4.f, p.y), cc, 2.f);
        _draw->drawLine(Vec2(p.x + 4.f, p.y), Vec2(p.x + 12.f, p.y), cc, 2.f);
        _draw->drawLine(Vec2(p.x, p.y - 12.f), Vec2(p.x, p.y - 4.f), cc, 2.f);
        _draw->drawLine(Vec2(p.x, p.y + 4.f), Vec2(p.x, p.y + 12.f), cc, 2.f);
    }

    // Werkzeugleiste: Wand / Ziel / Radierer (Ziffer darunter), aktives Werkzeug blau
    for (int i = 0; i < 3; ++i)
    {
        Vec2 c(60.f + i * 70.f, 590.f);
        roundedRect(c, Vec2(28.f, 28.f), 8.f, i == _tool ? Color(1.f, 1.f, 1.f, 1.f) : Color(0.85f, 0.89f, 0.96f, 1.f),
                    i == _tool ? accent : Color(0.62f, 0.70f, 0.85f, 1.f), i == _tool ? 3.f : 1.f);
        if (i == 0) _draw->drawLine(Vec2(c.x - 14, c.y - 10), Vec2(c.x + 14, c.y + 10), ink, 5.f);
        else if (i == 1) _draw->drawCircle(c, 12.f, Color(0.1f, 0.5f, 0.2f, 1.f), 3.f);
        else
        {
            _draw->drawLine(Vec2(c.x - 11, c.y - 11), Vec2(c.x + 11, c.y + 11), Color(0.9f, 0.2f, 0.2f, 1.f), 4.f);
            _draw->drawLine(Vec2(c.x - 11, c.y + 11), Vec2(c.x + 11, c.y - 11), Color(0.9f, 0.2f, 0.2f, 1.f), 4.f);
        }
        drawDigit(i + 1, Vec2(c.x - 4.f, c.y - 56.f), 4.f);
    }

    // Level-Nummer (Hoch/Runter) + Editor-Kennzeichen (orange)
    roundedRect(Vec2(kWin - 90.f, 590.f), Vec2(70.f, 28.f), 10.f, Color(1.f, 1.f, 1.f, 1.f), orange, 3.f);
    drawNumber(_level, Vec2(kWin - 90.f - (_level >= 10 ? 12.f : 4.f), 580.f), 6.f);
    _draw->drawLine(Vec2(kWin - 150.f, 580.f), Vec2(kWin - 130.f, 600.f), orange, 4.f);   // Stift

    // Aktionsleiste unten (auch per Touch bedienbar): Zurueck | Rueckgaengig | Alles loeschen | Level - Zahl + | Testen
    auto btn = [&](float x, bool hot) {
        roundedRect(Vec2(x, 45.f), Vec2(30.f, 26.f), 9.f, hot ? Color(0.85f, 1.f, 0.88f, 1.f) : Color(1.f, 1.f, 1.f, 1.f),
                    hot ? Color(0.1f, 0.55f, 0.25f, 1.f) : Color(0.62f, 0.70f, 0.85f, 1.f), 2.f);
    };
    btn(50.f, false);    // zurueck ins Menue
    _draw->drawLine(Vec2(62, 45), Vec2(38, 45), ink, 4.f);
    _draw->drawLine(Vec2(38, 45), Vec2(48, 55), ink, 4.f);
    _draw->drawLine(Vec2(38, 45), Vec2(48, 35), ink, 4.f);
    btn(115.f, false);   // letzte Wand zurueck
    for (int i = 0; i < 9; ++i)
    {
        float a0 = (-30.f + i * 25.f) * 3.14159265f / 180.f, a1 = (-5.f + i * 25.f) * 3.14159265f / 180.f;
        _draw->drawLine(Vec2(115.f + cosf(a0) * 12.f, 45.f + sinf(a0) * 12.f), Vec2(115.f + cosf(a1) * 12.f, 45.f + sinf(a1) * 12.f), ink, 3.f);
    }
    _draw->drawSolidTriangle(Vec2(103, 58), Vec2(103, 44), Vec2(115, 52), ink, ink, 1.f);
    btn(180.f, false);   // alle Waende loeschen
    _draw->drawLine(Vec2(170, 35), Vec2(190, 55), Color(0.9f, 0.2f, 0.2f, 1.f), 4.f);
    _draw->drawLine(Vec2(170, 55), Vec2(190, 35), Color(0.9f, 0.2f, 0.2f, 1.f), 4.f);
    btn(250.f, false);   // Level zurueck
    _draw->drawSolidTriangle(Vec2(240, 52), Vec2(260, 52), Vec2(250, 38), ink, ink, 1.f);
    drawNumber(_level, Vec2(295.f - (_level >= 10 ? 12.f : 4.f), 36.f), 6.f);
    btn(340.f, false);   // Level vor
    _draw->drawSolidTriangle(Vec2(330, 38), Vec2(350, 38), Vec2(340, 52), ink, ink, 1.f);
    btn(590.f, true);    // Testen
    _draw->drawSolidTriangle(Vec2(582, 33), Vec2(582, 57), Vec2(602, 45), Color(0.1f, 0.55f, 0.25f, 1.f), Color(0.1f, 0.55f, 0.25f, 1.f), 1.f);
}

// Klickbare Editor-Leisten (Touch): oben die Werkzeuge, unten die Aktionen. Gibt true zurueck, wenn der Tipp eine Leiste traf
// (dann wird keine Wand gezogen).
bool BoxScene::editorButton(Vec2 w)
{
    using K = KeyboardEvent::KeyCode;
    if (w.y > 555.f)   // Werkzeug-Leiste
    {
        for (int i = 0; i < 3; ++i)
            if (std::abs(w.x - (60.f + i * 70.f)) < 32.f) { _tool = i; sfx(Sfx::Click); }
        return true;
    }
    if (w.y < 90.f)    // Aktionsleiste
    {
        struct B { float x; K key; };
        const B bs[] = {{50.f, K::KEY_ESCAPE}, {115.f, K::KEY_Z}, {180.f, K::KEY_DELETE},
                        {250.f, K::KEY_DOWN_ARROW}, {340.f, K::KEY_UP_ARROW}, {590.f, K::KEY_T}};
        for (const B& b : bs)
            if (std::abs(w.x - b.x) < 34.f && std::abs(w.y - 45.f) < 32.f)
            {
                editorKey((int)b.key);
                if (b.key != K::KEY_Z && b.key != K::KEY_DELETE) sfx(Sfx::Click);
                break;
            }
        return true;
    }
    return false;
}

// Touch-Tasten im Spiel: links/rechts drehen (gedrueckt halten, Multitouch moeglich), Mitte unten = zurueck,
// Lautsprecher = Ton. Dieselbe Logik wie die Pfeiltasten.
void BoxScene::playTouch(int phase, intptr_t id, Vec2 p)
{
    if (phase == 2)
    {
        if (id == _tLId) { _tLeft = false;  _tLId = -1; }
        if (id == _tRId) { _tRight = false; _tRId = -1; }
        return;
    }
    auto _near = [&](Vec2 c, float r) { return (p - c).length() < r; };
    if (_control == Control::Buttons && _near(Vec2(70.f, 64.f), 58.f)) { _tLeft = true;  _tLId = id; }
    else if (_control == Control::Buttons && _near(Vec2(570.f, 64.f), 58.f)) { _tRight = true; _tRId = id; }
    else if (_near(Vec2(320.f, 46.f), 36.f)) { _state = _fromEditor ? State::Editor : State::Menu; sfx(Sfx::Click); }
    else if (_near(Vec2(kWin - 30.f, 545.f), 28.f)) { _sfxOn = !_sfxOn; sfx(Sfx::Click); }
}

void BoxScene::drawTouchButtons()
{
    const Color ink(0.08f, 0.14f, 0.35f, 1.f);
    auto arrow = [&](Vec2 c, bool ccw, bool pressed) {
        _draw->drawDot(c, 50.f, Color(0.f, 0.f, 0.25f, 0.10f));   // Schatten
        _draw->drawDot(c, 48.f, pressed ? Color(1.f, 1.f, 1.f, 0.95f) : Color(1.f, 1.f, 1.f, 0.55f));
        _draw->drawCircle(c, 48.f, pressed ? Color(0.15f, 0.45f, 1.f, 1.f) : ink, pressed ? 4.f : 2.f);
        // Kreisbogen-Pfeil (links = gegen den Uhrzeigersinn, rechts = im Uhrzeigersinn)
        const float R = 22.f, d2r = 3.14159265f / 180.f;
        const float s = ccw ? 1.f : -1.f;
        float a0 = (ccw ? -60.f : 240.f), step = 20.f * s;
        for (int i = 0; i < 12; ++i)
        {
            float x0 = (a0 + i * step) * d2r, x1 = (a0 + (i + 1) * step) * d2r;
            _draw->drawLine(Vec2(c.x + cosf(x0) * R, c.y + sinf(x0) * R), Vec2(c.x + cosf(x1) * R, c.y + sinf(x1) * R), ink, 4.f);
        }
        float ae = (a0 + 12 * step) * d2r;                       // Pfeilspitze am Bogenende, Richtung = Tangente
        Vec2 tip(c.x + cosf(ae) * R, c.y + sinf(ae) * R), dir(-sinf(ae) * s, cosf(ae) * s), nrm(-dir.y, dir.x);
        _draw->drawSolidTriangle(tip + dir * 10.f, tip - dir * 4.f + nrm * 9.f, tip - dir * 4.f - nrm * 9.f, ink, ink, 1.f);
    };
    if (_control == Control::Buttons)
    {
        arrow(Vec2(70.f, 64.f), true, _left || _tLeft);
        arrow(Vec2(570.f, 64.f), false, _right || _tRight);
    }
    else
    {   // Neigungs-Anzeige: Wasserwaage unten, die Kugel zeigt die Neigung
        const float L = 170.f, y = 92.f;
        _draw->drawLine(Vec2(320.f - L, y), Vec2(320.f + L, y), Color(1.f, 1.f, 1.f, 0.6f), 10.f);
        _draw->drawLine(Vec2(320.f - L, y), Vec2(320.f + L, y), ink, 2.f);
        _draw->drawLine(Vec2(320.f, y - 9.f), Vec2(320.f, y + 9.f), ink, 2.f);
        float t = std::max(-1.f, std::min(1.f, _tilt / 0.6f));
        _draw->drawDot(Vec2(320.f + t * L, y), 9.f, Color(0.15f, 0.45f, 1.f, 1.f));
        _draw->drawCircle(Vec2(320.f + t * L, y), 9.f, ink, 2.f);
    }

    // Zurueck (Mitte unten): Pfeil nach links
    Vec2 b(320.f, 46.f);
    _draw->drawDot(b, 30.f, Color(1.f, 1.f, 1.f, 0.55f));
    _draw->drawCircle(b, 30.f, ink, 2.f);
    _draw->drawLine(Vec2(b.x + 12.f, b.y), Vec2(b.x - 12.f, b.y), ink, 4.f);
    _draw->drawLine(Vec2(b.x - 12.f, b.y), Vec2(b.x - 2.f, b.y + 10.f), ink, 4.f);
    _draw->drawLine(Vec2(b.x - 12.f, b.y), Vec2(b.x - 2.f, b.y - 10.f), ink, 4.f);
}

// ------------------------------------------------------------------------------------------------
// Soundeffekte: Content/sfx/*.wav (mit tools/make_sfx.py erzeugt). M oder Klick auf den Lautsprecher = Ton an/aus.
// ------------------------------------------------------------------------------------------------
void BoxScene::sfx(Sfx id, float volume)
{
    if (!_sfxOn) return;
    static const char* const names[(int)Sfx::Count] = {"click", "select", "start", "hit_soft", "hit_hard",
                                                       "win",   "level",  "pop",   "erase",    "goal", "yeah"};
    static const float minGap[(int)Sfx::Count]      = {0.03f, 0.03f, 0.10f, 0.08f, 0.08f,
                                                       0.50f, 0.30f, 0.03f, 0.03f, 0.10f, 0.80f};
    const int i = (int)id;
    if (_menuT - _lastSfx[i] < minGap[i]) return;   // _menuT laeuft in allen Zustaenden (Menue, Spiel, Editor)
    _lastSfx[i] = _menuT;
    AudioEngine::play2d(std::string("sfx/") + names[i] + ".wav", false, std::max(0.f, std::min(1.f, volume)));
}

void BoxScene::drawSpeaker(Vec2 c, bool on)
{
    const Color ink(0.08f, 0.14f, 0.35f, 1.f), red(0.9f, 0.2f, 0.2f, 1.f);
    fillRect(Vec2(c.x - 9.f, c.y - 4.f), Vec2(c.x - 3.f, c.y + 4.f), ink);                    // Gehaeuse
    _draw->drawSolidTriangle(Vec2(c.x - 3.f, c.y - 4.f), Vec2(c.x + 5.f, c.y - 11.f), Vec2(c.x + 5.f, c.y + 11.f), ink, ink, 1.f);
    _draw->drawSolidTriangle(Vec2(c.x - 3.f, c.y - 4.f), Vec2(c.x + 5.f, c.y + 11.f), Vec2(c.x - 3.f, c.y + 4.f), ink, ink, 1.f);
    if (on)
        for (int k = 1; k <= 2; ++k)   // Schallwellen
        {
            float r = 6.f + k * 5.f;
            for (int i = -3; i < 3; ++i)
            {
                float a0 = i * 0.22f, a1 = (i + 1) * 0.22f;
                _draw->drawLine(Vec2(c.x + 4.f + cosf(a0) * r, c.y + sinf(a0) * r),
                                Vec2(c.x + 4.f + cosf(a1) * r, c.y + sinf(a1) * r), ink, 2.f);
            }
        }
    else
    {
        _draw->drawLine(Vec2(c.x + 9.f, c.y - 7.f), Vec2(c.x + 19.f, c.y + 7.f), red, 3.f);   // durchgestrichen
        _draw->drawLine(Vec2(c.x + 9.f, c.y + 7.f), Vec2(c.x + 19.f, c.y - 7.f), red, 3.f);
    }
}

void BoxScene::setAccelerometer(bool on)
{
    if (on == _accOn) return;
    _accOn = on;
    Device::setAccelerometerEnabled(on);
    if (!on) _tilt = 0.f;
}

// Umschalter fuer die Handy-Steuerung (C oder Tippen): links Bildschirm-Tasten, rechts Neigung (Beschleunigungssensor)
void BoxScene::drawControlSwitch(Vec2 c)
{
    const Color accent(0.15f, 0.45f, 1.f, 1.f), edge(0.62f, 0.70f, 0.85f, 1.f);
    roundedRect(c, Vec2(60.f, 30.f), 14.f, Color(1.f, 1.f, 1.f, 0.85f), edge, 2.f);
    for (int i = 0; i < 2; ++i)
    {
        const bool sel = (i == 0) == (_control == Control::Buttons);
        Vec2 m(c.x + (i == 0 ? -30.f : 30.f), c.y);
        if (sel) roundedRect(m, Vec2(26.f, 25.f), 11.f, Color(0.88f, 0.94f, 1.f, 1.f), accent, 3.f);
        const Color ic = sel ? accent : edge;
        if (i == 0)   // Tasten-Symbol: zwei Knoepfe mit Pfeilen
        {
            _draw->drawCircle(Vec2(m.x - 10.f, m.y), 8.f, ic, 2.5f);
            _draw->drawCircle(Vec2(m.x + 10.f, m.y), 8.f, ic, 2.5f);
            _draw->drawLine(Vec2(m.x - 13.f, m.y), Vec2(m.x - 7.f, m.y), ic, 2.f);
            _draw->drawLine(Vec2(m.x + 7.f, m.y), Vec2(m.x + 13.f, m.y), ic, 2.f);
        }
        else          // Neigungs-Symbol: gekipptes Handy + Bogen
        {
            float a = 0.35f + (sel ? 0.25f * sinf(_menuT * 4.f) : 0.f);
            auto R = [&](float x, float y) { return Vec2(m.x + x * cosf(a) - y * sinf(a), m.y + x * sinf(a) + y * cosf(a)); };
            Vec2 q[4] = {R(-6, -11), R(6, -11), R(6, 11), R(-6, 11)};
            for (int k = 0; k < 4; ++k) _draw->drawLine(q[k], q[(k + 1) & 3], ic, 2.5f);
            _draw->drawDot(R(0, -8), 1.5f, ic);
        }
    }
}
