// Animatoon - Axmol 3.0 port
// The editor : state of the program (document, tools, view) + the Dear ImGui interface.
#pragma once

#include "axmol/axmol.h"
#include "ImGui/imgui.h"

#include "core/AppData.h"
#include "core/Brush.h"
#include "core/Document.h"
#include "core/Tools.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

/// A GPU texture made from a CPU picture (previews, thumbnails, the canvas)
class GpuImage
{
public:
    ~GpuImage();
    /// (Re)create or update the texture from the picture
    void set(const ani::Image& img, bool opaque = false);
    void updateRect(const ani::Image& img, const ani::IRect& r);
    ax::Texture2D* tex() const { return _tex; }
    int width() const { return _w; }
    int height() const { return _h; }
    void reset();

private:
    ax::Texture2D* _tex = nullptr;
    int _w = 0, _h = 0;
    std::vector<uint8_t> _tmp;
};

class FileDialog
{
public:
    using Callback = std::function<void(const std::string&)>;
    void open(bool save, const std::string& title, const std::vector<std::string>& exts, const std::string& startDir,
              const std::string& defaultName, Callback cb);
    void draw();
    bool isOpen() const { return _open; }
    const std::string& dir() const { return _dir; }

private:
    void refresh();
    bool _open = false, _save = false, _popupRequested = false;
    std::string _title, _dir;
    std::vector<std::string> _exts;
    std::vector<ani::DirEntry> _entries;
    char _name[512] = {0};
    char _dirBuf[1024] = {0};
    Callback _cb;
    bool _confirmOverwrite = false;
};

class Editor
{
public:
    Editor();
    ~Editor();

    void init(ax::Sprite* canvasSprite);
    void shutdown();
    /// One frame : interface, input, update of the canvas texture. Called from the ImGui render loop.
    void frame(float pressure, bool penDevice);

    ani::Options opt;
    ani::Document doc;

private:
    // ---- tools
    ani::BrushParams brush[ani::Action_Count];
    int action = ani::Action_Brush;
    ani::BrushEngine engine;
    bool tipDirty = true;
    ani::BrushParams& cur() { return brush[action]; }
    bool isPaintAction(int a) const;
    void setAction(int a);
    void setColor(const ani::Color& c);
    void markTip() { tipDirty = true; }
    void changeSize(int dir);

    // ---- view
    float zoom = 1.f;
    float panX = 0, panY = 0; ///< position of the top left corner of the picture, in window coordinates
    ImVec2 canvasPos, canvasSize; ///< area of the window left to the picture
    void zoomAt(float newZoom, float sx, float sy);
    void centerView();
    void updateCanvas();
    ax::Sprite* _sprite = nullptr;
    GpuImage _canvasTex;
    bool _canvasRecreate = true;

    // ---- canvas input
    void handleCanvasInput(float pressure);
    void handleShortcuts();
    void drawOverlays();
    bool _dragging = false, _panning = false;
    int _dragAction = 0;
    float _startX = 0, _startY = 0, _lastX = 0, _lastY = 0;
    ani::Image _backup;
    ani::Image _floating; ///< moved selection
    ani::IRect _moveRect;
    ani::IRect _prevDirty;
    float _mouseDocX = 0, _mouseDocY = 0;
    bool _mouseOnCanvas = false;

    // ---- actions (menus)
    void newDocument(int w, int h);
    void afterDocumentChanged(bool resetView);
    void openFile();
    void saveFile(bool saveAs);
    void saveImage();
    void importImage();
    void exportLayer();
    void exportAllLayers();
    void requestExit();
    void ifSaved(std::function<void()> then);
    void undo();
    void redo();
    void copy(bool cut);
    void paste();
    void selectAll();
    void deselect();
    void selectAlpha();
    void addLayer();
    void deleteLayer();
    void applyFx(const char* label, std::function<void(ani::Image&, const ani::IRect&)> fx);
    void rotateFree(float angle, bool allLayers);
    void addText(const std::string& text, int size, int x, int y);
    void loadPattern(const std::string& file);
    void loadSwatch(const std::string& file);
    void loadPreset(const std::string& file);
    void message(const std::string& text);
    void status(const std::string& text);
    ani::IRect workRect() const;

    // ---- interface
    void drawMenuBar();
    void drawToolbar();
    void drawLeftPanel();
    void drawRightPanel();
    void drawStatusBar();
    void drawDialogs();
    void drawToolOptions();
    void drawColorPanel();
    void drawSwatchPanel();
    void drawPresetPanel();
    void drawLayerPanel();
    void drawBrushImagePanel();
    void drawPatternPanel();
    void drawPaperPanel();
    void applyTheme();
    bool iconButton(const char* icon, const char* tip, bool selected, float size = 26);
    ax::Texture2D* icon(const std::string& name);
    ax::Texture2D* thumb(const std::string& file, int size = 64);
    void openDialog(const char* name) { _dialogToOpen = name; }

    float _menuH = 0, _toolbarH = 0, _statusH = 0, _leftW = 280, _rightW = 270;
    std::map<std::string, ax::Texture2D*> _icons;
    std::map<std::string, GpuImage*> _thumbs;
    GpuImage _tipPreview, _strokePreview;
    std::string _dialogToOpen;
    std::string _message, _status;
    float _statusTime = 0;
    FileDialog _fileDialog;
    std::function<void()> _pendingAfterSave;

    // dialogs state
    int _newW = 1024, _newH = 768;
    std::vector<ani::DocTemplate> _templates;
    int _resizeW = 0, _resizeH = 0, _anchor = 4;
    bool _resizeSmooth = true, _resizeKeep = true;
    int _fxKind = 0;
    int _fxP[3] = {0, 0, 0};
    bool _fxFlag = false;
    ani::Image _fxBackup;
    void openFx(int kind);
    void runFx(ani::Image& img, const ani::IRect& r);
    float _rotAngle = 45;
    bool _rotAll = true;
    char _textBuf[1024] = {0};
    int _textSize = 32, _textX = 0, _textY = 0;
    char _renameBuf[128] = {0};
    char _presetName[128] = {0};

    // data lists
    ani::Swatch swatch;
    std::vector<std::string> _swatchFiles, _paperFiles, _patternFiles, _brushDirs, _brushFiles, _banks;
    struct PresetGroup
    {
        std::string name, dir;
        std::vector<std::string> files;
    };
    std::vector<PresetGroup> _presetGroups;
    std::string _bank;
    void refreshLists();
    void refreshPresets();
    void refreshBrushFiles();
    ani::Image _pattern;
    ani::Image _clipboard;
    double _autosaveTimer = 0;
    std::string _prefFile;
};
