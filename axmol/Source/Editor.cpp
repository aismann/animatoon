// Animatoon - Axmol 3.0 port
// Editor : state, tools, input on the canvas and the actions of the menus.
#include "Editor.h"

#include "ImGui/ImGuiPresenter.h"

#include <cmath>
#include <cstring>

using namespace ani;
USING_NS_AX_EXT;

// ------------------------------------------------------------------ GpuImage

GpuImage::~GpuImage()
{
    reset();
}

void GpuImage::reset()
{
    AX_SAFE_RELEASE_NULL(_tex);
    _w = _h = 0;
}

static void premultiply(const uint8_t* s, uint8_t* d, size_t n, bool opaque)
{
    if (opaque)
    {
        std::memcpy(d, s, n * 4);
        return;
    }
    for (size_t i = 0; i < n; ++i, s += 4, d += 4)
    {
        int a = s[3];
        d[0] = (uint8_t)(s[0] * a / 255), d[1] = (uint8_t)(s[1] * a / 255), d[2] = (uint8_t)(s[2] * a / 255);
        d[3] = (uint8_t)a;
    }
}

void GpuImage::set(const Image& img, bool opaque)
{
    if (!img.valid())
    {
        reset();
        return;
    }
    size_t n = (size_t)img.width() * img.height();
    _tmp.resize(n * 4);
    premultiply(img.data(), _tmp.data(), n, opaque);
    if (_tex && _w == img.width() && _h == img.height())
    {
        _tex->updateSubData(_tmp.data(), 0, 0, _w, _h);
        return;
    }
    reset();
    _tex = new ax::Texture2D();
    _tex->initWithData(_tmp.data(), (ssize_t)_tmp.size(), ax::rhi::PixelFormat::RGBA8, img.width(), img.height(), true);
    _w = img.width(), _h = img.height();
}

void GpuImage::updateRect(const Image& img, const IRect& rc)
{
    IRect r = rc.intersect(img.rect());
    if (!_tex || _w != img.width() || _h != img.height() || r.empty())
        return;
    _tmp.resize((size_t)r.w * r.h * 4);
    for (int y = 0; y < r.h; ++y)
        std::memcpy(&_tmp[(size_t)y * r.w * 4], img.at(r.x, r.y + y), (size_t)r.w * 4);
    _tex->updateSubData(_tmp.data(), r.x, r.y, r.w, r.h);
}

// ------------------------------------------------------------------ Editor

Editor::Editor() {}

Editor::~Editor()
{
    shutdown();
}

bool Editor::isPaintAction(int a) const
{
    return a == Action_Pen || a == Action_Brush || a == Action_Eraser || a == Action_Spray || a == Action_CloneStamp;
}

void Editor::init(ax::Sprite* canvasSprite)
{
    _sprite   = canvasSprite;
    // preferences, autosave and temporary files go to the writable folder of the user
    makeDirs(ax::FileUtils::getInstance()->getWritablePath());
    _prefFile = ax::FileUtils::getInstance()->getWritablePath() + "Pref.ini";
    if (fileExists(_prefFile))
        opt.load(_prefFile);
    else if (fileExists(resolvePath("Pref.ini")))
        opt.load(resolvePath("Pref.ini"));
    loadLanguage(opt.lang);
    _templates = loadTemplates();

    // default parameters of the tools (see structures.pb)
    for (int i = 0; i < Action_Count; ++i)
    {
        BrushParams& b = brush[i];
        b.name         = actionName(i);
        b.size         = 1;
        b.alpha        = 255;
        b.sizePressure = 0;
        b.color        = Color(0, 0, 0);
    }
    BrushParams& b = brush[Action_Brush];
    b.size = 30, b.alpha = 200, b.sizePressure = 1, b.pas = 15, b.trait = 1, b.randRot = 360, b.wash = 1;
    b.image = opt.brushName, b.brushDir = opt.brushDir;
    brush[Action_Eraser]              = b;
    brush[Action_Eraser].name         = "Eraser";
    brush[Action_Eraser].alpha        = 255;
    brush[Action_Eraser].randRot      = 0;
    brush[Action_Spray]               = b;
    brush[Action_Spray].name          = "Spray";
    brush[Action_Spray].size          = 8;
    brush[Action_CloneStamp]          = b;
    brush[Action_CloneStamp].name     = "Clone stamp";
    brush[Action_Pen].size            = 3;
    brush[Action_Pen].sizePressure    = 1;
    brush[Action_Line].size           = 3;
    brush[Action_Box].size            = 0;
    brush[Action_Circle].size         = 0;
    brush[Action_Gradient].colorFG    = Color(255, 255, 255);
    brush[Action_Gradient].alphaFG    = 0;

    doc.maxUndo = opt.maxUndo;
    newDocument(opt.imageW, opt.imageH);
    zoom = opt.zoom / 100.f;

    refreshLists();
    if (!opt.swatch.empty())
        loadSwatch(resolvePath(opt.swatch));
    if (swatch.colors.empty() && !_swatchFiles.empty())
        loadSwatch(resolvePath("data/Presets/Swatch/" + _swatchFiles[0]));
    if (!opt.patternFile.empty())
        loadPattern(resolvePath(opt.patternFile));
    else if (!_patternFiles.empty())
        loadPattern(pathJoin(resolvePath(opt.dirPattern), _patternFiles[0]));
    applyTheme();
    _canvasRecreate = true;
}

