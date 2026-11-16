// Animatoon - Axmol 3.0 port
#include "AppData.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <unordered_map>

namespace ani
{

// ---------------------------------------------------------------- options

void Options::load(const std::string& file)
{
    IniFile ini;
    if (!ini.load(file))
        return;
    const std::string g = "General";
    lang        = ini.getString("Lang", lang);
    zoom        = std::clamp(ini.getInt("Zoom", zoom, g), 10, 5000);
    autosave    = ini.getInt("autosave", autosave, g) != 0;
    autosaveTime = std::max(1, ini.getInt("autosaveTime", autosaveTime, g));
    confirmExit = ini.getInt("ConfirmExit", confirmExit, g) != 0;
    maxUndo     = std::clamp(ini.getInt("Maxundo", maxUndo, g), 1, 64);
    paper       = ini.getString("Paper", paper, g);
    areaBGColor = Color::fromPB(ini.getInt("AreaBGColor", areaBGColor.toPB(), g));
    dirPattern  = ini.getString("DirPattern", dirPattern, g);
    swatch      = ini.getString("Swatch", swatch, g);
    pathOpen    = ini.getString("PathOpen", pathOpen, g);
    pathSave    = ini.getString("PathSave", pathSave, g);
    dirPreset   = ini.getString("DirPresets", dirPreset, g);
    imageW      = std::clamp(ini.getInt("ImageW", imageW, g), 1, 16384);
    imageH      = std::clamp(ini.getInt("ImageH", imageH, g), 1, 16384);
    grid        = ini.getInt("Grid", grid, g) != 0;
    gridW       = std::max(2, ini.getInt("GridW", gridW, g));
    gridH       = std::max(2, ini.getInt("GridH", gridH, g));
    gridColor   = Color::fromPB(ini.getInt("GridColor", gridColor.toPB(), g));
    swatchColumns = std::clamp(ini.getInt("SwatchColumns", swatchColumns, g), 1, 32);
    theme       = ini.getString("Theme", theme, g);
    filtering   = ini.getInt("Filtering", filtering, g) != 0;
    paperAlpha  = ini.getInt("PaperAlpha", paperAlpha, g);
    paperScale  = ini.getInt("PaperScale", paperScale, g);
    paperIntensity  = ini.getInt("PaperIntensity", paperIntensity, g);
    paperBrightness = ini.getInt("PaperBrightness", paperBrightness, g);
    paperColor  = Color::fromPB(ini.getInt("Papercolor", paperColor.toPB(), g));
    showPanelTool   = ini.getInt("ShowPanelToolparameters", showPanelTool, g) != 0;
    showPanelColors = ini.getInt("ShowPanelColors", showPanelColors, g) != 0;
    showPanelLayers = ini.getInt("ShowPanelLayers", showPanelLayers, g) != 0;
    showPanelSwatch = ini.getInt("ShowPanelswatchs", showPanelSwatch, g) != 0;
    showPanelPresets = ini.getInt("ShowPanelPresets", showPanelPresets, g) != 0;
    showStatusbar   = ini.getInt("Statusbar", showStatusbar, g) != 0;
    useSameColorForAllTools = ini.getInt("UseSameColorForAllTool", useSameColorForAllTools, g) != 0;
    brushName   = ini.getString("BrushName", brushName, g);
    brushDir    = ini.getString("BrushDir", brushDir, g);
    patternFile = ini.getString("PatternFile", patternFile, g);
    uiScale     = std::clamp(ini.getInt("UIScale", 100, g) / 100.f, 0.5f, 3.f);
}

void Options::save(const std::string& file) const
{
    IniFile ini;
    const std::string g = "General";
    ini.set("Lang", lang);
    ini.set("Zoom", zoom, g);
    ini.set("autosave", autosave, g);
    ini.set("autosaveTime", autosaveTime, g);
    ini.set("ConfirmExit", confirmExit, g);
    ini.set("Maxundo", maxUndo, g);
    ini.set("Paper", paper, g);
    ini.set("AreaBGColor", areaBGColor.toPB(), g);
    ini.set("DirPattern", dirPattern, g);
    ini.set("Swatch", swatch, g);
    ini.set("PathOpen", pathOpen, g);
    ini.set("PathSave", pathSave, g);
    ini.set("DirPresets", dirPreset, g);
    ini.set("ImageW", imageW, g);
    ini.set("ImageH", imageH, g);
    ini.set("Grid", grid, g);
    ini.set("GridW", gridW, g);
    ini.set("GridH", gridH, g);
    ini.set("GridColor", gridColor.toPB(), g);
    ini.set("SwatchColumns", swatchColumns, g);
    ini.set("Theme", theme, g);
    ini.set("Filtering", filtering, g);
    ini.set("PaperAlpha", paperAlpha, g);
    ini.set("PaperScale", paperScale, g);
    ini.set("PaperIntensity", paperIntensity, g);
    ini.set("PaperBrightness", paperBrightness, g);
    ini.set("Papercolor", paperColor.toPB(), g);
    ini.set("ShowPanelToolparameters", showPanelTool, g);
    ini.set("ShowPanelColors", showPanelColors, g);
    ini.set("ShowPanelLayers", showPanelLayers, g);
    ini.set("ShowPanelswatchs", showPanelSwatch, g);
    ini.set("ShowPanelPresets", showPanelPresets, g);
    ini.set("Statusbar", showStatusbar, g);
    ini.set("UseSameColorForAllTool", useSameColorForAllTools, g);
    ini.set("BrushName", brushName, g);
    ini.set("BrushDir", brushDir, g);
    ini.set("PatternFile", patternFile, g);
    ini.set("UIScale", (int)(uiScale * 100), g);
    ini.save(file, "Animatoon - Preferences File");
}

// ---------------------------------------------------------------- language

static std::unordered_map<std::string, std::string> s_lang;

static std::string langKey(const std::string& s)
{
    // keys are compared without case and without spaces (as "ReplaceString(text, " ", "")" does)
    std::string k;
    for (char c : s)
        if (c != ' ' && c != '\t')
            k += (char)std::tolower((unsigned char)c);
    return k;
}

void loadLanguage(const std::string& lang)
{
    s_lang.clear();
    IniFile ini;
    if (!ini.load(resolvePath("data/Lang/" + lang + ".ini")))
        return;
    for (auto& kv : ini.entries())
        if (!kv.second.empty())
            s_lang[langKey(kv.first)] = kv.second;
}

const char* L(const char* text)
{
    if (s_lang.empty())
        return text;
    auto it = s_lang.find(langKey(text));
    return it == s_lang.end() ? text : it->second.c_str();
}

std::vector<std::string> availableLanguages()
{
    std::vector<std::string> out;
    for (auto& e : listDir(resolvePath("data/Lang"), {"ini"}))
        if (!e.isDir)
            out.push_back(pathStem(e.name));
    if (out.empty())
        out.push_back("eng");
    return out;
}

// ---------------------------------------------------------------- swatch

bool Swatch::load(const std::string& file)
{
    std::string text = readTextFile(file);
    if (text.empty())
        return false;
    colors.clear();
    name = pathStem(file);
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line.rfind("GIMP", 0) == 0)
            continue;
        if (line.rfind("Name:", 0) == 0)
        {
            name = trim(line.substr(5));
            continue;
        }
        if (line.rfind("Columns:", 0) == 0)
        {
            columns = std::clamp(std::atoi(line.substr(8).c_str()), 1, 32);
            continue;
        }
        std::istringstream ls(line);
        int r, g, b;
        if (!(ls >> r >> g >> b))
            continue;
        SwatchColor sc;
        sc.color = Color(r, g, b);
        std::getline(ls, sc.name);
        sc.name = trim(sc.name);
        colors.push_back(sc);
    }
    return true;
}

