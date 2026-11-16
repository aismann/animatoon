// Drawing3D for Axmol 3.0 - port of the PureBasic "Drawing3D.pbi" module.
//
// The PureBasic original is a per-pixel software renderer (pixel lists sorted by
// depth, composited back-to-front). Axmol's DrawNode is a 2D triangle batcher, so
// this port does the 3D work on the CPU (rotation, camera, lighting, projection,
// near-plane clipping) and hands DrawNode a list of projected, per-vertex coloured
// triangles, sorted far-to-near (painter's algorithm).
//
// This header/Drawing3D.cpp have NO dependency on Axmol, so the maths can be
// tested headless. The glue to ax::DrawNode lives in Drawing3DAx.h/.cpp.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace d3d
{

// ---------------------------------------------------------------------------
// Colours
// ---------------------------------------------------------------------------

// PureBasic colour layout: $AABBGGRR (what RGBA(r,g,b,a) returns in PureBasic).
using ColorPB = int64_t;

// "Ignore a colour parameter" (PureBasic: #Drawing3D_Ignore = -$100000000).
constexpr ColorPB Ignore = -0x100000000LL;

constexpr ColorPB rgba(int r, int g, int b, int a = 255)
{
    return static_cast<ColorPB>(static_cast<uint32_t>(r & 255) | (static_cast<uint32_t>(g & 255) << 8) |
                                (static_cast<uint32_t>(b & 255) << 16) | (static_cast<uint32_t>(a & 255) << 24));
}

// Premultiplied float colour (what the original calls Color4).
struct Color4
{
    float r = 0, g = 0, b = 0, a = 0;
};

Color4 colorFromPB(ColorPB color);

// ---------------------------------------------------------------------------
// Maths
// ---------------------------------------------------------------------------

struct Vec3
{
    float x = 0, y = 0, z = 0;
};

inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }

struct Matrix3
{
    float m[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
};

// ---------------------------------------------------------------------------
// Draw modes
// ---------------------------------------------------------------------------

namespace Mode
{
enum : int
{
    Default   = 0,
    Outline   = 1,  // triangles/planes/boxes are drawn as lines
    FrontOnly = 2,  // back-face culling
};
}

enum class Move
{
    Absolute,  // #PB_Absolute
    Relative,  // #PB_Relative
};

// ---------------------------------------------------------------------------
// Image3D (texture for DrawImage3D / DrawText3D)
// ---------------------------------------------------------------------------

class Image3D
{
public:
    int width  = 0;
    int height = 0;
    std::vector<Color4> pixels;  // row-major, top row first, premultiplied

    static std::shared_ptr<Image3D> create(int width, int height);
    // 8-bit RGBA data (top row first).
    static std::shared_ptr<Image3D> fromRGBA8(const uint8_t* rgba, int width, int height, bool premultiplied);

    Color4& at(int x, int y) { return pixels[static_cast<size_t>(y) * width + x]; }
    const Color4& at(int x, int y) const { return pixels[static_cast<size_t>(y) * width + x]; }

    // Bilinear sample, u/v in [0,1], clamped to the edge.
    Color4 sample(float u, float v) const;
};
using Image3DPtr = std::shared_ptr<Image3D>;

// ---------------------------------------------------------------------------
// Result of a render pass: projected triangles in *PureBasic screen space*
// (origin top-left, y down), premultiplied colours, sorted far -> near.
// ---------------------------------------------------------------------------

struct ScreenTriangle
{
    float x[3], y[3];
    Color4 c[3];
    float depth;  // view-space z (negative, more negative = further away)
};

// A line segment in *scene space* (scene rotation applied, camera position not subtracted),
// premultiplied colours. For rendering wireframes with a real 3D line renderer (DrawNode3D).
struct Line3D
{
    Vec3 a, b;
    Color4 ca, cb;
};

// ---------------------------------------------------------------------------
// The renderer
// ---------------------------------------------------------------------------

class Drawing3D
{
public:
    // Rasterises text to a white image whose alpha channel is the glyph coverage.
    // Needed only for drawText3D. See Drawing3DAx.h for an Axmol implementation.
    using TextRasterizer = std::function<Image3DPtr(const std::string& text)>;

    // --- StartDrawing3D / StopDrawing3D -----------------------------------
    // Starts a new scene for a canvas of the given pixel size.
    // Light list, geometry and camera position are reset; scene orientation
    // (rotation) and mode persist between scenes, exactly like the original.
    void start(int width, int height, float fieldOfView = 75.0f);
    // Sorts everything far -> near. After this, triangles() is ready to render.
    void stop();

    // --- Scene state -------------------------------------------------------
    void setMode(int mode) { mode_ = mode; }                                         // Drawing3DMode
    void setPosition(float x, float y, float z, Move move = Move::Absolute);         // Drawing3DPosition
    void setRotation(float rx, float ry, float rz, Move move = Move::Absolute);     // Drawing3DRotation (degrees)
    void setBackground(ColorPB color = 0xFF000000LL);                                // Drawing3DBackground
    void addLight(float dx, float dy, float dz, ColorPB color);                      // Drawing3DLight

