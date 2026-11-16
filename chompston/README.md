# Chompston – Axmol 3.0 Port (nur DrawNode)

Port des PureBasic-Spiels *Chompston* nach **Axmol 3.0** (C++20), geprüft gegen den aktuellen `dev`-Stand der Engine
(neue API: `PointerEventListener`, `KeyboardEvent`, `applicationWillLaunch`, `RenderView`). Bitte immer die neueste Axmol-Version verwenden.

* **Kein Sprite, keine Textur, kein Label** – alles (3D-Labyrinth, Würfel, Schatten, Explosionen,
  HUD, Pixel-Schrift, Titel-Logo) wird mit `ax::DrawNode` gezeichnet (`drawSolidPoly` / `drawSolidRect` / `drawSolidCircle`).
* **BASS → `ax::AudioEngine`**: `bass.dll/.lib/.pbi` werden nicht mehr gebraucht. Die Sounds
  werden beim ersten Start als WAV synthetisiert (`<writable path>/chompston_audio/`).
* `settings.ini` und `chomp.cfg` werden im bisherigen Format gelesen/geschrieben (`Content/`).

> Hinweis: Der PureBasic-Quellcode lag nicht vor, nur die Assets/Configs/Screenshots.
> Spielregeln, Labyrinth-Generator und Steuerung sind deshalb eine **Rekonstruktion** nach den Screenshots
> (Verfolger-Kamera hinter gelbem Chomp-Würfel, lila Gegner, Herzen, Bomben, Highscore).

## Bauen

```bash
# einmalig: Axmol 3.0 installieren und AX_ROOT setzen (siehe https://axmol.dev)
cd chompston
axmol build -p linux     # oder: -p win32 / mac
# oder direkt mit CMake:
cmake -S . -B build && cmake --build build
```

## Ansicht

Fenster 4:3 (768×576, Design 512×384). Titelbild mit animiertem Regenbogen-Logo (Sinuswelle), Menü START / OPTIONS / EXIT.
Im Spiel (Standard): feste 2D-Draufsicht, Gänge sind die erhöhten Flächen (lila Kantenband), Wände sind schwarze Gruben,
bunte Pellets, Herzen oben links. Mit **C** wechselst du zur 3D-Verfolgerkamera.

## Steuerung

| Taste | Aktion |
|---|---|
| Pfeile / WASD | Flache Ansicht: absolute Richtung (wird an der nächsten Kreuzung übernommen). Verfolgerkamera: ←/→ abbiegen, ↑/↓ umdrehen |
| Menü | ↑/↓ wählen, Enter/Leertaste bestätigen, ←/→ Optionen ändern, Esc zurück |
| Leertaste / Strg | Bombe (Radius ~3 Felder) |
| C | Kamera: flache Draufsicht / 3D-Verfolger |
| P / Esc | Pause |
| M / V / F / G | Musik an/aus · Lautstärke 0–5 · Fenster/Vollbild · Glättung |
| Maus/Touch | Menüeintrag antippen; im Spiel Richtung zum Tippunkt (flache Ansicht) |

## Konfiguration

`settings.ini`: `Windowwidth` (Höhe = 3/4 der Breite), `Vsync`, `FrameSkip` (0 = variabler Schritt, >0 = fester 60-Hz-Schritt mit
bis zu n Aufhol-Schritten), `StoreUserDataLocally` (1 = `chomp.cfg` neben dem Spiel, 0 = Benutzerverzeichnis).
`JoystickId`, `AudioOutputBufferSize`, `AudioUpdatePeriod` sind BASS-spezifisch; sie werden gelesen und erhalten,
haben aber in Axmol keine Wirkung.

`chomp.cfg`: `AudioVolume` (0–5, Skala angenommen), `Music`, `Windowed`, `Smoothing` (1 = Polygonkanten mit 1 px Rand),
`HiScore`.