void Editor::shutdown()
{
    if (_prefFile.empty())
        return;
    opt.zoom            = (int)(zoom * 100);
    opt.brushName       = brush[Action_Brush].image;
    opt.brushDir        = brush[Action_Brush].brushDir;
    opt.paper           = doc.paper.name;
    opt.paperAlpha      = doc.paper.alpha;
    opt.paperScale      = doc.paper.scale;
    opt.paperIntensity  = doc.paper.intensity;
    opt.paperBrightness = doc.paper.brightness;
    opt.paperColor      = doc.paper.color;
    opt.save(_prefFile);
    for (auto& kv : _thumbs)
        delete kv.second;
    _thumbs.clear();
    _prefFile.clear();
}

void Editor::refreshLists()
{
    auto names = [](const std::string& dir, const std::vector<std::string>& exts, bool dirs) {
        std::vector<std::string> out;
        for (auto& e : listDir(dir, exts))
            if (e.isDir == dirs)
                out.push_back(e.name);
        return out;
    };
    const std::vector<std::string> img = {"png", "jpg", "jpeg", "bmp"};
    _swatchFiles  = names(resolvePath("data/Presets/Swatch"), {"gpl"}, false);
    _paperFiles   = names(resolvePath("data/Paper"), img, false);
    _patternFiles = names(resolvePath(opt.dirPattern), img, false);
    _brushDirs    = names(resolvePath("data/Presets/Brush"), {}, true);
    _banks        = names(resolvePath("data/Presets/Bank"), {}, true);
    // the preference keeps the bank as a path (data/Presets/Bank/BD/)
    std::string want = toLower(pathFile(pathDir(pathJoin(opt.dirPreset, "x"))));
    _bank.clear();
    for (auto& b : _banks)
        if (toLower(b) == want)
            _bank = b;
    if (_bank.empty() && !_banks.empty())
        _bank = _banks[0];
    refreshPresets();
    refreshBrushFiles();
}

void Editor::refreshPresets()
{
    _presetGroups.clear();
    std::string root = resolvePath("data/Presets/Bank/" + _bank);
    PresetGroup top;
    top.name = _bank, top.dir = root;
    for (auto& e : listDir(root, {"abp"}))
    {
        if (!e.isDir)
        {
            top.files.push_back(e.name);
            continue;
        }
        PresetGroup g;
        g.name = e.name, g.dir = pathJoin(root, e.name);
        for (auto& f : listDir(g.dir, {"abp"}))
            if (!f.isDir)
                g.files.push_back(f.name);
        _presetGroups.push_back(g);
    }
    if (!top.files.empty())
        _presetGroups.insert(_presetGroups.begin(), top);
    opt.dirPreset = "data/Presets/Bank/" + _bank;
}

void Editor::refreshBrushFiles()
{
    _brushFiles.clear();
    for (auto& e : listDir(resolvePath("data/Presets/Brush/" + brush[Action_Brush].brushDir), {"png", "jpg", "jpeg", "bmp"}))
        if (!e.isDir)
            _brushFiles.push_back(e.name);
}

void Editor::setAction(int a)
{
    if (a == Action_Clear)
    {
        if (doc.canPaint())
        {
            doc.pushUndoLayer(doc.layerId, "Clear");
            doc.clearLayer(doc.layerId);
        }
        return;
    }
    if (opt.useSameColorForAllTools)
    {
        Color c = cur().color;
        brush[a].color = c;
    }
    action = a;
    markTip();
}

void Editor::setColor(const Color& c)
{
    Color col(c.r, c.g, c.b, 255);
    if (opt.useSameColorForAllTools)
        for (auto& b : brush)
            b.color = col;
    else
        cur().color = col;
}

void Editor::changeSize(int dir)
{
    BrushParams& b = cur();
    int step       = b.size >= 100 ? 5 : (b.size > 20 ? 2 : 1);
    b.size         = std::clamp(b.size + dir * step, 1, 1000);
    b.sizeMin      = std::min(b.sizeMin, b.size);
    markTip();
}

void Editor::message(const std::string& text)
{
    _message = text;
    openDialog("Message");
}

void Editor::status(const std::string& text)
{
    _status     = text;
    _statusTime = 4.f;
}

IRect Editor::workRect() const
{
    return doc.selection.active ? doc.selection.rect.intersect(doc.rect()) : doc.rect();
}

// ------------------------------------------------------------------ view

void Editor::zoomAt(float nz, float sx, float sy)
{
    nz = std::clamp(nz, 0.05f, 50.f);
    float dx = (sx - panX) / zoom, dy = (sy - panY) / zoom;
    zoom = nz;
    panX = sx - dx * zoom;
    panY = sy - dy * zoom;
}

void Editor::centerView()
{
    panX = canvasPos.x + (canvasSize.x - doc.width() * zoom) * 0.5f;
    panY = canvasPos.y + (canvasSize.y - doc.height() * zoom) * 0.5f;
}

