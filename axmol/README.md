# Animatoon for Axmol 3.0

This folder is the rewrite of Animatoon (originally written in PureBasic, see the
root of the repository) for the [Axmol 3.0](https://axmol.dev) engine, in C++.

The data of the program (papers, brush images, brush presets `*.abp`, swatches `*.gpl`,
patterns, themes, languages) are the ones of the PureBasic version: they stay in
`<repo>/data` and are linked into `Content/data` when CMake configures the project.
The `.ani` documents (zip of one png per layer + a text file) can be read and written
by both versions.

## Build

1. Install Axmol 3.0 (`dev` branch at the time of writing) and run its `setup.ps1`
   so that the `AX_ROOT` environment variable is set.
2. From this folder:

```bash
axmol build
```

   or directly with CMake:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build
```

The program is `build/bin/Animatoon/Animatoon`. Only the 2D core of Axmol and the
ImGui extension are built (see `cmake/modules/AXGameEngineOptions.cmake`).

Tested on Linux (OpenGL). The `proj.win32` and `proj.ios_mac/mac` folders are the ones
of the Axmol template; they have not been built here.

## How it is made

| File | Content |
| --- | --- |
| `Source/core/AniImage.*` | CPU picture, RGBA8 with straight alpha (the PureBasic images) |
| `Source/core/BlendModes.*` | the blend modes of the layers (same order as `#Bm_xxx`) |
| `Source/core/Document.*` | paper, layers, selection, undo/redo, compositing |
| `Source/core/Brush.*` | tool parameters, `*.abp` presets, brush tip, stroke engine (dots, scatter, rotation, colour mixing, water, symmetry) |
| `Source/core/Tools.*` | anti-aliased line/box/ellipse, gradients, flood fill, adjustments and filters |
| `Source/core/AppData.*` | `Pref.ini`, languages, swatches, templates, `.ani` documents |
| `Source/core/Util.*` | paths, PureBasic preference files, zip |
| `Source/Editor.*`, `Source/EditorUI.cpp` | the editor: tools, input, menus, panels and dialogs (Dear ImGui) |
| `Source/MainScene.*` | the Axmol scene: one `ax::Sprite` showing the picture + the ImGui render loop |

The document is composited on the CPU (only the modified rectangle) and uploaded to
the texture of the sprite with `Texture2D::updateSubData`. Zoom and pan are the scale
and position of the sprite.

## What is ported

- Tools: pen, brush, spray, pattern stamp, eraser, line, box, circle, gradient, fill,
  text, colour picker, selection, move, hand, zoom, clear.
- Brush engine: brush images, size/opacity (with the pressure of a pen through
  `PointerEvent::getPressure()`), random size/opacity/rotation, scatter, spacing,
  hardness/intensity, colour mixing (4 types, viscosity), water, symmetry (X, Y, both,
  4 views), tool types (light, dark, colour, water, smudge, glass, noise, pixel, blur,
  solarize), lock alpha.
- Layers: add, duplicate, delete, move, merge (down / all / visible / linked), opacity,
  blend modes, locks, rename.
- Paper (background) with texture, colour, scale, contrast and brightness, and presets.
- File: new (templates), open/save `.ani`, open/import/save pictures, export of the layers.
- Edit: undo/redo, cut/copy/paste, clear, fill, fill with pattern.
- Image: resize document/canvas, crop, trim, rotate, mirror, brightness/contrast, level,
  colour balance, hue/saturation, posterize, desaturation, inverse.
- Filters: noise, clouds, blur, sharpen, offset.
- Swatches (`.gpl`), brush presets (bank / groups, save of a preset), patterns,
  languages (`data/Lang/*.ini`), preferences.

## Not ported

Things that were unfinished or Windows only in the PureBasic version: scripts, the
rough board, plugins (`.dll`), particles / shape / transform tools, layer styles and
alpha masks, zip export of the layers, the old `.abi` / `.teo` formats. Preferences
are saved in the writable folder of the user instead of next to the program.

## Shortcuts

`B` brush, `P` pen, `E` eraser, `K` fill, `G` gradient, `L` line, `T` text, `I` colour
picker, `M` selection, `V` move, `H` hand, `Z` zoom, `D` / `F` size - / +, `X` swap the
colours, `Alt`+click picks a colour, space or middle button pans, wheel zooms.
`Ctrl+N/O/S/I/E`, `Ctrl+Z/Y`, `Ctrl+X/C/V`, `Ctrl+A/D`, `Ctrl+W` clear, `Ctrl+K` fill,
`Ctrl+G` grid, `Ctrl +/-/0` zoom.
