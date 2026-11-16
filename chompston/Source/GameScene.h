#pragma once

#include "Game.h"
#include "Gfx3D.h"
#include "axmol/axmol.h"

class GameScene : public ax::Scene
{
public:
    GameScene();
    ~GameScene() override;

    bool init() override;
    void update(float delta) override;

private:
    // input
    bool onPointerDown(ax::PointerEvent* ev);
    void onKeyPressed(ax::KeyboardEvent* ev);
    void confirm();
    void toggleFullscreen();
    void applyAndSave();

    // rendering (everything is DrawNode geometry)
    void updateCamera(float dt);
    void renderWorld();
    void drawSky();
    void drawFloor();
    void drawEntities();
    void drawEffects();
    void drawPlayer();
    void drawGhost(const Ghost& g);
    void drawPickup(const Pickup& p);
    void renderHud();
    void renderOverlay();
    void drawHeart(ax::DrawNode* dn, ax::Vec2 pos, float px, const ax::Color& c);

    Game _game;
    Gfx3D _gfx;
    ax::DrawNode* _world   = nullptr;
    ax::DrawNode* _hud     = nullptr;
    ax::DrawNode* _overlay = nullptr;

    float _yawVis   = 0.f;   // smoothed heading used by camera + player body
    bool _topView   = true;  // top-down camera, corridors are the raised part
    float _baseY    = 0.f;   // height of the surface the actors stand on
    float _time     = 0.f;
    float _acc      = 0.f;
    std::string _hudKey;
    std::vector<std::pair<float, std::function<void()>>> _drawList;
};
