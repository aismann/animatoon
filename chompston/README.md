# Chompston – Axmol 3.0 Port (nur DrawNode)

Port des PureBasic-Spiels *Chompston* nach **Axmol 3.0** (C++20).

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

## Steuerung

| Taste | Aktion |
|---|---|
| ←/→ oder A/D | links/rechts abbiegen (an der nächsten Kreuzung, im Stand sofort) |
| ↓ oder S | umdrehen |
| Leertaste / ↑ / Strg | Bombe (Radius ~3 Felder) |
| P / Esc | Pause |
| Enter | Start / nach Game Over zurück zum Titel |
| M / V / F / G | Musik an/aus · Lautstärke 0–5 · Fenster/Vollbild · Glättung |
| Touch/Maus | linke/rechte Bildhälfte = abbiegen, unten Mitte = Bombe |

## Konfiguration

`settings.ini`: `Windowwidth` (quadratisches Fenster), `Vsync`, `FrameSkip` (0 = variabler Schritt, >0 = fester 60-Hz-Schritt mit
bis zu n Aufhol-Schritten), `StoreUserDataLocally` (1 = `chomp.cfg` neben dem Spiel, 0 = Benutzerverzeichnis).
`JoystickId`, `AudioOutputBufferSize`, `AudioUpdatePeriod` sind BASS-spezifisch; sie werden gelesen und erhalten,
haben aber in Axmol keine Wirkung.

`chomp.cfg`: `AudioVolume` (0–5, Skala angenommen), `Music`, `Windowed`, `Smoothing` (1 = Polygonkanten mit 1 px Rand),
`HiScore`.
