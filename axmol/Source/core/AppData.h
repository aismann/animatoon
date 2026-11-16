// Animatoon - Axmol 3.0 port
// Program options (Pref.ini), language, swatches and the document files (.ani).
#pragma once

#include "AniImage.h"
#include "Document.h"
#include "Util.h"

#include <string>
#include <vector>

namespace ani
{

#define ANI_PROGRAM_NAME "Animatoon"
#define ANI_PROGRAM_VERSION "0.7.0"

// ---------------------------------------------------------------- options (OptionsIE)
struct Options
{
    std::string lang     = "eng";
    std::string theme    = "data/Themes/Animatoon";
    std::string paper    = "paper0.png";
    std::string swatch   = "data/Presets/Swatch/classic-kit.gpl";
    std::string dirPreset = "data/Presets/Bank/blendman";
    std::string dirPattern = "data/Presets/Patterns";
    std::string pathOpen, pathSave;
    std::string brushName = "brush1.png", brushDir = "blendman";
    std::string patternFile;

    int zoom        = 100;
    int maxUndo     = 16;
    bool confirmExit = true;
    bool autosave    = false;
    int autosaveTime = 10; ///< minutes
    Color areaBGColor{120, 120, 120, 255};

    bool grid = false;
    int gridW = 32, gridH = 32;
    Color gridColor{0, 0, 0, 255};
    bool filtering = true; ///< smooth display of the picture when zoomed

    int imageW = 1024, imageH = 768;
    int swatchColumns = 7;

    bool showPanelTool   = true;
    bool showPanelColors = true;
    bool showPanelLayers = true;
    bool showPanelSwatch = true;
    bool showPanelPresets = true;
    bool showStatusbar   = true;
    bool useSameColorForAllTools = true;
    float uiScale = 1.0f;

    // paper
    int paperAlpha = 255, paperScale = 10, paperIntensity = 0, paperBrightness = 0;
    Color paperColor{255, 255, 255, 255};

    void load(const std::string& file);
    void save(const std::string& file) const;
};

// ---------------------------------------------------------------- language
/// Load data/Lang/<lang>.ini
void loadLanguage(const std::string& lang);
/// Translation of an english text (returns the text itself when unknown)
const char* L(const char* text);
std::vector<std::string> availableLanguages();

// ---------------------------------------------------------------- swatches (GIMP palette .gpl)
struct SwatchColor
{
    Color color;
    std::string name;
};
struct Swatch
{
    std::string name = "Animatoon";
    int columns      = 7;
    std::vector<SwatchColor> colors;
    bool load(const std::string& gplFile);
    bool save(const std::string& gplFile) const;
    void createFromImage(const Image& img, int maxColors);
};

// ---------------------------------------------------------------- templates (New document)
struct DocTemplate
{
    std::string name;
    int dpi = 100, w = 1024, h = 768;
};
std::vector<DocTemplate> loadTemplates();

// ---------------------------------------------------------------- documents
/// Open a picture (png, jpg, bmp...) or an Animatoon document (.ani = zip of png + a text file)
bool openDocument(Document& doc, const std::string& file, std::string* error = nullptr);
bool saveDocument(Document& doc, const std::string& file, std::string* error = nullptr);
/// Load the paper picture named doc.paper.name from data/Paper
void loadPaper(Document& doc);
/// Read / write the background presets of data/Presets/Background
bool loadPaperPreset(Document& doc, const std::string& file);

} // namespace ani
