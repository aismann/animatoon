// Animatoon - Axmol 3.0 port
// Tools parameters ("Brush" structure of the PureBasic version), brush presets (*.abp),
// brush tip and the stroke engine (dots along the stroke, colour mixing, water...).
#pragma once

#include "AniImage.h"
#include "Document.h"

#include <string>
#include <vector>

namespace ani
{

// Same order as #Action_xxx of the original
enum Action
{
    Action_Pen = 0,
    Action_Brush,
    Action_Spray,
    Action_CloneStamp,
    Action_Particles,
    Action_Line,
    Action_Box,
    Action_Circle,
    Action_Shape,
    Action_Gradient,
    Action_Fill,
    Action_Text,
    Action_Eraser,
    Action_Clear,
    Action_Pipette,
    Action_Select,
    Action_Move,
    Action_Transform,
    Action_Rotate,
    Action_Hand,
    Action_Zoom,
    Action_Count
};
const char* actionName(int action);

// Same order as #ToolType_xxx
enum ToolType
{
    ToolType_Brush = 0,
    ToolType_Pen,
    ToolType_Eraser,
    ToolType_Light,
    ToolType_Dark,
    ToolType_Color,
    ToolType_Shape,
    ToolType_Water,
    ToolType_Smudge,
    ToolType_Glass,
    ToolType_Noise,
    ToolType_Pixel,
    ToolType_Blur,
    ToolType_Sol,
    ToolType_Line,
    ToolType_CloneStamp,
    ToolType_Count
};
const char* toolTypeName(int type);

struct BrushParams
{
    std::string name;
    int version = 6;
    int type    = ToolType_Brush;

    // size
    int size         = 20;
    int sizeW        = 100; ///< % of the size
    int sizeH        = 100;
    int sizeRand     = 0;
    int sizeMin      = 1;
    int sizePressure = 1;
    int sizeRndFactor = 0;

    // alpha
    int alpha         = 255;
    int alphaPressure = 0;
    int alphaRand     = 0;
    int alphaMin      = 0;
    int alphaVsTime   = 0;
    int alphaFactorVsTime = 0;
    int alphaFG       = 255;

    // dynamics
    int scatter       = 0;
    int rotate        = 0;
    int randRot       = 0;
    int rotateByAngle = 0;
    int rotateMin     = 0;

    // render of the tip
    int hardness  = 0;
    int softness  = 0;
    int intensity = 0;
    int smooth    = 1;
    int trim      = 0;
    int addNoiseRandomOnImage = 0;
    int addNoiseOnImgMin = 0, addNoiseOnImgMax = 0;

    // stroke
    int pas   = 15; ///< space between two dots, % of the size
    int trait = 1;  ///< draw a line of dots between two mouse positions

    // colour
    int mix      = 0;
    int mixType  = 2; ///< 0 classic (viscosity), 1 inverse (smear), 2 old, 3 new
    int visco    = 0;
    int wash     = 1; ///< the brush takes its colour back at mouse up
    int water    = 0;
    int mixLayer = 1; ///< 0 all layers, 1 current layer, 2 current layer + the ones below
    int blendmode = Bm_Normal;
    int filter   = 20; ///< strength of light / dark / noise... tools
    int symetry  = 0;  ///< 0 none, 1 X, 2 Y, 3 XY, 4 the 4 views

    Color color{0, 0, 0, 255};     ///< main colour ("background" colour box of the original UI)
    Color colorFG{255, 255, 255, 255}; ///< second colour (gradient, outline)

    // brush image
    std::string image    = "brush1.png";
    std::string brushDir = "blendman";
    int id               = 1;

    // spray
    int spray   = 40; ///< radius
    int nbSpray = 6;  ///< dots per step

    // shapes
    int shapeOutSize = 0;
    int shapePlain   = 1;
    int roundX = 0, roundY = 0;
    int gradientType = 0; ///< 0 linear, 1 circular, 2 elliptical, 3 conical, 4 boxed

    // fill
    int tolerance = 10;
    int fillAllLayers = 0;

    // pipette : 0 current layer, 1 all layers
    int pickerAllLayers = 1;

    bool load(const std::string& abpFile);
    bool save(const std::string& abpFile) const;
};

/// Alpha mask of the brush at its nominal size
struct BrushTip
{
    int w = 0, h = 0;
    std::vector<uint8_t> mask;
    bool valid() const { return w > 0 && h > 0; }
    /// Build the mask from the brush picture and the parameters (BrushUpdateImage)
    void build(const Image& source, const BrushParams& p);
    /// Soft round tip used when the picture is missing
    void buildRound(const BrushParams& p);
    /// Picture of the tip (for the UI previews)
    Image toImage(const Color& c) const;
};

class BrushEngine
{
public:
    /// Rebuild the tip after a change of image / size / parameters
    void updateTip(const BrushParams& p);
    const BrushTip& tip() const { return _tip; }
    const Image& sourceImage() const { return _source; }

    /// Pattern used by the clone stamp tool
    void setPattern(const Image* pattern) { _pattern = pattern; }

    void beginStroke(Document& doc, const BrushParams& p, int action, float x, float y, float pressure);
    void strokeTo(Document& doc, BrushParams& p, float x, float y, float pressure);
    void endStroke(Document& doc, BrushParams& p);
    bool inStroke() const { return _active; }

    /// One dot of the brush. Public so that the previews can use it.
    void dab(Image& img, const BrushParams& p, int type, float cx, float cy, float scale, float angleDeg,
             const Color& c, int alpha, const IRect& clip, bool lockAlpha, IRect* dirty);

    /// Draw a sample stroke in an image (preset preview)
    void drawPreview(Image& img, const BrushParams& p);

private:
    void placeDot(Document& doc, BrushParams& p, float x, float y, float pressure, float dirAngle);
    Color mixedColor(Document& doc, BrushParams& p, int x, int y);
    void dabWithSymetry(Document& doc, const BrushParams& p, float x, float y, float scale, float angle,
                        const Color& c, int alpha);

    BrushTip _tip;
    Image _source;
    std::string _sourceKey;
    const Image* _pattern = nullptr;

    bool _active  = false;
    int _action   = Action_Brush;
    int _type     = ToolType_Brush;
    float _lastX = 0, _lastY = 0;
    float _carry = 0; ///< distance already travelled since the last dot
    bool _first  = true;
    IRect _clip;

    // colour mixing state
    Color _baseColor;         ///< colour chosen by the user at the start of the stroke
    Color _col, _oldCol, _newCol;
    int _viscoCur = 0;

    // smudge
    Image _smudge;
    bool _hasSmudge = false;
    std::vector<uint8_t> _scratch;
};

} // namespace ani
