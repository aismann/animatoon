#include "GameScene.h"
#include "GameData.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#define AX_ENABLE_AUDIO 1
#if AX_ENABLE_AUDIO
#    include "axmol/audio/AudioEngine.h"
#endif

using namespace ax;

namespace
{
constexpr const char* MUSIC_FILE = "music.ogg";

// The 16 colours of the original (C64 palette), stored as R,G,B,0
Color32 pal(int i)
{
    i &= 15;
    return Color32(gamedata::palette[i * 4], gamedata::palette[i * 4 + 1], gamedata::palette[i * 4 + 2], 255);
}

Texture2D* makeTexture(const std::vector<Color32>& px, int w, int h, const char* key)
{
    auto image = new Image();
    image->initWithRawData(reinterpret_cast<const uint8_t*>(px.data()), px.size() * 4, w, h, 8);
    auto tex = Director::getInstance()->getTextureCache()->addImage(image, key);
    image->release();
    tex->setAliasTexParameters();   // nearest neighbour: keep the chunky C64 pixels
    return tex;
}

SpriteFrame* makeFrame(Texture2D* tex, float x, float y, float w, float h)
{
    auto f = SpriteFrame::createWithTexture(tex, Rect(x, y, w, h));
    f->retain();
    return f;
}
}  // namespace

// =============================================================================================
//  Setup
// =============================================================================================

