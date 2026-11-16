#pragma once
// "Chompston" rainbow logo drawn from thick round-capped strokes (DrawNode::drawSegment only).

#include "axmol/axmol.h"

#include <cmath>
#include <string>
#include <vector>

namespace logo
{
struct P
{
    float x, y;
};
using Stroke = std::vector<P>;
struct Glyph
{
    float adv;
    std::vector<Stroke> strokes;
};

inline void arc(Stroke& s, float cx, float cy, float rx, float ry, float a0, float a1, int n = 14)
{
    for (int i = 0; i <= n; ++i)
    {
        float a = (a0 + (a1 - a0) * i / n) * 3.14159265f / 180.f;
        s.push_back({cx + rx * std::cos(a), cy + ry * std::sin(a)});
    }
}

// glyph coordinates: x right, y up, baseline 0, x-height 0.7, ascender 1.4
inline Glyph make(char c)
{
    Glyph g;
    auto add = [&](Stroke s) { g.strokes.push_back(std::move(s)); };
    auto serif = [&](float x, float y, float w = 0.13f) { add({{x - w, y}, {x + w, y}}); };
    switch (c)
    {
    case 'C':
    {
        g.adv = 1.0f;
        Stroke s;
        arc(s, 0.52f, 0.62f, 0.44f, 0.62f, 42, 318, 22);
        add(s);
        serif(0.92f, 1.0f, 0.0f);
        break;
    }
    case 'h':
    {
        g.adv = 0.95f;
        add({{0.10f, 0.f}, {0.10f, 1.40f}});
        Stroke s;
        arc(s, 0.46f, 0.38f, 0.36f, 0.32f, 180, 0, 14);
        s.push_back({0.82f, 0.f});
        add(s);
        serif(0.10f, 0.f);
        serif(0.82f, 0.f);
        serif(0.10f, 1.40f, 0.10f);
        break;
    }
    case 'o':
    {
        g.adv = 0.95f;
        Stroke s;
        arc(s, 0.48f, 0.35f, 0.40f, 0.35f, 0, 360, 22);
        add(s);
        break;
    }
    case 'm':
    {
        g.adv = 1.45f;
        add({{0.10f, 0.f}, {0.10f, 0.70f}});
        Stroke s;
        arc(s, 0.36f, 0.40f, 0.26f, 0.30f, 180, 0, 12);
        s.push_back({0.62f, 0.f});
        add(s);
        Stroke t;
        arc(t, 0.88f, 0.40f, 0.26f, 0.30f, 180, 0, 12);
        t.push_back({1.14f, 0.f});
        add(t);
        serif(0.10f, 0.f);
        serif(0.62f, 0.f);
        serif(1.14f, 0.f);
        break;
    }
    case 'p':
    {
        g.adv = 1.0f;
        add({{0.10f, 0.70f}, {0.10f, -0.55f}});
        Stroke s;
        arc(s, 0.52f, 0.35f, 0.40f, 0.35f, 0, 360, 22);
        add(s);
        serif(0.10f, -0.55f);
        break;
    }
    case 's':
    {
        g.adv = 0.78f;
        Stroke s;
        arc(s, 0.40f, 0.52f, 0.30f, 0.18f, 35, 270, 12);
        arc(s, 0.40f, 0.17f, 0.32f, 0.17f, 90, -145, 14);
        add(s);
        break;
    }
    case 't':
    {
        g.adv = 0.78f;
        Stroke s{{0.34f, 1.05f}, {0.34f, 0.16f}};
        arc(s, 0.50f, 0.16f, 0.16f, 0.16f, 180, 270, 6);
        s.push_back({0.62f, 0.f});
        add(s);
        add({{0.08f, 0.70f}, {0.66f, 0.70f}});
        break;
    }
    case 'n':
    {
        g.adv = 0.95f;
        add({{0.10f, 0.f}, {0.10f, 0.70f}});
        Stroke s;
        arc(s, 0.46f, 0.40f, 0.36f, 0.30f, 180, 0, 14);
        s.push_back({0.82f, 0.f});
        add(s);
        serif(0.10f, 0.f);
        serif(0.82f, 0.f);
        break;
    }
    default: g.adv = 0.6f; break;
    }
    return g;
}

inline ax::Color rainbow(float t)
{
    static const float stops[7][3] = {{1.00f, 0.42f, 0.08f}, {1.00f, 0.80f, 0.10f}, {0.55f, 0.85f, 0.20f},
                                      {0.10f, 0.85f, 0.65f}, {0.20f, 0.45f, 1.00f}, {0.55f, 0.25f, 0.95f},
                                      {0.82f, 0.20f, 0.80f}};
    t       = std::fmin(std::fmax(t, 0.f), 0.9999f) * 6.f;
    int i   = (int)t;
    float f = t - i;
    return ax::Color(stops[i][0] + (stops[i + 1][0] - stops[i][0]) * f, stops[i][1] + (stops[i + 1][1] - stops[i][1]) * f,
                     stops[i][2] + (stops[i + 1][2] - stops[i][2]) * f);
}

inline float width(const std::string& s, float unit)
{
    float w = 0;
    for (char c : s)
        w += make(c).adv;
    return w * unit;
}

/// Draws one glyph (outline, colour, highlight) with its left edge at xLeft.
/// `x0`/`total` describe the whole text so the rainbow runs across all letters.
inline void drawChar(ax::DrawNode* dn, char c, float xLeft, float baseY, float unit, float radius, float x0, float total)
{
    Glyph g = make(c);
    for (int pass = 0; pass < 3; ++pass)  // 0: dark outline, 1: colour, 2: highlight
        for (auto& s : g.strokes)
            for (size_t i = 0; i + 1 < s.size(); ++i)
            {
                ax::Vec2 a(xLeft + s[i].x * unit, baseY + s[i].y * unit), b(xLeft + s[i + 1].x * unit, baseY + s[i + 1].y * unit);
                ax::Color col = rainbow(((a.x + b.x) * 0.5f - x0) / total);
                if (pass == 0)
                    dn->drawSegment(a, b, radius + 2.2f, ax::Color(0.f, 0.f, 0.f, 1.f));
                else if (pass == 1)
                    dn->drawSegment(a, b, radius, ax::Color(col.r * 0.85f, col.g * 0.85f, col.b * 0.85f));
                else
                {
                    ax::Vec2 o(-radius * 0.25f, radius * 0.3f);
                    dn->drawSegment(a + o, b + o, radius * 0.45f,
                                    ax::Color(fminf(1.f, col.r * 1.2f + 0.1f), fminf(1.f, col.g * 1.2f + 0.1f),
                                              fminf(1.f, col.b * 1.2f + 0.1f)));
                }
            }
}

/// Draws `text` centred horizontally on `centerX`, baseline at `baseY` (all letters into one node).
inline void draw(ax::DrawNode* dn, const std::string& text, float centerX, float baseY, float unit, float radius)
{
    const float total = width(text, unit);
    float x           = centerX - total * 0.5f;
    const float x0    = x;
    for (char c : text)
    {
        drawChar(dn, c, x, baseY, unit, radius, x0, total);
        x += make(c).adv * unit;
    }
}
}  // namespace logo
