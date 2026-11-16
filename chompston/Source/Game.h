#pragma once
// Game rules / simulation. No rendering code in here.

#include <cstdint>
#include <random>
#include <vector>

enum class State
{
    Title,
    Ready,
    Playing,
    Dying,
    LevelClear,
    GameOver,
    Paused
};

constexpr int kGrid = 15;  // 7x7 maze cells -> 15x15 tiles (odd coordinates are cells)

inline constexpr int kDX[4] = {0, 1, 0, -1};  // heading 0=N(+z) 1=E(+x) 2=S 3=W
inline constexpr int kDZ[4] = {1, 0, -1, 0};

struct Ghost
{
    enum class Mode { Normal, Scared, Dead };
    float x = 0, z = 0;
    int tx = 0, tz = 0;
    int dx = 0, dz = 0;
    bool moving = false;
    Mode mode   = Mode::Normal;
    float timer = 0;  // Dead: seconds until respawn
    float wait  = 0;  // seconds until it starts roaming
    int homeC = 1, homeR = 1;
    int id = 0;
    float bob = 0;
};

struct Pickup
{
    enum class Kind { Pellet, Power, Heart, Bomb };
    Kind kind = Kind::Pellet;
    int c = 0, r = 0;
    bool alive = false;
};

struct Explosion
{
    float x, z, t;
};

struct Popup
{
    float x, z, t;
    int value;
};

class Game
{
public:
    // --- state exposed to the renderer ---
    State state = State::Title;
    int score = 0, lives = 3, bombs = 1, level = 1;
    int hiScore = 0;
    bool newHiScore = false;

    uint8_t wall[kGrid][kGrid];
    std::vector<Pickup> pickups;
    std::vector<Ghost> ghosts;
    std::vector<Explosion> explosions;
    std::vector<Popup> popups;

    // player
    float px = 7, pz = 7;
    int heading = 0;         // 0..3
    bool pMoving = false;
    int ptx = 7, ptz = 7;
    float chompAnim = 0;     // grows while moving, drives the mouth
    float invuln    = 0;
    float scaredT   = 0;     // remaining scared time
    float stateT    = 0;     // time spent in the current state
    float hitFlash  = 0;
    int pelletsLeft = 0, pelletsTotal = 0;

    Game();

    void startGame();        // from title
    void toTitle();
    void update(float dt);

    // input
    void turnLeft();
    void turnRight();
    void reverse();
    void dropBomb();
    void togglePause();

    bool open(int c, int r) const { return c >= 0 && r >= 0 && c < kGrid && r < kGrid && !wall[r][c]; }

private:
    std::mt19937 rng{std::random_device{}()};
    int pendingTurn = 0;
    bool chompAlt   = false;
    int ghostCombo  = 0;
    int nextBonusAt = 3000;
    bool heartSpawned = false;
    float lastTurnSfx = 0;

    int rnd(int n) { return int(rng() % (unsigned)n); }
    void buildLevel();
    void resetActors();
    void generateMaze();
    float playerSpeed() const;
    float ghostSpeed(const Ghost& g) const;

    void updatePlayer(float dt);
    void playerArrive();
    void eatAt(int c, int r);
    void updateGhosts(float dt, bool collide);
    void ghostChoose(Ghost& g);
    void killPlayer();
    void addScore(int v);
    void tickEffects(float dt);
    void setState(State s);
};
