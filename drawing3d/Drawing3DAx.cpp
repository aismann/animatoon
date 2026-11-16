#include "Drawing3DAx.h"

namespace d3d
{

void renderToDrawNode(const Drawing3D& g, ax::DrawNode* node)
{
    node->clear();
    // Colours of d3d are premultiplied, which is also what DrawNode's default blend expects.
    node->setBlendFunc(ax::BlendFunc::ALPHA_PREMULTIPLIED);

    const float W = static_cast<float>(g.width());
    const float H = static_cast<float>(g.height());

    const Color4 bg = g.background();
    if (bg.a > 0.0f)
    {
        const ax::Color c(bg.r, bg.g, bg.b, bg.a);
        const ax::Vec2 a[3] = {{0, 0}, {W, 0}, {0, H}};
        const ax::Vec2 b[3] = {{W, 0}, {W, H}, {0, H}};
        const ax::Color col[3] = {c, c, c};
        node->drawColoredTriangle(a, col);
        node->drawColoredTriangle(b, col);
    }

    for (const ScreenTriangle& t : g.triangles())
    {
        // d3d works in PureBasic screen space (y down) - flip into Axmol's y-up space.
        const ax::Vec2 v[3] = {{t.x[0], H - t.y[0]}, {t.x[1], H - t.y[1]}, {t.x[2], H - t.y[2]}};
        const ax::Color c[3] = {{t.c[0].r, t.c[0].g, t.c[0].b, t.c[0].a},
                                {t.c[1].r, t.c[1].g, t.c[1].b, t.c[1].a},
                                {t.c[2].r, t.c[2].g, t.c[2].b, t.c[2].a}};
        node->drawColoredTriangle(v, c);
    }
}

Image3DPtr image3DFromImage(ax::Image* image)
{
    if (!image || !image->getData())
        return nullptr;
    const int w = image->getWidth(), h = image->getHeight();
    const uint8_t* src = image->getData();
    std::vector<uint8_t> rgba(static_cast<size_t>(w) * h * 4);
    switch (image->getPixelFormat())
    {
    case ax::rhi::PixelFormat::RGBA8:
        std::copy(src, src + rgba.size(), rgba.begin());
        break;
    case ax::rhi::PixelFormat::BGRA8:
        for (size_t i = 0; i < static_cast<size_t>(w) * h; ++i)
        {
            rgba[i * 4 + 0] = src[i * 4 + 2];
            rgba[i * 4 + 1] = src[i * 4 + 1];
            rgba[i * 4 + 2] = src[i * 4 + 0];
            rgba[i * 4 + 3] = src[i * 4 + 3];
        }
        break;
    case ax::rhi::PixelFormat::RGB8:
        for (size_t i = 0; i < static_cast<size_t>(w) * h; ++i)
        {
            rgba[i * 4 + 0] = src[i * 3 + 0];
            rgba[i * 4 + 1] = src[i * 3 + 1];
            rgba[i * 4 + 2] = src[i * 3 + 2];
            rgba[i * 4 + 3] = 255;
        }
        break;
    default:
        return nullptr;
    }
    return Image3D::fromRGBA8(rgba.data(), w, h, image->hasPremultipliedAlpha());
}

Image3DPtr image3DFromFile(std::string_view path)
{
    ax::Image* image = new (std::nothrow) ax::Image();
    if (!image)
        return nullptr;
    Image3DPtr result;
    if (image->initWithImageFile(path))
        result = image3DFromImage(image);
    image->release();
    return result;
}

Drawing3D::TextRasterizer makeTextRasterizer(std::string fontName, int fontSize)
{
    return [fontName = std::move(fontName), fontSize](const std::string& text) -> Image3DPtr {
        ax::FontDefinition def;
        def._fontName      = fontName;
        def._fontSize      = fontSize;
        def._fontFillColor = ax::Color32::white;
        def._dimensions    = ax::Vec2::zero;  // size to fit

        int w = 0, h = 0;
        bool premultiplied = false;
        ax::Data data =
            ax::Device::getTextureDataForText(text, def, ax::Device::TextAlign::CENTER, w, h, premultiplied);
        if (data.isNull() || w <= 0 || h <= 0)
            return nullptr;
        // Only the alpha channel (glyph coverage) is used; the caller tints it.
        return Image3D::fromRGBA8(data.getBytes(), w, h, premultiplied);
    };
}

bool savePNG(const Drawing3D& g, const std::string& path)
{
    std::vector<uint8_t> rgba;
    g.rasterize(rgba);
    ax::Image image;
    if (!image.initWithRawData(rgba.data(), static_cast<ssize_t>(rgba.size()), g.width(), g.height(), 8))
        return false;
    return image.saveToFile(path, false);
}

}  // namespace d3d
