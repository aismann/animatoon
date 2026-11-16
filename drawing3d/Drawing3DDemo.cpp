#include "Drawing3DDemo.h"

#include <chrono>
#include <cmath>

#include "Drawing3DScenes.h"

using namespace ax;

namespace d3d
{

namespace
{
constexpr float kPi = 3.14159265358979323846f;
const Vec2 kTopLeft(0.0f, 1.0f);  // anchor point

Label* makeLabel(std::string_view text, float size)
{
    // createWithSystemFont uses the platform font renderer, so umlauts work without bundling a TTF.
    return Label::createWithSystemFont(text, "Arial", size);
}
}  // namespace

// ---------------------------------------------------------------------------
// Demo.pb
// ---------------------------------------------------------------------------

Drawing3DDemo* Drawing3DDemo::create()
{
    auto* p = new (std::nothrow) Drawing3DDemo();
    if (p && p->init())
    {
        p->autorelease();
        return p;
    }
    delete p;
    return nullptr;
}

bool Drawing3DDemo::init()
{
    if (!Scene::init())
        return false;

    image_ = image3DFromFile("Image.png");
    g_.setTextRasterizer(makeTextRasterizer("Verdana", 24));

    // Canvas (CanvasGadget 10,10,580,580): clipped so geometry cannot spill over the panel.
    auto* clip = ClippingRectangleNode::create(Rect(0, 0, kCanvas, kCanvas));
    clip->setPosition(10, 10);
    canvas_ = DrawNode::create();
    clip->addChild(canvas_);
    addChild(clip);

    // Scene list (ListViewGadget)
    auto* frame = makeLabel("Szenenauswahl", 16);
    frame->setAnchorPoint(kTopLeft);
    frame->setPosition(610, 590);
    addChild(frame);

    for (int i = 0; i < kDemoSceneCount; ++i)
    {
        auto* label = makeLabel(demoSceneName(i), 14);
        label->setAnchorPoint(kTopLeft);
        label->setPosition(615, 560 - i * 22.0f);
        addChild(label);
        items_.push_back(label);
    }

    title_ = makeLabel("Left mouse to rotate the scene, wheel to zoom", 13);
    title_->setAnchorPoint(kTopLeft);
    title_->setPosition(610, 260);
    addChild(title_);

    status_ = makeLabel("", 13);
    status_->setAnchorPoint(kTopLeft);
    status_->setPosition(610, 235);
    addChild(status_);

    auto* save = MenuItemLabel::create(makeLabel("[ Save real 32Bit-Image ]", 14), [this](Object*) { saveImage(); });
    save->setAnchorPoint(kTopLeft);
    auto* menu = Menu::create(save, nullptr);
    menu->setPosition(610, 190);
    addChild(menu);

    // One pointer listener for mouse, touch and pen.
    auto* listener = PointerEventListener::create();
    listener->onPointerDown = [this](PointerEvent* e) {
        const Vec2 p = e->getWorldPoint();
        // click in the scene list?
        for (int i = 0; i < static_cast<int>(items_.size()); ++i)
        {
            if (items_[i]->getBoundingBox().containsPoint(p))
            {
                selectScene(i);
                return true;
            }
        }
        // press inside the canvas starts a rotation
        if (Rect(10, 10, kCanvas, kCanvas).containsPoint(p))
        {
            dragging_ = true;
            return true;
        }
        return false;
    };
    listener->onPointerMove = [this](PointerEvent* e) {
        if (!dragging_)
            return;
        const Vec2 d = e->getWorldPoint() - e->getPrevWorldPoint();
        rotateBy(d.x, -d.y);  // PureBasic's y axis points down
    };
    listener->onPointerUp     = [this](PointerEvent*) { dragging_ = false; };
    listener->onPointerCancel = [this](PointerEvent*) { dragging_ = false; };
    listener->onPointerScroll = [this](PointerEvent* e) {
        distance_ *= std::pow(1.1f, e->getScrollY());
        redraw();
        e->stopPropagation();
    };
    _eventDispatcher->addEventListenerWithFixedPriority(listener, 1);

    g_.setRotation(30, 30, 90);  // Drawing3DRotation(30, 30, 90)
    selectScene(scene_);
    return true;
}

void Drawing3DDemo::selectScene(int index)
{
    scene_ = index;
    for (int i = 0; i < static_cast<int>(items_.size()); ++i)
        items_[i]->setColor(i == scene_ ? Color32(255, 200, 0) : Color32::white);
    redraw();
}

// Drawing3DRotation(Radian(dy)*15, Radian(dx)*15, 0, #PB_Relative)
// (the original passes radians where degrees are expected, so the effective gain is ~0.26 deg/pixel)
void Drawing3DDemo::rotateBy(float dx, float dyDown)
{
    g_.setRotation(dyDown * (kPi / 180.0f) * 15.0f, dx * (kPi / 180.0f) * 15.0f, 0, Move::Relative);
    redraw();
}

void Drawing3DDemo::redraw()
{
    const auto t0 = std::chrono::steady_clock::now();

    g_.start(kCanvas, kCanvas);
    g_.setBackground(0xFFFFFFFF);
    g_.setPosition(0, 0, -distance_);
    drawDemoScene(g_, scene_, image_);
    g_.stop();
    renderToDrawNode(g_, canvas_);

    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    status_->setString(fmt::format("Renderzeit: {} ms ({} Dreiecke)", ms, g_.triangles().size()));
}

// Button "Save real 32Bit-Image": renders 600x600 with a transparent background into a PNG.
void Drawing3DDemo::saveImage()
{
    g_.start(600, 600);
    g_.setBackground(0x00FFFFFF);
    g_.setPosition(0, 0, -distance_);
    drawDemoScene(g_, scene_, image_);
    g_.stop();

    const std::string path = FileUtils::getInstance()->getWritablePath() + "Drawing3D.png";
    const bool ok          = savePNG(g_, path);
    redraw();
    status_->setString(ok ? "Gespeichert: " + path : "Speichern fehlgeschlagen");
}

// ---------------------------------------------------------------------------
// Example01-03.pb
// ---------------------------------------------------------------------------

Drawing3DExample* Drawing3DExample::create(int example)
{
    auto* p = new (std::nothrow) Drawing3DExample();
    if (p && p->init(example))
    {
        p->autorelease();
        return p;
    }
    delete p;
    return nullptr;
}

bool Drawing3DExample::init(int example)
{
    if (!Scene::init())
        return false;

    constexpr int kSize = 500;  // OpenWindow(#Window, 0, 0, 500, 500, ...)
    static Drawing3D g;         // examples draw once; keep it simple
    switch (example)
    {
    case 1: drawExample01(g, kSize, kSize); break;
    case 2: drawExample02(g, kSize, kSize); break;
    default: drawExample03(g, kSize, kSize); break;
    }

    auto* clip = ClippingRectangleNode::create(Rect(0, 0, kSize, kSize));
    auto* node = DrawNode::create();
    clip->addChild(node);
    addChild(clip);
    renderToDrawNode(g, node);
    return true;
}

}  // namespace d3d
