#include "Game.h"

#include "Config.h"
#include "Sound.h"

#include <algorithm>
#include <cmath>

namespace
{
float dist2(float ax, float az, float bx, float bz) { return (ax - bx) * (ax - bx) + (az - bz) * (az - bz); }
}  // namespace

Game::Game()
{
    hiScore = Settings::get().hiScore;
    buildLevel();
    state = State::Title;
}

// ---------------------------------------------------------------- level setup

void Game::generateMaze()
{
    for (auto& row : wall)
        for (auto& w : row)
            w = 1;
    ++mazeId;

    // nodes on the lattice, edges between neighbours; boundary edges are always corridors so the
    // outer ring exists, interior edges are picked at random (spanning + a few loops)
    struct Edge { int x0, y0, x1, y1; bool ring; };
    std::vector<Edge> edges;
    for (int j = 0; j < kNodesY; ++j)
        for (int i = 0; i < kNodesX; ++i)
        {
            if (i + 1 < kNodesX) edges.push_back({i, j, i + 1, j, j == 0 || j == kNodesY - 1});
            if (j + 1 < kNodesY) edges.push_back({i, j, i, j + 1, i == 0 || i == kNodesX - 1});
        }
    std::vector<int> parent(kNodesX * kNodesY);
    for (size_t k = 0; k < parent.size(); ++k)
        parent[k] = (int)k;
    auto find = [&](int a) {
        while (parent[a] != a)
            a = parent[a] = parent[parent[a]];
        return a;
    };
    auto carve = [&](const Edge& e) {
        for (int t = 0; t <= kSpacing; ++t)
            wall[e.y0 * kSpacing + (e.y1 - e.y0) * t][e.x0 * kSpacing + (e.x1 - e.x0) * t] = 0;
    };
    std::vector<Edge> interior;
    for (auto& e : edges)
    {
        if (e.ring)
        {
            carve(e);
            parent[find(e.y0 * kNodesX + e.x0)] = find(e.y1 * kNodesX + e.x1);
        }
        else
            interior.push_back(e);
    }
    std::shuffle(interior.begin(), interior.end(), rng);
    for (auto& e : interior)
    {
        int a = find(e.y0 * kNodesX + e.x0), b = find(e.y1 * kNodesX + e.x1);
        if (a != b || rnd(100) < 38)
        {
            carve(e);
            parent[a] = b;
        }
    }
    // interior nodes might still be isolated if the ring never reached them: connect every component
    for (auto& e : interior)
    {
        int a = find(e.y0 * kNodesX + e.x0), b = find(e.y1 * kNodesX + e.x1);
        if (a != b)
        {
            carve(e);
            parent[a] = b;
        }
    }
}

void Game::buildLevel()
{
    generateMaze();
    pickups.clear();
    explosions.clear();
    popups.clear();
    heartSpawned = false;
    pelletsTotal = 0;

    const int sc = 2 * kSpacing, sr = kSpacing;  // start node (12,6)
    int powerC[4] = {0, kCols - 1, 0, kCols - 1}, powerR[4] = {kSpacing, kSpacing, 2 * kSpacing, 2 * kSpacing};
    for (int r = 0; r < kRows; ++r)
        for (int c = 0; c < kCols; ++c)
        {
            if (wall[r][c] || (c == sc && r == sr))
                continue;
            Pickup p;
            p.c = c;
            p.r = r;
            p.alive = true;
            p.kind  = Pickup::Kind::Pellet;
            for (int k = 0; k < 4; ++k)
                if (c == powerC[k] && r == powerR[k])
                    p.kind = Pickup::Kind::Power;
            pickups.push_back(p);
            ++pelletsTotal;
        }
    pelletsLeft = pelletsTotal;

    // one bomb somewhere far from the start
    for (int tries = 0; tries < 200; ++tries)
    {
        int c = rnd(kCols), r = rnd(kRows);
        if (wall[r][c] || dist2((float)c, (float)r, (float)sc, (float)sr) < 64.f)
            continue;
        for (auto& p : pickups)
            if (p.c == c && p.r == r && p.kind == Pickup::Kind::Pellet)
            {
                p.kind = Pickup::Kind::Bomb;
                --pelletsLeft;  // bonus items don't have to be collected
                --pelletsTotal;
                tries = 999;
                break;
            }
    }
    resetActors();
}