bool GameScene::init()
{
    if (!Scene::init())
        return false;

    buildTextures();
    buildScene();
    resetGame();

    auto kb          = EventListenerKeyboard::create();
    kb->onKeyPressed  = AX_CALLBACK_1(GameScene::onKeyPressed, this);
    kb->onKeyReleased = AX_CALLBACK_1(GameScene::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(kb, this);

#if AX_ENABLE_AUDIO
    AudioEngine::play2d(MUSIC_FILE, true, 0.6f);
#endif

    scheduleUpdate();
    return true;
}

// Converts the C64 sprites / characters of the original into RGBA textures (already zoomed 2x,
// as ZoomSprite() did in the PureBasic version).
void GameScene::buildTextures()
{
    const Color32 transparent(255, 0, 255, 0);

    // ---- sprites: 13 frames x 6 variants (3 players x right/left), 48x42 each ---------------------
    constexpr int FW = 48, FH = 42;
    const int atlasW = NS * FW, atlasH = 6 * FH;
    std::vector<Color32> atlas(atlasW * atlasH, transparent);

    const int playerColor[3] = {6, 2, 12};   // blue (you), red, grey
    for (int player = 0; player < 3; ++player)
    {
        const Color32 p[4] = {transparent, Color32(0, 0, 0, 255), pal(playerColor[player]), pal(10)};
        for (int f = 0; f < NS; ++f)
        {
            const uint8_t* src = gamedata::sprites + f * 64;
            Color32 s24[24 * 21];
            int l = 0;
            for (int k = 0; k < 21; ++k)
                for (int i = 0; i < 3; ++i, ++l)
                    for (int j = 0, d = 6; j < 4; ++j, d -= 2)
                    {
                        Color32 c            = p[(src[l] >> d) & 3];
                        s24[k * 24 + i * 8 + j * 2]     = c;
                        s24[k * 24 + i * 8 + j * 2 + 1] = c;
                    }

            for (int dir = 0; dir < 2; ++dir)
            {
                const int variant = player * 2 + dir;
                const int idx     = (f + 1) + NS * variant;
                auto& mask        = _masks[idx];
                mask.assign(FW * FH, 0);
                for (int y = 0; y < FH; ++y)
                    for (int x = 0; x < FW; ++x)
                    {
                        int sx = x / 2;
                        if (dir == Left)
                            sx = 23 - sx;       // InverseSprite256Coul()
                        Color32 c = s24[(y / 2) * 24 + sx];
                        atlas[(variant * FH + y) * atlasW + f * FW + x] = c;
                        mask[y * FW + x]                                = c.a ? 1 : 0;
                    }
                _frames[idx] = nullptr;  // created below, once the texture exists
            }
        }
    }
    Texture2D* sprTex = makeTexture(atlas, atlasW, atlasH, "snowball/sprites");
    for (int variant = 0; variant < 6; ++variant)
        for (int f = 0; f < NS; ++f)
            _frames[(f + 1) + NS * variant] = makeFrame(sprTex, f * FW, variant * FH, FW, FH);

    // ---- tiles: 256 characters, 16x16 each, 16x16 grid -----------------------------------------------
    std::vector<Color32> tiles(256 * 256, transparent);
    for (int t = 0; t < 256; ++t)
    {
        const Color32 p[4] = {transparent, pal(1), pal(15), pal(std::max(0, gamedata::charColors[t] - 8))};
        for (int k = 0; k < 8; ++k)
        {
            const uint8_t row = gamedata::chars[t * 8 + k];
            for (int x = 0; x < 16; ++x)
            {
                Color32 c = p[(row >> (6 - 2 * (x / 4))) & 3];
                for (int dy = 0; dy < 2; ++dy)
                    tiles[((t / 16) * 16 + k * 2 + dy) * 256 + (t % 16) * 16 + x] = c;
            }
        }
    }
    Texture2D* tileTex = makeTexture(tiles, 256, 256, "snowball/tiles");
    for (int t = 0; t < 256; ++t)
        _tileFrames[t] = makeFrame(tileTex, (t % 16) * 16, (t / 16) * 16, 16, 16);

    // ---- 2x2 white pixel block for snow flakes and the ground ----------------------------------------
    std::vector<Color32> white(4, Color32(255, 255, 255, 255));
    _pixelFrame = makeFrame(makeTexture(white, 2, 2, "snowball/white"), 0, 0, 2, 2);
}

void GameScene::buildScene()
{
    Director::getInstance()->setClearColor(Color(pal(14)));

    // position helper: the original uses a top-left origin with y pointing down
    auto place = [](Node* n, float x, float y) { n->setPosition(Vec2(x, SCREEN_H - y)); };

    // ground
    auto ground = Sprite::createWithSpriteFrame(_pixelFrame);
    ground->setAnchorPoint(Vec2(0, 1));
    ground->setScale(SCREEN_W / 2.0f, 80 / 2.0f);
    place(ground, 0, 400);
    addChild(ground, 0);

    // tile grid (41 x 25 visible tiles)
    _tileNodes.resize(41 * 25);
    _tileIds.assign(41 * 25, -1);
    for (int i = 0; i < 25; ++i)
        for (int j = 0; j < 41; ++j)
        {
            auto s = Sprite::createWithSpriteFrame(_tileFrames[1]);
            s->setAnchorPoint(Vec2(0, 1));
            place(s, j * 16, i * 16);
            s->setVisible(false);
            addChild(s, 1);
            _tileNodes[i * 41 + j] = s;
        }

    // actors
    for (int i = 0; i < 3; ++i)
    {
        auto s = Sprite::createWithSpriteFrame(_frames[1 + NS * i * 2]);
        s->setAnchorPoint(Vec2(0, 1));
        addChild(s, 2);
        _actors[i].node = s;
    }

    // snow flakes
    _flakes.resize(NUM_FLAKES);
    for (auto& f : _flakes)
    {
        f.node = Sprite::createWithSpriteFrame(_pixelFrame);
        f.node->setAnchorPoint(Vec2(0, 1));
        addChild(f.node, 4);
    }

    // HUD: one portrait + score per player
    for (int k = 0; k < 3; ++k)
    {
        _hudIcons[k] = Sprite::createWithSpriteFrame(_frames[1 + NS * 2 * k]);
        _hudIcons[k]->setAnchorPoint(Vec2(0, 1));
        place(_hudIcons[k], 580, k * 42);
        addChild(_hudIcons[k], 5);

        _hudScores[k] = Label::createWithTTF("0", "fonts/arial.ttf", 16);
        _hudScores[k]->setAnchorPoint(Vec2(0, 1));
        _hudScores[k]->enableOutline(Color32::black, 2);
        place(_hudScores[k], 620, k * 42 + 18);
        addChild(_hudScores[k], 6);
    }

    _endLabel = Label::createWithTTF("", "fonts/arial.ttf", 28);
    _endLabel->enableOutline(Color32::black, 3);
    place(_endLabel, SCREEN_W / 2, 232);
    addChild(_endLabel, 7);
}

// = Init macro of the original
void GameScene::resetGame()
{
    for (int i = 0; i < 3; ++i)
    {
        Actor& a      = _actors[i];
        ax::Sprite* n = a.node;
        a             = Actor{};
        a.node        = n;
        a.number      = i;
        a.y           = ACTOR_Y;
        a.x           = rnd(0, 320) & ~1;
        _score[i]     = 0;
    }
    for (auto& b : _balls)
        b.node->removeFromParent();
    _balls.clear();
    _result = 0;
    _endLabel->setString("");

    for (auto& f : _flakes)
    {
        f.x = rnd(0, 1279);
        f.y = static_cast<float>(rnd(0, 479));
        f.v = static_cast<float>(rnd(1, 2));   // Random(2,1)+Random(9,1)/10 : integer division in PureBasic
    }
}

int GameScene::rnd(int lo, int hi)
{
    return std::uniform_int_distribution<int>(lo, hi)(_rng);
}

// =============================================================================================
//  Input
// =============================================================================================

void GameScene::onKeyPressed(KeyboardEvent* ev)
{
    _keys.insert(static_cast<int>(ev->getKeyCode()));
    if (ev->getKeyCode() == KeyboardEvent::KeyCode::KEY_ESCAPE)
        Director::getInstance()->end();
}

void GameScene::onKeyReleased(KeyboardEvent* ev)
{
    _keys.erase(static_cast<int>(ev->getKeyCode()));
}

bool GameScene::keyDown(KeyboardEvent::KeyCode k) const
{
    return _keys.count(static_cast<int>(k)) != 0;
}

// =============================================================================================
//  Game logic
// =============================================================================================

void GameScene::update(float dt)
{
    // the original is tied to a 60 Hz screen refresh: run the logic with a fixed time step
    _accumulator = std::min(_accumulator + dt, 5 * FIXED_STEP);
    while (_accumulator >= FIXED_STEP * 0.999f)
    {
        step();
        _accumulator -= FIXED_STEP;
    }
}

void GameScene::changeState(Actor& a, int state)
{
    a.state       = state;
    a.animCounter = 1;
    a.animIndex   = 1;
}

// Advances the animation of `a`; returns true when the animation has reached its end.
bool GameScene::animate(Actor& a, int anim)
{
    const uint8_t* table = gamedata::anims + (anim << 4);
    bool finished        = false;

    if (--a.animCounter == 0)
    {
        a.animCounter = table[0];
        if (table[a.animIndex] == 0)
        {
            finished    = true;
            a.animIndex = 1;
        }
        a.sprite = table[a.animIndex] + a.number * NS * 2 + a.direction * NS;
        a.animIndex++;
    }
    return finished;
}

bool GameScene::pixelCollision(int spriteA, int ax_, int ay, int spriteB, int bx, int by) const
{
    if (spriteA <= 0 || spriteB <= 0)
        return false;
    constexpr int W = 48, H = 42;
    const int x0 = std::max(ax_, bx), x1 = std::min(ax_ + W, bx + W);
    const int y0 = std::max(ay, by), y1 = std::min(ay + H, by + H);
    const auto& ma = _masks[spriteA];
    const auto& mb = _masks[spriteB];
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
            if (ma[(y - ay) * W + (x - ax_)] && mb[(y - by) * W + (x - bx)])
                return true;
    return false;
}

void GameScene::updateActor(Actor& a)
{
    bool shoot = false, left = false, right = false, down = false;

    if (a.number != 0)
    {
        // ---- the two computer players -----------------------------------------------------------------
        Actor* me   = &_actors[0];
        Actor* other = &_actors[a.number ^ 3];

        switch (a.phase)
        {
        case ChooseTarget:
            a.target = (rnd(0, 1) == 1) ? me : other;
            a.phase  = PickUpSnow;
            break;

        case PickUpSnow:
            if (a.snowball == 0)
                down = true;
            else
                a.phase = Approach;
            break;

        case Approach:
        {
            const int d = a.x - a.target->x;
            if (std::abs(d) > SHOOT_DIST)          // get closer
            {
                (d < 0 ? right : left) = true;
            }
            else if (std::abs(d) < SHOOT_DIST)     // back off
            {
                (d < 0 ? left : right) = true;
            }
            else
            {
                a.direction = (d < 0) ? Right : Left;
                a.phase     = Shoot;
            }
            break;
        }

        case Shoot:
            if (a.snowball == 1)
                shoot = true;
            else
            {
                a.phase = Retreat;
                a.tempo = TEMPO;
                a.direction ^= 1;
            }
            break;

        case Retreat:
            a.tempo--;
            (a.direction == Left ? left : right) = true;
            if (a.tempo == 0)
                a.phase = ChooseTarget;
            break;
        }
    }
    else
    {
        // ---- the human player ------------------------------------------------------------------------------
        shoot = keyDown(KeyboardEvent::KeyCode::KEY_SPACE);
        left  = keyDown(KeyboardEvent::KeyCode::KEY_LEFT_ARROW);
        right = keyDown(KeyboardEvent::KeyCode::KEY_RIGHT_ARROW);
        down  = keyDown(KeyboardEvent::KeyCode::KEY_DOWN_ARROW);
    }

    switch (a.state)
    {
    case FallForward:
        if (animate(a, FallForward))
            changeState(a, Idle);
        break;

    case FallBack:
        if (animate(a, FallBack))
            changeState(a, Idle);
        break;

    case PickUp:
        if (animate(a, PickUp))
        {
            a.snowball = 1;
            changeState(a, Idle);
        }
        break;

    case Throw:
        if (a.animIndex == 2 && a.animCounter == 1)   // release frame: the ball leaves the hand
        {
            a.snowball = 0;
            Ball b{};
            b.shooter = a.number;
            b.y       = static_cast<float>(a.y + 10);
            b.vy      = -2.5f;
            if (a.direction == Right)
            {
                b.x  = static_cast<float>(a.x + 45);
                b.vx = 7;
            }
            else
            {
                b.x  = static_cast<float>(a.x - 5);
                b.vx = -7;
            }
            b.node = Sprite::createWithSpriteFrame(_frames[SNOWBALL_SPRITE]);
            b.node->setAnchorPoint(Vec2(0, 1));
            addChild(b.node, 3);
            _balls.push_back(b);
        }
        if (animate(a, Throw))
            changeState(a, Idle);
        break;

    default:   // Idle, Walk
        if (down)
            changeState(a, PickUp);
        else if (shoot && a.snowball == 1)
            changeState(a, Throw);
        else if (right)
        {
            a.direction = Right;
            if (a.state != Walk)
                changeState(a, Walk);
            a.x += 2;
        }
        else if (left)
        {
            a.direction = Left;
            if (a.state != Walk)
                changeState(a, Walk);
            a.x -= 2;
        }
        else
            changeState(a, Idle);

        animate(a, a.state);
        break;
    }
}

void GameScene::updateTiles(int ox)
{
    const int x  = ox / 16;
    const int dx = ox % 16;
    for (int i = 0; i < 25; ++i)
        for (int j = 0; j < 41; ++j)
        {
            const int id    = gamedata::map[i * 80 + x + j];
            const int cell  = i * 41 + j;
            Sprite* s       = _tileNodes[cell];
            if (_tileIds[cell] != id)
            {
                _tileIds[cell] = id;
                s->setVisible(id != 0);
                if (id)
                    s->setSpriteFrame(_tileFrames[id]);
            }
            s->setPositionX(static_cast<float>(j * 16 - dx));
        }
}

void GameScene::step()
{
    // ---- actors ----------------------------------------------------------------------------------------
    if (_result == 0)
        for (auto& a : _actors)
            updateActor(a);

    // ---- camera: follow the player ------------------------------------------------------------------------
    _ox = std::clamp(_actors[0].x + 24 - 320, 0, 640);
    updateTiles(_ox);

    // ---- actors and snowballs --------------------------------------------------------------------------------
    for (auto& a : _actors)
    {
        a.node->setVisible(_result == 0);
        if (_result == 0 && a.sprite > 0)
        {
            a.node->setSpriteFrame(_frames[a.sprite]);
            a.node->setPosition(Vec2(static_cast<float>(a.x - _ox), static_cast<float>(SCREEN_H - a.y)));
        }
    }

    if (_result == 0)
    {
        for (size_t b = 0; b < _balls.size();)
        {
            Ball& ball = _balls[b];
            ball.x += ball.vx;
            ball.y += ball.vy;
            ball.vy += GRAVITY;

            bool remove = false;
            if (ball.y > 420)
                remove = true;
            else
            {
                ball.node->setPosition(Vec2(std::round(ball.x - _ox), SCREEN_H - std::round(ball.y)));
                if (ball.y < 410)
                    for (auto& a : _actors)
                    {
                        if (a.state != FallBack && a.state != FallForward &&
                            pixelCollision(SNOWBALL_SPRITE, (int)std::lround(ball.x), (int)std::lround(ball.y), a.sprite,
                                           a.x, a.y))
                        {
                            _score[a.number]--;
                            _score[ball.shooter]++;
                            const bool hitFromFront =
                                (a.direction == Right && ball.vx < 0) || (a.direction == Left && ball.vx > 0);
                            changeState(a, hitFromFront ? FallBack : FallForward);
                            remove = true;
                            break;
                        }
                    }
            }

            if (remove)
            {
                ball.node->removeFromParent();
                _balls.erase(_balls.begin() + b);
            }
            else
                ++b;
        }
    }
    else
    {
        for (auto& b : _balls)
            b.node->setVisible(false);
    }

    // ---- snow flakes (only the visible ones move, as in the original) --------------------------------------------
    for (auto& f : _flakes)
    {
        const bool visible = f.x > _ox && f.x < _ox + SCREEN_W;
        f.node->setVisible(visible);
        if (visible)
        {
            f.node->setPosition(Vec2(static_cast<float>(f.x - _ox), SCREEN_H - std::floor(f.y)));
            f.y += f.v;
            if (f.y > 420)
                f.y = 0;
        }
    }

    // ---- score / end of game ---------------------------------------------------------------------------------------
    for (int k = 0; k < 3; ++k)
        _hudScores[k]->setString(std::to_string(_score[k]));

    if (_score[1] == WIN_SCORE || _score[2] == WIN_SCORE)
        _result = 2;
    else if (_score[0] == WIN_SCORE)
        _result = 1;

    if (_result)
    {
        _endLabel->setString(_result == 1 ? "You win !" : "You lose !");
        if (keyDown(KeyboardEvent::KeyCode::KEY_SPACE))
            resetGame();
    }
}
