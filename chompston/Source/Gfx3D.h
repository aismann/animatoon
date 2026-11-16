#pragma once
// Tiny software 3D pipeline: world -> camera -> perspective -> ax::DrawNode polygons.
// Everything on screen (floor, walls, cubes, shadows, explosions) is a flat-shaded polygon.

#include "axmol/axmol.h"

#include <algorithm>
#include <cmath>
#include <vector>

struct V3
{
    float x = 0, y = 0, z = 0;
    V3() = default;
    V3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
    V3 operator+(const V3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    V3 operator-(const V3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    V3 operator*(float f) const { return {x * f, y * f, z * f}; }
    float dot(const V3& o) const { return x * o.x + y * o.y + z * o.z; }
};

inline ax::Color shade(const ax::Color& c, float k) { return ax::Color(c.r * k, c.g * k, c.b * k, c.a); }
inline ax::Color mix(const ax::Color& a, const ax::Color& b, float t)
{
    t = std::clamp(t, 0.f, 1.f);
    return ax::Color(a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t);
}

class Gfx3D
{
public:
    // camera
    V3 pos{0, 1.2f, 0};
    float yaw   = 0.f;    // heading, forward = (sin yaw, 0, cos yaw)
    float pitch = 0.10f;  // radians, positive = look down
    float focal = 380.f;
    ax::Vec2 center{256.f, 190.f};

    ax::Color fogColor{0.16f, 0.13f, 0.30f, 1.f};
    float fogStart = 4.f, fogEnd = 13.f;
    bool smooth    = true;
    V3 light{-0.45f, 0.80f, -0.40f};

    ax::DrawNode* dn = nullptr;

    void begin(ax::DrawNode* target)
    {
        dn = target;
        float cy = std::cos(yaw), sy = std::sin(yaw);
        fwd   = {sy, 0, cy};
        right = {cy, 0, -sy};
        cp    = std::cos(pitch);
        sp    = std::sin(pitch);
        float l = std::sqrt(light.dot(light));
        lightN  = light * (1.f / l);
    }

    // camera space: x right, y up, z forward (depth)
    V3 toCam(const V3& p) const
    {
        V3 d = p - pos;
        float xr = d.dot(right), yu = d.y, zf = d.dot(fwd);
        return {xr, yu * cp + zf * sp, zf * cp - yu * sp};
    }
    float depthOf(const V3& p) const { return toCam(p).z; }
    ax::Vec2 project(const V3& c) const { return {center.x + focal * c.x / c.z, center.y + focal * c.y / c.z}; }

    ax::Color fog(const ax::Color& c, float depth) const
    {
        return mix(c, fogColor, (depth - fogStart) / (fogEnd - fogStart));
    }

    // Draws a convex world-space polygon (near-plane clipped). `depth` is used for fog.
    void poly(const V3* pts, int n, const ax::Color& col, bool fogged = true)
    {
        V3 in[16], out[24];
        int cnt = 0;
        for (int i = 0; i < n && i < 16; ++i)
            in[cnt++] = toCam(pts[i]);
        // clip against z = near
        const float nearZ = 0.2f;
        int m             = 0;
        float zsum        = 0;
        for (int i = 0; i < cnt; ++i)
        {
            V3 a = in[i], b = in[(i + 1) % cnt];
            bool ai = a.z >= nearZ, bi = b.z >= nearZ;
            if (ai)
                out[m++] = a;
            if (ai != bi)
            {
                float t = (nearZ - a.z) / (b.z - a.z);
                out[m++] = V3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, nearZ);
            }
        }
        if (m < 3)
            return;
        ax::Vec2 sp2[24];
        for (int i = 0; i < m; ++i)
        {
            sp2[i] = project(out[i]);
            zsum += out[i].z;
        }
        ax::Color c = fogged ? fog(col, zsum / m) : col;
        dn->drawSolidPoly(sp2, m, c, smooth ? 1.f : 0.f, c, true);
    }
    void quad(const V3& a, const V3& b, const V3& c, const V3& d, const ax::Color& col, bool fogged = true)
    {
        V3 p[4] = {a, b, c, d};
        poly(p, 4, col, fogged);
    }

    // Filled disc lying on the plane y (shadows, explosion rings, glow)
    void disc(float cx, float y, float cz, float r, const ax::Color& col, int segs = 18, bool fogged = true)
    {
        V3 p[24];
        segs = std::min(segs, 24);
        for (int i = 0; i < segs; ++i)
        {
            float a = 6.2831853f * i / segs;
            p[i]    = V3(cx + std::cos(a) * r, y, cz + std::sin(a) * r);
        }
        poly(p, segs, col, fogged);
    }

    struct BoxStyle
    {
        ax::Color color;                 // side colour (before shading)
        ax::Color top;                   // top colour (not shaded when useTopColor)
        bool useTopColor = false;
        float ambient    = 0.55f;        // minimum brightness of a face
    };

    // Rotated box: centre c, half extents h, rotation around Y.  Draws the faces turned toward the camera.
    // Returns bit mask of faces drawn: 1=+x' 2=-x' 4=+z' 8=-z' (local axes), 16=top.
    int box(const V3& c, const V3& h, float rotY, const BoxStyle& st, int skip = 0)
    {
        float cs = std::cos(rotY), sn = std::sin(rotY);
        V3 ax_{cs, 0, -sn};  // local +x in world  (matches `right` for rotY == yaw)
        V3 az_{sn, 0, cs};   // local +z in world  (matches `fwd`)
        V3 ay_{0, 1, 0};
        auto corner = [&](int sx, int sy, int sz) {
            return c + ax_ * (h.x * sx) + ay_ * (h.y * sy) + az_ * (h.z * sz);
        };
        V3 p[8];
        for (int i = 0; i < 8; ++i)
            p[i] = corner((i & 1) ? 1 : -1, (i & 2) ? 1 : -1, (i & 4) ? 1 : -1);
        // face table: 4 corner indices (ccw seen from outside) + normal
        struct F { int i[4]; V3 n; int bit; } faces[5] = {
            {{1, 5, 7, 3}, ax_, 1},
            {{0, 2, 6, 4}, ax_ * -1.f, 2},
            {{4, 6, 7, 5}, az_, 4},
            {{0, 1, 3, 2}, az_ * -1.f, 8},
            {{2, 3, 7, 6}, ay_, 16},
        };
        int mask = 0;
        for (auto& f : faces)
        {
            if (skip & f.bit)
                continue;
            V3 fc = c + f.n * (f.bit == 16 ? h.y : (f.bit <= 2 ? h.x : h.z));
            if (f.n.dot(pos - fc) <= 0.f)
                continue;
            float l = std::max(0.f, f.n.dot(lightN));
            ax::Color col = (f.bit == 16 && st.useTopColor) ? st.top : shade(st.color, st.ambient + (1.f - st.ambient) * l);
            quad(p[f.i[0]], p[f.i[1]], p[f.i[2]], p[f.i[3]], col);
            mask |= f.bit;
        }
        return mask;
    }

    // Flat rectangle on a box face: origin + u*a + v*b with a,b in [0,1] of the face spans.
    void decal(const V3& origin, const V3& u, const V3& v, float a0, float b0, float a1, float b1, const ax::Color& col)
    {
        quad(origin + u * a0 + v * b0, origin + u * a1 + v * b0, origin + u * a1 + v * b1, origin + u * a0 + v * b1,
             col);
    }

private:
    V3 fwd, right, lightN;
    float cp = 1, sp = 0;
};
