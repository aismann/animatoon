# BoX (Axmol 3.0)

Port von `BoX.pb` (PureBasic + Chipmunk), Physik jetzt Box2D 3.x. Gezeichnet wird nur mit `ax::DrawNode`
(auch die Zeit als 7-Segment-Anzeige, kein Label/Sprite).

Einbinden in ein Axmol-Projekt (`axmol init -p com.example.box -n BoX`):
1. `BoxScene.h/.cpp` nach `Source/` kopieren (bei `GLOB_RECURSE` in der CMakeLists automatisch dabei).
2. In `AppDelegate::applicationDidFinishLaunching`: Design-Resolution `640x640` (`ResolutionPolicy::SHOW_ALL`),
   Fenster 640x640, dann `director->runWithScene(BoxScene::create());`
3. Box2D 3.x muss aktiv sein (in Axmol 3.0 gebuendelt, Header `box2d/box2d.h`).

Levels 1-14 (1-4 aus dem Original, 5-14 neu: Zickzack, Trichter, Pegs, Trennwand, S-Waende, Tresor, Tunnel, Ring, Slalom, Finale - ungetestet, ggf. Luecken anpassen). 1-4: Level 3 und 4 sind laut Vorlage identisch: Rahmen + Ziel in der Mitte, Ziel-Radius 16). Nach dem Treffer folgt nach 1,5 s das naechste Level, die Zeit startet pro Level neu; oben rechts Level-Ziffer und Fortschrittspunkte.

Menue (Maus: Karte anklicken startet Ball/Fluid bzw. den Editor, Pfeile/Punkte waehlen das Level; nur DrawNode, ohne Text; Titel BOX aus Linienbuchstaben, Hintergrund und Fluid-Icon als Metaballs): links = ein Ball (1), rechts = Fluid (2); Links/Rechts oder 1/2 waehlt,
Hoch/Runter waehlt das Start-Level (1-14), Enter/Space startet. Im Spiel bringt Esc zurueck ins Menue, im Menue beendet Esc.
Fluid: 15 Kreise (3 x 5, r = 2 px), als Metaballs gezeichnet (Marching Squares, nur DrawNode; Glanz ist ein eigenes Metaball-Feld), als Metaballs gezeichnet (Marching Squares, nur DrawNode) mit Viskositaet + Kohaesion (Box2D 3.x hat kein eingebautes Fluid);
gewonnen, wenn 1 Partikel im Ziel sind.

Steuerung: Pfeil links/rechts dreht den Kasten (Winkelgeschwindigkeit summiert sich wie im Original), Esc beendet.
Ziel: Ball in den kleinen Kreis unten rechts bringen; Zeit (ms) friert dann ein und wird blau.

