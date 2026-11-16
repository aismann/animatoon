#pragma once
// Shared cube characters (used by the game and by the title screen backdrop).

#include "Gfx3D.h"

namespace actors
{
inline const ax::Color kChompYellow(1.0f, 0.86f, 0.05f);

// Chompston: yellow cube with eyes and a chomping mouth. Returns the visible-face mask of Gfx3D::box().
// `faceTop` additionally paints an upright face on the top face (used by the top-down camera).
inline int chompCube(Gfx3D& g, const V3& c, float h, float rot, float open, bool faceTop = true)
{
    Gfx3D::BoxStyle st;
    st.color   = kChompYellow;
    st.ambient = 0.62f;
    int mask   = g.box(c, V3(h, h, h), rot, st);
    if (h <= 0.f)
        return mask;

    float cs = std::cos(rot), sn = std::sin(rot);
    V3 axv(cs, 0, -sn), azv(sn, 0, cs);
    auto face = [&](V3 origin, V3 u, V3 v) {
        const ax::Color black(0.04f, 0.03f, 0.02f);
        g.decal(origin, u, v, 0.20f, 0.52f, 0.32f, 0.86f, black);  // eyes
        g.decal(origin, u, v, 0.68f, 0.52f, 0.80f, 0.86f, black);
        float top = 0.16f + 0.10f + 0.20f * open;
        g.decal(origin, u, v, 0.12f, 0.16f, 0.88f, top, black);  // mouth
        g.decal(origin, u, v, 0.16f, top - 0.07f, 0.84f, top - 0.01f, ax::Color(0.82f, 0.82f, 0.86f));  // teeth
    };
    if (faceTop && (mask & 16))
        face(c + axv * (-h) + azv * (-h) + V3(0, h + 0.003f, 0), axv * (2 * h), azv * (2 * h));
    if (mask & 8)  // back face: seen from behind, screen right == local +x
        face(c + axv * (-h) + azv * (-h - 0.003f) + V3(0, -h, 0), axv * (2 * h), V3(0, 2 * h, 0));
    if (mask & 4)  // front face
        face(c + axv * h + azv * (h + 0.003f) + V3(0, -h, 0), axv * (-2 * h), V3(0, 2 * h, 0));
    return mask;
}

// Enemy cube; the eyes are on the local +z face (direction of travel).
inline void ghostCube(Gfx3D& g, const V3& c, float h, float rot, const ax::Color& col, bool scared)
{
    Gfx3D::BoxStyle st;
    st.color   = col;
    st.ambient = 0.6f;
    int mask   = g.box(c, V3(h, h, h), rot, st);
    if (!(mask & 4))
        return;
    float cs = std::cos(rot), sn = std::sin(rot);
    V3 axv(cs, 0, -sn), azv(sn, 0, cs);
    V3 origin = c + axv * h + azv * (h + 0.003f) + V3(0, -h, 0);
    V3 u      = axv * (-2 * h);
    V3 v(0, 2 * h, 0);
    ax::Color eye = scared ? ax::Color(1.f, 1.f, 1.f) : ax::Color(0.05f, 0.04f, 0.1f);
    g.decal(origin, u, v, 0.18f, 0.50f, 0.34f, 0.82f, eye);
    g.decal(origin, u, v, 0.66f, 0.50f, 0.82f, 0.82f, eye);
    if (scared)
        g.decal(origin, u, v, 0.20f, 0.20f, 0.80f, 0.30f, ax::Color(1.f, 1.f, 1.f));
}
}  // namespace actors
