# Drawing3D for Axmol 3.0

Port of the PureBasic module `Drawing3D.pbi` (plus `Demo.pb`, `Example01-03.pb`) to C++ / Axmol 3.0,
rendering through `ax::DrawNode`.

| File | Content |
|---|---|
| `Drawing3D.h/.cpp` | Renderer core (no Axmol dependency): camera, rotation, lights, clipping, projection, depth sort |
| `Drawing3DAx.h/.cpp` | Axmol glue: `renderToDrawNode`, `Image3D` from `ax::Image`, text rasteriser, PNG export |
| `DrawNode3D.h/.cpp` | Axmol's `DrawNode3D` (3D lines/cubes with GPU depth test), taken from the Axmol cpp-tests |
| `Drawing3DScenes.h/.cpp` | The 13 scenes of `Demo.pb` and the three examples (no Axmol dependency) |
| `Drawing3DDemo.h/.cpp` | `Drawing3DDemo` (Demo.pb UI) and `Drawing3DExample` (Example01-03) as `ax::Scene` |
| `demo/Image.png` | Texture for scene 5 (copy it into the app's Resources) |
| `test/` | Headless test: renders all scenes with the CPU rasteriser to PNG |

## Usage

```cpp
#include "Drawing3DAx.h"

d3d::Drawing3D g;
auto* node = ax::DrawNode::create();           // put it in a ClippingRectangleNode to cut at the canvas
addChild(node);

g.start(500, 500);                             // StartDrawing3D
g.setBackground(0xFFFFFFFF);                   // Drawing3DBackground
g.setPosition(0, 0, -700);
g.setRotation(30, 30, 0);
g.addLight(1, 1, 1, 0x40FFFFFF);
g.drawBox3D(0, 0, 0, 200, 200, 200, 30, 30, 0, 0xFF804000);
g.stop();                                      // StopDrawing3D (sorts)
d3d::renderToDrawNode(g, node);                // fills the DrawNode
```

Add the `.cpp` files to the target (`target_sources(...)`), and run `Drawing3DDemo::create()` or
`Drawing3DExample::create(1..3)` as the first scene. The demo needs a window of at least 900x600.

Colours use the PureBasic layout `$AABBGGRR` (`d3d::rgba(r,g,b,a)` == `RGBA()`), so literals from the
`.pb` files can be reused unchanged. Angles are in degrees, as in the original.

## PureBasic -> C++

| PureBasic | C++ (`d3d::Drawing3D`) |
|---|---|
| `StartDrawing3D(out, fov)` / `StopDrawing3D()` | `start(w, h, fov)` / `stop()` + `renderToDrawNode()` |
| `Drawing3DMode`, `Position`, `Rotation`, `Background`, `Light` | `setMode`, `setPosition`, `setRotation`, `setBackground`, `addLight` (`Move::Relative` = `#PB_Relative`) |
| `DrawPoint3D`, `DrawLine3D`, `DrawTriangle3D`, `DrawPlane3D`, `DrawDisk3D`, `DrawBox3D`, `DrawCylinder3D`, `DrawImage3D`, `DrawTranformedImage3D`, `DrawText3D` | same names, lower-case first letter (`drawTransformedImage3D` fixes the typo) |
| `#Drawing3D_Ignore` | `d3d::Ignore` |
| `CreateImage3D(#id, image)` / `FreeImage3D` | `image3DFromFile()` / `image3DFromImage()` returning a `shared_ptr` (no numeric ids) |
| `Drawing3DStyle`, clusters | not ported (empty / unfinished in the original) |

## Wireframes with DrawNode3D

`Drawing3D` also records every line (outline mode, `drawLine3D`) as a real 3D segment (`lines3D()`, scene space).
`d3d::renderLinesToDrawNode3D(g, node3d)` fills an `ax::DrawNode3D` with them, so wireframes can be drawn by an
`ax::Camera` with a hardware depth test instead of the projected 2D path. `DrawNode3D::drawLine` takes one colour
per line, so gradient lines use the mean colour. `DrawNode3D.cpp` must be part of the target (it is the Axmol test class).

## Differences to the original (DrawNode has no per-pixel depth buffer)

* **Depth order**: the original sorts per pixel. Here whole triangles are sorted far-to-near
  (painter's algorithm). Surfaces that *penetrate* each other are only resolved by splitting:
  `setSubdivision(px)` splits triangles whose projected edge is longer than `px` (the demo uses it for the
  intersecting planes/boxes/cylinders). Intersection edges are therefore as coarse as that cell size.
* **Textures** (`drawImage3D`, `drawText3D`): DrawNode cannot sample textures, so the image is sampled at the
  vertices of a fine triangle grid (`setImageDetail(px)`, default 8 px) and Gouraud-shaded.
* **No anti-aliasing**; lines are 1 px wide quads (`setLineWidth`), points are 1 px quads, so scene 6 ("Punkte")
  produces ~320k triangles. Lines with two colours are split so the gradient stays perspective-correct.
* `drawDisk3D` is a 48-gon fan; in outline mode it draws the rim (the original draws nothing).
* `drawText3D` uses `ax::Device::getTextureDataForText` (system font; "Verdana" must exist, otherwise the
  platform substitutes one).
* Scene 9 (3D function) uses float division. The PureBasic code divides integers there (`-X*X/100`), which gives a
  stepped surface.
* `renderToDrawNode` flips the y axis (PureBasic: origin top-left, Axmol: bottom-left).
* `Drawing3D::rasterize()` / `savePNG()` is a small CPU rasteriser; it backs the "Save real 32Bit-Image" button and the tests.

## Headless test

```bash
g++ -std=c++17 -O2 -I. test/test_render.cpp Drawing3D.cpp Drawing3DScenes.cpp -o test_render
./test_render outdir image.rgba && python3 test/to_png.py outdir     # image.rgba = raw 256x256 RGBA of demo/Image.png
```