void Editor::updateCanvas()
{
    if (!_sprite)
        return;
    IRect r = doc.updateComposite();
    const Image& comp = doc.composite();
    if (_canvasRecreate || _canvasTex.width() != comp.width() || _canvasTex.height() != comp.height())
    {
        _canvasTex.reset();
        _canvasTex.set(comp, true);
        _sprite->setTexture(_canvasTex.tex());
        _sprite->setTextureRect(ax::Rect(0, 0, (float)comp.width(), (float)comp.height()));
        _canvasRecreate = false;
    }
    else if (!r.empty())
        _canvasTex.updateRect(comp, r);

    if (auto tex = _canvasTex.tex())
    {
        if (opt.filtering && zoom < 4.f)
            tex->setAntiAliasTexParameters();
        else
            tex->setAliasTexParameters();
    }
    // window coordinates (origin top left) -> scene coordinates (origin bottom left)
    auto vis = ax::Director::getInstance()->getVisibleSize();
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    float kx = ds.x > 0 ? vis.width / ds.x : 1.f, ky = ds.y > 0 ? vis.height / ds.y : 1.f;
    _sprite->setAnchorPoint(ax::Vec2(0, 1));
    _sprite->setPosition(std::round(panX) * kx, vis.height - std::round(panY) * ky);
    _sprite->setScale(zoom * kx, zoom * ky);
    auto c = opt.areaBGColor;
    ax::Director::getInstance()->setClearColor(ax::Color(c.r / 255.f, c.g / 255.f, c.b / 255.f, 1.f));
}

// ------------------------------------------------------------------ documents

void Editor::afterDocumentChanged(bool resetView)
{
    doc.maxUndo     = opt.maxUndo;
    _canvasRecreate = true;
    _dragging       = false;
    doc.invalidateAll();
    if (resetView && canvasSize.x > 0)
    {
        // show the whole picture if it is larger than the window
        float fit = std::min(canvasSize.x / (doc.width() + 40.f), canvasSize.y / (doc.height() + 40.f));
        zoom      = fit < 1.f ? fit : 1.f;
        centerView();
    }
}

void Editor::newDocument(int w, int h)
{
    Paper keep = doc.paper;
    doc.create(w, h);
    doc.name = "Untitled";
    doc.filePath.clear();
    if (keep.texture.valid() || keep.name != "paper0.png")
        doc.paper = keep;
    else
    {
        doc.paper.name       = opt.paper;
        doc.paper.alpha      = opt.paperAlpha;
        doc.paper.scale      = std::max(1, opt.paperScale);
        doc.paper.intensity  = opt.paperIntensity;
        doc.paper.brightness = opt.paperBrightness;
        doc.paper.color      = opt.paperColor;
        loadPaper(doc);
    }
    afterDocumentChanged(true);
}

void Editor::ifSaved(std::function<void()> then)
{
    if (doc.changed)
    {
        _pendingAfterSave = std::move(then);
        openDialog("Unsaved changes");
    }
    else
        then();
}

static const std::vector<std::string> kImageExts = {"png", "jpg", "jpeg", "bmp", "webp", "tga"};

void Editor::openFile()
{
    ifSaved([this] {
        std::vector<std::string> exts = kImageExts;
        exts.insert(exts.begin(), "ani");
        _fileDialog.open(false, L("Open an image or an animatoon document"), exts, opt.pathOpen, "",
                         [this](const std::string& f) {
            std::string err;
            opt.pathOpen = pathDir(f);
            if (!openDocument(doc, f, &err))
                message(std::string(L("Unable to open the file")) + "\n" + f + "\n" + err);
            else
                status("Opened " + f);
            afterDocumentChanged(true);
        });
    });
}

void Editor::saveFile(bool saveAs)
{
    auto doSave = [this](const std::string& f) {
        std::string err;
        if (!saveDocument(doc, f, &err))
            message(err);
        else
        {
            opt.pathSave = pathDir(doc.filePath);
            status("Saved " + doc.filePath);
            if (_pendingAfterSave)
            {
                auto fn = std::move(_pendingAfterSave);
                _pendingAfterSave = nullptr;
                fn();
            }
        }
    };
    if (!saveAs && !doc.filePath.empty())
        doSave(doc.filePath);
    else
        _fileDialog.open(true, L("Save Document"), {"ani"}, opt.pathSave, doc.name + ".ani", doSave);
}

void Editor::saveImage()
{
    _fileDialog.open(true, L("Save Image"), {"png", "jpg"}, opt.pathSave, doc.name + ".png", [this](const std::string& f) {
        std::string file = f;
        if (pathExt(file) != "png" && pathExt(file) != "jpg")
            file += ".png";
        Image flat = doc.flatten(true);
        bool ok    = pathExt(file) == "jpg" ? flat.saveFlattened(file, doc.paper.color) : flat.savePNG(file);
        opt.pathSave = pathDir(file);
        if (ok)
            status("Saved " + file);
        else
            message(std::string(L("Unable to save the image")) + "\n" + file);
    });
}

void Editor::importImage()
{
    _fileDialog.open(false, L("Import"), kImageExts, opt.pathOpen, "", [this](const std::string& f) {
        Image img;
        if (!img.load(f))
        {
            message(std::string(L("Unable to open the file")) + "\n" + f);
            return;
        }
        opt.pathOpen = pathDir(f);
        doc.pushUndoDocument("Import");
        int i = doc.addLayer(pathStem(f));
        doc.layers[i].image.copyFrom(img, 0, 0);
        doc.invalidateAll();
    });
}

