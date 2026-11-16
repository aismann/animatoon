/****************************************************************************
 Snowball - Axmol 3.0 port of the PureBasic game by Majikeyric
 (XMas 2014 PurePunch contest, original C64 graphics by Ashley Routledge).
 ****************************************************************************/
#pragma once

#include "axmol/axmol.h"
#include <array>
#include <random>
#include <unordered_set>
#include <vector>

class GameScene : public ax::Scene
{
public:
    bool init() override;
    void update(float dt) override;

private:
    // ---- constants of the original ---------------------------------------------------------
    static constexpr int SCREEN_W      = 640;
    static constexpr int SCREEN_H      = 480;
    static constexpr int NS            = 13;    // frames per player and direction
    static constexpr int NUM_FLAKES    = 200;
    static constexpr int ACTOR_Y       = 380;   // vertical position of the players
    static constexpr int TEMPO         = 90;    // ticks a player retreats after shooting
    static constexpr int SHOOT_DIST    = 194;   // distance the bots keep to their target
    static constexpr int WIN_SCORE     = 10;
    static constexpr float GRAVITY     = 0.2f;
    static constexpr float FIXED_STEP  = 1.0f / 60.0f;
    static constexpr int SNOWBALL_SPRITE = NS + 4 * NS;  // index of the snowball picture

    enum Phase { ChooseTarget, PickUpSnow, Approach, Shoot, Retreat };
    enum State { Idle, Walk, PickUp, Throw, FallForward, FallBack };
    enum Dir { Right, Left };

    struct Actor
    {
        int number       = 0;
        int direction    = Right;
        int x = 0, y     = 0;
        int state        = Idle;
        int phase        = ChooseTarget;
        Actor* target    = nullptr;
        int tempo        = 0;
        int snowball     = 0;   // 1 = holds a snowball
        int animCounter  = 1;
        int animIndex    = 1;
        int sprite       = 0;   // frame + NS * (number * 2 + direction), 0 = none yet
        ax::Sprite* node = nullptr;
    };

    struct Ball
    {
        int shooter;
        float x, y, vx, vy;
        ax::Sprite* node;
    };

    struct Flake
    {
        int x;
        float y, v;
        ax::Sprite* node;
    };

    // ---- setup -------------------------------------------------------------------------------
    void buildTextures();
    void buildScene();
    void resetGame();

    // ---- game logic (one call = one frame of the original main loop) -------------------------
    void step();
    void updateActor(Actor& a);
    void updateTiles(int ox);
    bool animate(Actor& a, int anim);
    void changeState(Actor& a, int state);
    bool pixelCollision(int spriteA, int ax_, int ay, int spriteB, int bx, int by) const;
    int rnd(int lo, int hi);

    void onKeyPressed(ax::KeyboardEvent* ev);
    void onKeyReleased(ax::KeyboardEvent* ev);
    bool keyDown(ax::KeyboardEvent::KeyCode k) const;

    // ---- graphics ------------------------------------------------------------------------------
    std::array<ax::SpriteFrame*, NS * 6 + 1> _frames{};          // [frame + NS * variant], 1-based
    std::array<std::vector<uint8_t>, NS * 6 + 1> _masks;          // 48x42 alpha masks for pixel collision
    std::array<ax::SpriteFrame*, 256> _tileFrames{};
    ax::SpriteFrame* _pixelFrame = nullptr;                       // 2x2 white

    std::vector<ax::Sprite*> _tileNodes;                          // 41 x 25 grid
    std::vector<int> _tileIds;
    std::array<ax::Sprite*, 3> _hudIcons{};
    std::array<ax::Label*, 3> _hudScores{};
    ax::Label* _endLabel = nullptr;

    // ---- game state --------------------------------------------------------------------------------
    std::array<Actor, 3> _actors;
    std::array<int, 3> _score{};
    std::vector<Ball> _balls;
    std::vector<Flake> _flakes;
    int _ox            = 0;       // horizontal camera offset
    int _result        = 0;       // 0 = running, 1 = win, 2 = lose
    float _accumulator = 0.0f;
    std::mt19937 _rng{std::random_device{}()};
    std::unordered_set<int> _keys;   // currently pressed keys
};
