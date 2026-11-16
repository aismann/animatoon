#pragma once
// BoX - Port von PureBasic (Chipmunk) nach Axmol, Physik jetzt Box2D 3.x. Gezeichnet wird ausschliesslich mit ax::DrawNode.
#include "axmol.h"
#include "box2d/box2d.h"
#include <vector>

class BoxScene : public ax::Scene
{
public:
    static BoxScene* create();
    bool init() override;
    void update(float dt) override;
    ~BoxScene() override;

private:
    void buildWorld(int level);
    void startLevel(int level);
    void draw();
    void drawMenu();
    void startGame();
    void stepFluid();
    int countParticlesInGoal() const;
    void drawDigit(int digit, ax::Vec2 pos, float size);
    void drawTime(long ms, ax::Vec2 pos);

    enum class State { Menu, Play };
    enum class Mode { Ball, Fluid };

    struct Seg { b2Vec2 a, b; };   // lokale Container-Koordinaten (Meter)

    ax::DrawNode* _draw = nullptr;
    b2WorldId _world    = b2_nullWorldId;
    b2BodyId _boxBody   = b2_nullBodyId;  // rotierender Container (kinematisch)
    b2BodyId _ball      = b2_nullBodyId;
    b2Vec2 _goalLocal{};
    float _goalR = 0.1f;      // Meter
    int _level = 1;           // 1..4
    float _winTimer = 0.f;    // Pause nach dem Treffer, dann naechstes Level
    bool _finished = false;   // alle Levels geschafft
    std::vector<Seg> _segs;
    std::vector<b2BodyId> _parts;   // Fluid-Partikel

    State _state   = State::Menu;
    Mode _mode     = Mode::Ball;    // im Menue gewaehlt
    int _menuLevel = 1;             // Start-Level im Menue
    float _menuT   = 0.f;           // Animationszeit Menue
    float _finishTimer = 0.f;

    bool _left = false, _right = false, _won = false;
    float _w         = 0.f;   // Winkelgeschwindigkeit des Containers
    double _elapsed  = 0.0;   // Sekunden
};
