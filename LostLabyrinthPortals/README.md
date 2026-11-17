# Lost Labyrinth: Portals – Axmol 3.0 Port

Port des PureBasic-Spiels [Lost-Labyrinth-Portals](https://github.com/aismann/Lost-Labyrinth-Portals) (3.0.2) nach C++ / **Axmol 3.0** (getestet mit dem `dev`-Stand `ec79d15`, Nachfolger von `v3.0.0-alpha34`).

## Aufbau

| Pfad | Inhalt |
|---|---|
| `Source/pb/` | Dünne „PureBasic-Schicht“: Zeichenbefehle (Blit/Box/Text), `flip()`, `delayMs()`, Tastatur/Maus, XML-Leser/-Schreiber |
| `Source/game/` | 1:1-Port der `.pb`-Dateien (`Core`=map_handling/preferences/messages/sound, `Graphics`=graphics+menu, `Character`, `Monster`, `Powers`, `Items`, `GameMain`=main.pb) |
| `Source/MainScene.cpp` | Axmol-Renderer: stellt die Frames des Spiel-Threads mit Sprites/Labels dar und gibt Eingaben weiter |
| `Source/GameHost.cpp` | Startet/stoppt den Spiel-Thread |
| `Content/` | Spieldaten (XML, Grafiken, Sounds, Karten, Tilesets) aus dem Original, Schrift DejaVu Sans |

Das Original ist ein klassisches Immediate-Mode-Programm mit blockierenden Schleifen (`FlipBuffers`, `Delay`, verschachtelte Menüs). Statt alles in Zustandsautomaten umzubauen, läuft die Spiellogik unverändert blockierend in einem eigenen Thread. Jeder `flip()` übergibt die fertige Zeichenliste an Axmol und wartet auf die Darstellung (so entsteht auch das Tempo wie bei VSync).

## Bauen (Linux)

Voraussetzungen: CMake ≥ 3.22, Ninja, GCC/Clang, PowerShell (`pwsh`, von Axmol für das Kopieren der Ressourcen benötigt), `axslcc` (Axmol-Shader-Compiler), GTK3/X11/OpenGL/ALSA-Entwicklungspakete.

```bash
git clone --depth 1 https://github.com/axmolengine/axmol axmol-sdk   # dev-Zweig
export AX_ROOT=$PWD/axmol-sdk
cmake -S LostLabyrinthPortals -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build LostLabyrinthPortals -j2
build/bin/LostLabyrinthPortals/LostLabyrinthPortals
```

Nicht benötigte Engine-Teile (Lua, FairyGUI, 3D, Physik, …) sind in `cmake/modules/AXGameEngineOptions.cmake` abgeschaltet, das verkürzt den Build deutlich.

Spielstände und Einstellungen liegen im beschreibbaren Axmol-Verzeichnis (`~/.config/.../LostLabyrinthPortals/{savegame,config}`).

## Portierungsnotizen

* PureBasic-Colorkey: Bei `DisplayTransparentSprite` gilt reines Schwarz als transparent, auch bei PNGs mit Alphakanal. Das wird beim Laden in den Alphakanal gebacken (außer bei den `light*.png`-Overlays).
* Die XML-Dateien sind ISO-8859-1 und werden beim Laden nach UTF-8 gewandelt.
* `Random(n)` liefert wie in PureBasic `0..n` inklusive.
* Quirks des Originals sind bewusst übernommen (z. B. `monster_adjust_damage` wirkt nur für Resistenz, ganzzahlige Division bei `adjustment_percentage / 100`, `character_gets_item` zählt eine Einheit mehr).

## Noch offen / nicht portiert

* Der Karteneditor (`map_editor*.pb`) und `Debug.pb` sind nicht enthalten.
* ALT+Enter (Vollbild) ist nicht verdrahtet; das Fenster ist skalierbar.
* Tastenbelegung ist fest (Pfeile, `C U P I E`), `config/keys.xml` war auch im Original leer.
* Nur Linux wurde gebaut und getestet (unter Xvfb mit Software-OpenGL). Windows/macOS sollten mit der Axmol-Standardtoolchain bauen, sind aber ungetestet.
