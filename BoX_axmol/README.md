# BoX (Axmol 3.0)

Port von `BoX.pb` (PureBasic + Chipmunk), Physik jetzt Box2D 3.x. Gezeichnet wird nur mit `ax::DrawNode`
(auch die Zeit als 7-Segment-Anzeige, kein Label/Sprite).

Einbinden in ein Axmol-Projekt (`axmol init -p com.example.box -n BoX`):
1. `BoxScene.h/.cpp` nach `Source/` kopieren (bei `GLOB_RECURSE` in der CMakeLists automatisch dabei).
2. In `AppDelegate::applicationDidFinishLaunching`: Design-Resolution `640x640` (`ResolutionPolicy::SHOW_ALL`),
   Fenster 640x640, dann `director->runWithScene(BoxScene::create());`
3. Box2D 3.x muss aktiv sein (in Axmol 3.0 gebuendelt, Header `box2d/box2d.h`).

Steuerung: Pfeil links/rechts dreht den Kasten (Winkelgeschwindigkeit summiert sich wie im Original), Esc beendet.
Ziel: Ball in den kleinen Kreis unten rechts bringen; Zeit (ms) friert dann ein und wird blau.

Gegen Axmol `dev` (3.x: KeyboardEvent/KeyboardEventListener, axmol/scene/*) geprueft.

Hinweis: Box2D 3.x (Meter, 100 px = 1 m) statt Chipmunk; y-Achse ist gespiegelt (oben = +y).
Nicht kompiliert (kein Axmol-SDK in dieser Umgebung) - API-Namen gegen deine 3.0-Version pruefen.
