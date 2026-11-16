// Animatoon - Axmol 3.0 port
// The document : paper (background), layers, selection, undo history and the
// compositing of everything into the picture shown on screen.
#pragma once

#include "AniImage.h"
#include "BlendModes.h"

#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace ani
{

enum LayerType
{
    Layer_TypBitmap = 0,
    Layer_TypText,
    Layer_TypBG,
    Layer_TypShape,
    Layer_TypGroup,
    Layer_TypVecto
};

struct Layer
{
    std::string name;
    int uid       = 0; ///< unique id, used by the undo history
    Image image;
    int type      = Layer_TypBitmap;
    int bm        = Bm_Normal;
    int alpha     = 255;
    bool view     = true;
    bool locked   = false; ///< lock everything
    bool lockAlpha = false;
    bool lockMove  = false;
    bool lockPaint = false;
    bool link      = false;
    bool okForColorPick = true;
    // text layers
    std::string text, fontName = "arial";
    int fontSize = 24;
    Color fontColor{0, 0, 0, 255};
    // temporary offset used while the layer is moved with the move tool
    int offX = 0, offY = 0;
};

struct Paper
{
    std::string name = "paper0.png"; ///< file in data/Paper
    int alpha       = 255;           ///< opacity of the texture over the colour
    int scale       = 10;            ///< 10 = 100 %
    int intensity   = 0;             ///< contrast of the texture
    int brightness  = 0;             ///< brightness of the texture
    Color color{255, 255, 255, 255};
    int pos         = 100;
    Image texture;       ///< original picture
    Image tile;          ///< picture after scale / contrast / brightness
    void rebuildTile();
};

struct Selection
{
    bool active = false;
    IRect rect;
};

class Document
{
public:
    Document();

    void create(int w, int h, bool transparentLayer = true);
    int width() const { return _w; }
    int height() const { return _h; }
    IRect rect() const { return {0, 0, _w, _h}; }

    std::string name = "Untitled";
    std::string filePath;
    int dpi          = 100;
    bool changed     = false;

    Paper paper;
    Selection selection;
    std::vector<Layer> layers; ///< index 0 = bottom
    int layerId = 0;           ///< current layer

    Layer& cur() { return layers[std::clamp(layerId, 0, (int)layers.size() - 1)]; }
    bool canPaint() const;

    // ---- layers ----
    int addLayer(const std::string& name = "", int index = -1);
    void deleteLayer(int i);
    void duplicateLayer(int i);
    void moveLayer(int i, int dir);
    /// mode 0 = merge down, 1 = merge all, 2 = merge visible, 3 = merge linked
    void mergeLayers(int mode);
    void clearLayer(int i);
    void fillLayer(int i, const Color& c);
    void fillLayerPattern(int i, const Image& pattern);

    // ---- document transforms (act on every layer) ----
    void resizeDocument(int w, int h, bool smooth);                ///< scales the pictures
    void resizeCanvas(int w, int h, int anchorX = 0, int anchorY = 0); ///< keeps the pictures, anchor -1/0/1
    void crop(const IRect& r);
    void rotate90(int quarterTurns);
    void mirror(bool horizontal, bool allLayers);

    // ---- rendering ----
    /// Marks an area of the picture as needing to be composited again
    void invalidate(const IRect& r);
    void invalidateAll() { invalidate(rect()); }
    /// Composite the dirty area. Returns the rect which changed (empty if nothing)
    IRect updateComposite();
    const Image& composite() const { return _composite; }
    /// Flattened picture. withPaper = false gives a transparent background.
    Image flatten(bool withPaper, bool onlyVisible = true) const;
    /// Colour of the picture under a point. mode 0 = all layers, 1 = current layer, 2 = current + below
    Color pickColor(int x, int y, int mode) const;

    // ---- undo / redo ----
    int maxUndo = 16;
    /// Save the pixels of the layer before it is modified (stroke, fill, filter...)
    void pushUndoLayer(int layerIndex, const char* label);
    /// Save the whole document (layer list, size) before a structural change
    void pushUndoDocument(const char* label);
    bool undo();
    bool redo();
    bool canUndo() const { return !_undo.empty(); }
    bool canRedo() const { return !_redo.empty(); }
    const char* undoLabel() const { return _undo.empty() ? "" : _undo.back()->label.c_str(); }
    const char* redoLabel() const { return _redo.empty() ? "" : _redo.back()->label.c_str(); }
    void clearHistory();

    int layerIndexFromUid(int uid) const;

private:
    struct State
    {
        std::string label;
        bool fullDoc = false;
        // layer state
        int layerUid = 0;
        Image image;
        // full document state
        int w = 0, h = 0, layerId = 0;
        std::vector<Layer> layers;
    };
    std::unique_ptr<State> captureLike(const State& s) const;
    void apply(const State& s);
    void compositeRect(const IRect& r, Image& dst, bool withPaper, bool onlyVisible, int maxLayer) const;

    int _w = 0, _h = 0;
    int _nextUid = 1;
    Image _composite;
    IRect _dirty;
    std::deque<std::unique_ptr<State>> _undo, _redo;
};

} // namespace ani
