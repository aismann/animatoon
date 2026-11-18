#pragma once
// BoX - Port von PureBasic (Chipmunk) nach Axmol, Physik jetzt Box2D 3.x. Gezeichnet wird ausschliesslich mit ax::DrawNode.
#include "axmol/axmol.h"
#include "axmol/audio/AudioEngine.h"
#include "box2d/box2d.h"
#include <array>
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
    void drawAll();
    void drawBackground();   // blauer Verlauf (Menue + Spiel)
    enum class Sfx { Click, Select, Start, HitSoft, HitHard, Win, Level, Pop, Erase, Goal, Yeah, Count };
    void sfx(Sfx id, float volume = 1.f);               // spielt Content/sfx/<name>.wav (mit Mindestabstand)
    void drawSpeaker(ax::Vec2 c, bool on);
    // Touch/Mobile: Bildschirm-Tasten (Drehen links/rechts halten, Zurueck, Ton) + klickbare Editor-Leisten
    void playTouch(int phase, intptr_t id, ax::Vec2 p);   // 0 = down, 2 = up/cancel
    void drawTouchButtons();
    void drawControlSwitch(ax::Vec2 c);   // Menue: Umschalter Tasten <-> Neigung (Beschleunigungssensor)
    void setAccelerometer(bool on);
    bool editorButton(ax::Vec2 world);
    void drawMenu();
    void drawLevelPreview(ax::Vec2 c, float size, int level);   // Miniatur des Levels (Menue)
    void drawNumber(int n, ax::Vec2 pos, float size);
    void enforceBounds();
    void startGame();
    void stepFluid();
    void drawMetaballs();                 // Fluid im Spiel
    struct MBPoint { ax::Vec2 p; float s; };                       // Position (px) + Groessenfaktor (1 = 20 px Kern)
    struct MBPalette { ax::Color c[4]; float t[4]; ax::Color soft, core; bool shine; float cell = 4.f, smooth = 1.5f; float inset[4] = {0.f, 3.5f, 8.f, 13.f}; };
    void renderMetaballs(const std::vector<MBPoint>& pts, const MBPalette& pal);
    void fillRect(ax::Vec2 lo, ax::Vec2 hi, const ax::Color& col);
    void roundedRect(ax::Vec2 c, ax::Vec2 half, float r, const ax::Color& fill, const ax::Color& border, float thick);
    void strokeLetter(char ch, ax::Vec2 o, float w, float h, const ax::Color& col, float thick);
    int countParticlesInGoal() const;
    bool goalTouched() const;   // Ball/Schleim beruehrt das Ziel
    float slimeFieldAtGoal() const;   // groesster Metaball-Feldwert auf der Zielscheibe (gleiches Feld wie beim Zeichnen)
    void drawDigit(int digit, ax::Vec2 pos, float size);
    void drawTime(long ms, ax::Vec2 pos);

    enum class State { Menu, Play, Editor };

    // Level: Ziel + innere Waende in Pixeln relativ zur Kastenmitte (y nach unten wie im Original)
    struct LevelData { float gx = 0, gy = 0; std::vector<std::array<float, 4>> segs; };
    static LevelData builtinLevel(int level);
    void loadLevels();
    void saveLevels() const;
    void startEditor();
    void drawEditor();
    void editorPointer(int phase, ax::Vec2 world, int button);   // 0 = down, 1 = move, 2 = up
    void editorKey(int key);
    ax::Vec2 toLocal(ax::Vec2 screen) const;
    enum class Mode { Ball, Fluid };

    struct Seg { b2Vec2 a, b; };   // lokale Container-Koordinaten (Meter)

    ax::DrawNode* _draw = nullptr;
    ax::DrawNode* _drawTop = nullptr;   // eigener DrawNode ueber allem (Level-Vorschau im Menue)
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

    std::vector<LevelData> _levels;   // Index 1..kLevels (eingebaut, ueberschrieben durch gespeicherte Editor-Levels)
    bool _editorOn   = false;         // Schalter im Menue
    bool _fromEditor = false;         // Testspiel aus dem Editor
    int _tool        = 0;             // 0 = Wand, 1 = Ziel, 2 = Radierer
    bool _drag       = false;
    ax::Vec2 _dragA, _dragB, _cursor; // lokale Editor-Koordinaten
    float _swAnim    = 0.f;           // Animation des Menue-Schalters 0..1

    bool _shineOn = false;                    // Schleim-Glanz an/aus (Standard: aus; Schalter im Menue, Taste G)
    float _shAnim = 0.f;                      // Animation des Glanz-Schalters 0..1
    bool _sfxOn = false;                      // Ton an/aus (Standard: aus; M oder Klick auf Lautsprecher)
    float _lastSfx[(int)Sfx::Count] = {};

    State _state   = State::Menu;
    Mode _mode     = Mode::Ball;    // im Menue gewaehlt
    int _menuLevel = 1;             // Start-Level im Menue
    float _menuT   = 0.f;           // Animationszeit Menue
    float _finishTimer = 0.f;

    bool _left = false, _right = false, _won = false;
    enum class Control { Buttons, Tilt };     // Steuerung auf dem Handy: Bildschirm-Tasten oder Beschleunigungssensor
    Control _control = Control::Buttons;
    bool _hasAccel   = false;                 // Beschleunigungssensor vorhanden (es kamen Sensor-Events)
    bool _probing    = false;                 // Sensor-Erkennung laeuft (nur Android/iOS, kurz nach dem Start)
    float _probeT    = 0.f;
    float _tilt      = 0.f;                   // aktuelle Neigung (Beschleunigung x, in g)
    bool _accOn      = false;                 // Sensor gerade aktiv (nur im Spiel mit Neigungssteuerung)
    bool _tLeft = false, _tRight = false;      // Touch-Tasten gedrueckt
    intptr_t _tLId = -1, _tRId = -1;           // zugehoerige Pointer-IDs (Multitouch)
    float _w         = 0.f;   // Winkelgeschwindigkeit des Containers
    double _elapsed  = 0.0;   // Sekunden
};
