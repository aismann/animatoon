// Drawing3D for Axmol 3.0 - core (no Axmol dependency). See Drawing3D.h.
#include "Drawing3D.h"

#include <algorithm>
#include <cmath>

namespace d3d
{

namespace
{
constexpr float kPi            = 3.14159265358979323846f;
constexpr int kMaxSplitDepth   = 10;
constexpr size_t kMaxTriangles = 4000000;  // safety net against runaway subdivision

inline float radian(float deg) { return deg * (kPi / 180.0f); }

inline Vec3 cross(Vec3 a, Vec3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float length(Vec3 a) { return std::sqrt(dot(a, a)); }

Vec3 rotate(const Matrix3& M, Vec3 v)
{
    return {v.x * M.m[0][0] + v.y * M.m[0][1] + v.z * M.m[0][2], v.x * M.m[1][0] + v.y * M.m[1][1] + v.z * M.m[1][2],
            v.x * M.m[2][0] + v.y * M.m[2][1] + v.z * M.m[2][2]};
}

// Drawing3D_Orientation (angles in radians)
Matrix3 orientation(float rx, float ry, float rz)
{
    const float cz = std::cos(rz), cy = std::cos(ry), cx = std::cos(rx);
    const float sz = std::sin(rz), sy = std::sin(ry), sx = std::sin(rx);
    Matrix3 M;
    M.m[0][0] = cy * cz;
    M.m[0][1] = -cy * sz;
    M.m[0][2] = sy;
    M.m[1][0] = sx * sy * cz + cx * sz;
    M.m[1][1] = -sx * sy * sz + cx * cz;
    M.m[1][2] = -sx * cy;
    M.m[2][0] = -cx * sy * cz + sx * sz;
    M.m[2][1] = cx * sy * sz + sx * cz;
    M.m[2][2] = cx * cy;
    return M;
}

// Drawing3D_Multiply: A = A * B
Matrix3 multiply(const Matrix3& A, const Matrix3& B)
{
    Matrix3 R;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            R.m[i][j] = A.m[i][0] * B.m[0][j] + A.m[i][1] * B.m[1][j] + A.m[i][2] * B.m[2][j];
    return R;
}

inline Color4 lerp(const Color4& a, const Color4& b, float t)
{
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
}

inline Color4 pick(ColorPB value, ColorPB fallback) { return colorFromPB(value == Ignore ? fallback : value); }
}  // namespace

// ---------------------------------------------------------------------------
// Colour / image
// ---------------------------------------------------------------------------

Color4 colorFromPB(ColorPB color)
{
    const uint32_t c = static_cast<uint32_t>(color & 0xFFFFFFFFLL);
    const float a    = static_cast<float>(c >> 24) / 255.0f;
    // SetColor(): premultiply
    return {static_cast<float>(c & 255) / 255.0f * a, static_cast<float>((c >> 8) & 255) / 255.0f * a,
            static_cast<float>((c >> 16) & 255) / 255.0f * a, a};
}

std::shared_ptr<Image3D> Image3D::create(int width, int height)
{
    auto img    = std::make_shared<Image3D>();
    img->width  = std::max(width, 0);
    img->height = std::max(height, 0);
    img->pixels.assign(static_cast<size_t>(img->width) * img->height, Color4{});
    return img;
}

std::shared_ptr<Image3D> Image3D::fromRGBA8(const uint8_t* rgba, int width, int height, bool premultiplied)
{
    auto img = create(width, height);
    for (size_t i = 0; i < img->pixels.size(); ++i)
    {
        const float a = rgba[i * 4 + 3] / 255.0f;
        const float k = premultiplied ? 1.0f : a;
        img->pixels[i] = {rgba[i * 4 + 0] / 255.0f * k, rgba[i * 4 + 1] / 255.0f * k, rgba[i * 4 + 2] / 255.0f * k, a};
    }
    return img;
}

Color4 Image3D::sample(float u, float v) const
{
    if (pixels.empty())
        return {};
    const float fx = u * width - 0.5f, fy = v * height - 0.5f;
    const int x0   = static_cast<int>(std::floor(fx)), y0 = static_cast<int>(std::floor(fy));
    const float tx = fx - x0, ty = fy - y0;
    auto P = [&](int x, int y) -> const Color4& {
        return at(std::clamp(x, 0, width - 1), std::clamp(y, 0, height - 1));
    };
    return lerp(lerp(P(x0, y0), P(x0 + 1, y0), tx), lerp(P(x0, y0 + 1), P(x0 + 1, y0 + 1), tx), ty);
}

// ---------------------------------------------------------------------------
// Scene state
// ---------------------------------------------------------------------------

void Drawing3D::start(int width, int height, float fieldOfView)
{
    width_   = width;
    height_  = height;
    centerX_ = static_cast<float>(width / 2);
    centerY_ = static_cast<float>(height / 2);
    focal_   = static_cast<float>(height) / std::tan(radian(fieldOfView) * 0.5f);
    position_ = {0, 0, focal_};
    background_ = {};
    lights_.clear();
    triangles_.clear();
    lines3D_.clear();
}

void Drawing3D::stop()
{
    // Painter's algorithm: furthest (most negative z) first, stable for equal depth.
    std::stable_sort(triangles_.begin(), triangles_.end(),
                     [](const ScreenTriangle& a, const ScreenTriangle& b) { return a.depth < b.depth; });
}

void Drawing3D::setPosition(float x, float y, float z, Move move)
{
    // Like the original: the stored position is the *negated* scene position.
    if (move == Move::Relative)
        position_ = position_ + Vec3{-x, -y, -z};
    else
        position_ = {-x, -y, -z};
}

void Drawing3D::setRotation(float rx, float ry, float rz, Move move)
{
    const Matrix3 r = orientation(radian(rx), radian(ry), radian(rz));
    orientation_    = (move == Move::Relative) ? multiply(r, orientation_) : r;
}

void Drawing3D::setBackground(ColorPB color) { background_ = colorFromPB(color); }

void Drawing3D::addLight(float dx, float dy, float dz, ColorPB color)
{
    Vec3 d      = {dx, dy, dz};
    const float len = length(d);
    if (len > 0)
        d = {d.x / len, d.y / len, d.z / len};
    lights_.push_back({d, colorFromPB(color)});
}

// ---------------------------------------------------------------------------
// Transform / projection
// ---------------------------------------------------------------------------

// object rotation -> + object position -> scene rotation -> - camera position
Vec3 Drawing3D::toView(const Matrix3& object, Vec3 offset, Vec3 local) const
{
    return rotate(orientation_, rotate(object, local) + offset) - position_;
}

Drawing3D::Vtx Drawing3D::makeVtx(Vec3 view, Color4 color) const
{
    Vtx v;
    v.p = view;
    v.c = color;
    return v;
}

void Drawing3D::project(Vec3 p, float& x, float& y) const
{
    const float w = 1.0f / p.z;  // p.z < 0 (in front of the camera)
    x             = -p.x * focal_ * w + centerX_;
    y             = p.y * focal_ * w + centerY_;
}

// ---------------------------------------------------------------------------
// Rendering core
// ---------------------------------------------------------------------------

void Drawing3D::emitTriangle(float x0, float y0, float x1, float y1, float x2, float y2, Color4 c0, Color4 c1,
                             Color4 c2, float depth)
{
    if (triangles_.size() >= kMaxTriangles)
        return;
    ScreenTriangle t;
    t.x[0] = x0; t.y[0] = y0; t.c[0] = c0;
    t.x[1] = x1; t.y[1] = y1; t.c[1] = c1;
    t.x[2] = x2; t.y[2] = y2; t.c[2] = c2;
    t.depth = depth;
    triangles_.push_back(t);
}

void Drawing3D::subdivideAndEmit(const Vtx& a, const Vtx& b, const Vtx& c, const Image3D* texture, int depth)
{
    float x[3], y[3];
    project(a.p, x[0], y[0]);
    project(b.p, x[1], y[1]);
    project(c.p, x[2], y[2]);

    // Completely outside the canvas?
    const float minX = std::min({x[0], x[1], x[2]}), maxX = std::max({x[0], x[1], x[2]});
    const float minY = std::min({y[0], y[1], y[2]}), maxY = std::max({y[0], y[1], y[2]});
    if (maxX < 0 || maxY < 0 || minX > static_cast<float>(width_) || minY > static_cast<float>(height_))
        return;

    const float edgeMax = texture ? imageDetail_ : subdivision_;
    if (edgeMax > 0 && depth < kMaxSplitDepth)
    {
        auto len2 = [&](int i, int j) {
            const float dx = x[i] - x[j], dy = y[i] - y[j];
            return dx * dx + dy * dy;
        };
        if (std::max({len2(0, 1), len2(1, 2), len2(2, 0)}) > edgeMax * edgeMax)
        {
            auto mid = [](const Vtx& p, const Vtx& q) {
                Vtx m;
                m.p = {(p.p.x + q.p.x) * 0.5f, (p.p.y + q.p.y) * 0.5f, (p.p.z + q.p.z) * 0.5f};
                m.c = lerp(p.c, q.c, 0.5f);
                m.u = (p.u + q.u) * 0.5f;
                m.v = (p.v + q.v) * 0.5f;
                return m;
            };
            const Vtx ab = mid(a, b), bc = mid(b, c), ca = mid(c, a);
            subdivideAndEmit(a, ab, ca, texture, depth + 1);
            subdivideAndEmit(ab, b, bc, texture, depth + 1);
            subdivideAndEmit(ca, bc, c, texture, depth + 1);
            subdivideAndEmit(ab, bc, ca, texture, depth + 1);
            return;
        }
    }

    Color4 col[3] = {a.c, b.c, c.c};
    if (texture)
    {
        col[0] = texture->sample(a.u, a.v);
        col[1] = texture->sample(b.u, b.v);
        col[2] = texture->sample(c.u, c.v);
    }
    emitTriangle(x[0], y[0], x[1], y[1], x[2], y[2], col[0], col[1], col[2], (a.p.z + b.p.z + c.p.z) / 3.0f);
}

void Drawing3D::pushTriangle(Vtx a, Vtx b, Vtx c, const Image3D* texture)
{
    // Sign of the original's screen-space "T" equals the sign of dot(n, a) - this form
    // stays valid for triangles that reach behind the camera.
    const Vec3 n3 = cross(b.p - a.p, c.p - a.p);
    const float t = dot(n3, a.p);
    if (t == 0.0f)
        return;
    if ((mode_ & Mode::FrontOnly) && t < 0.0f)
        return;

    if (!texture && !lights_.empty())
    {
        const float len = length(n3);
        if (len > 0)
        {
            const Vec3 norm = {-n3.x / len, -n3.y / len, -n3.z / len};  // normalize(V2 x V1)
            const float sgn = t > 0 ? 1.0f : -1.0f;
            float lightR = 0, lightG = 0, lightB = 0;
            for (const Light& l : lights_)
            {
                const float f = dot(norm, l.dir) * sgn;
                if (f > 0)
                {
                    lightR += f * l.color.r;
                    lightG += f * l.color.g;
                    lightB += f * l.color.b;
                }
            }
            for (Vtx* v : {&a, &b, &c})
            {
                // un-premultiply, add light, normalise, premultiply (original: NormalizeColor)
                const float inv = v->c.a > 0 ? 1.0f / v->c.a : 0.0f;
                float r = v->c.r * inv + lightR, g = v->c.g * inv + lightG, bl = v->c.b * inv + lightB;
                const float mx = std::max({r, g, bl});
                if (mx > 1)
                {
                    r /= mx;
                    g /= mx;
                    bl /= mx;
                }
                v->c = {r * v->c.a, g * v->c.a, bl * v->c.a, v->c.a};
            }
        }
    }

    // Clip against the near plane (z <= -nearPlane_).
    const Vtx in[3] = {a, b, c};
    Vtx poly[4];
    int n = 0;
    for (int i = 0; i < 3; ++i)
    {
        const Vtx& p = in[i];
        const Vtx& q = in[(i + 1) % 3];
        const bool pin = p.p.z <= -nearPlane_, qin = q.p.z <= -nearPlane_;
        if (pin)
            poly[n++] = p;
        if (pin != qin)
        {
            const float k = (-nearPlane_ - p.p.z) / (q.p.z - p.p.z);
            Vtx m;
            m.p = {p.p.x + (q.p.x - p.p.x) * k, p.p.y + (q.p.y - p.p.y) * k, -nearPlane_};
            m.c = lerp(p.c, q.c, k);
            m.u = p.u + (q.u - p.u) * k;
            m.v = p.v + (q.v - p.v) * k;
            poly[n++] = m;
        }
    }
    for (int i = 1; i + 1 < n; ++i)
        subdivideAndEmit(poly[0], poly[i], poly[i + 1], texture, 0);
}

void Drawing3D::pushLine(Vtx a, Vtx b)
{
    lines3D_.push_back({a.p + position_, b.p + position_, a.c, b.c});
    // Clip against the near plane.
    const bool ain = a.p.z <= -nearPlane_, bin = b.p.z <= -nearPlane_;
    if (!ain && !bin)
        return;
    if (ain != bin)
    {
        const Vtx& in = ain ? a : b;
        Vtx& out      = ain ? b : a;
        const float k = (-nearPlane_ - in.p.z) / (out.p.z - in.p.z);
        out.p         = {in.p.x + (out.p.x - in.p.x) * k, in.p.y + (out.p.y - in.p.y) * k, -nearPlane_};
        out.c         = lerp(in.c, out.c, k);
    }

    float x0, y0, x1, y1;
    project(a.p, x0, y0);
    project(b.p, x1, y1);
    const float lenPx = std::hypot(x1 - x0, y1 - y0);
    if (lenPx < 1e-4f)
        return;

    // Split long gradient lines so colours stay perspective-correct.
    const bool gradient = std::fabs(a.c.r - b.c.r) + std::fabs(a.c.g - b.c.g) + std::fabs(a.c.b - b.c.b) +
                              std::fabs(a.c.a - b.c.a) > 1e-4f;
    const int segments = gradient ? std::clamp(static_cast<int>(lenPx / 12.0f), 1, 64) : 1;
    const float half   = lineWidth_ * 0.5f;

    Vtx prev = a;
    float px = x0, py = y0;
    for (int i = 1; i <= segments; ++i)
    {
        const float k = static_cast<float>(i) / segments;
        Vtx cur;
        cur.p = {a.p.x + (b.p.x - a.p.x) * k, a.p.y + (b.p.y - a.p.y) * k, a.p.z + (b.p.z - a.p.z) * k};
        cur.c = lerp(a.c, b.c, k);
        float cx, cy;
        project(cur.p, cx, cy);

        const float dx = cx - px, dy = cy - py;
        const float l  = std::hypot(dx, dy);
        if (l > 1e-6f)
        {
            const float nx = -dy / l * half, ny = dx / l * half;
            const float depth = (prev.p.z + cur.p.z) * 0.5f;
            emitTriangle(px + nx, py + ny, px - nx, py - ny, cx + nx, cy + ny, prev.c, prev.c, cur.c, depth);
            emitTriangle(cx + nx, cy + ny, px - nx, py - ny, cx - nx, cy - ny, cur.c, prev.c, cur.c, depth);
        }
        prev = cur;
        px   = cx;
        py   = cy;
    }
}

void Drawing3D::pushPoint(Vtx a)
{
    if (a.p.z > -nearPlane_)
        return;
    float x, y;
    project(a.p, x, y);
    if (x < -1 || y < -1 || x > width_ + 1 || y > height_ + 1)
        return;
    const float h = 0.5f;
    emitTriangle(x - h, y - h, x + h, y - h, x - h, y + h, a.c, a.c, a.c, a.p.z);
    emitTriangle(x + h, y - h, x + h, y + h, x - h, y + h, a.c, a.c, a.c, a.p.z);
}

void Drawing3D::drawTexturedQuad(const Image3DPtr& image, const Vec3 view[4])
{
    if (!image || image->pixels.empty())
        return;
    Vtx v[4];
    for (int i = 0; i < 4; ++i)
        v[i] = makeVtx(view[i], {});
    // Same UV layout as the original: u runs P0->P1, v runs P0->P2.
    v[0].u = 0; v[0].v = 0;
    v[1].u = 1; v[1].v = 0;
    v[2].u = 0; v[2].v = 1;
    v[3].u = 1; v[3].v = 1;
    pushTriangle(v[0], v[1], v[2], image.get());
    pushTriangle(v[3], v[2], v[1], image.get());
}

// ---------------------------------------------------------------------------
// Public primitives
// ---------------------------------------------------------------------------

void Drawing3D::drawPoint3D(float x, float y, float z, ColorPB color)
{
    pushPoint(makeVtx(toView(Matrix3{}, {}, {x, y, z}) , colorFromPB(color)));
}

void Drawing3D::drawLine3D(float x1, float y1, float z1, float x2, float y2, float z2, ColorPB color1,
                           ColorPB color2)
{
    const Matrix3 I;
    pushLine(makeVtx(toView(I, {}, {x1, y1, z1}), colorFromPB(color1)),
             makeVtx(toView(I, {}, {x2, y2, z2}), pick(color2, color1)));
}

void Drawing3D::drawTriangle3D(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3,
                               float z3, ColorPB color1, ColorPB color2, ColorPB color3)
{
    const Matrix3 I;
    const Vtx a = makeVtx(toView(I, {}, {x1, y1, z1}), colorFromPB(color1));
    const Vtx b = makeVtx(toView(I, {}, {x2, y2, z2}), pick(color2, color1));
    const Vtx c = makeVtx(toView(I, {}, {x3, y3, z3}), pick(color3, color1));
    if (mode_ & Mode::Outline)
    {
        pushLine(a, b);
        pushLine(b, c);
        pushLine(c, a);
    }
    else
        pushTriangle(a, b, c, nullptr);
}

void Drawing3D::drawPlane3D(float x, float y, float z, float width, float height, float rx, float ry, float rz,
                            ColorPB color1, ColorPB color2, ColorPB color3, ColorPB color4)
{
    const Matrix3 o = orientation(radian(rx), radian(ry), radian(rz));
    const Vec3 pos  = {x, y, z};
    const float w = width * 0.5f, h = height * 0.5f;
    const Vtx v[4] = {makeVtx(toView(o, pos, {-w, h, 0}), colorFromPB(color1)),
                      makeVtx(toView(o, pos, {w, h, 0}), pick(color2, color1)),
                      makeVtx(toView(o, pos, {-w, -h, 0}), pick(color3, color1)),
                      makeVtx(toView(o, pos, {w, -h, 0}), pick(color4, color1))};
    if (mode_ & Mode::Outline)
    {
        pushLine(v[0], v[1]);
        pushLine(v[1], v[3]);
        pushLine(v[3], v[2]);
        pushLine(v[2], v[0]);
    }
    else
    {
        pushTriangle(v[0], v[1], v[2], nullptr);
        pushTriangle(v[3], v[2], v[1], nullptr);
    }
}

void Drawing3D::drawDisk3D(float x, float y, float z, float radius, float rx, float ry, float rz, ColorPB color)
{
    // The original tests every pixel against a circle in the plane; here the disk is a triangle fan.
    constexpr int kDetail = 48;
    const Matrix3 o       = orientation(radian(rx), radian(ry), radian(rz));
    const Vec3 pos        = {x, y, z};
    const Color4 col      = colorFromPB(color);
    const Vtx center      = makeVtx(toView(o, pos, {0, 0, 0}), col);
    Vtx prev              = makeVtx(toView(o, pos, {radius, 0, 0}), col);
    const Vtx first       = prev;
    for (int i = 1; i <= kDetail; ++i)
    {
        const float ang = 2.0f * kPi * i / kDetail;
        const Vtx cur   = (i == kDetail) ? first : makeVtx(toView(o, pos, {std::cos(ang) * radius, std::sin(ang) * radius, 0}), col);
        if (mode_ & Mode::Outline)
            pushLine(prev, cur);
        else
            pushTriangle(center, prev, cur, nullptr);
        prev = cur;
    }
}

void Drawing3D::drawBox3D(float x, float y, float z, float width, float height, float depth, float rx, float ry,
                          float rz, ColorPB color)
{
    const Matrix3 o = orientation(radian(rx), radian(ry), radian(rz));
    const Vec3 pos  = {x, y, z};
    const float w = width * 0.5f, h = height * 0.5f, d = depth * 0.5f;
    const Color4 col = colorFromPB(color);
    const Vec3 corner[8] = {{-w, h, d}, {w, h, d}, {-w, -h, d}, {w, -h, d}, {-w, h, -d}, {w, h, -d}, {-w, -h, -d}, {w, -h, -d}};
    Vtx v[8];
    for (int i = 0; i < 8; ++i)
        v[i] = makeVtx(toView(o, pos, corner[i]), col);

    if (mode_ & Mode::Outline)
    {
        static const int edges[12][2] = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3},
                                         {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        for (const auto& e : edges)
            pushLine(v[e[0]], v[e[1]]);
    }
    else
    {
        static const int tris[12][3] = {{0, 1, 2}, {3, 2, 1}, {5, 4, 7}, {6, 7, 4}, {1, 5, 3}, {7, 3, 5},
                                        {4, 0, 6}, {2, 6, 0}, {4, 5, 0}, {1, 0, 5}, {2, 3, 6}, {7, 6, 3}};
        for (const auto& t : tris)
            pushTriangle(v[t[0]], v[t[1]], v[t[2]], nullptr);
    }
}

void Drawing3D::drawCylinder3D(float x, float y, float z, float radius, float height, float rx, float ry, float rz,
                               ColorPB color1, ColorPB color2, int detail)
{
    if (detail < 3)
        detail = 3;
    const Matrix3 o  = orientation(radian(rx), radian(ry), radian(rz));
    const Vec3 pos   = {x, y, z};
    const Color4 c1  = colorFromPB(color1), c2 = pick(color2, color1);
    const float h    = height * 0.5f;
    const Vtx top    = makeVtx(toView(o, pos, {0, h, 0}), c1);
    const Vtx bottom = makeVtx(toView(o, pos, {0, -h, 0}), c2);
    Vtx v0           = makeVtx(toView(o, pos, {radius, h, 0}), c1);
    Vtx v1           = makeVtx(toView(o, pos, {radius, -h, 0}), c2);
    for (int i = 1; i <= detail; ++i)
    {
        const float ang = 2.0f * kPi * i / detail;
        const float cx = std::cos(ang) * radius, cz = std::sin(ang) * radius;
        const Vtx v2 = makeVtx(toView(o, pos, {cx, h, cz}), c1);
        const Vtx v3 = makeVtx(toView(o, pos, {cx, -h, cz}), c2);
        pushTriangle(v0, v1, v2, nullptr);
        pushTriangle(v3, v2, v1, nullptr);
        pushTriangle(v0, v2, top, nullptr);
        pushTriangle(v1, v3, bottom, nullptr);
        v0 = v2;
        v1 = v3;
    }
}

void Drawing3D::drawImage3D(const Image3DPtr& image, float x, float y, float z, float width, float height, float rx,
                            float ry, float rz)
{
    const Matrix3 o = orientation(radian(rx), radian(ry), radian(rz));
    const Vec3 pos  = {x, y, z};
    const float w = width * 0.5f, h = height * 0.5f;
    const Vec3 view[4] = {toView(o, pos, {-w, h, 0}), toView(o, pos, {w, h, 0}), toView(o, pos, {-w, -h, 0}),
                          toView(o, pos, {w, -h, 0})};
    drawTexturedQuad(image, view);
}

void Drawing3D::drawTransformedImage3D(const Image3DPtr& image, float x1, float y1, float z1, float x2, float y2,
                                       float z2, float x3, float y3, float z3, float x4, float y4, float z4)
{
    const Matrix3 I;
    const Vec3 view[4] = {toView(I, {}, {x1, y1, z1}), toView(I, {}, {x2, y2, z2}), toView(I, {}, {x3, y3, z3}),
                          toView(I, {}, {x4, y4, z4})};
    drawTexturedQuad(image, view);
}

void Drawing3D::drawText3D(float x, float y, float z, float rx, float ry, float rz, const std::string& text,
                           ColorPB color)
{
    if (!textRasterizer_)
        return;
    Image3DPtr mask = textRasterizer_(text);
    if (!mask || mask->pixels.empty())
        return;
    // Tint the coverage mask with the requested colour (alpha of colour * coverage).
    const Color4 tint = colorFromPB(color);
    auto img          = Image3D::create(mask->width, mask->height);
    for (size_t i = 0; i < img->pixels.size(); ++i)
    {
        const float cov = mask->pixels[i].a;
        img->pixels[i]  = {tint.r * cov, tint.g * cov, tint.b * cov, tint.a * cov};
    }
    drawImage3D(img, x, y, z, static_cast<float>(img->width), static_cast<float>(img->height), rx, ry, rz);
}

// ---------------------------------------------------------------------------
// CPU reference rasteriser
// ---------------------------------------------------------------------------

void Drawing3D::rasterize(std::vector<uint8_t>& rgba) const
{
    const int W = width_, H = height_;
    std::vector<Color4> buf(static_cast<size_t>(W) * H, background_);

    for (const ScreenTriangle& t : triangles_)
    {
        const float area = (t.x[1] - t.x[0]) * (t.y[2] - t.y[0]) - (t.x[2] - t.x[0]) * (t.y[1] - t.y[0]);
        if (std::fabs(area) < 1e-9f)
            continue;
        const int xa = std::max(0, static_cast<int>(std::floor(std::min({t.x[0], t.x[1], t.x[2]}))));
        const int xb = std::min(W - 1, static_cast<int>(std::ceil(std::max({t.x[0], t.x[1], t.x[2]}))));
        const int ya = std::max(0, static_cast<int>(std::floor(std::min({t.y[0], t.y[1], t.y[2]}))));
        const int yb = std::min(H - 1, static_cast<int>(std::ceil(std::max({t.y[0], t.y[1], t.y[2]}))));
        const float inv = 1.0f / area;
        for (int y = ya; y <= yb; ++y)
        {
            for (int x = xa; x <= xb; ++x)
            {
                const float px = x + 0.5f, py = y + 0.5f;
                const float l1 = ((px - t.x[0]) * (t.y[2] - t.y[0]) - (t.x[2] - t.x[0]) * (py - t.y[0])) * inv;
                const float l2 = ((t.x[1] - t.x[0]) * (py - t.y[0]) - (px - t.x[0]) * (t.y[1] - t.y[0])) * inv;
                const float l0 = 1.0f - l1 - l2;
                if (l0 < 0 || l1 < 0 || l2 < 0)
                    continue;
                const Color4 s = {l0 * t.c[0].r + l1 * t.c[1].r + l2 * t.c[2].r,
                                  l0 * t.c[0].g + l1 * t.c[1].g + l2 * t.c[2].g,
                                  l0 * t.c[0].b + l1 * t.c[1].b + l2 * t.c[2].b,
                                  l0 * t.c[0].a + l1 * t.c[1].a + l2 * t.c[2].a};
                Color4& d = buf[static_cast<size_t>(y) * W + x];
                const float k = 1.0f - s.a;  // premultiplied "over"
                d             = {d.r * k + s.r, d.g * k + s.g, d.b * k + s.b, d.a * k + s.a};
            }
        }
    }

    rgba.resize(static_cast<size_t>(W) * H * 4);
    for (size_t i = 0; i < buf.size(); ++i)
    {
        const Color4& c = buf[i];
        const float inv = c.a > 0 ? 1.0f / c.a : 0.0f;
        auto to8 = [](float v) { return static_cast<uint8_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f); };
        rgba[i * 4 + 0] = to8(c.r * inv);
        rgba[i * 4 + 1] = to8(c.g * inv);
        rgba[i * 4 + 2] = to8(c.b * inv);
        rgba[i * 4 + 3] = to8(c.a);
    }
}

}  // namespace d3d