bool Swatch::save(const std::string& file) const
{
    std::string out = "GIMP Palette\nName: " + name + "\nColumns: " + std::to_string(columns) + "\n#\n";
    for (auto& c : colors)
        out += std::to_string(c.color.r) + " " + std::to_string(c.color.g) + " " + std::to_string(c.color.b) + "\t" +
               (c.name.empty() ? "Untitled" : c.name) + "\n";
    return writeTextFile(file, out);
}

void Swatch::createFromImage(const Image& img, int maxColors)
{
    colors.clear();
    if (!img.valid())
        return;
    // histogram of the colours reduced to 4 bits per channel, most used first
    std::map<int, std::pair<int, Color>> hist;
    int step = std::max(1, std::max(img.width(), img.height()) / 256);
    for (int y = 0; y < img.height(); y += step)
        for (int x = 0; x < img.width(); x += step)
        {
            Color c = img.get(x, y);
            if (c.a < 128)
                continue;
            int key = (c.r >> 4) << 8 | (c.g >> 4) << 4 | (c.b >> 4);
            auto& h = hist[key];
            h.first++;
            h.second = c;
        }
    std::vector<std::pair<int, Color>> v;
    for (auto& kv : hist)
        v.push_back(kv.second);
    std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.first > b.first; });
    for (size_t i = 0; i < v.size() && (int)i < maxColors; ++i)
        colors.push_back({Color(v[i].second.r, v[i].second.g, v[i].second.b), ""});
}

