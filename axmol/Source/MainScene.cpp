// Animatoon - Axmol 3.0 port
#include "MainScene.h"

#include "ImGui/ImGuiPresenter.h"

using namespace ax;
USING_NS_AX_EXT;

bool MainScene::init()
{
    if (!Scene::init())
        return false;

    // The picture : the document is composited on the CPU into a texture shown by this sprite
    _canvas = Sprite::create();
    _canvas->setAnchorPoint(Vec2(0, 1));
    addChild(_canvas);

    // The pressure of a pen / tablet comes with the pointer events of Axmol 3
    auto listener = PointerEventListener::create();
    auto track    = [this](PointerEvent* ev) {
        _pen      = ev->getPointerType() == PointerType::Pen || ev->getPointerType() == PointerType::Touch;
        _pressure = _pen ? ev->getPressure() : 1.f;
    };
    listener->onPointerDown = [track](PointerEvent* ev) {
        track(ev);
        return true;
    };
    listener->onPointerMove = track;
    listener->onPointerUp   = [this](PointerEvent*) { _pressure = 1.f; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    return true;
}

void MainScene::onEnter()
{
    Scene::onEnter();
    auto presenter = ImGuiPresenter::getInstance();
    presenter->addFont(FileUtils::getInstance()->fullPathForFilename("fonts/arial.ttf"), 15.f);
    presenter->addRenderLoop("#animatoon", AX_CALLBACK_0(MainScene::onImGuiDraw, this), this);
}

void MainScene::onExit()
{
    _editor.shutdown();
    ImGuiPresenter::getInstance()->removeRenderLoop("#animatoon");
    Scene::onExit();
}

void MainScene::onImGuiDraw()
{
    // The design resolution follows the size of the window : 1 unit = 1 pixel of the window
    auto director = Director::getInstance();
    auto view     = director->getRenderView();
    Vec2 win      = view->getWindowSize();
    if (win.x > 0 && win.y > 0 && !win.equals(director->getCanvasSize()))
        view->setDesignResolutionSize(win.x, win.y, ResolutionPolicy::EXACT_FIT);

    if (!_started)
    {
        _started = true;
        _editor.init(_canvas);
    }
    _editor.frame(_pressure, _pen);
}
