#pragma once
// BoX - Port von PureBasic/Chipmunk nach Axmol 3.0. Gezeichnet wird ausschliesslich mit ax::DrawNode.
#include "axmol.h"
#include "chipmunk/chipmunk.h"

class BoxScene : public ax::Scene
{
public:
    static BoxScene* create();
    bool init() override;
    void update(float dt) override;
    ~BoxScene() override;

private:
    void buildWorld();
    void draw();
    void drawDigit(int digit, ax::Vec2 pos, float size);
    void drawTime(long ms, ax::Vec2 pos);

    ax::DrawNode* _draw = nullptr;
    cpSpace* _space     = nullptr;
    cpBody* _boxBody    = nullptr;  // rotierender Container (kinematisch)
    cpBody* _ball       = nullptr;
    cpShape* _goal      = nullptr;
    cpVect _goalLocal{};

    bool _left = false, _right = false, _won = false;
    float _w         = 0.f;   // Winkelgeschwindigkeit des Containers
    double _elapsed  = 0.0;   // Sekunden
    float _accum     = 0.f;
};
