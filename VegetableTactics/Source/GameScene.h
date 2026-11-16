// Vegetable Tactics ("Veg Soup") - rebuilt with Axmol 3.0.
// Every visual is drawn with ax::DrawNode only: no sprites, textures or fonts.
#pragma once

#include "axmol/axmol.h"
#include <string_view>
#include <vector>

class GameScene : public ax::Scene
{
public:
    ax::CameraMode getDefaultCameraMode() const override { return ax::CameraMode::Ortho; }

    bool init() override;
    void update(float dt) override;

    bool onPointerDown(ax::PointerEvent* ev);
    void onPointerMove(ax::PointerEvent* ev);
    void onPointerUp(ax::PointerEvent* ev);
    void onKeyPressed(ax::KeyboardEvent* ev);

private:
    enum class State { Ready, Go, Playing, GameOver };

    enum VegType { Beetroot, Carrot, EggPlant, Onion, Pea, RedPepper, Sweetcorn, VegCount };

    struct Veg
    {
        ax::Vec2 pos;
        ax::Vec2 vel;
        float angle = 0.f;
        VegType type = Carrot;
        bool selected = false;
        float still = 0.f;  // seconds spent resting above the rim
    };

    struct Particle
    {
        ax::Vec2 pos, vel;
        float life = 0.f, maxLife = 0.f, size = 0.f;
        ax::Color color{1, 1, 1, 1};
    };

    struct Floater  // rising "+score" text
    {
        ax::Vec2 pos;
        float life = 0.f;
        int value = 0;
    };

    // ---- setup / drawing ----
    void buildStatic();
    void drawText(ax::DrawNode* n, std::string_view s, float x, float y, float scale, const ax::Color& c,
                  const ax::Color& shadow, bool centered = false);
    float textWidth(std::string_view s, float scale) const;
    void drawVeg(ax::DrawNode* n, const Veg& v, bool glow);
    void render();

    // ---- game logic ----
    void restart();
    void queueVeg(int count);
    void spawnOne();
    void stepPhysics(float dt);
    void addVegPressed();
    void shakePressed();
    std::vector<int> groupAt(int index) const;
    int vegAt(const ax::Vec2& p) const;
    void removeGroup(const std::vector<int>& idx);
    void potOut();
    void levelUp();
    void burst(const ax::Vec2& p, const ax::Color& c, int n);
    void play(const char* file, float vol = 1.f);
    int typesThisLevel() const;
    bool addEnabled() const;
    bool shakeEnabled() const;

    // ---- nodes ----
    ax::Node* _root       = nullptr;
    ax::DrawNode* _back   = nullptr;  // static background, chef, pot interior
    ax::DrawNode* _dyn    = nullptr;  // vegetables, hud numbers, effects
    ax::DrawNode* _front  = nullptr;  // static pot walls drawn over the vegetables

    // ---- state ----
    State _state = State::Ready;
    float _stateTime = 0.f;
    std::vector<Veg> _veg;
    std::vector<Particle> _parts;
    std::vector<Floater> _floats;
    int _pendingSpawn = 0;
    float _spawnTimer = 0.f;
    float _addCooldown = 0.f;
    float _shakeCooldown = 0.f;
    float _shakeFx = 0.f;
    float _autoTimer = 0.f;
    float _banner = 0.f;
    int _bannerKind = 0;  // 1 pot out, 2 level up, 3 show off
    int _score = 0, _level = 1, _chance = 3, _cleared = 0;
    int _lastMult = 3;
    float _multTimer = 0.f;
    int _selected = -1;
    ax::Vec2 _pointer;
    float _time = 0.f;
    unsigned _rng = 12345u;
    float rnd();
    float rnd(float a, float b) { return a + (b - a) * rnd(); }

    ax::PointerEventListener* _pointerListener   = nullptr;
    ax::EventListenerKeyboard* _keyboardListener = nullptr;
};
