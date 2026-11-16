// Animatoon - Axmol 3.0 port
#pragma once

#include "axmol/axmol.h"
#include "Editor.h"

/// The only scene of the program : a sprite showing the picture, and the editor (Dear ImGui) on top.
class MainScene : public ax::Scene
{
public:
    bool init() override;
    void onEnter() override;
    void onExit() override;

private:
    void onImGuiDraw();

    Editor _editor;
    ax::Sprite* _canvas = nullptr;
    float _pressure     = 1.f;
    bool _pen           = false;
    bool _started       = false;
};