void Editor::exportLayer()
{
    _fileDialog.open(true, L("Export layer image"), {"png"}, opt.pathSave, doc.cur().name + ".png",
                     [this](const std::string& f) {
        std::string file = pathExt(f) == "png" ? f : f + ".png";
        if (doc.cur().image.savePNG(file))
            status("Saved " + file);
        else
            message(std::string(L("Unable to save the image")) + "\n" + file);
    });
}

void Editor::exportAllLayers()
{
    _fileDialog.open(true, L("Export layers (in png)"), {"png"}, opt.pathSave, doc.name, [this](const std::string& f) {
        std::string base = pathExt(f) == "png" ? f.substr(0, f.size() - 4) : f;
        int n = 0;
        for (size_t i = 0; i < doc.layers.size(); ++i)
            if (doc.layers[i].image.savePNG(base + "_Layer" + std::to_string(i) + ".png"))
                ++n;
        status(std::to_string(n) + " layers exported");
    });
}

void Editor::requestExit()
{
    auto quit = [] { ax::Director::getInstance()->end(); };
    if (opt.confirmExit)
        ifSaved(quit);
    else
        quit();
}

// ------------------------------------------------------------------ edition

void Editor::undo()
{
    if (_dragging)
        return;
    int w = doc.width(), h = doc.height();
    if (doc.undo() && (w != doc.width() || h != doc.height()))
        _canvasRecreate = true;
}

void Editor::redo()
{
    if (_dragging)
        return;
    int w = doc.width(), h = doc.height();
    if (doc.redo() && (w != doc.width() || h != doc.height()))
        _canvasRecreate = true;
}

void Editor::copy(bool cut)
{
    IRect r    = workRect();
    _clipboard = doc.cur().image.cropped(r);
    if (cut && doc.canPaint())
    {
        doc.pushUndoLayer(doc.layerId, "Cut");
        doc.clearLayer(doc.layerId);
    }
    status(cut ? "Cut" : "Copied");
}

void Editor::paste()
{
    if (!_clipboard.valid())
        return;
    doc.pushUndoDocument("Paste");
    IRect r = workRect();
    int i   = doc.addLayer("Paste");
    doc.layers[i].image.copyFrom(_clipboard, doc.selection.active ? r.x : (doc.width() - _clipboard.width()) / 2,
                                 doc.selection.active ? r.y : (doc.height() - _clipboard.height()) / 2);
    doc.invalidateAll();
}

void Editor::selectAll()
{
    doc.selection.active = true;
    doc.selection.rect   = doc.rect();
}

void Editor::deselect()
{
    doc.selection.active = false;
}

void Editor::selectAlpha()
{
    IRect b = doc.cur().image.opaqueBounds();
    doc.selection.active = !b.empty();
    doc.selection.rect   = b;
}

void Editor::addLayer()
{
    doc.pushUndoDocument("Add layer");
    doc.addLayer();
    doc.invalidateAll();
}

void Editor::deleteLayer()
{
    if (doc.layers.size() <= 1)
    {
        message(L("You can't delete a unique layer. You need at least 2 layers to delete 1 layer."));
        return;
    }
    doc.pushUndoDocument("Delete layer");
    doc.deleteLayer(doc.layerId);
}

void Editor::applyFx(const char* label, std::function<void(Image&, const IRect&)> fx)
{
    if (!doc.canPaint())
    {
        status(L("The layer is locked or hidden"));
        return;
    }
    doc.pushUndoLayer(doc.layerId, label);
    fx(doc.cur().image, workRect());
    doc.invalidateAll();
}

void Editor::rotateFree(float angle, bool all)
{
    doc.pushUndoDocument("Rotate");
    for (int i = 0; i < (int)doc.layers.size(); ++i)
    {
        if (!all && i != doc.layerId)
            continue;
        Image rot = doc.layers[i].image.rotated(angle, true);
        Image n(doc.width(), doc.height());
        n.copyFrom(rot, (doc.width() - rot.width()) / 2, (doc.height() - rot.height()) / 2);
        doc.layers[i].image = std::move(n);
    }
    doc.invalidateAll();
}

void Editor::addText(const std::string& text, int size, int x, int y)
{
    if (text.empty())
        return;
    ax::FontDefinition def;
    def._fontName      = ax::FileUtils::getInstance()->fullPathForFilename("fonts/arial.ttf");
    def._fontSize      = size;
    def._fontFillColor = ax::Color32(255, 255, 255, 255);
    def._alignment     = ax::TextHAlignment::LEFT;
    int w = 0, h = 0;
    bool pma = false;
    ax::Data data = ax::Device::getTextureDataForText(text, def, ax::Device::TextAlign::TOP_LEFT, w, h, pma);
    if (data.isNull() || w <= 0 || h <= 0)
    {
        message("The text can't be rendered on this platform.");
        return;
    }
    // the bitmap gives the coverage of the glyphs, the colour is the one of the tool
    Color c = cur().color;
    Image img(w, h);
    const uint8_t* s = data.getBytes();
    for (int i = 0; i < w * h; ++i)
        img.set(i % w, i / w, Color(c.r, c.g, c.b, s[i * 4 + 3]));
    doc.pushUndoDocument("Text");
    int i = doc.addLayer(text.substr(0, 24));
    Layer& l = doc.layers[i];
    l.type = Layer_TypText, l.text = text, l.fontSize = size, l.fontColor = c;
    l.image.drawImage(img, x, y);
    doc.invalidateAll();
}

