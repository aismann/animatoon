// The scenes of Demo.pb and Example01-03.pb, ported to d3d::Drawing3D.
// No Axmol dependency: they only talk to the Drawing3D class.
#pragma once
#include "Drawing3D.h"

namespace d3d
{

constexpr int kDemoSceneCount = 13;

// German captions of the ListViewGadget in Demo.pb.
const char* demoSceneName(int index);

// Draws scene `index` (0..12) of Demo.pb. The caller has already called start() and
// setBackground(), and calls stop() afterwards.
// `image` is the texture for scene 5, the text rasteriser of `g` is used for scene 12.
void drawDemoScene(Drawing3D& g, int index, const Image3DPtr& image);

// Example01.pb "Cube with lines", Example02.pb "Cube with light", Example03.pb "Two transparent cubes".
// Each does the whole start()/draw/stop() sequence for a canvas of the given size.
void drawExample01(Drawing3D& g, int width, int height);
void drawExample02(Drawing3D& g, int width, int height);
void drawExample03(Drawing3D& g, int width, int height);

}  // namespace d3d
