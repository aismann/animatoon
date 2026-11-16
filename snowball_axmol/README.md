# Snowball – Axmol 3.0 port

Port of **Snowball** (PureBasic, XMas 2014 PurePunch contest) by Majikeyric to
[Axmol](https://axmol.dev) 3.0 (C++). Original C64 graphics by Ashley Routledge.

You are the blue player. Hit another player: +1 point, get hit: −1 point. First to 10 wins.

| Key | Action |
|---|---|
| ← / → | move |
| ↓ | pick up snow |
| Space | throw (also restarts after the game is over) |
| Esc | quit |

## Build

```bash
export AX_ROOT=/path/to/axmol        # Axmol 3.0 checkout, set up with setup.ps1
cd snowball_axmol
axmol build -p linux                 # or: -p win32 / -p osx / -p android ...
```

## What was ported

* **Game logic** (`Source/GameScene.cpp`) – 1:1 from the PureBasic main loop: actor state machine,
  bot AI (choose target → pick up snow → keep 194 px distance → shoot → retreat), ballistic snowballs
  (`vx=±7`, `vy=-2.5`, gravity 0.2), pixel-perfect hit test, camera scrolling, 200 snow flakes,
  score/win/lose. The logic runs at a fixed 60 Hz like the original's vsync-bound loop.
* **Graphics** (`buildTextures()`) – the C64 multicolor sprites/characters embedded in the original are
  decoded at start-up into RGBA textures (2× zoom, nearest filtering), one atlas for sprites
  (3 players × left/right) and one for the 256 tiles. Coordinates are converted from PureBasic's
  top-left origin to Axmol's bottom-left origin.
* **Data** (`Source/GameData.cpp`) – the LZMA-packed hex `Data` block of the original was unpacked
  with `tools/extract_assets.py` (palette, sprites, charset, 80×25 tile map, animation tables).
* **Music** – Axmol's `AudioEngine` cannot play MIDI, so the original MIDI is rendered by
  `tools/extract_assets.py` into `Content/music.ogg` with a small sine-based synthesizer
  (it sounds like a music box, not like the original General-MIDI playback). Replace the file if you
  have a better rendering.

Original source: `original/snowball.pb`. Regenerate generated files: `python3 tools/extract_assets.py`
(needs numpy + ffmpeg).

`Content/fonts/arial.ttf` is the font shipped with Axmol's project template (used for score / end text).

## Known differences

* Score and end-game texts use outlined text instead of PureBasic's default `DrawText` look; the
  typo "You loose !" became "You lose !".
* The original keeps the MIDI player running through `MovieStatus`; here the OGG simply loops.