void Editor::loadPattern(const std::string& file)
{
    if (_pattern.load(file))
    {
        opt.patternFile = file;
        engine.setPattern(&_pattern);
    }
}

void Editor::loadSwatch(const std::string& file)
{
    if (swatch.load(file))
        opt.swatch = file;
}

void Editor::loadPreset(const std::string& file)
{
    BrushParams p;
    if (!p.load(file))
    {
        status("Unable to read " + file);
        return;
    }
    int target = isPaintAction(action) && action != Action_Pen ? action : Action_Brush;
    p.color    = brush[target].color;
    p.colorFG  = brush[target].colorFG;
    p.symetry  = brush[target].symetry;
    if (target == Action_Eraser)
        p.type = ToolType_Eraser;
    brush[target] = p;
    action        = target;
    refreshBrushFiles();
    markTip();
    status("Preset : " + p.name);
}

// ------------------------------------------------------------------ canvas input

void Editor::handleCanvasInput(float pressure)
{
    ImGuiIO& io = ImGui::GetIO();
    float mx = io.MousePos.x, my = io.MousePos.y;
    float dx = (mx - panX) / zoom, dy = (my - panY) / zoom;
    _mouseDocX = dx, _mouseDocY = dy;
    bool inArea = mx >= canvasPos.x && my >= canvasPos.y && mx < canvasPos.x + canvasSize.x &&
                  my < canvasPos.y + canvasSize.y;
    bool hover     = inArea && !io.WantCaptureMouse && !_fileDialog.isOpen();
    _mouseOnCanvas = hover;

    // ---- zoom with the wheel, pan with the middle button or space
    if (hover && io.MouseWheel != 0 && !_dragging)
        zoomAt(zoom * (io.MouseWheel > 0 ? 1.15f : 1 / 1.15f), mx, my);
    bool space = ImGui::IsKeyDown(ImGuiKey_Space) && !io.WantTextInput;
    if (hover && !_dragging && !_panning && (ImGui::IsMouseClicked(2) || (space && ImGui::IsMouseClicked(0))))
        _panning = true;
    if (_panning)
    {
        panX += io.MouseDelta.x, panY += io.MouseDelta.y;
        if (!ImGui::IsMouseDown(2) && !ImGui::IsMouseDown(0))
            _panning = false;
        return;
    }

    BrushParams& b = cur();
    Layer& layer   = doc.cur();
    const bool shift = io.KeyShift;
    auto pipette = [&] {
        int ix = (int)std::floor(dx), iy = (int)std::floor(dy);
        if (!doc.rect().contains(ix, iy))
            return;
        Color c = doc.pickColor(ix, iy, brush[Action_Pipette].pickerAllLayers ? 0 : 1);
        if (c.a > 0)
            setColor(c);
    };

    // ---- start
    if (hover && !_dragging && ImGui::IsMouseClicked(0))
    {
        int a = action;
        if (io.KeyAlt && isPaintAction(a))
            a = Action_Pipette;
        const bool needsLayer = isPaintAction(a) || a == Action_Line || a == Action_Box || a == Action_Circle ||
                                a == Action_Gradient || a == Action_Fill || a == Action_Move;
        if (needsLayer && (!doc.canPaint() || (a == Action_Move && layer.lockMove)))
        {
            status(L("The layer is locked or hidden"));
            return;
        }
        _dragging   = true;
        _dragAction = a;
        _startX = _lastX = dx, _startY = _lastY = dy;
        _prevDirty = IRect();
        switch (a)
        {
        case Action_Pen:
        case Action_Brush:
        case Action_Eraser:
        case Action_Spray:
        case Action_CloneStamp:
            if (tipDirty)
            {
                engine.updateTip(b);
                tipDirty = false;
            }
            doc.pushUndoLayer(doc.layerId, actionName(a));
            engine.beginStroke(doc, b, a, dx, dy, pressure);
            break;
        case Action_Line:
        case Action_Box:
        case Action_Circle:
        case Action_Gradient:
            doc.pushUndoLayer(doc.layerId, actionName(a));
            _backup = layer.image;
            break;
        case Action_Fill:
        {
            doc.pushUndoLayer(doc.layerId, "Fill");
            Color c(b.color.r, b.color.g, b.color.b, b.alpha);
            IRect r;
            if (b.fillAllLayers)
            {
                Image ref = doc.flatten(false);
                r = floodFill(layer.image, ref, (int)dx, (int)dy, c, b.tolerance, workRect(), layer.lockAlpha);
            }
            else
            {
                Image ref = layer.image;
                r = floodFill(layer.image, ref, (int)dx, (int)dy, c, b.tolerance, workRect(), layer.lockAlpha);
            }
            doc.invalidate(r);
            _dragging = false;
            break;
        }
        case Action_Pipette:
            pipette();
            break;
        case Action_Move:
            doc.pushUndoLayer(doc.layerId, "Move");
            _backup = layer.image;
            if (doc.selection.active)
            {
                _moveRect = doc.selection.rect.intersect(doc.rect());
                _floating = _backup.cropped(_moveRect);
                _backup.fillRect(_moveRect, Color(0, 0, 0, 0));
            }
            break;
        case Action_Zoom:
            zoomAt(zoom * ((io.KeyAlt || io.KeyCtrl) ? 0.5f : 2.f), mx, my);
            _dragging = false;
            break;
        case Action_Text:
            _textX = (int)dx, _textY = (int)dy;
            openDialog("Text");
            _dragging = false;
            break;
        default:
            break;
        }
        return;
    }
    if (hover && !_dragging && ImGui::IsMouseClicked(1))
    {
        if (action == Action_Zoom)
            zoomAt(zoom * 0.5f, mx, my);
        else if (isPaintAction(action))
            pipette();
    }
    if (!_dragging)
        return;

    // ---- continue
    const bool down = ImGui::IsMouseDown(0);
    float ex = dx, ey = dy;
    if (shift && (_dragAction == Action_Box || _dragAction == Action_Circle || _dragAction == Action_Select))
    {
        float s = std::max(std::fabs(ex - _startX), std::fabs(ey - _startY));
        ex = _startX + (ex < _startX ? -s : s), ey = _startY + (ey < _startY ? -s : s);
    }
    else if (shift && (_dragAction == Action_Line || _dragAction == Action_Gradient))
    {
        // straight lines every 45 degrees
        float ang = std::atan2(ey - _startY, ex - _startX), len = std::hypot(ex - _startX, ey - _startY);
        ang = std::round(ang / 0.7853982f) * 0.7853982f;
        ex = _startX + std::cos(ang) * len, ey = _startY + std::sin(ang) * len;
    }
    const bool moved = dx != _lastX || dy != _lastY;

    switch (_dragAction)
    {
    case Action_Pen:
    case Action_Brush:
    case Action_Eraser:
    case Action_Spray:
    case Action_CloneStamp:
        if (down && (moved || _dragAction == Action_Spray))
            engine.strokeTo(doc, b, dx, dy, pressure);
        if (!down)
            engine.endStroke(doc, b);
        break;
    case Action_Line:
    case Action_Box:
    case Action_Circle:
    case Action_Gradient:
        if (moved || !down)
        {
            layer.image = _backup;
            IRect dirty;
            IRect clip = workRect();
            Color c(b.color.r, b.color.g, b.color.b, b.alpha);
            Color fg(b.colorFG.r, b.colorFG.g, b.colorFG.b, b.alphaFG);
            if (_dragAction == Action_Line)
                drawLine(layer.image, _startX, _startY, ex, ey, (float)std::max(b.size, 1), c, clip, layer.lockAlpha, &dirty);
            else if (_dragAction == Action_Box)
            {
                if (b.shapePlain)
                    drawBox(layer.image, _startX, _startY, ex, ey, c, true, 0, (float)b.size, clip, layer.lockAlpha, &dirty);
                if (b.shapeOutSize > 0)
                    drawBox(layer.image, _startX, _startY, ex, ey, b.shapePlain ? Color(fg.r, fg.g, fg.b, 255) : c, false,
                            (float)b.shapeOutSize, (float)b.size, clip, layer.lockAlpha, &dirty);
            }
            else if (_dragAction == Action_Circle)
            {
                if (b.shapePlain)
                    drawEllipse(layer.image, _startX, _startY, ex, ey, c, true, 0, clip, layer.lockAlpha, &dirty);
                if (b.shapeOutSize > 0)
                    drawEllipse(layer.image, _startX, _startY, ex, ey, b.shapePlain ? Color(fg.r, fg.g, fg.b, 255) : c,
                                false, (float)b.shapeOutSize, clip, layer.lockAlpha, &dirty);
            }
            else
            {
                drawGradient(layer.image, _startX, _startY, ex, ey, c, fg, b.gradientType, clip, layer.lockAlpha);
                dirty = clip;
            }
            IRect all = dirty;
            all.unite(_prevDirty);
            doc.invalidate(all);
            _prevDirty = dirty;
        }
        break;
    case Action_Pipette:
        if (down)
            pipette();
        break;
    case Action_Select:
    {
        int x0 = (int)std::floor(std::min(_startX, ex)), y0 = (int)std::floor(std::min(_startY, ey));
        int x1 = (int)std::ceil(std::max(_startX, ex)), y1 = (int)std::ceil(std::max(_startY, ey));
        IRect r = IRect(x0, y0, x1 - x0, y1 - y0).intersect(doc.rect());
        doc.selection.active = r.w >= 2 && r.h >= 2;
        doc.selection.rect   = r;
        break;
    }
    case Action_Move:
        if (moved || !down)
        {
            int ox = (int)std::lround(dx - _startX), oy = (int)std::lround(dy - _startY);
            if (b.symetry == 1) // lock X / Y (reuse of the option)
                oy = 0;
            if (doc.selection.active && _floating.valid())
            {
                layer.image = _backup;
                layer.image.drawImage(_floating, _moveRect.x + ox, _moveRect.y + oy);
                doc.selection.rect = IRect(_moveRect.x + ox, _moveRect.y + oy, _moveRect.w, _moveRect.h);
            }
            else
            {
                layer.image.clear();
                layer.image.copyFrom(_backup, ox, oy);
            }
            doc.invalidateAll();
        }
        break;
    case Action_Hand:
        panX += io.MouseDelta.x, panY += io.MouseDelta.y;
        break;
    default:
        break;
    }
    _lastX = dx, _lastY = dy;
    if (!down)
    {
        _dragging = false;
        _backup   = Image();
        _floating = Image();
        if (_dragAction == Action_Select && doc.selection.active)
            doc.selection.rect = doc.selection.rect.intersect(doc.rect());
    }
}

