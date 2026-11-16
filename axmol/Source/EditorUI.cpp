// Animatoon - Axmol 3.0 port
// Editor : the Dear ImGui interface (menus, toolbar, panels, dialogs).
#include "Editor.h"

#include "ImGui/ImGuiPresenter.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

using namespace ani;
USING_NS_AX_EXT;

static const ImGuiWindowFlags kPanelFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;

enum FxKind
{
    Fx_BrightnessContrast = 1,
    Fx_Level,
    Fx_Posterize,
    Fx_ColorBalance,
    Fx_HueSaturation,
    Fx_Noise,
    Fx_Blur,
    Fx_Sharpen,
    Fx_Offset
};

static ImVec4 toIm(const Color& c)
{
    return ImVec4(c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f);
}

static Color fromIm(const float* f)
{
    return Color((int)std::lround(f[0] * 255), (int)std::lround(f[1] * 255), (int)std::lround(f[2] * 255));
}

static void tooltip(const char* tip)
{
    if (tip && *tip && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ImGui::SetTooltip("%s", tip);
}

// ------------------------------------------------------------------ resources

void Editor::applyTheme()
{
    ImGui::StyleColorsDark();
    ImGuiStyle& s     = ImGui::GetStyle();
    s.WindowRounding  = 0;
    s.FrameRounding   = 3;
    s.GrabRounding    = 3;
    s.WindowBorderSize = 1;
    s.ItemSpacing     = ImVec2(6, 5);
    s.Colors[ImGuiCol_WindowBg]      = ImVec4(0.17f, 0.17f, 0.18f, 1.f);
    s.Colors[ImGuiCol_MenuBarBg]     = ImVec4(0.13f, 0.13f, 0.14f, 1.f);
    s.Colors[ImGuiCol_Header]        = ImVec4(0.26f, 0.42f, 0.62f, 0.75f);
    s.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.50f, 0.74f, 0.85f);
    s.Colors[ImGuiCol_PopupBg]       = ImVec4(0.14f, 0.14f, 0.15f, 0.98f);
}

ax::Texture2D* Editor::icon(const std::string& name)
{
    auto it = _icons.find(name);
    if (it != _icons.end())
        return it->second;
    std::string path   = resolvePath(opt.theme + "/icones/" + name + ".png");
    ax::Texture2D* tex = fileExists(path) ? ax::Director::getInstance()->getTextureCache()->addImage(path) : nullptr;
    _icons[name]       = tex;
    return tex;
}

ax::Texture2D* Editor::thumb(const std::string& file, int size)
{
    auto it = _thumbs.find(file);
    if (it != _thumbs.end())
        return it->second->tex();
    auto g = new GpuImage();
    Image img;
    if (img.load(file))
    {
        float k = std::min(1.f, (float)size / std::max(img.width(), img.height()));
        if (k < 1.f)
            img = img.resized(std::max(1, (int)(img.width() * k)), std::max(1, (int)(img.height() * k)), true);
        g->set(img);
    }
    _thumbs[file] = g;
    return g->tex();
}

bool Editor::iconButton(const char* iconName, const char* tip, bool selected, float size)
{
    bool pressed = false;
    if (selected)
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
    ImGui::PushID(iconName);
    if (auto tex = icon(iconName))
        pressed = ImGuiPresenter::getInstance()->imageButton(tex, ImVec2(size, size));
    else
        pressed = ImGui::Button(tip, ImVec2(0, size + 6));
    ImGui::PopID();
    if (selected)
        ImGui::PopStyleColor();
    tooltip(tip);
    return pressed;
}

// ------------------------------------------------------------------ menu

