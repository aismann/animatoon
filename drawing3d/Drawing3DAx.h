// Axmol 3.0 glue for Drawing3D: renders a finished d3d::Drawing3D pass into an ax::DrawNode.
#pragma once

#include <string>
#include <string_view>

#include "Drawing3D.h"
#include "axmol.h"

namespace d3d
{

// Draws the result of a finished scene (after Drawing3D::stop()) into `node`:
// the node is cleared first, and the canvas occupies (0,0)..(g.width(), g.height()) in node space
// with the origin at the *bottom left* (Axmol convention; the PureBasic version has it top left).
// Wrap the node in an ax::ClippingRectangleNode to cut off geometry beyond the canvas.
void renderToDrawNode(const Drawing3D& g, ax::DrawNode* node);

// Image3D from an ax::Image (RGBA8, RGB8, BGRA8 supported) or an image file.
Image3DPtr image3DFromImage(ax::Image* image);
Image3DPtr image3DFromFile(std::string_view path);

// Text rasteriser for Drawing3D::drawText3D based on the platform text renderer
// (ax::Device::getTextureDataForText). Use with Drawing3D::setTextRasterizer().
Drawing3D::TextRasterizer makeTextRasterizer(std::string fontName = "Verdana", int fontSize = 24);

// Renders the scene with the CPU rasteriser and saves it as 32-bit PNG (with alpha).
bool savePNG(const Drawing3D& g, const std::string& path);

}  // namespace d3d