void Editor::handleShortcuts()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput || _fileDialog.isOpen() || _dragging)
        return;
    auto key = [](ImGuiKey k) { return ImGui::IsKeyPressed(k, false); };
    auto rep = [](ImGuiKey k) { return ImGui::IsKeyPressed(k, true); };
    if (io.KeyCtrl)
    {
        if (key(ImGuiKey_N))
            ifSaved([this] { openDialog("New document"); });
        if (key(ImGuiKey_O))
            openFile();
        if (key(ImGuiKey_S))
            saveFile(io.KeyShift);
        if (key(ImGuiKey_I))
            importImage();
        if (key(ImGuiKey_E))
            io.KeyAlt ? exportAllLayers() : exportLayer();
        if (rep(ImGuiKey_Z))
            io.KeyShift ? redo() : undo();
        if (rep(ImGuiKey_Y))
            redo();
        if (key(ImGuiKey_X))
            copy(true);
        if (key(ImGuiKey_C))
            copy(false);
        if (key(ImGuiKey_V))
            paste();
        if (key(ImGuiKey_W))
            setAction(Action_Clear);
        if (key(ImGuiKey_K) && doc.canPaint())
        {
            doc.pushUndoLayer(doc.layerId, "Fill");
            doc.fillLayer(doc.layerId, cur().color);
        }
        if (key(ImGuiKey_A))
            selectAll();
        if (key(ImGuiKey_D))
            deselect();
        if (key(ImGuiKey_G))
            opt.grid = !opt.grid;
        ImVec2 c(canvasPos.x + canvasSize.x * 0.5f, canvasPos.y + canvasSize.y * 0.5f);
        if (rep(ImGuiKey_KeypadAdd) || rep(ImGuiKey_Equal))
            zoomAt(zoom * 1.25f, c.x, c.y);
        if (rep(ImGuiKey_KeypadSubtract) || rep(ImGuiKey_Minus))
            zoomAt(zoom / 1.25f, c.x, c.y);
        if (key(ImGuiKey_Keypad0) || key(ImGuiKey_0))
        {
            zoomAt(1.f, c.x, c.y);
            centerView();
        }
        if (key(ImGuiKey_Keypad5))
            centerView();
        return;
    }
    if (io.KeyAlt)
        return;
    if (key(ImGuiKey_B))
        setAction(Action_Brush);
    if (key(ImGuiKey_P))
        setAction(Action_Pen);
    if (key(ImGuiKey_E))
        setAction(Action_Eraser);
    if (key(ImGuiKey_K))
        setAction(Action_Fill);
    if (key(ImGuiKey_G))
        setAction(Action_Gradient);
    if (key(ImGuiKey_H))
        setAction(Action_Hand);
    if (key(ImGuiKey_Z))
        setAction(Action_Zoom);
    if (key(ImGuiKey_V))
        setAction(Action_Move);
    if (key(ImGuiKey_M))
        setAction(Action_Select);
    if (key(ImGuiKey_I))
        setAction(Action_Pipette);
    if (key(ImGuiKey_L))
        setAction(Action_Line);
    if (key(ImGuiKey_T))
        setAction(Action_Text);
    if (rep(ImGuiKey_D))
        changeSize(-1);
    if (rep(ImGuiKey_F))
        changeSize(1);
    if (key(ImGuiKey_X))
    {
        // swap the two colours
        Color a = cur().color, f = cur().colorFG;
        setColor(f);
        for (auto& br : brush)
            br.colorFG = Color(a.r, a.g, a.b, 255);
    }
    if (key(ImGuiKey_Delete) && doc.canPaint())
        setAction(Action_Clear);
}

