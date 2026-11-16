// Axmol 3.0 port of Demo.pb and Example01-03.pb.
//
//   director->runWithScene(d3d::Drawing3DDemo::create());       // Demo.pb
//   director->runWithScene(d3d::Drawing3DExample::create(1));   // Example01.pb (2, 3 likewise)
//
// The demo expects "Image.png" (texture of scene 5) in the resources, a window of at least 900x600
// and a Unicode-capable system font for the German captions.
#pragma once

#include "Drawing3D.h"
#include "Drawing3DAx.h"
#include "axmol.h"

namespace d3d
{

class Drawing3DDemo : public ax::Scene
{
public:
    static Drawing3DDemo* create();
    bool init() override;

private:
    void selectScene(int index);
    void redraw();
    void rotateBy(float dxPixels, float dyPixelsDown);
    void saveImage();

    Drawing3D g_;
    Image3DPtr image_;
    ax::DrawNode* canvas_ = nullptr;
    ax::Label* title_     = nullptr;
    ax::Label* status_    = nullptr;
    std::vector<ax::Label*> items_;

    int scene_      = 11;      // SetGadgetState(#ListViewGadget, 11)
    float distance_ = 700.0f;  // Static Distance.f = 700
    bool dragging_  = false;

    static constexpr int kCanvas = 580;
};

// Example01.pb ("Cube with lines"), Example02.pb ("Cube with light"), Example03.pb ("Two transparent cubes")
class Drawing3DExample : public ax::Scene
{
public:
    static Drawing3DExample* create(int example);
    bool init(int example);
};

}  // namespace d3d
