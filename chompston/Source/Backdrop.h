#pragma once
// Start-screen backdrop: black sky, grey-blue floor, dark slabs with magenta/purple sides,
// a big Chompston cube in front and two enemy cubes on a distant slab. Pure Gfx3D / DrawNode polygons.

#include "Actors.h"

#include <algorithm>
#include <functional>
#include <vector>

namespace backdrop
{
inline void draw(Gfx3D& g, ax::DrawNode* dn, float t, float W, float H)
{
    using ax::Color;
    g.pos         = V3(std::sin(t * 0.35f) * 0.18f, 1.4f, 0.f);
    g.yaw         = std::sin(t * 0.27f) * 0.02f;
    g.pitch       = 0.08f;
    g.focal       = 380.f;
    g.center      = ax::Vec2(W * 0.5f, H * 0.43f);
    g.fogColor    = Color(0.02f, 0.02f, 0.05f);
    g.fogStart    = 11.f;
    g.fogEnd      = 30.f;
    g.begin(dn);

    float horizon = g.center.y + g.focal * std::tan(g.pitch);
    dn->drawSolidRect(ax::Vec2(0, 0), ax::Vec2(W, H), Color(0.f, 0.f, 0.f));  // black sky
    // floor: two bands, lighter near the viewer
    dn->drawSolidRect(ax::Vec2(0, 0), ax::Vec2(W, horizon), Color(0.24f, 0.24f, 0.33f));
    dn->drawSolidRect(ax::Vec2(0, horizon * 0.62f), ax::Vec2(W, horizon), Color(0.20f, 0.20f, 0.29f));
    dn->drawSolidRect(ax::Vec2(0, horizon * 0.86f), ax::Vec2(W, horizon), Color(0.15f, 0.14f, 0.24f));

    std::vector<std::pair<float, std::function<void()>>> list;
    auto slab = [&](float x0, float x1, float z0, float z1, float h, Color side) {
        V3 c((x0 + x1) * 0.5f, h * 0.5f, (z0 + z1) * 0.5f);
        V3 half((x1 - x0) * 0.5f, h * 0.5f, (z1 - z0) * 0.5f);
        float dx = c.x - g.pos.x, dz = c.z - g.pos.z;
        list.emplace_back(dx * dx + dz * dz, [&g, c, half, side] {
            Gfx3D::BoxStyle st;
            st.color       = side;
            st.top         = Color(0.01f, 0.01f, 0.03f);
            st.useTopColor = true;
            st.ambient     = 0.5f;
            g.box(c, half, 0.f, st);
        });
    };
    slab(-13.f, -6.f, 5.f, 13.f, 0.45f, Color(0.30f, 0.22f, 0.62f));
    slab(-6.5f, 0.2f, 7.5f, 16.f, 0.85f, Color(0.28f, 0.24f, 0.66f));
    slab(0.8f, 5.4f, 11.f, 19.f, 0.55f, Color(0.42f, 0.34f, 0.80f));
    slab(1.9f, 8.f, 3.8f, 9.f, 0.9f, Color(0.48f, 0.12f, 0.50f));

    // enemy cubes standing on the far slab, facing the camera
    float bob = std::fabs(std::sin(t * 3.f)) * 0.05f;
    for (int i = 0; i < 2; ++i)
    {
        V3 c(i == 0 ? 2.2f : 3.5f, 0.55f + 0.30f + (i ? bob : 0.04f - bob), i == 0 ? 12.f : 13.2f);
        float dx = c.x - g.pos.x, dz = c.z - g.pos.z;
        list.emplace_back(dx * dx + dz * dz, [&g, c] {
            actors::ghostCube(g, c, 0.30f, 3.14159265f, Color(0.70f, 0.64f, 0.92f), false);
        });
    }
    // the hero
    {
        float chompOpen = 0.12f + 0.1f * std::sin(t * 2.f);
        V3 c(-1.35f, 0.55f + std::fabs(std::sin(t * 1.5f)) * 0.04f, 3.7f);
        float rot = 0.45f + std::sin(t * 0.9f) * 0.05f;
        float dx = c.x - g.pos.x, dz = c.z - g.pos.z;
        list.emplace_back(dx * dx + dz * dz, [&g, c, rot, chompOpen] { actors::chompCube(g, c, 0.55f, rot, chompOpen, false); });
        g.disc(c.x, 0.01f, c.z, 0.75f, Color(0, 0, 0, 0.35f), 16);
    }
    std::sort(list.begin(), list.end(), [](auto& a, auto& b) { return a.first > b.first; });
    for (auto& l : list)
        l.second();
}
}  // namespace backdrop