// ---------------------------------------------------------------- templates

std::vector<DocTemplate> loadTemplates()
{
    std::vector<DocTemplate> out;
    std::istringstream in(readTextFile(resolvePath("data/Presets/Template/templates.txt")));
    std::string line;
    while (std::getline(in, line))
    {
        auto f = split(trim(line), '|');
        if (f.size() < 5)
            continue;
        DocTemplate t;
        t.name = f[0];
        t.dpi  = std::atoi(f[1].c_str());
        t.w    = std::atoi(f[3].c_str());
        t.h    = std::atoi(f[4].c_str());
        if (t.w > 0 && t.h > 0)
            out.push_back(t);
    }
    return out;
}

// ---------------------------------------------------------------- paper

void loadPaper(Document& doc)
{
    doc.paper.texture = Image();
    std::string path  = resolvePath("data/Paper/" + doc.paper.name);
    if (fileExists(path))
        doc.paper.texture.load(path);
    doc.paper.rebuildTile();
    doc.invalidateAll();
}

bool loadPaperPreset(Document& doc, const std::string& file)
{
    std::string text = readTextFile(file);
    if (text.empty())
        return false;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line))
    {
        auto f = split(trim(line), ',');
        if (f.size() >= 6 && f[0] == "paper")
        {
            doc.paper.name       = f[1];
            doc.paper.alpha      = std::atoi(f[2].c_str());
            doc.paper.scale      = std::max(1, std::atoi(f[3].c_str()));
            doc.paper.intensity  = std::atoi(f[4].c_str());
            doc.paper.brightness = std::atoi(f[5].c_str());
        }
        else if (f.size() >= 2 && f[0] == "color")
        {
            int c = std::atoi(f[1].c_str());
            doc.paper.color = c < 0 ? Color(255, 255, 255) : Color::fromPB(c);
        }
    }
    loadPaper(doc);
    return true;
}

// ---------------------------------------------------------------- documents

static bool openAni(Document& doc, const std::string& file, std::string* error)
{
    std::map<std::string, std::vector<uint8_t>> files;
    if (!readZip(file, files))
    {
        if (error)
            *error = "Unable to read the archive";
        return false;
    }
    std::string info;
    for (auto& kv : files)
        if (pathExt(kv.first) == "txt")
            info = toUtf8(std::string(kv.second.begin(), kv.second.end()));
    if (info.empty())
    {
        if (error)
            *error = "Not an Animatoon document (no information file)";
        return false;
    }

    struct LayerInfo
    {
        Layer layer;
        int index = 0, order = 0;
    };
    std::vector<LayerInfo> infos;
    int w = 0, h = 0, n = 0;
    std::istringstream in(info);
    std::string line;
    while (std::getline(in, line))
    {
        auto f = split(trim(line), '|');
        if (f.empty())
            continue;
        auto I = [&](size_t i, int def = 0) { return i < f.size() && !f[i].empty() ? std::atoi(f[i].c_str()) : def; };
        auto S = [&](size_t i) { return i < f.size() ? f[i] : std::string(); };
        if (f[0] == "Background")
        {
            doc.paper.name      = S(1);
            doc.paper.alpha     = I(2, 255);
            doc.paper.scale     = std::max(1, I(3, 10));
            doc.paper.intensity = I(4);
            doc.paper.color     = Color::fromPB(I(5, 0xFFFFFF));
            doc.paper.pos       = I(6, 100);
        }
        else if (f[0] == "Image")
        {
            w = I(1), h = I(2);
        }
        else if (f[0] == "Layer")
        {
            LayerInfo li;
            li.index           = n++;
            li.layer.name      = S(1);
            li.layer.alpha     = I(2, 255);
            li.layer.bm        = std::clamp(I(3), 0, Bm_Count - 1);
            li.layer.lockAlpha = I(4) != 0;
            li.layer.locked    = I(5) != 0;
            li.layer.lockMove  = I(6) != 0;
            li.layer.lockPaint = I(7) != 0;
            li.order           = I(8);
            li.layer.view      = I(9, 1) != 0;
            li.layer.offX      = I(10);
            li.layer.offY      = I(11);
            li.layer.link      = I(15) != 0;
            li.layer.type      = I(16);
            li.layer.text      = S(17);
            li.layer.fontColor = Color::fromPB(I(21));
            li.layer.fontName  = S(22);
            li.layer.fontSize  = I(23, 24);
            infos.push_back(li);
        }
    }
    if (w <= 0 || h <= 0 || infos.empty())
    {
        if (error)
            *error = "The document has no size or no layer";
        return false;
    }

    doc.create(w, h);
    doc.layers.clear();
    std::stable_sort(infos.begin(), infos.end(), [](const LayerInfo& a, const LayerInfo& b) { return a.order < b.order; });
    for (auto& li : infos)
    {
        int idx = doc.addLayer(li.layer.name, (int)doc.layers.size());
        Layer& l = doc.layers[idx];
        int uid  = l.uid;
        Image keep = std::move(l.image);
        l       = li.layer;
        l.uid   = uid;
        l.image = std::move(keep);
        // the picture : <name>_Layer<i>.png
        std::string suffix = "_layer" + std::to_string(li.index) + ".png";
        for (auto& kv : files)
        {
            std::string lower = toLower(kv.first);
            if (lower.size() >= suffix.size() && lower.compare(lower.size() - suffix.size(), suffix.size(), suffix) == 0)
            {
                Image img;
                if (img.loadFromMemory(kv.second.data(), kv.second.size()))
                    l.image.copyFrom(img, l.offX, l.offY);
                break;
            }
        }
        l.offX = l.offY = 0;
    }
    doc.layerId = (int)doc.layers.size() - 1;
    return true;
}

