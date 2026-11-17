// Renders the frames produced by the game thread and forwards keyboard / mouse input to it.
#pragma once

#include <array>
#include <vector>

#include "axmol/axmol.h"
#include "pb/Platform.h"

class MainScene : public ax::Scene
{
public:
    bool init() override;
    void update(float delta) override;
    ~MainScene() override;

private:
    struct FontCfg
    {
        const char* file;
        float size;
        float lineHeight;
    };

    void measureFonts();
    ax::Texture2D* loadSheetTexture(const std::string& relPath);
    void present(const std::vector<pb::Cmd>& cmds, uint32_t clearColor);
    ax::Sprite* nextSprite();
    ax::Label* nextLabel(int font);

    ax::Node* _layer = nullptr;
    ax::LayerColor* _background = nullptr;
    std::vector<ax::Sprite*> _sprites;
    size_t _spriteUsed = 0;
    std::array<std::vector<ax::Label*>, pb::FONT_COUNT> _labels;
    std::array<size_t, pb::FONT_COUNT> _labelUsed{};
    std::vector<ax::Texture2D*> _textures;  // per sheet
    ax::Texture2D* _white = nullptr;
    int _z = 0;
    bool _started = false;
    bool _altDown = false;

    ax::EventListenerKeyboard* _keyboard = nullptr;
    ax::PointerEventListener* _pointer = nullptr;
};
