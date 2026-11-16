#pragma once
#include "axmol/axmol.h"
class MainScene : public ax::Scene
{
public:
    bool init() override;
    void onEnter() override;
    void onExit() override;
    void onImGuiDraw();
};