// ------------------------------------------------------------------ frame

void Editor::frame(float pressure, bool)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::GetStyle().FontScaleMain = opt.uiScale;

    drawMenuBar();
    drawToolbar();
    drawStatusBar();
    drawLeftPanel();
    drawRightPanel();

    // area left for the picture
    float left  = (opt.showPanelTool || opt.showPanelColors || opt.showPanelSwatch || opt.showPanelPresets) ? _leftW : 0;
    float right = opt.showPanelLayers ? _rightW : 0;
    ImVec2 old  = canvasSize;
    canvasPos   = ImVec2(left, _menuH + _toolbarH);
    canvasSize  = ImVec2(std::max(50.f, io.DisplaySize.x - left - right),
                         std::max(50.f, io.DisplaySize.y - _menuH - _toolbarH - _statusH));
    if (old.x <= 0 && canvasSize.x > 50)
        afterDocumentChanged(true); // first frame : center the picture

    drawDialogs();
    _fileDialog.draw();
    handleShortcuts();
    handleCanvasInput(pressure);

    if (tipDirty && !_dragging)
    {
        engine.updateTip(cur());
        tipDirty = false;
        _tipPreview.set(engine.tip().toImage(Color(20, 20, 20)));
        Image prev(240, 64);
        BrushParams p = cur();
        if (p.size > 40)
        {
            p.size = 40;
            p.sizeMin = std::min(p.sizeMin, 40);
            BrushEngine tmp;
            tmp.updateTip(p);
            tmp.drawPreview(prev, p);
        }
        else
            engine.drawPreview(prev, p);
        _strokePreview.set(prev);
    }

    drawOverlays();
    updateCanvas();

    if (_statusTime > 0)
        _statusTime -= io.DeltaTime;
    if (opt.autosave && doc.changed)
    {
        _autosaveTimer += io.DeltaTime;
        if (_autosaveTimer > opt.autosaveTime * 60.0 && !_dragging)
        {
            _autosaveTimer = 0;
            // the autosave never replaces the document of the user
            std::string keepPath = doc.filePath, keepName = doc.name;
            bool keepChanged = doc.changed;
            saveDocument(doc, ax::FileUtils::getInstance()->getWritablePath() + "autosave.ani");
            doc.filePath = keepPath, doc.name = keepName, doc.changed = keepChanged;
            status("Autosave");
        }
    }
    else
        _autosaveTimer = 0;
}

