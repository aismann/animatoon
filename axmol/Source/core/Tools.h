// Animatoon - Axmol 3.0 port
// Drawing primitives used by the tools : anti-aliased line / box / ellipse, gradients,
// flood fill, and the picture adjustments / filters of the "Image" and "Filters" menus.
#pragma once

#include "AniImage.h"

namespace ani
{

// ---- shapes (the colour alpha is the opacity of the shape) ----
void drawLine(Image& img, float x0, float y0, float x1, float y1, float width, const Color& c, const IRect& clip,
              bool lockAlpha = false, IRect* dirty = nullptr);
/// Box with optional rounded corners. outline = 0 gives a filled box.
void drawBox(Image& img, float x0, float y0, float x1, float y1, const Color& c, bool filled, float outline,
             float radius, const IRect& clip, bool lockAlpha = false, IRect* dirty = nullptr);
void drawEllipse(Image& img, float x0, float y0, float x1, float y1, const Color& c, bool filled, float outline,
                 const IRect& clip, bool lockAlpha = false, IRect* dirty = nullptr);
/// type : 0 linear, 1 circular, 2 elliptical, 3 conical, 4 boxed. From colour a (start) to b (end)
void drawGradient(Image& img, float x0, float y0, float x1, float y1, const Color& a, const Color& b, int type,
                  const IRect& clip, bool lockAlpha = false);

/// Flood fill of img. "reference" is the picture used to find the area (the layer itself or the
/// flattened document). Returns the modified rectangle.
IRect floodFill(Image& img, const Image& reference, int x, int y, const Color& c, int tolerance, const IRect& clip,
                bool lockAlpha = false);

// ---- adjustments (applied to the rect) ----
void fxInvert(Image& img, const IRect& r);
void fxDesaturate(Image& img, const IRect& r);
void fxBrightnessContrast(Image& img, const IRect& r, int brightness, int contrast); ///< -255..255 each
void fxPosterize(Image& img, const IRect& r, int levels);
void fxLevel(Image& img, const IRect& r, int minimum, int maximum);
void fxColorBalance(Image& img, const IRect& r, int dr, int dg, int db);
void fxHueSaturation(Image& img, const IRect& r, int hue, int saturation, int value);

// ---- filters ----
void fxNoise(Image& img, const IRect& r, int amount, bool mono);
void fxClouds(Image& img, const IRect& r, const Color& a, const Color& b, int scale);
void fxBlur(Image& img, const IRect& r, int radius);
void fxSharpen(Image& img, const IRect& r, float amount);
void fxOffset(Image& img, int dx, int dy); ///< wraps the picture around (seamless textures)
/// Keeps only the outline : the lighter a pixel, the more transparent (Layer_TransformToLine)
void fxWhiteToAlpha(Image& img, const IRect& r);

} // namespace ani
