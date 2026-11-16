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
    enum class Menu { Main, Options };

    // input
    void onKeyPressed(ax::KeyboardEvent* ev);
    bool onPointerDown(ax::PointerEvent* ev);
    void confirm();
    void menuActivate();
    void menuMove(int d);
    void menuAdjust(int d);
    void toggleFullscreen();
    void applyAndSave();

    // flat (default) view
    ax::Vec2 tilePos(float x, float z) const;
    void rebuildMaze();
    void renderFlat();
    // 3D chase view (key C)
    void updateCamera(float dt);
    void renderChase();
    void drawSky();
    void drawFloor();
    void drawEntities();
    void drawEffects();
    void drawPickup3D(const Pickup& p);

    void renderHud();
    void renderOverlay();
    void renderTitle();
    void drawHeart(ax::DrawNode* dn, ax::Vec2 pos, float px, const ax::Color& c);

    Game _game;
    Gfx3D _gfx;
    ax::DrawNode* _bg      = nullptr;  // cached flat maze
    ax::DrawNode* _world   = nullptr;  // per-frame scene
    ax::Node* _logo        = nullptr;  // title logo: one cached DrawNode per letter, animated on a sine wave
    std::vector<ax::DrawNode*> _logoLetters;
    std::vector<float> _logoX;          // letter centres (for the pivot)
    ax::DrawNode* _hud     = nullptr;
    ax::DrawNode* _overlay = nullptr;

    bool _flat       = true;
    int _mazeId      = -1;
    bool _bgFlat     = true;
    Menu _menu       = Menu::Main;
    int _sel         = 0;
    float _yawVis    = 0.f;
    float _time      = 0.f;
    float _acc       = 0.f;
    std::string _hudKey;
    std::vector<std::pair<float, std::function<void()>>> _drawList;
};
