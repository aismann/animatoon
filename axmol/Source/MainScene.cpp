#include "MainScene.h"
#include "ImGui/ImGuiPresenter.h"
using namespace ax;
USING_NS_AX_EXT;
bool MainScene::init() { return Scene::init(); }
void MainScene::onEnter()
{
    Scene::onEnter();
    ImGuiPresenter::getInstance()->addRenderLoop("#ani", AX_CALLBACK_0(MainScene::onImGuiDraw, this), this);
}
void MainScene::onExit()
{
    ImGuiPresenter::getInstance()->removeRenderLoop("#ani");
    Scene::onExit();
}
void MainScene::onImGuiDraw()
{
    ImGui::Begin("window");
    ImGui::Text("hello");
    ImGui::End();
}