Gegen Axmol `dev` (3.x: KeyboardEvent/KeyboardEventListener, axmol/scene/*) geprueft.

Hinweis: Box2D 3.x (Meter, 100 px = 1 m) statt Chipmunk; y-Achse ist gespiegelt (oben = +y).
Nicht kompiliert (kein Axmol-SDK in dieser Umgebung) - API-Namen gegen deine 3.0-Version pruefen.

Ausbruchsschutz: Waende sind Kapseln (5 px dick, auch so gezeichnet), Ball/Partikel sind Bullets, die Kasten-Drehzahl ist auf 8 rad/s begrenzt,
und `enforceBounds()` setzt jeden Koerper, der trotzdem ausserhalb des Rahmens waere, nach jedem Physik-Schritt zurueck.

Metaballs mit Clipper2 (`Source/MetaballMesh.h`, header-only): Marching-Squares-Zellen -> `Union` (saubere Aussen-/Lochkonturen)
-> `InflatePaths` +/- (Closing) -> Chaikin -> `Triangulate` (Delaunay). Wenige, nicht ueberlappende Dreiecke, daher
sind halbtransparente Wasserschichten (`kWaterAlpha`, Standard 0,32) ohne Doppelblending moeglich.
Clipper2 ist in Axmol bereits gebuendelt (`clipper2/clipper.h`, `clipper2/clipper.triangulation.h`).

Level-Editor (nur DrawNode): im Menue mit **E** oder per Mausklick den Schalter (unten rechts, Stift-Symbol) einschalten
(grau = aus, orange = an), dann startet **Enter/Space** den Editor fuer das gewaehlte Level (Hoch/Runter).
- Werkzeuge: **1** Wand (mit der Maus ziehen, Raster 10 px), **2** Ziel setzen (Klick), **3** Radierer (Klick auf Wand);
  rechte Maustaste loescht immer.
- **Z** letzte Wand zurueck, **Entf/Backspace** alle Waende loeschen, **R** Original-Level wiederherstellen,
  **Hoch/Runter** Level wechseln, **T/Enter** Testspiel (nach dem Treffer oder mit Esc zurueck in den Editor),
  **S** speichern, **Esc** speichern + Menue.
- Die orange Leiste oben ist der Startbereich von Ball/Fluid - dort keine Waende setzen.
- Gespeichert wird bei jeder Aenderung in `box_levels.txt` im beschreibbaren Pfad (`FileUtils::getWritablePath()`);
  beim Start ueberschreiben diese Levels die eingebauten.

Schleim-Verlauf (gruen): `drawColoredTriangle` mit Eckenfarben. `mb::buildShaded()` (MetaballMesh.h) erzeugt per Clipper2 Ringe
(`InflatePaths` nach innen), Baender (`Difference`) und Delaunay-Dreiecke; jede Ecke kennt ihren Ring und bekommt dessen Farbe
(`kWater[0..3]`, Alpha steigt zum Kern). Ring-Abstaende: `MBPalette::inset` (Standard 0 / 3,5 / 8 / 13 px).

Sound: `Content/sfx/*.wav` (11 Effekte: click, select, start, hit_soft, hit_hard, win, yeah, level, pop, erase, goal) nach
`Content/` deines Projekts kopieren, sodass `sfx/click.wav` gefunden wird. Die WAVs sind synthetisch und werden mit
`python3 tools/make_sfx.py Content/sfx` (numpy) erzeugt - einfach durch eigene Dateien gleichen Namens ersetzbar.
Ausloeser: Menue (Auswahl/Klick/Start), Aufprall des Balls (leise/hart je nach Wucht), Ziel erreicht,
Levelwechsel, "Yeaahhh" (Formant-Synthese) beim Erreichen des Ziels, Editor (Wand/Ziel setzen, loeschen). **M** oder Klick auf den Lautsprecher schaltet den Ton an/aus.
Der Ball ist im Spiel wie im Menue schattiert gezeichnet (Licht oben links, Rollmarke dreht sich mit).

Glanz-Schalter: Variable `_shineOn` (Standard aus). Im Menue links unten (Funkel-Symbol) per Klick oder **G** (auch im Spiel) schaltet
den Glanz auf dem Schleim an/aus; aus = das zweite Glanz-Feld wird gar nicht erst berechnet (spart Rechenzeit).

Der Ton ist standardmaessig **aus** (`_sfxOn = false` in `BoxScene.h`); mit **M** oder per Klick auf den Lautsprecher schaltest du ihn ein.

Mobile / Touch:
- Spiel: zwei grosse Tasten unten links/rechts (gegen/im Uhrzeigersinn drehen, gedrueckt halten, Multitouch moeglich, wie die Pfeiltasten),
  Mitte unten = Zurueck (Menue bzw. Editor), Lautsprecher rechts = Ton an/aus.
- Menue: alles per Tippen (Karte = Start, Pfeile/Punkte = Level, Schalter fuer Editor und Glanz, Lautsprecher).
- Editor: Werkzeug-Leiste oben (Wand/Ziel/Radierer) und Aktionsleiste unten (Zurueck, Rueckgaengig, Alles loeschen, Level -/+,
  Testen) sind antippbar; Waende mit dem Finger ziehen.
- Die Tippbereiche gehen von der Design-Aufloesung 640x640 aus (`setDesignResolutionSize(640, 640, ResolutionPolicy::SHOW_ALL)`);
  bei anderen Seitenverhaeltnissen entstehen Raender, die Koordinaten bleiben gueltig. Auf dem Geraet Querformat/Hochformat sperren,
  wie es zu deinem Projekt passt.

Steuerung waehlbar (Menue links oben, oder Taste **C**): linkes Symbol = Bildschirm-Tasten, rechtes Symbol = **Neigung**
(Beschleunigungssensor, `Device::setAccelerometerEnabled`, nur waehrend des Spiels aktiv). Neigen nach rechts dreht den Kasten im
Uhrzeigersinn, die Wasserwaage unten zeigt die Neigung. Tastatur funktioniert in beiden Modi weiter.
Einstellungen am Anfang von `BoxScene.cpp`: `kTiltGain` (Empfindlichkeit), `kTiltDead` (Totzone), `kTiltSign` (auf -1 setzen, falls dein Geraet
die Richtung verkehrt herum meldet). Das Mapping nimmt Hochformat an (x-Achse = rechts); im Querformat musst du ggf. die Achse tauschen.
Am Desktop gibt es keinen Sensor - dort bleibt die Neigung 0.