    // --- Primitives --------------------------------------------------------
    void drawPoint3D(float x, float y, float z, ColorPB color);
    void drawLine3D(float x1, float y1, float z1, float x2, float y2, float z2, ColorPB color1,
                    ColorPB color2 = Ignore);
    void drawTriangle3D(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3,
                        ColorPB color1, ColorPB color2 = Ignore, ColorPB color3 = Ignore);
    void drawPlane3D(float x, float y, float z, float width, float height, float rx, float ry, float rz,
                     ColorPB color1, ColorPB color2 = Ignore, ColorPB color3 = Ignore, ColorPB color4 = Ignore);
    void drawDisk3D(float x, float y, float z, float radius, float rx, float ry, float rz, ColorPB color);
    void drawBox3D(float x, float y, float z, float width, float height, float depth, float rx, float ry, float rz,
                   ColorPB color);
    void drawCylinder3D(float x, float y, float z, float radius, float height, float rx, float ry, float rz,
                        ColorPB color1, ColorPB color2 = Ignore, int detail = 36);
    void drawImage3D(const Image3DPtr& image, float x, float y, float z, float width, float height, float rx = 0,
                     float ry = 0, float rz = 0);
    void drawTransformedImage3D(const Image3DPtr& image, float x1, float y1, float z1, float x2, float y2, float z2,
                                float x3, float y3, float z3, float x4, float y4, float z4);
    void drawText3D(float x, float y, float z, float rx, float ry, float rz, const std::string& text, ColorPB color);

    // --- Port-specific tuning ---------------------------------------------
    // DrawNode has no depth buffer, so intersecting surfaces can only be resolved
    // by splitting them. Triangles whose longest projected edge exceeds this many
    // pixels are split recursively (0 = off). Use ~8-16 for scenes in which
    // surfaces penetrate each other.
    void setSubdivision(float maxEdgePixels) { subdivision_ = maxEdgePixels; }
    // Cell size (pixels) used when sampling Image3D textures onto triangles.
    void setImageDetail(float pixels) { imageDetail_ = pixels; }
    // Thickness of lines (pixels).
    void setLineWidth(float pixels) { lineWidth_ = pixels; }
    void setTextRasterizer(TextRasterizer rasterizer) { textRasterizer_ = std::move(rasterizer); }

    // --- Results -----------------------------------------------------------
    const std::vector<ScreenTriangle>& triangles() const { return triangles_; }
    // All lines of the scene (outline mode, drawLine3D) in scene space, unclipped.
    const std::vector<Line3D>& lines3D() const { return lines3D_; }
    Vec3 cameraPosition() const { return position_; }
    Color4 background() const { return background_; }
    int width() const { return width_; }
    int height() const { return height_; }
    float focalLength() const { return focal_; }

    // CPU reference rasteriser: renders the current result into an RGBA8
    // (non-premultiplied, top row first) buffer of width() x height() pixels,
    // including the background. Used for saving PNGs and for tests.
    void rasterize(std::vector<uint8_t>& rgba) const;

private:
    struct Vtx
    {
        Vec3 p;      // view space
        Color4 c;    // premultiplied
        float u = 0, v = 0;
    };

    Vec3 toView(const Matrix3& object, Vec3 offset, Vec3 local) const;
    Vtx makeVtx(Vec3 view, Color4 color) const;

    void pushTriangle(Vtx a, Vtx b, Vtx c, const Image3D* texture);
    void pushLine(Vtx a, Vtx b);
    void pushPoint(Vtx a);
    void subdivideAndEmit(const Vtx& a, const Vtx& b, const Vtx& c, const Image3D* texture, int depth);
    void emitTriangle(float x0, float y0, float x1, float y1, float x2, float y2, Color4 c0, Color4 c1, Color4 c2,
                      float depth);
    void project(Vec3 p, float& x, float& y) const;
    void drawTexturedQuad(const Image3DPtr& image, const Vec3 view[4]);

    int width_ = 0, height_ = 0;
    float centerX_ = 0, centerY_ = 0, focal_ = 1;
    Vec3 position_;
    Matrix3 orientation_;
    Color4 background_;
    int mode_ = Mode::Default;

    struct Light
    {
        Vec3 dir;
        Color4 color;
    };
    std::vector<Light> lights_;
    std::vector<ScreenTriangle> triangles_;
    std::vector<Line3D> lines3D_;

    float nearPlane_   = 1.0f;
    float subdivision_ = 0.0f;
    float imageDetail_ = 8.0f;
    float lineWidth_   = 1.0f;
    TextRasterizer textRasterizer_;
};

}  // namespace d3d