void Game::resetActors()
{
    px = ptx = 2 * kSpacing;
    pz = ptz = kSpacing;
    heading  = 0;
    pMoving  = false;
    pendingDir = -1;
    invuln   = 2.0f;
    scaredT  = 0;

    const int homes[6][2] = {{0, 0}, {kCols - 1, kRows - 1}, {kCols - 1, 0}, {0, kRows - 1}, {2 * kSpacing, 0}, {2 * kSpacing, kRows - 1}};
    int count = std::min(6, 3 + (level - 1) / 2);
    ghosts.assign(count, Ghost());
    for (int i = 0; i < count; ++i)
    {
        Ghost& g = ghosts[i];
        g.id = i;
        g.homeC = homes[i][0];
        g.homeR = homes[i][1];
        // make sure the home is open (braided mazes always are for odd/odd coordinates, but be safe)
        wall[g.homeR][g.homeC] = 0;
        g.x = g.tx = (float)g.homeC;
        g.z = g.tz = (float)g.homeR;
        g.wait = 1.5f + i * 1.6f;
        g.bob  = i * 1.3f;
    }
}

void Game::startGame()
{
    score = 0;
    lives = 3;
    bombs = 1;
    level = 1;
    newHiScore  = false;
    nextBonusAt = 3000;
    buildLevel();
    setState(State::Ready);
    Sound::play(Sfx::Start);
}

void Game::toTitle()
{
    level = 1;
    buildLevel();
    setState(State::Title);
}

void Game::setState(State s)
{
    state  = s;
    stateT = 0;
}

// ---------------------------------------------------------------- input

void Game::steer(int dir)
{
    if (state != State::Playing)
        return;
    dir &= 3;
    if (dir == ((heading + 2) & 3))
    {
        reverse();
        return;
    }
    int cc = (int)std::lround(px), cr = (int)std::lround(pz);
    if (dir == heading)
        pendingDir = -1;
    else if (!pMoving && open(cc + kDX[dir], cr + kDZ[dir]))
    {
        heading    = dir;
        pendingDir = -1;
    }
    else
        pendingDir = dir;  // taken at the next tile centre where it is possible
    if (!pMoving && open(cc + kDX[heading], cr + kDZ[heading]))
    {
        pMoving = true;
        ptx     = cc + kDX[heading];
        ptz     = cr + kDZ[heading];
    }
}

void Game::turnLeft() { steer((heading + 3) & 3); }
void Game::turnRight() { steer((heading + 1) & 3); }

void Game::reverse()
{
    if (state != State::Playing)
        return;
    int cc = (int)std::lround(px), cr = (int)std::lround(pz);
    if (pMoving)
    {
        // turn around in the middle of a corridor: the cell we came from becomes the target
        int sx = ptx - kDX[heading], sz = ptz - kDZ[heading];
        heading = (heading + 2) & 3;
        ptx     = sx;
        ptz     = sz;
        pendingDir = -1;
    }
    else
    {
        heading = (heading + 2) & 3;
        if (open(cc + kDX[heading], cr + kDZ[heading]))
        {
            pMoving = true;
            ptx     = cc + kDX[heading];
            ptz     = cr + kDZ[heading];
        }
    }
}

void Game::dropBomb()
{
    if (state != State::Playing || bombs <= 0)
        return;
    --bombs;
    Sound::play(Sfx::Bomb);
    explosions.push_back({px, pz, 0.f});
    for (auto& g : ghosts)
    {
        if (g.mode == Ghost::Mode::Dead)
            continue;
        if (dist2(px, pz, g.x, g.z) <= 3.3f * 3.3f)
        {
            g.mode  = Ghost::Mode::Dead;
            g.timer = 5.f;
            g.moving = false;
            addScore(150);
            popups.push_back({g.x, g.z, 0.f, 150});
        }
    }
}

void Game::togglePause()
{
    if (state == State::Playing)
        setState(State::Paused);
    else if (state == State::Paused)
        setState(State::Playing);
}

// ---------------------------------------------------------------- simulation

float Game::playerSpeed() const { return std::min(3.9f, 3.3f + 0.05f * (level - 1)); }

float Game::ghostSpeed(const Ghost& g) const
{
    if (g.mode == Ghost::Mode::Scared)
        return 1.5f;
    return std::min(3.4f, 2.3f + 0.15f * (level - 1));
}

void Game::addScore(int v)
{
    score += v;
    if (score >= nextBonusAt)
    {
        nextBonusAt += 3000;
        if (lives < 5)
        {
            ++lives;
            Sound::play(Sfx::Pickup);
        }
    }
    if (score > hiScore)
    {
        hiScore    = score;
        newHiScore = true;
    }
}

void Game::tickEffects(float dt)
{
    for (auto& e : explosions)
        e.t += dt;
    explosions.erase(std::remove_if(explosions.begin(), explosions.end(), [](const Explosion& e) { return e.t > 0.9f; }),
                     explosions.end());
    for (auto& p : popups)
        p.t += dt;
    popups.erase(std::remove_if(popups.begin(), popups.end(), [](const Popup& p) { return p.t > 1.2f; }), popups.end());
    if (hitFlash > 0)
        hitFlash = std::max(0.f, hitFlash - dt);
}