bool openDocument(Document& doc, const std::string& file, std::string* error)
{
    std::string ext = pathExt(file);
    bool ok         = false;
    if (ext == "ani")
        ok = openAni(doc, file, error);
    else
    {
        Image img;
        if (img.load(file))
        {
            doc.create(img.width(), img.height());
            doc.layers[0].image = std::move(img);
            doc.layers[0].name  = pathStem(file);
            ok = true;
        }
        else if (error)
            *error = "Unable to load the picture";
    }
    if (!ok)
        return false;
    doc.name     = pathStem(file);
    doc.filePath = ext == "ani" ? file : "";
    loadPaper(doc);
    doc.clearHistory();
    doc.changed = false;
    doc.invalidateAll();
    return true;
}

bool saveDocument(Document& doc, const std::string& fileIn, std::string* error)
{
    std::string file = fileIn;
    if (pathExt(file) != "ani")
        file += ".ani";
    std::string name = pathStem(file);
    ZipWriter zip;
    std::string info = "; Made By Animatoon \n";
    info += std::string("Version|") + ANI_PROGRAM_VERSION + "|\n";
    info += "Background|" + doc.paper.name + "|" + std::to_string(doc.paper.alpha) + "|" + std::to_string(doc.paper.scale) +
            "|" + std::to_string(doc.paper.intensity) + "|" + std::to_string(doc.paper.color.toPB()) + "|" +
            std::to_string(doc.paper.pos) + "\n";
    int nb = (int)doc.layers.size() - 1;
    info += "Image|" + std::to_string(doc.width()) + "|" + std::to_string(doc.height()) + "|" + std::to_string(nb) +
            "|100|0|0|\n";
    for (int i = 0; i <= nb; ++i)
    {
        const Layer& l = doc.layers[i];
        std::vector<uint8_t> png;
        if (!l.image.encodePNG(png))
        {
            if (error)
                *error = "Unable to encode the layer " + l.name;
            return false;
        }
        zip.add(name + "_Layer" + std::to_string(i) + ".png", png);
        std::string nm = l.name;
        std::replace(nm.begin(), nm.end(), '|', ' ');
        std::string txt = l.text;
        std::replace(txt.begin(), txt.end(), '|', ' ');
        std::replace(txt.begin(), txt.end(), '\n', ' ');
        info += "Layer|" + nm + "|" + std::to_string(l.alpha) + "|" + std::to_string(l.bm) + "|" +
                std::to_string(l.lockAlpha) + "|" + std::to_string(l.locked) + "|" + std::to_string(l.lockMove) + "|" +
                std::to_string(l.lockPaint) + "|" + std::to_string(i) + "|" + std::to_string(l.view) + "|0|0|" +
                std::to_string(doc.width()) + "|" + std::to_string(doc.height()) + "|0|" + std::to_string(l.link) + "|" +
                std::to_string(l.type) + "|" + txt + "|0|0|0|" + std::to_string(l.fontColor.toPB()) + "|" + l.fontName +
                "|" + std::to_string(l.fontSize) + "|0|\n";
    }
    zip.add(name + ".txt", info);
    if (!zip.save(file))
    {
        if (error)
            *error = "Unable to write the file " + file;
        return false;
    }
    doc.filePath = file;
    doc.name     = name;
    doc.changed  = false;
    return true;
}

} // namespace ani