void Editor::drawMenuBar()
{
    if (!ImGui::BeginMainMenuBar())
        return;
    _menuH = ImGui::GetWindowSize().y;
    const bool paint = doc.canPaint();
    if (ImGui::BeginMenu(L("File")))
    {
        if (ImGui::MenuItem(L("New"), "Ctrl+N"))
            ifSaved([this] { openDialog("New document"); });
        if (ImGui::MenuItem(L("Open"), "Ctrl+O"))
            openFile();
        if (ImGui::MenuItem(L("Import"), "Ctrl+I"))
            importImage();
        ImGui::Separator();
        if (ImGui::MenuItem(L("Save"), "Ctrl+S"))
            saveFile(false);
        if (ImGui::MenuItem(L("Save As"), "Ctrl+Shift+S"))
            saveFile(true);
        if (ImGui::MenuItem(L("Save image")))
            saveImage();
        ImGui::Separator();
        if (ImGui::MenuItem(L("Export layer image"), "Ctrl+E"))
            exportLayer();
        if (ImGui::MenuItem(L("Export layers (in png)"), "Ctrl+Alt+E"))
            exportAllLayers();
        ImGui::Separator();
        if (ImGui::MenuItem(L("Preferences")))
            openDialog("Preferences");
        ImGui::Separator();
        if (ImGui::MenuItem(L("Exit")))
            requestExit();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(L("Edit")))
    {
        std::string u = std::string(L("Undo")) + (doc.canUndo() ? std::string(" ") + doc.undoLabel() : "");
        std::string r = std::string(L("Redo")) + (doc.canRedo() ? std::string(" ") + doc.redoLabel() : "");
        if (ImGui::MenuItem(u.c_str(), "Ctrl+Z", false, doc.canUndo()))
            undo();
        if (ImGui::MenuItem(r.c_str(), "Ctrl+Y", false, doc.canRedo()))
            redo();
        ImGui::Separator();
        if (ImGui::MenuItem(L("Cut"), "Ctrl+X"))
            copy(true);
        if (ImGui::MenuItem(L("Copy"), "Ctrl+C"))
            copy(false);
        if (ImGui::MenuItem(L("Paste"), "Ctrl+V", false, _clipboard.valid()))
            paste();
        ImGui::Separator();
        if (ImGui::MenuItem(L("Clear"), "Ctrl+W", false, paint))
            setAction(Action_Clear);
        if (ImGui::MenuItem(L("Fill with BG color"), "Ctrl+K", false, paint))
        {
            doc.pushUndoLayer(doc.layerId, "Fill");
            doc.fillLayer(doc.layerId, cur().color);
        }
        if (ImGui::MenuItem(L("Fill with pattern"), nullptr, false, paint && _pattern.valid()))
        {
            doc.pushUndoLayer(doc.layerId, "Fill");
            doc.fillLayerPattern(doc.layerId, _pattern);
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(L("View")))
    {
        ImVec2 c(canvasPos.x + canvasSize.x * 0.5f, canvasPos.y + canvasSize.y * 0.5f);
        ImGui::MenuItem(L("Grid"), "Ctrl+G", &opt.grid);
        ImGui::MenuItem(L("Filtering"), nullptr, &opt.filtering);
        ImGui::Separator();
        if (ImGui::MenuItem(L("Zoom +"), "Ctrl +"))
            zoomAt(zoom * 1.25f, c.x, c.y);
        if (ImGui::MenuItem(L("Zoom -"), "Ctrl -"))
            zoomAt(zoom / 1.25f, c.x, c.y);
        for (int z : {50, 100, 200, 300, 400, 500, 1000})
        {
            std::string n = std::string(L("Zoom")) + " " + std::to_string(z) + "%";
            if (ImGui::MenuItem(n.c_str(), z == 100 ? "Ctrl+0" : nullptr))
                zoomAt(z / 100.f, c.x, c.y);
        }
        ImGui::Separator();
        if (ImGui::MenuItem(L("Reset view")))
        {
            zoom = 1.f;
            panX = canvasPos.x, panY = canvasPos.y;
        }
        if (ImGui::MenuItem(L("Center view"), "Ctrl+Pad5"))
            centerView();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(L("Select")))
    {
        if (ImGui::MenuItem(L("Select all"), "Ctrl+A"))
            selectAll();
        if (ImGui::MenuItem(L("Deselect"), "Ctrl+D", false, doc.selection.active))
            deselect();
        if (ImGui::MenuItem(L("Select alpha of the layer")))
            selectAlpha();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(L("Image")))
    {
        if (ImGui::MenuItem(L("Resize document")))
        {
            _resizeW = doc.width(), _resizeH = doc.height();
            openDialog("Resize document");
        }
        if (ImGui::MenuItem(L("Resize canvas")))
        {
            _resizeW = doc.width(), _resizeH = doc.height();
            openDialog("Resize canvas");
        }
        if (ImGui::MenuItem(L("Crop"), nullptr, false, doc.selection.active))
        {
            doc.pushUndoDocument("Crop");
            doc.crop(doc.selection.rect);
            afterDocumentChanged(false);
        }
        if (ImGui::MenuItem(L("Trim")))
        {
            IRect b;
            for (auto& l : doc.layers)
                b.unite(l.image.opaqueBounds());
            if (!b.empty())
            {
                doc.pushUndoDocument("Trim");
                doc.crop(b);
                afterDocumentChanged(false);
            }
        }
        ImGui::Separator();
        for (int q = 1; q <= 3; ++q)
        {
            std::string n = std::string(L("Rotate")) + " " + std::to_string(q * 90);
            if (ImGui::MenuItem(n.c_str()))
            {
                doc.pushUndoDocument("Rotate");
                doc.rotate90(q);
                afterDocumentChanged(false);
            }
        }
        if (ImGui::MenuItem(L("Free rotation")))
        {
            _rotAll = true;
            openDialog("Rotate");
        }
        if (ImGui::MenuItem(L("Mirror horizontal")))
        {
            doc.pushUndoDocument("Mirror");
            doc.mirror(true, true);
        }
        if (ImGui::MenuItem(L("Mirror vertical")))
        {
            doc.pushUndoDocument("Mirror");
            doc.mirror(false, true);
        }
        ImGui::Separator();
        if (ImGui::MenuItem(L("Brightness/Contrast"), nullptr, false, paint))
            openFx(Fx_BrightnessContrast);
        if (ImGui::MenuItem(L("Level"), nullptr, false, paint))
            openFx(Fx_Level);
        if (ImGui::MenuItem(L("Color balance"), nullptr, false, paint))
            openFx(Fx_ColorBalance);
        if (ImGui::MenuItem(L("Hue/Saturation"), nullptr, false, paint))
            openFx(Fx_HueSaturation);
        if (ImGui::MenuItem(L("Posterize"), nullptr, false, paint))
            openFx(Fx_Posterize);
        if (ImGui::MenuItem(L("Desaturation"), nullptr, false, paint))
            applyFx("Desaturation", [](Image& i, const IRect& r) { fxDesaturate(i, r); });
        if (ImGui::MenuItem(L("Inverse color"), nullptr, false, paint))
            applyFx("Inverse", [](Image& i, const IRect& r) { fxInvert(i, r); });
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(L("Layer")))
    {
        if (ImGui::MenuItem(L("Add a layer")))
            addLayer();
        if (ImGui::MenuItem(L("Duplicate")))
        {
            doc.pushUndoDocument("Duplicate layer");
            doc.duplicateLayer(doc.layerId);
        }
        if (ImGui::MenuItem(L("Delete")))
            deleteLayer();
        ImGui::Separator();
        if (ImGui::MenuItem(L("Move up"), nullptr, false, doc.layerId < (int)doc.layers.size() - 1))
        {
            doc.pushUndoDocument("Move layer");
            doc.moveLayer(doc.layerId, 1);
        }
        if (ImGui::MenuItem(L("Move down"), nullptr, false, doc.layerId > 0))
        {
            doc.pushUndoDocument("Move layer");
            doc.moveLayer(doc.layerId, -1);
        }
        ImGui::Separator();
        static const char* mergeNames[] = {"Merge down", "Merge all", "Merge all visible", "Merge linked"};
        for (int m = 0; m < 4; ++m)
            if (ImGui::MenuItem(L(mergeNames[m]), nullptr, false, doc.layers.size() > 1 && (m != 0 || doc.layerId > 0)))
            {
                doc.pushUndoDocument("Merge");
                doc.mergeLayers(m);
            }
        ImGui::Separator();
        if (ImGui::MenuItem(L("Mirror horizontal"), nullptr, false, paint))
        {
            doc.pushUndoLayer(doc.layerId, "Mirror");
            doc.mirror(true, false);
        }
        if (ImGui::MenuItem(L("Mirror vertical"), nullptr, false, paint))
        {
            doc.pushUndoLayer(doc.layerId, "Mirror");
            doc.mirror(false, false);
        }
        if (ImGui::MenuItem(L("Rotate"), nullptr, false, paint))
        {
            _rotAll = false;
            openDialog("Rotate");
        }
        if (ImGui::MenuItem(L("Transform to line"), nullptr, false, paint))
            applyFx("To line", [](Image& i, const IRect& r) { fxWhiteToAlpha(i, r); });
        if (ImGui::MenuItem(L("Rename")))
        {
            snprintf(_renameBuf, sizeof(_renameBuf), "%s", doc.cur().name.c_str());
            openDialog("Layer name");
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(L("Filters")))
    {
        if (ImGui::MenuItem(L("Noise"), nullptr, false, paint))
            openFx(Fx_Noise);
        if (ImGui::MenuItem(L("Clouds"), nullptr, false, paint))
        {
            Color a = cur().color, b = cur().colorFG;
            applyFx("Clouds", [a, b](Image& i, const IRect& r) { fxClouds(i, r, a, b, 96); });
        }
        if (ImGui::MenuItem(L("Blur"), nullptr, false, paint))
            openFx(Fx_Blur);
        if (ImGui::MenuItem(L("Sharpen"), nullptr, false, paint))
            openFx(Fx_Sharpen);
        if (ImGui::MenuItem(L("Offset"), nullptr, false, paint))
            openFx(Fx_Offset);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(L("Window")))
    {
        ImGui::MenuItem(L("Tool parameters"), nullptr, &opt.showPanelTool);
        ImGui::MenuItem(L("Presets"), nullptr, &opt.showPanelPresets);
        ImGui::MenuItem(L("Colors"), nullptr, &opt.showPanelColors);
        ImGui::MenuItem(L("Swatchs"), nullptr, &opt.showPanelSwatch);
        ImGui::MenuItem(L("Layers"), nullptr, &opt.showPanelLayers);
        ImGui::MenuItem(L("Statusbar"), nullptr, &opt.showStatusbar);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(L("Help")))
    {
        if (ImGui::MenuItem(L("About")))
            openDialog("About");
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

// ------------------------------------------------------------------ toolbar

void Editor::drawToolbar()
{
    ImGuiIO& io = ImGui::GetIO();
    _toolbarH   = 44 * opt.uiScale;
    ImGui::SetNextWindowPos(ImVec2(0, _menuH));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, _toolbarH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 4));
    ImGui::Begin("##toolbar", nullptr, kPanelFlags | ImGuiWindowFlags_NoScrollbar);
    float sz = 26 * opt.uiScale;
    if (iconButton("new", L("New"), false, sz))
        ifSaved([this] { openDialog("New document"); });
    ImGui::SameLine();
    if (iconButton("open", L("Open"), false, sz))
        openFile();
    ImGui::SameLine();
    if (iconButton("save", L("Save"), false, sz))
        saveFile(false);
    ImGui::SameLine();
    if (iconButton("export", L("Save image"), false, sz))
        saveImage();
    ImGui::SameLine(0, 18);

    struct Tool
    {
        int action;
        const char* icon;
        const char* tip;
    };
    static const Tool tools[] = {
        {Action_Pen, "pen", "Pen (P)"},
        {Action_Brush, "brush", "Brush (B)"},
        {Action_Spray, "spray", "Spray"},
        {Action_CloneStamp, "pattern", "Pattern stamp"},
        {Action_Eraser, "eraser", "Eraser (E)"},
        {Action_Line, "line", "Line (L)"},
        {Action_Box, "box", "Box"},
        {Action_Circle, "circle", "Circle"},
        {Action_Gradient, "gradient", "Gradient (G)"},
        {Action_Fill, "fill", "Fill (K)"},
        {Action_Text, "text", "Text (T)"},
        {Action_Pipette, "colorpicker", "Color picker (I)"},
        {Action_Select, "select", "Selection (M)"},
        {Action_Move, "move", "Move (V)"},
        {Action_Hand, "hand", "Hand (H, space)"},
        {Action_Zoom, "zoom", "Zoom (Z)"},
        {Action_Clear, "clear", "Clear the layer (Ctrl+W)"},
    };
    for (auto& t : tools)
    {
        if (t.action == Action_Line || t.action == Action_Pipette || t.action == Action_Clear)
            ImGui::SameLine(0, 14);
        else
            ImGui::SameLine();
        if (iconButton(t.icon, L(t.tip), action == t.action, sz))
            setAction(t.action);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void Editor::drawStatusBar()
{
    _statusH = 0;
    if (!opt.showStatusbar)
        return;
    ImGuiIO& io = ImGui::GetIO();
    _statusH    = ImGui::GetFrameHeight() + 6;
    ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - _statusH));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, _statusH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 3));
    ImGui::Begin("##status", nullptr, kPanelFlags | ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("%s%s  |  %d x %d  |  %s %d%%  |  %s : %s  |  %s : %s", doc.name.c_str(), doc.changed ? "*" : "",
                doc.width(), doc.height(), L("Zoom"), (int)std::lround(zoom * 100), L("Tool"), L(actionName(action)),
                L("Layer"), doc.cur().name.c_str());
    if (_mouseOnCanvas)
    {
        ImGui::SameLine();
        ImGui::Text(" |  %d / %d", (int)std::floor(_mouseDocX), (int)std::floor(_mouseDocY));
    }
    if (_statusTime > 0 && !_status.empty())
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 0.85f, 0.4f, 1), " |  %s", _status.c_str());
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