void Game::eatAt(int c, int r)
{
    for (auto& p : pickups)
    {
        if (!p.alive || p.c != c || p.r != r)
            continue;
        p.alive = false;
        switch (p.kind)
        {
        case Pickup::Kind::Pellet:
            --pelletsLeft;
            addScore(10);
            Sound::play((chompAlt = !chompAlt) ? Sfx::Chomp1 : Sfx::Chomp2);
            break;
        case Pickup::Kind::Power:
            --pelletsLeft;
            addScore(50);
            scaredT   = std::max(2.5f, 8.f - 0.6f * (level - 1));
            ghostCombo = 0;
            for (auto& g : ghosts)
                if (g.mode == Ghost::Mode::Normal)
                    g.mode = Ghost::Mode::Scared;
            Sound::play(Sfx::Power);
            break;
        case Pickup::Kind::Heart:
            if (lives < 5)
                ++lives;
            addScore(100);
            Sound::play(Sfx::Pickup);
            break;
        case Pickup::Kind::Bomb:
            bombs = std::min(bombs + 1, 5);
            addScore(100);
            Sound::play(Sfx::Pickup);
            break;
        }
    }
}

void Game::playerArrive()
{
    px = (float)ptx;
    pz = (float)ptz;
    eatAt(ptx, ptz);

    if (pendingDir >= 0 && open(ptx + kDX[pendingDir], ptz + kDZ[pendingDir]))
    {
        heading    = pendingDir;
        pendingDir = -1;
        Sound::play(Sfx::Turn);
    }
    if (open(ptx + kDX[heading], ptz + kDZ[heading]))
    {
        ptx += kDX[heading];
        ptz += kDZ[heading];
        pMoving = true;
    }
    else
        pMoving = false;
}

void Game::updatePlayer(float dt)
{
    if (!pMoving && pendingDir >= 0 && open((int)std::lround(px) + kDX[pendingDir], (int)std::lround(pz) + kDZ[pendingDir]))
    {
        heading = pendingDir;
        pendingDir = -1;
        ptx = (int)std::lround(px) + kDX[heading];
        ptz = (int)std::lround(pz) + kDZ[heading];
        pMoving = true;
    }
    if (!pMoving)
        return;
    float remaining = playerSpeed() * dt;
    chompAnim += dt * 9.f;
    while (remaining > 0.f && pMoving)
    {
        float vx = ptx - px, vz = ptz - pz;
        float d = std::sqrt(vx * vx + vz * vz);
        if (d <= remaining)
        {
            remaining -= d;
            playerArrive();
        }
        else
        {
            px += vx / d * remaining;
            pz += vz / d * remaining;
            remaining = 0.f;
        }
    }
}

void Game::ghostChoose(Ghost& g)
{
    int cc = (int)std::lround(g.x), cr = (int)std::lround(g.z);
    int back = -1;
    for (int d = 0; d < 4; ++d)
        if (kDX[d] == -g.dx && kDZ[d] == -g.dz && (g.dx || g.dz))
            back = d;
    int opts[4], n = 0;
    for (int d = 0; d < 4; ++d)
        if (d != back && open(cc + kDX[d], cr + kDZ[d]))
            opts[n++] = d;
    if (!n && back >= 0)
        opts[n++] = back;  // dead end
    if (!n)
    {
        g.moving = false;
        return;
    }

    int pick = opts[rnd(n)];
    float chase = 0.55f + 0.05f * std::min(level, 8);
    bool smart  = (rng() % 1000) / 1000.f < chase;
    if (g.mode == Ghost::Mode::Scared || (g.mode == Ghost::Mode::Normal && smart))
    {
        float best = g.mode == Ghost::Mode::Scared ? -1e9f : 1e9f;
        for (int i = 0; i < n; ++i)
        {
            int d  = opts[i];
            // ghosts have different personalities: odd ones aim a few tiles ahead of the player
            float tx = px + (g.id & 1 ? kDX[heading] * 3.f : 0.f);
            float tz = pz + (g.id & 1 ? kDZ[heading] * 3.f : 0.f);
            float s  = dist2((float)(cc + kDX[d]), (float)(cr + kDZ[d]), tx, tz);
            if (g.mode == Ghost::Mode::Scared ? s > best : s < best)
            {
                best = s;
                pick = d;
            }
        }
    }
    g.dx = kDX[pick];
    g.dz = kDZ[pick];
    g.tx = cc + g.dx;
    g.tz = cr + g.dz;
    g.moving = true;
}