void Editor::drawOverlays()
{
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);
    ImVec2 p0(std::round(panX), std::round(panY));
    ImVec2 p1(p0.x + doc.width() * zoom, p0.y + doc.height() * zoom);
    dl->AddRect(ImVec2(p0.x - 1, p0.y - 1), ImVec2(p1.x + 1, p1.y + 1), IM_COL32(0, 0, 0, 120));

    if (opt.grid && opt.gridW * zoom >= 4 && opt.gridH * zoom >= 4)
    {
        ImU32 col = IM_COL32(opt.gridColor.r, opt.gridColor.g, opt.gridColor.b, 70);
        for (int x = opt.gridW; x < doc.width(); x += opt.gridW)
            dl->AddLine(ImVec2(p0.x + x * zoom, p0.y), ImVec2(p0.x + x * zoom, p1.y), col);
        for (int y = opt.gridH; y < doc.height(); y += opt.gridH)
            dl->AddLine(ImVec2(p0.x, p0.y + y * zoom), ImVec2(p1.x, p0.y + y * zoom), col);
    }
    if (doc.selection.active)
    {
        const IRect& r = doc.selection.rect;
        ImVec2 a(p0.x + r.x * zoom, p0.y + r.y * zoom), b(p0.x + r.right() * zoom, p0.y + r.bottom() * zoom);
        dl->AddRect(a, b, IM_COL32(255, 255, 255, 255));
        // dashes
        float t = (float)std::fmod(ImGui::GetTime() * 20.0, 12.0);
        for (float x = a.x - 12 + t; x < b.x; x += 12)
        {
            float xa = std::max(x, a.x), xb = std::min(x + 6, b.x);
            if (xb > xa)
            {
                dl->AddLine(ImVec2(xa, a.y), ImVec2(xb, a.y), IM_COL32(0, 0, 0, 255));
                dl->AddLine(ImVec2(xa, b.y), ImVec2(xb, b.y), IM_COL32(0, 0, 0, 255));
            }
        }
        for (float y = a.y - 12 + t; y < b.y; y += 12)
        {
            float ya = std::max(y, a.y), yb = std::min(y + 6, b.y);
            if (yb > ya)
            {
                dl->AddLine(ImVec2(a.x, ya), ImVec2(a.x, yb), IM_COL32(0, 0, 0, 255));
                dl->AddLine(ImVec2(b.x, ya), ImVec2(b.x, yb), IM_COL32(0, 0, 0, 255));
            }
        }
    }
    // symmetry axis
    if (isPaintAction(action) && cur().symetry > 0)
    {
        ImU32 col = IM_COL32(60, 140, 255, 160);
        int s = cur().symetry;
        if (s == 1 || s >= 3)
            dl->AddLine(ImVec2((p0.x + p1.x) * 0.5f, p0.y), ImVec2((p0.x + p1.x) * 0.5f, p1.y), col);
        if (s == 2 || s >= 3)
            dl->AddLine(ImVec2(p0.x, (p0.y + p1.y) * 0.5f), ImVec2(p1.x, (p0.y + p1.y) * 0.5f), col);
    }
    // cursor of the brush : a circle of the size of the tool
    if (_mouseOnCanvas && !_panning)
    {
        ImVec2 m = ImGui::GetIO().MousePos;
        if (isPaintAction(action))
        {
            float r = action == Action_Spray ? (float)cur().spray : cur().size * 0.5f * std::max(cur().sizeW, cur().sizeH) / 100.f;
            r = std::max(r * zoom, 1.5f);
            dl->AddCircle(m, r, IM_COL32(0, 0, 0, 200), 0, 1.0f);
            dl->AddCircle(m, r + 1, IM_COL32(255, 255, 255, 160), 0, 1.0f);
        }
    }
    dl->PopClipRect();
}