// ------------------------------------------------------------------ left panel

void Editor::drawLeftPanel()
{
    if (!(opt.showPanelTool || opt.showPanelColors || opt.showPanelSwatch || opt.showPanelPresets))
        return;
    ImGuiIO& io = ImGui::GetIO();
    _leftW      = 290 * opt.uiScale;
    ImGui::SetNextWindowPos(ImVec2(0, _menuH + _toolbarH));
    ImGui::SetNextWindowSize(ImVec2(_leftW, io.DisplaySize.y - _menuH - _toolbarH - _statusH));
    ImGui::Begin("##left", nullptr, kPanelFlags);
    if (opt.showPanelColors && ImGui::CollapsingHeader(L("Colors"), ImGuiTreeNodeFlags_DefaultOpen))
        drawColorPanel();
    if (ImGui::BeginTabBar("##lefttabs"))
    {
        if (opt.showPanelTool && ImGui::BeginTabItem(L("Tool parameters")))
        {
            ImGui::BeginChild("##tooloptions");
            drawToolOptions();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (opt.showPanelPresets && ImGui::BeginTabItem(L("Presets")))
        {
            ImGui::BeginChild("##presets");
            drawPresetPanel();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (opt.showPanelSwatch && ImGui::BeginTabItem(L("Swatchs")))
        {
            ImGui::BeginChild("##swatch");
            drawSwatchPanel();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void Editor::drawColorPanel()
{
    BrushParams& b = cur();
    float col[3]   = {b.color.r / 255.f, b.color.g / 255.f, b.color.b / 255.f};
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 70);
    if (ImGui::ColorPicker3("##picker", col,
                            ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview |
                                ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB |
                                ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_NoLabel))
        setColor(fromIm(col));
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::ColorButton("##main", toIm(Color(b.color.r, b.color.g, b.color.b)), ImGuiColorEditFlags_NoTooltip, ImVec2(52, 40));
    tooltip(L("Color of the tool"));
    float fg[3] = {b.colorFG.r / 255.f, b.colorFG.g / 255.f, b.colorFG.b / 255.f};
    if (ImGui::ColorEdit3("##fg", fg, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
        for (auto& br : brush)
            br.colorFG = fromIm(fg);
    tooltip(L("Second color (gradient, outline)"));
    ImGui::SameLine();
    if (ImGui::SmallButton("<>"))
    {
        Color a = b.color, f = b.colorFG;
        setColor(f);
        for (auto& br : brush)
            br.colorFG = Color(a.r, a.g, a.b);
    }
    tooltip(L("Swap the colors (X)"));
    ImGui::EndGroup();
}

void Editor::drawSwatchPanel()
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 90);
    if (ImGui::BeginCombo("##swfile", swatch.name.c_str()))
    {
        for (auto& f : _swatchFiles)
            if (ImGui::Selectable(f.c_str()))
                loadSwatch(resolvePath("data/Presets/Swatch/" + f));
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("+"))
        swatch.colors.push_back({cur().color, "Untitled"});
    tooltip(L("Add the current color"));
    ImGui::SameLine();
    if (ImGui::Button(L("Save")))
    {
        if (swatch.save(resolvePath(opt.swatch)))
            status("Swatch saved");
    }
    ImGui::SameLine();
    if (ImGui::Button("Img"))
        swatch.createFromImage(doc.flatten(true), 64);
    tooltip(L("Create the swatch from the image"));

    float avail = ImGui::GetContentRegionAvail().x;
    float bs    = 20 * opt.uiScale;
    int cols    = std::max(1, (int)(avail / (bs + 4)));
    int remove  = -1;
    for (int i = 0; i < (int)swatch.colors.size(); ++i)
    {
        if (i % cols)
            ImGui::SameLine(0, 4);
        ImGui::PushID(i);
        if (ImGui::ColorButton("##sw", toIm(swatch.colors[i].color), ImGuiColorEditFlags_NoTooltip, ImVec2(bs, bs)))
            setColor(swatch.colors[i].color);
        if (ImGui::IsItemClicked(1))
            remove = i;
        tooltip(swatch.colors[i].name.c_str());
        ImGui::PopID();
    }
    if (remove >= 0)
        swatch.colors.erase(swatch.colors.begin() + remove);
    ImGui::TextDisabled("%s", L("Right click : delete the color"));
}

void Editor::drawToolOptions()
{
    BrushParams& b = cur();
    bool ch = false;
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
    auto check = [&](const char* label, int& v) {
        bool f = v != 0;
        if (ImGui::Checkbox(label, &f))
        {
            v  = f ? 1 : 0;
            ch = true;
        }
    };
    const bool named = (action == Action_Brush || action == Action_Eraser) && !b.name.empty();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1, 1), "%s%s%s", L(actionName(action)), named ? " - " : "",
                       named ? b.name.c_str() : "");

    if (isPaintAction(action))
    {
        if (action != Action_Pen)
        {
            auto p = ImGuiPresenter::getInstance();
            if (_strokePreview.tex())
                p->image(_strokePreview.tex(), ImVec2(240, 64));
        }
        ch |= ImGui::SliderInt(L("Size"), &b.size, 1, 500, "%d", ImGuiSliderFlags_Logarithmic);
        ch |= ImGui::SliderInt(L("Opacity"), &b.alpha, 0, 255);
        check(L("Size with pressure"), b.sizePressure);
        check(L("Opacity with pressure"), b.alphaPressure);
        static const char* sym[] = {"None", "Horizontal", "Vertical", "Both (opposite)", "4 views"};
        ImGui::Combo(L("Symmetry"), &b.symetry, sym, 5);

        if (action == Action_Spray)
        {
            ImGui::SliderInt(L("Radius"), &b.spray, 2, 300);
            ImGui::SliderInt(L("Dots"), &b.nbSpray, 1, 40);
        }
        if (action != Action_Pen && ImGui::TreeNodeEx(L("Size / dynamics"), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ch |= ImGui::SliderInt(L("Width %"), &b.sizeW, 5, 300);
            ch |= ImGui::SliderInt(L("Height %"), &b.sizeH, 5, 300);
            ch |= ImGui::SliderInt(L("Size min"), &b.sizeMin, 0, std::max(b.size, 1));
            check(L("Random size"), b.sizeRand);
            ch |= ImGui::SliderInt(L("Opacity min"), &b.alphaMin, 0, 255);
            check(L("Random opacity"), b.alphaRand);
            ch |= ImGui::SliderInt(L("Spacing %"), &b.pas, 1, 300);
            check(L("Line between the dots"), b.trait);
            ch |= ImGui::SliderInt(L("Scatter"), &b.scatter, 0, 500);
            ch |= ImGui::SliderInt(L("Rotation"), &b.rotate, 0, 360);
            ch |= ImGui::SliderInt(L("Random rotation"), &b.randRot, 0, 360);
            check(L("Rotation follows the stroke"), b.rotateByAngle);
            ImGui::TreePop();
        }
        if (action != Action_Pen && ImGui::TreeNode(L("Brush image")))
        {
            ch |= ImGui::SliderInt(L("Hardness"), &b.hardness, 0, 255);
            ch |= ImGui::SliderInt(L("Intensity"), &b.intensity, -255, 255);
            check(L("Smooth"), b.smooth);
            check(L("Trim"), b.trim);
            if (_tipPreview.tex())
            {
                float k = std::min(1.f, 96.f / std::max(_tipPreview.width(), _tipPreview.height()));
                ImGuiPresenter::getInstance()->image(_tipPreview.tex(),
                                                     ImVec2(_tipPreview.width() * k, _tipPreview.height() * k));
            }
            ImGui::TextDisabled("%s/%s", b.brushDir.c_str(), b.image.c_str());
            ImGui::TreePop();
        }
        if ((action == Action_Brush || action == Action_Spray) && ImGui::TreeNode(L("Color mixing")))
        {
            static const char* types[] = {"Brush", "Pen", "Eraser", "Light", "Dark", "Color", "Shape", "Water",
                                          "Smudge", "Glass", "Noise", "Pixel", "Blur", "Solarize"};
            ImGui::Combo(L("Type"), &b.type, types, 14);
            if (b.type == ToolType_Pen || b.type == ToolType_Shape)
                b.type = ToolType_Brush;
            static const char* bm[] = {"Normal", "Multiply", "Dissolve"};
            int bi = b.blendmode == Bm_Multiply ? 1 : (b.blendmode == Bm_Dissolve ? 2 : 0);
            if (ImGui::Combo(L("Blend"), &bi, bm, 3))
                b.blendmode = bi == 1 ? Bm_Multiply : (bi == 2 ? Bm_Dissolve : Bm_Normal);
            ImGui::SliderInt(L("Mix"), &b.mix, 0, 100);
            static const char* mt[] = {"Classic (viscosity)", "Inverse (smear)", "Simple", "New"};
            b.mixType = std::clamp(b.mixType, 0, 3);
            ImGui::Combo(L("Mix type"), &b.mixType, mt, 4);
            ImGui::SliderInt(L("Viscosity"), &b.visco, 0, 100);
            static const char* ml[] = {"All layers", "Current layer", "Layers below"};
            b.mixLayer = std::clamp(b.mixLayer, 0, 2);
            ImGui::Combo(L("Mix with"), &b.mixLayer, ml, 3);
            check(L("Wash the brush at mouse up"), b.wash);
            ImGui::SliderInt(L("Water"), &b.water, 0, 255);
            ImGui::SliderInt(L("Strength"), &b.filter, 0, 100);
            ImGui::TreePop();
        }
        if (action != Action_Pen)
        {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
            ImGui::InputTextWithHint("##pname", L("Preset name"), _presetName, sizeof(_presetName));
            ImGui::SameLine();
            if (ImGui::Button(L("Save preset")) && _presetName[0])
            {
                BrushParams p = b;
                p.name        = _presetName;
                std::string dir = resolvePath("data/Presets/Bank/" + _bank + "/user");
                makeDirs(dir);
                if (p.save(pathJoin(dir, std::string(_presetName) + ".abp")))
                {
                    status("Preset saved");
                    refreshPresets();
                }
            }
        }
    }
    else
    {
        switch (action)
        {
        case Action_Line:
            ImGui::SliderInt(L("Size"), &b.size, 1, 200);
            ImGui::SliderInt(L("Opacity"), &b.alpha, 0, 255);
            ImGui::TextDisabled("%s", L("Shift : every 45 degrees"));
            break;
        case Action_Box:
        case Action_Circle:
            if (action == Action_Box)
                ImGui::SliderInt(L("Round"), &b.size, 0, 200);
            check(L("Plain"), b.shapePlain);
            ImGui::SliderInt(L("Outline"), &b.shapeOutSize, 0, 100);
            if (!b.shapePlain && b.shapeOutSize < 1)
                b.shapeOutSize = 1;
            ImGui::SliderInt(L("Opacity"), &b.alpha, 0, 255);
            ImGui::TextDisabled("%s", L("Shift : keep the proportions"));
            break;
        case Action_Gradient:
        {
            static const char* gt[] = {"Linear", "Circular", "Elliptical", "Conical", "Boxed"};
            ImGui::Combo(L("Type"), &b.gradientType, gt, 5);
            ImGui::SliderInt(L("Opacity"), &b.alpha, 0, 255);
            ImGui::SliderInt(L("Opacity (2nd color)"), &b.alphaFG, 0, 255);
            break;
        }
        case Action_Fill:
            ImGui::SliderInt(L("Tolerance"), &b.tolerance, 0, 255);
            ImGui::SliderInt(L("Opacity"), &b.alpha, 0, 255);
            check(L("Use all the layers"), b.fillAllLayers);
            break;
        case Action_Pipette:
            check(L("Pick on all the layers"), b.pickerAllLayers);
            break;
        case Action_Select:
            if (doc.selection.active)
                ImGui::Text("%d, %d  -  %d x %d", doc.selection.rect.x, doc.selection.rect.y, doc.selection.rect.w,
                            doc.selection.rect.h);
            if (ImGui::Button(L("Select all")))
                selectAll();
            ImGui::SameLine();
            if (ImGui::Button(L("Deselect")))
                deselect();
            break;
        case Action_Move:
            check(L("Lock the vertical move"), b.symetry);
            ImGui::TextDisabled("%s", L("Moves the layer or the selection"));
            break;
        case Action_Text:
            ImGui::SliderInt(L("Size"), &_textSize, 6, 300);
            ImGui::TextDisabled("%s", L("Click on the image to add a text"));
            break;
        case Action_Zoom:
            ImGui::TextDisabled("%s", L("Click : zoom +, Alt or right click : zoom -"));
            break;
        default:
            break;
        }
    }
    ImGui::PopItemWidth();
    if (ch)
    {
        b.sizeMin = std::min(b.sizeMin, b.size);
        markTip();
    }
}

void Editor::drawPresetPanel()
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::BeginCombo("##bank", _bank.c_str()))
    {
        for (auto& bk : _banks)
            if (ImGui::Selectable(bk.c_str(), bk == _bank))
            {
                _bank = bk;
                refreshPresets();
            }
        ImGui::EndCombo();
    }
    for (auto& g : _presetGroups)
    {
        if (!ImGui::TreeNode(g.name.c_str()))
            continue;
        for (auto& f : g.files)
        {
            std::string label = pathStem(f);
            bool sel = isPaintAction(action) && cur().name == label;
            if (ImGui::Selectable(label.c_str(), sel))
                loadPreset(pathJoin(g.dir, f));
        }
        ImGui::TreePop();
    }
}

// ------------------------------------------------------------------ right panel

void Editor::drawRightPanel()
{
    if (!opt.showPanelLayers)
        return;
    ImGuiIO& io = ImGui::GetIO();
    _rightW     = 280 * opt.uiScale;
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - _rightW, _menuH + _toolbarH));
    ImGui::SetNextWindowSize(ImVec2(_rightW, io.DisplaySize.y - _menuH - _toolbarH - _statusH));
    ImGui::Begin("##right", nullptr, kPanelFlags);
    if (ImGui::CollapsingHeader(L("Layers"), ImGuiTreeNodeFlags_DefaultOpen))
        drawLayerPanel();
    if (ImGui::CollapsingHeader(L("Brush images"), ImGuiTreeNodeFlags_DefaultOpen))
        drawBrushImagePanel();
    if (ImGui::CollapsingHeader(L("Patterns")))
        drawPatternPanel();
    if (ImGui::CollapsingHeader(L("Background")))
        drawPaperPanel();
    ImGui::End();
}