void Game::updateGhosts(float dt, bool collide)
{
    for (auto& g : ghosts)
    {
        g.bob += dt * 4.f;
        if (g.mode == Ghost::Mode::Dead)
        {
            g.timer -= dt;
            if (g.timer <= 0.f)
            {
                g.mode = Ghost::Mode::Normal;
                g.x = g.tx = (float)g.homeC;
                g.z = g.tz = (float)g.homeR;
                g.dx = g.dz = 0;
                g.moving    = false;
                g.wait      = 1.0f;
            }
            continue;
        }
        if (g.wait > 0.f)
        {
            g.wait -= dt;
            continue;
        }
        if (g.mode == Ghost::Mode::Scared && scaredT <= 0.f)
            g.mode = Ghost::Mode::Normal;

        float remaining = ghostSpeed(g) * dt;
        int guard = 0;
        while (remaining > 0.f && guard++ < 8)
        {
            if (!g.moving)
            {
                ghostChoose(g);
                if (!g.moving)
                    break;
            }
            float vx = g.tx - g.x, vz = g.tz - g.z;
            float d  = std::sqrt(vx * vx + vz * vz);
            if (d <= remaining)
            {
                remaining -= d;
                g.x = (float)g.tx;
                g.z = (float)g.tz;
                g.moving = false;
            }
            else
            {
                g.x += vx / d * remaining;
                g.z += vz / d * remaining;
                remaining = 0.f;
            }
        }

        if (collide && dist2(px, pz, g.x, g.z) < 0.62f * 0.62f)
        {
            if (g.mode == Ghost::Mode::Scared)
            {
                g.mode  = Ghost::Mode::Dead;
                g.timer = 4.f;
                int v   = 200 << std::min(ghostCombo, 3);
                ++ghostCombo;
                addScore(v);
                popups.push_back({g.x, g.z, 0.f, v});
                Sound::play(Sfx::EatGhost);
            }
            else if (invuln <= 0.f)
            {
                killPlayer();
                return;
            }
        }
    }
}

void Game::killPlayer()
{
    --lives;
    hitFlash = 0.35f;
    Sound::stopMusic();
    Sound::play(Sfx::Die);
    setState(State::Dying);
}

void Game::update(float dt)
{
    stateT += dt;
    tickEffects(dt);

    switch (state)
    {
    case State::Title:
        updateGhosts(dt, false);
        // keep the title demo lively: respawn scared state never happens, ghosts just roam
        break;

    case State::Ready:
        for (auto& g : ghosts)
            g.bob += dt * 4.f;
        if (stateT > 2.2f)
        {
            setState(State::Playing);
            Sound::startMusic();
            // start rolling immediately, pick the first open direction (current heading, right, left, back)
            for (int k : {0, 1, 3, 2})
            {
                int h = (heading + k) & 3;
                if (open((int)px + kDX[h], (int)pz + kDZ[h]))
                {
                    heading = h;
                    break;
                }
            }
            if (open((int)px + kDX[heading], (int)pz + kDZ[heading]))
            {
                ptx     = (int)px + kDX[heading];
                ptz     = (int)pz + kDZ[heading];
                pMoving = true;
            }
        }
        break;

    case State::Playing:
        if (invuln > 0.f)
            invuln -= dt;
        if (scaredT > 0.f)
            scaredT -= dt;
        updatePlayer(dt);
        updateGhosts(dt, true);
        if (!heartSpawned && lives < 5 && pelletsLeft < pelletsTotal / 2)
        {
            heartSpawned = true;
            for (int tries = 0; tries < 200; ++tries)
            {
                int c = rnd(kCols), r = rnd(kRows);
                if (wall[r][c] || (c == (int)std::lround(px) && r == (int)std::lround(pz)))
                    continue;
                Pickup p;
                p.kind = Pickup::Kind::Heart;
                p.c = c;
                p.r = r;
                p.alive = true;
                pickups.push_back(p);
                break;
            }
        }
        if (pelletsLeft <= 0 && state == State::Playing)
        {
            Sound::stopMusic();
            Sound::play(Sfx::LevelUp);
            addScore(500 + 100 * level);
            setState(State::LevelClear);
        }
        break;

    case State::Dying:
        if (stateT > 2.0f)
        {
            if (lives <= 0)
            {
                auto& st = Settings::get();
                if (hiScore > st.hiScore)
                {
                    st.hiScore = hiScore;
                    st.saveUserData();
                }
                setState(State::GameOver);
            }
            else
            {
                resetActors();
                setState(State::Ready);
            }
        }
        break;

    case State::LevelClear:
        if (stateT > 2.6f)
        {
            ++level;
            int keepBombs = bombs;
            buildLevel();
            bombs = keepBombs;
            setState(State::Ready);
        }
        break;

    case State::GameOver:
    case State::Paused: break;
    }
}