void Editor::drawLayerPanel()
{
    Layer& l = doc.cur();
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
    int bm = l.bm;
    if (ImGui::BeginCombo(L("Blend"), blendModeName(bm)))
    {
        for (int i = 0; i < Bm_Custom; ++i)
            if (ImGui::Selectable(blendModeName(i), i == bm))
            {
                l.bm = i;
                doc.changed = true;
                doc.invalidateAll();
            }
        ImGui::EndCombo();
    }
    if (ImGui::SliderInt(L("Opacity"), &l.alpha, 0, 255))
    {
        doc.changed = true;
        doc.invalidateAll();
    }
    ImGui::PopItemWidth();
    ImGui::Checkbox(L("Lock alpha"), &l.lockAlpha);
    ImGui::SameLine();
    ImGui::Checkbox(L("Lock"), &l.locked);
    ImGui::Checkbox(L("Lock paint"), &l.lockPaint);
    ImGui::SameLine();
    ImGui::Checkbox(L("Lock move"), &l.lockMove);

    float h = std::clamp((float)doc.layers.size() * (ImGui::GetFrameHeight() + 4) + 8, 80.f, 240.f);
    ImGui::BeginChild("##layers", ImVec2(0, h), ImGuiChildFlags_Borders);
    for (int i = (int)doc.layers.size() - 1; i >= 0; --i)
    {
        Layer& li = doc.layers[i];
        ImGui::PushID(li.uid);
        if (ImGui::Checkbox("##view", &li.view))
            doc.invalidateAll();
        tooltip(L("Show / hide the layer"));
        ImGui::SameLine();
        if (ImGui::Checkbox("##link", &li.link))
            doc.changed = true;
        tooltip(L("Link"));
        ImGui::SameLine();
        std::string label = li.name + (li.locked ? "  [lock]" : "") + (li.lockAlpha ? "  [a]" : "");
        if (ImGui::Selectable(label.c_str(), i == doc.layerId, ImGuiSelectableFlags_AllowDoubleClick))
        {
            doc.layerId = i;
            if (ImGui::IsMouseDoubleClicked(0))
            {
                snprintf(_renameBuf, sizeof(_renameBuf), "%s", li.name.c_str());
                openDialog("Layer name");
            }
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    if (ImGui::Button("+"))
        addLayer();
    tooltip(L("Add a layer"));
    ImGui::SameLine();
    if (ImGui::Button("-"))
        deleteLayer();
    tooltip(L("Delete"));
    ImGui::SameLine();
    if (ImGui::Button(L("Copy##layer")))
    {
        doc.pushUndoDocument("Duplicate layer");
        doc.duplicateLayer(doc.layerId);
    }
    tooltip(L("Duplicate"));
    ImGui::SameLine();
    if (ImGui::ArrowButton("##up", ImGuiDir_Up) && doc.layerId < (int)doc.layers.size() - 1)
    {
        doc.pushUndoDocument("Move layer");
        doc.moveLayer(doc.layerId, 1);
    }
    tooltip(L("Move up"));
    ImGui::SameLine();
    if (ImGui::ArrowButton("##down", ImGuiDir_Down) && doc.layerId > 0)
    {
        doc.pushUndoDocument("Move layer");
        doc.moveLayer(doc.layerId, -1);
    }
    tooltip(L("Move down"));
    ImGui::SameLine();
    if (ImGui::Button(L("Merge")) && doc.layerId > 0)
    {
        doc.pushUndoDocument("Merge");
        doc.mergeLayers(0);
    }
    tooltip(L("Merge down"));
}

void Editor::drawBrushImagePanel()
{
    BrushParams& b = brush[isPaintAction(action) && action != Action_Pen ? action : Action_Brush];
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::BeginCombo("##bdir", b.brushDir.c_str()))
    {
        for (auto& d : _brushDirs)
            if (ImGui::Selectable(d.c_str(), d == b.brushDir))
            {
                brush[Action_Brush].brushDir = d;
                b.brushDir = d;
                refreshBrushFiles();
            }
        ImGui::EndCombo();
    }
    ImGui::BeginChild("##brushes", ImVec2(0, 190), ImGuiChildFlags_Borders);
    float bs = 40 * opt.uiScale;
    int cols = std::max(1, (int)(ImGui::GetContentRegionAvail().x / (bs + 12)));
    std::string dir = resolvePath("data/Presets/Brush/" + b.brushDir);
    auto p = ImGuiPresenter::getInstance();
    ImGuiListClipper clip;
    int rows = ((int)_brushFiles.size() + cols - 1) / cols;
    clip.Begin(rows, bs + 12);
    while (clip.Step())
        for (int row = clip.DisplayStart; row < clip.DisplayEnd; ++row)
            for (int c = 0; c < cols; ++c)
            {
                int i = row * cols + c;
                if (i >= (int)_brushFiles.size())
                    break;
                if (c)
                    ImGui::SameLine();
                ImGui::PushID(i);
                auto tex = thumb(pathJoin(dir, _brushFiles[i]), 48);
                bool sel = _brushFiles[i] == b.image;
                bool hit = tex ? p->imageButton(tex, ImVec2(bs, bs), ImVec2(0, 0), ImVec2(1, 1),
                                                sel ? ImVec4(0.3f, 0.5f, 0.8f, 1) : ImVec4(0.85f, 0.85f, 0.85f, 1))
                               : ImGui::Button("?", ImVec2(bs + 8, bs + 8));
                if (hit)
                {
                    b.image = _brushFiles[i];
                    markTip();
                }
                tooltip(_brushFiles[i].c_str());
                ImGui::PopID();
            }
    ImGui::EndChild();
}

void Editor::drawPatternPanel()
{
    float bs = 56 * opt.uiScale;
    int cols = std::max(1, (int)(ImGui::GetContentRegionAvail().x / (bs + 12)));
    std::string dir = resolvePath(opt.dirPattern);
    auto p = ImGuiPresenter::getInstance();
    for (int i = 0; i < (int)_patternFiles.size(); ++i)
    {
        if (i % cols)
            ImGui::SameLine();
        ImGui::PushID(i);
        std::string f = pathJoin(dir, _patternFiles[i]);
        auto tex = thumb(f, 64);
        bool hit = tex ? p->imageButton(tex, ImVec2(bs, bs), ImVec2(0, 0), ImVec2(1, 1),
                                        f == opt.patternFile ? ImVec4(0.3f, 0.5f, 0.8f, 1) : ImVec4(0, 0, 0, 0))
                       : ImGui::Button(_patternFiles[i].c_str());
        if (hit)
            loadPattern(f);
        tooltip(_patternFiles[i].c_str());
        ImGui::PopID();
    }
    ImGui::TextDisabled("%s", L("Used by the pattern stamp and Edit > Fill with pattern"));
}

void Editor::drawPaperPanel()
{
    Paper& pa = doc.paper;
    bool ch   = false;
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
    if (ImGui::BeginCombo(L("Paper"), pa.name.c_str()))
    {
        for (auto& f : _paperFiles)
            if (ImGui::Selectable(f.c_str(), f == pa.name))
            {
                pa.name = f;
                loadPaper(doc);
            }
        ImGui::EndCombo();
    }
    float col[3] = {pa.color.r / 255.f, pa.color.g / 255.f, pa.color.b / 255.f};
    if (ImGui::ColorEdit3(L("Color"), col, ImGuiColorEditFlags_NoInputs))
    {
        pa.color = fromIm(col);
        doc.invalidateAll();
    }
    if (ImGui::SliderInt(L("Opacity##paper"), &pa.alpha, 0, 255))
        doc.invalidateAll();
    ch |= ImGui::SliderInt(L("Scale"), &pa.scale, 1, 60);
    ch |= ImGui::SliderInt(L("Contrast"), &pa.intensity, 0, 1000);
    ch |= ImGui::SliderInt(L("Brightness"), &pa.brightness, 0, 200);
    ImGui::PopItemWidth();
    // rebuilding the paper is slow : wait for the end of the edition
    static bool pending = false;
    pending |= ch;
    if (pending && !ImGui::IsAnyItemActive())
    {
        pending = false;
        pa.rebuildTile();
        doc.invalidateAll();
    }
    if (ImGui::BeginCombo(L("Preset"), ""))
    {
        for (auto& e : listDir(resolvePath("data/Presets/Background"), {"txt"}))
            if (!e.isDir && ImGui::Selectable(pathStem(e.name).c_str()))
                loadPaperPreset(doc, resolvePath("data/Presets/Background/" + e.name));
        ImGui::EndCombo();
    }
}

// ------------------------------------------------------------------ dialogs

void Editor::openFx(int kind)
{
    if (!doc.canPaint())
        return;
    _fxKind   = kind;
    _fxBackup = doc.cur().image;
    _fxFlag   = true;
    _fxP[0] = _fxP[1] = _fxP[2] = 0;
    switch (kind)
    {
    case Fx_Level:
        _fxP[0] = 0, _fxP[1] = 255;
        break;
    case Fx_Posterize:
        _fxP[0] = 6;
        break;
    case Fx_Noise:
        _fxP[0] = 30;
        break;
    case Fx_Blur:
        _fxP[0] = 2;
        break;
    case Fx_Sharpen:
        _fxP[0] = 100;
        break;
    case Fx_Offset:
        _fxP[0] = doc.width() / 2, _fxP[1] = doc.height() / 2;
        break;
    default:
        break;
    }
    openDialog("Adjust");
}

void Editor::runFx(Image& img, const IRect& r)
{
    switch (_fxKind)
    {
    case Fx_BrightnessContrast:
        fxBrightnessContrast(img, r, _fxP[0], _fxP[1]);
        break;
    case Fx_Level:
        fxLevel(img, r, _fxP[0], _fxP[1]);
        break;
    case Fx_Posterize:
        fxPosterize(img, r, _fxP[0]);
        break;
    case Fx_ColorBalance:
        fxColorBalance(img, r, _fxP[0], _fxP[1], _fxP[2]);
        break;
    case Fx_HueSaturation:
        fxHueSaturation(img, r, _fxP[0], _fxP[1], _fxP[2]);
        break;
    case Fx_Noise:
        fxNoise(img, r, _fxP[0], _fxFlag);
        break;
    case Fx_Blur:
        fxBlur(img, r, _fxP[0]);
        break;
    case Fx_Sharpen:
        fxSharpen(img, r, _fxP[0] / 100.f);
        break;
    case Fx_Offset:
        fxOffset(img, _fxP[0], _fxP[1]);
        break;
    default:
        break;
    }
}

void Editor::drawDialogs()
{
    if (!_dialogToOpen.empty())
    {
        ImGui::OpenPopup(_dialogToOpen.c_str());
        _dialogToOpen.clear();
    }
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
    auto begin = [&](const char* name) {
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        return ImGui::BeginPopupModal(name, nullptr, flags);
    };
    auto okCancel = [&](bool& ok) {
        ImGui::Separator();
        ok = ImGui::Button(L("Ok"), ImVec2(110, 0)) ||
             (!io.WantTextInput && (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)));
        ImGui::SameLine();
        bool cancel = ImGui::Button(L("Cancel"), ImVec2(110, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false);
        return cancel;
    };

    if (begin("New document"))
    {
        ImGui::InputInt(L("Width"), &_newW);
        ImGui::InputInt(L("Height"), &_newH);
        _newW = std::clamp(_newW, 1, 16384), _newH = std::clamp(_newH, 1, 16384);
        if (ImGui::BeginCombo(L("Template"), ""))
        {
            for (auto& t : _templates)
                if (ImGui::Selectable(t.name.c_str()))
                    _newW = t.w, _newH = t.h;
            ImGui::EndCombo();
        }
        bool ok;
        bool cancel = okCancel(ok);
        if (ok)
        {
            opt.imageW = _newW, opt.imageH = _newH;
            newDocument(_newW, _newH);
        }
        if (ok || cancel)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    for (const char* name : {"Resize document", "Resize canvas"})
    {
        if (!begin(name))
            continue;
        bool canvas = std::strcmp(name, "Resize canvas") == 0;
        float ratio = doc.height() / (float)doc.width();
        if (ImGui::InputInt(L("Width"), &_resizeW) && _resizeKeep && !canvas)
            _resizeH = (int)std::lround(_resizeW * ratio);
        if (ImGui::InputInt(L("Height"), &_resizeH) && _resizeKeep && !canvas)
            _resizeW = (int)std::lround(_resizeH / ratio);
        _resizeW = std::clamp(_resizeW, 1, 16384), _resizeH = std::clamp(_resizeH, 1, 16384);
        if (canvas)
        {
            ImGui::TextUnformatted(L("Position of the image"));
            for (int i = 0; i < 9; ++i)
            {
                if (i % 3)
                    ImGui::SameLine();
                ImGui::PushID(i);
                if (ImGui::RadioButton("##a", _anchor == i))
                    _anchor = i;
                ImGui::PopID();
            }
        }
        else
        {
            ImGui::Checkbox(L("Keep the proportions"), &_resizeKeep);
            ImGui::Checkbox(L("Smooth"), &_resizeSmooth);
        }
        bool ok;
        bool cancel = okCancel(ok);
        if (ok)
        {
            doc.pushUndoDocument(canvas ? "Resize canvas" : "Resize");
            if (canvas)
                doc.resizeCanvas(_resizeW, _resizeH, _anchor % 3 - 1, _anchor / 3 - 1);
            else
                doc.resizeDocument(_resizeW, _resizeH, _resizeSmooth);
            afterDocumentChanged(false);
        }
        if (ok || cancel)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (begin("Adjust"))
    {
        bool ch = false;
        ImGui::PushItemWidth(260);
        switch (_fxKind)
        {
        case Fx_BrightnessContrast:
            ch |= ImGui::SliderInt(L("Brightness"), &_fxP[0], -255, 255);
            ch |= ImGui::SliderInt(L("Contrast"), &_fxP[1], -255, 255);
            break;
        case Fx_Level:
            ch |= ImGui::SliderInt(L("Minimum"), &_fxP[0], 0, 254);
            ch |= ImGui::SliderInt(L("Maximum"), &_fxP[1], 1, 255);
            break;
        case Fx_Posterize:
            ch |= ImGui::SliderInt(L("Level"), &_fxP[0], 2, 32);
            break;
        case Fx_ColorBalance:
            ch |= ImGui::SliderInt(L("Red"), &_fxP[0], -255, 255);
            ch |= ImGui::SliderInt(L("Green"), &_fxP[1], -255, 255);
            ch |= ImGui::SliderInt(L("Blue"), &_fxP[2], -255, 255);
            break;
        case Fx_HueSaturation:
            ch |= ImGui::SliderInt(L("Hue"), &_fxP[0], -180, 180);
            ch |= ImGui::SliderInt(L("Saturation"), &_fxP[1], -100, 100);
            ch |= ImGui::SliderInt(L("Value"), &_fxP[2], -100, 100);
            break;
        case Fx_Noise:
            ch |= ImGui::SliderInt(L("Amount"), &_fxP[0], 0, 255);
            ch |= ImGui::Checkbox(L("Monochrome"), &_fxFlag);
            break;
        case Fx_Blur:
            ch |= ImGui::SliderInt(L("Radius"), &_fxP[0], 1, 30);
            break;
        case Fx_Sharpen:
            ch |= ImGui::SliderInt(L("Amount"), &_fxP[0], 0, 400);
            break;
        case Fx_Offset:
            ch |= ImGui::SliderInt("X", &_fxP[0], 0, doc.width());
            ch |= ImGui::SliderInt("Y", &_fxP[1], 0, doc.height());
            break;
        default:
            break;
        }
        ImGui::PopItemWidth();
        // live preview on the layer
        if ((ch || ImGui::IsWindowAppearing()) && _fxBackup.valid())
        {
            doc.cur().image = _fxBackup;
            runFx(doc.cur().image, workRect());
            doc.invalidateAll();
        }
        bool ok;
        bool cancel = okCancel(ok);
        if (ok || cancel)
        {
            doc.cur().image = _fxBackup;
            if (ok)
            {
                doc.pushUndoLayer(doc.layerId, "Adjust");
                runFx(doc.cur().image, workRect());
            }
            doc.invalidateAll();
            _fxBackup = Image();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (begin("Rotate"))
    {
        ImGui::SliderFloat(L("Angle"), &_rotAngle, -180, 180, "%.1f");
        ImGui::Checkbox(L("All the layers"), &_rotAll);
        bool ok;
        bool cancel = okCancel(ok);
        if (ok)
            rotateFree(_rotAngle, _rotAll);
        if (ok || cancel)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (begin("Text"))
    {
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        ImGui::InputTextMultiline("##text", _textBuf, sizeof(_textBuf), ImVec2(360, 110));
        ImGui::SliderInt(L("Size"), &_textSize, 6, 300);
        bool ok;
        bool cancel = okCancel(ok);
        if (ok)
            addText(_textBuf, _textSize, _textX, _textY);
        if (ok || cancel)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (begin("Layer name"))
    {
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        bool enter = ImGui::InputText("##name", _renameBuf, sizeof(_renameBuf), ImGuiInputTextFlags_EnterReturnsTrue);
        bool ok;
        bool cancel = okCancel(ok);
        if ((ok || enter) && _renameBuf[0])
        {
            doc.cur().name = _renameBuf;
            doc.changed    = true;
        }
        if (ok || enter || cancel)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (begin("Preferences"))
    {
        ImGui::PushItemWidth(220);
        if (ImGui::BeginCombo(L("Language"), opt.lang.c_str()))
        {
            for (auto& l : availableLanguages())
                if (ImGui::Selectable(l.c_str(), l == opt.lang))
                {
                    opt.lang = l;
                    loadLanguage(l);
                }
            ImGui::EndCombo();
        }
        float sc = opt.uiScale;
        if (ImGui::SliderFloat(L("Interface scale"), &sc, 0.75f, 2.5f, "%.2f") && !ImGui::IsItemActive())
            opt.uiScale = sc;
        else if (ImGui::IsItemDeactivatedAfterEdit())
            opt.uiScale = sc;
        ImGui::SliderInt(L("Undo levels"), &opt.maxUndo, 1, 64);
        doc.maxUndo = opt.maxUndo;
        ImGui::Checkbox(L("Confirm exit"), &opt.confirmExit);
        ImGui::Checkbox(L("Autosave"), &opt.autosave);
        ImGui::SliderInt(L("Autosave (minutes)"), &opt.autosaveTime, 1, 60);
        ImGui::Checkbox(L("Use the same color for all tools"), &opt.useSameColorForAllTools);
        ImGui::Separator();
        ImGui::Checkbox(L("Grid"), &opt.grid);
        ImGui::InputInt(L("Grid width"), &opt.gridW);
        ImGui::InputInt(L("Grid height"), &opt.gridH);
        opt.gridW = std::max(2, opt.gridW), opt.gridH = std::max(2, opt.gridH);
        float gc[3] = {opt.gridColor.r / 255.f, opt.gridColor.g / 255.f, opt.gridColor.b / 255.f};
        if (ImGui::ColorEdit3(L("Grid color"), gc, ImGuiColorEditFlags_NoInputs))
            opt.gridColor = fromIm(gc);
        float bg[3] = {opt.areaBGColor.r / 255.f, opt.areaBGColor.g / 255.f, opt.areaBGColor.b / 255.f};
        if (ImGui::ColorEdit3(L("Area color"), bg, ImGuiColorEditFlags_NoInputs))
            opt.areaBGColor = fromIm(bg);
        ImGui::Checkbox(L("Filtering"), &opt.filtering);
        ImGui::PopItemWidth();
        ImGui::Separator();
        if (ImGui::Button(L("Ok"), ImVec2(110, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        {
            opt.save(_prefFile);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (begin("About"))
    {
        ImGui::Text("%s %s", ANI_PROGRAM_NAME, ANI_PROGRAM_VERSION);
        ImGui::Separator();
        ImGui::TextUnformatted("2D painting program - free and open-source (GPLv3).\n"
                               "Created by Blendman (2011-2021) in PureBasic, with the help of\n"
                               "the PureBasic community.\n\n"
                               "This version is the port to the Axmol 3.0 engine (C++),\n"
                               "interface made with Dear ImGui.");
        ImGui::Text("Axmol %s", AX_VERSION_STR);
        ImGui::Separator();
        if (ImGui::Button(L("Ok"), ImVec2(110, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (begin("Message"))
    {
        ImGui::TextUnformatted(_message.c_str());
        ImGui::Separator();
        if (ImGui::Button(L("Ok"), ImVec2(110, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (begin("Unsaved changes"))
    {
        ImGui::TextUnformatted(L("The document has been modified. Do you want to save it ?"));
        ImGui::Separator();
        if (ImGui::Button(L("Save"), ImVec2(110, 0)))
        {
            ImGui::CloseCurrentPopup();
            saveFile(false); // runs _pendingAfterSave once saved
        }
        ImGui::SameLine();
        if (ImGui::Button(L("Don't save"), ImVec2(110, 0)))
        {
            ImGui::CloseCurrentPopup();
            auto fn = std::move(_pendingAfterSave);
            _pendingAfterSave = nullptr;
            if (fn)
                fn();
        }
        ImGui::SameLine();
        if (ImGui::Button(L("Cancel"), ImVec2(110, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        {
            _pendingAfterSave = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// ------------------------------------------------------------------ file dialog

void FileDialog::open(bool save, const std::string& title, const std::vector<std::string>& exts,
                      const std::string& startDir, const std::string& defaultName, Callback cb)
{
    _save  = save;
    _title = title + "###filedialog";
    _exts  = exts;
    _cb    = std::move(cb);
    std::string dir = startDir.empty() ? _dir : startDir;
    if (!dir.empty() && !dirExists(dir))
        dir = resolvePath(dir);
    if (dir.empty() || !dirExists(dir))
    {
        const char* home = std::getenv("HOME");
        if (!home)
            home = std::getenv("USERPROFILE");
        dir = home ? home : ".";
    }
    _dir = dir;
    snprintf(_name, sizeof(_name), "%s", defaultName.c_str());
    _open             = true;
    _popupRequested   = true;
    _confirmOverwrite = false;
    refresh();
}

void FileDialog::refresh()
{
    _entries = listDir(_dir, _exts);
    snprintf(_dirBuf, sizeof(_dirBuf), "%s", _dir.c_str());
}

void FileDialog::draw()
{
    if (!_open)
        return;
    if (_popupRequested)
    {
        ImGui::OpenPopup("###filedialog");
        _popupRequested = false;
    }
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_Appearing);
    bool keep = true;
    if (!ImGui::BeginPopupModal(_title.c_str(), &keep, ImGuiWindowFlags_NoSavedSettings))
    {
        _open = false;
        return;
    }
    bool accept = false;
    if (ImGui::Button(".."))
    {
        std::string up = pathDir(_dir);
        if (up.empty())
            up = "/";
        _dir = up;
        refresh();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##dir", _dirBuf, sizeof(_dirBuf), ImGuiInputTextFlags_EnterReturnsTrue) && dirExists(_dirBuf))
    {
        _dir = _dirBuf;
        refresh();
    }
    ImGui::BeginChild("##files", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2 - 6), ImGuiChildFlags_Borders);
    std::string enter;
    for (auto& e : _entries)
    {
        std::string label = (e.isDir ? "[" + e.name + "]" : e.name);
        if (ImGui::Selectable(label.c_str(), !e.isDir && e.name == _name, ImGuiSelectableFlags_AllowDoubleClick |
                                                                              ImGuiSelectableFlags_NoAutoClosePopups))
        {
            if (e.isDir)
            {
                if (ImGui::IsMouseDoubleClicked(0))
                    enter = e.name;
            }
            else
            {
                snprintf(_name, sizeof(_name), "%s", e.name.c_str());
                if (ImGui::IsMouseDoubleClicked(0))
                    accept = true;
            }
        }
    }
    ImGui::EndChild();
    if (!enter.empty())
    {
        _dir = ani::pathJoin(_dir, enter);
        refresh();
    }
    ImGui::SetNextItemWidth(-1);
    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();
    if (ImGui::InputTextWithHint("##name", ani::L("File name"), _name, sizeof(_name), ImGuiInputTextFlags_EnterReturnsTrue))
        accept = true;
    if (ImGui::Button(_save ? ani::L("Save") : ani::L("Open"), ImVec2(110, 0)))
        accept = true;
    ImGui::SameLine();
    if (ImGui::Button(ani::L("Cancel"), ImVec2(110, 0)) || !keep)
    {
        _open = false;
        ImGui::CloseCurrentPopup();
    }
    if (_confirmOverwrite)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 0.7f, 0.3f, 1), "%s", ani::L("The file already exists. Click again to overwrite it."));
    }
    if (accept && _name[0])
    {
        std::string full = ani::pathJoin(_dir, _name);
        if (ani::dirExists(full))
        {
            _dir = full;
            _name[0] = 0;
            refresh();
        }
        else if (!_save && !ani::fileExists(full))
            ; // nothing to open
        else
        {
            // a save without extension gets the first one of the filter
            if (_save && ani::pathExt(full).empty() && !_exts.empty())
                full += "." + _exts[0];
            if (_save && ani::fileExists(full) && !_confirmOverwrite)
                _confirmOverwrite = true;
            else
            {
                _open = false;
                ImGui::CloseCurrentPopup();
                auto cb = _cb;
                if (cb)
                    cb(full);
            }
        }
    }
    ImGui::EndPopup();
}
