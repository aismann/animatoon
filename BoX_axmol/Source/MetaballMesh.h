#pragma once
// Metaball-Mesh mit Clipper2: Skalarfeld -> Marching-Squares-Zellen -> Union (saubere Aussen-/Lochkonturen)
// -> Offset-Closing + Chaikin (glatte Kurven) -> Delaunay-Triangulierung. Ergebnis sind wenige, saubere Dreiecke
// (statt hunderter Zellen-Faecher). Header-only, unabhaengig von Axmol.
#include "clipper2/clipper.h"
#include "clipper2/clipper.triangulation.h"
#include <vector>
#include <set>
#include <cmath>
#include <cstdlib>

namespace mb
{

struct Mesh
{
    Clipper2Lib::PathsD tris;       // je Pfad genau 3 Punkte
    Clipper2Lib::PathsD outlines;   // geschlossene Konturen (Aussen + Loecher), fuer optionale Strich-Zeichnung
};

// Gemeinsamer Teil: Feld -> Zellen -> Union -> Closing -> Vereinfachen -> Chaikin. region = Aussen- + Lochkonturen.
inline bool buildRegion(const std::vector<float>& f, int N, float cell, int minX, int minY, int maxX, int maxY,
                        float T, float smooth, int chaikin, Clipper2Lib::PathsD& region, Clipper2Lib::PathsD& cells)
{
    using namespace Clipper2Lib;
    region.clear();
    cells.clear();
    auto val = [&](int x, int y) { return f[(size_t)y * N + x]; };

    for (int y = minY; y < maxY; ++y)
    {
        int runStart = -1;   // Lauf aus komplett gefuellten Zellen wird zu einem Rechteck zusammengefasst
        auto flushRun = [&](int xEnd) {
            if (runStart < 0) return;
            double x0 = runStart * cell, x1 = xEnd * cell, y0 = y * cell, y1 = (y + 1) * cell;
            cells.push_back({{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}});
            runStart = -1;
        };
        for (int x = minX; x < maxX; ++x)
        {
            const int cx[4] = {x, x + 1, x + 1, x}, cy[4] = {y, y, y + 1, y + 1};
            float v[4];
            int inside = 0;
            for (int i = 0; i < 4; ++i)
            {
                v[i] = val(cx[i], cy[i]);
                inside += v[i] >= T;
            }
            if (inside == 4)
            {
                if (runStart < 0) runStart = x;
                continue;
            }
            flushRun(x);
            if (inside == 0) continue;

            PathD poly;
            for (int i = 0; i < 4; ++i)
            {
                int j = (i + 1) & 3;
                PointD a(cx[i] * cell, cy[i] * cell), b(cx[j] * cell, cy[j] * cell);
                if (v[i] >= T) poly.push_back(a);
                if ((v[i] >= T) != (v[j] >= T))
                {
                    double t = (T - v[i]) / (v[j] - v[i]);
                    poly.push_back(PointD(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t));
                }
            }
            if (poly.size() >= 3) cells.push_back(std::move(poly));
        }
        flushRun(maxX);
    }
    if (cells.empty()) return false;

    region = Union(cells, FillRule::NonZero, 2);
    if (region.empty()) return false;

    if (smooth > 0.f)   // Closing: aufblasen und wieder schrumpfen -> schliesst enge Spalten, rundet Knicke
    {
        region = InflatePaths(region, smooth, JoinType::Round, EndType::Polygon, 2.0, 2, 0.6);
        region = InflatePaths(region, -smooth, JoinType::Round, EndType::Polygon, 2.0, 2, 0.6);
    }
    region = SimplifyPaths(region, 0.8, true);

    for (int it = 0; it < chaikin; ++it)   // Chaikin-Eckenschnitt auf geschlossenen Pfaden
    {
        for (PathD& p : region)
        {
            if (p.size() < 3) continue;
            PathD q;
            q.reserve(p.size() * 2);
            for (size_t i = 0; i < p.size(); ++i)
            {
                const PointD& a = p[i];
                const PointD& b = p[(i + 1) % p.size()];
                q.push_back(PointD(a.x * 0.75 + b.x * 0.25, a.y * 0.75 + b.y * 0.25));
                q.push_back(PointD(a.x * 0.25 + b.x * 0.75, a.y * 0.25 + b.y * 0.75));
            }
            p = std::move(q);
        }
    }
    return !region.empty();
}

// f: N*N Feldwerte, Wert (x,y) liegt bei (x*cell, y*cell). Bereich [minX,maxX) x [minY,maxY) in Zellen.
// T: Iso-Wert. smooth: Closing-Radius in Pixeln (0 = aus). chaikin: Anzahl Eckenschnitt-Durchgaenge.
inline bool buildMesh(const std::vector<float>& f, int N, float cell, int minX, int minY, int maxX, int maxY, float T,
                      float smooth, int chaikin, Mesh& out)
{
    using namespace Clipper2Lib;
    out.tris.clear();
    out.outlines.clear();

    PathsD region, cells;
    if (!buildRegion(f, N, cell, minX, minY, maxX, maxY, T, smooth, chaikin, region, cells)) return false;
    out.outlines = region;

    PathsD tris;
    if (Triangulate(region, 2, tris, true) == TriangulateResult::success && !tris.empty())
    {
        out.tris = std::move(tris);
        return true;
    }

    // Fallback (z.B. numerische Selbstueberschneidung): Zellen direkt als Faecher
    for (const PathD& c : cells)
        for (size_t i = 1; i + 1 < c.size(); ++i) out.tris.push_back({c[0], c[i], c[i + 1]});
    return !out.tris.empty();
}

// Dreieck mit Ring-Index je Ecke (0 = Aussenkontur, k = Kontur um insets[k] nach innen versetzt).
// Damit lassen sich Eckenfarben (Verlauf Rand -> Kern) setzen, z.B. fuer DrawNode::drawColoredTriangle.
struct ColTri
{
    Clipper2Lib::PointD p[3];
    int ring[3];
};

// Verlauf per Clipper2: Ringe = InflatePaths(region, -inset). Band k = Difference(Ring k, Ring k+1) wird
// triangulierte, der innerste Ring wird komplett gefuellt. Ecken auf dem inneren Ring bekommen Index k+1,
// alle anderen k - so entsteht ein weicher Farbverlauf vom Rand zum Kern.
inline bool buildShaded(const std::vector<float>& f, int N, float cell, int minX, int minY, int maxX, int maxY, float T,
                        float smooth, int chaikin, const std::vector<double>& insets, std::vector<ColTri>& out)
{
    using namespace Clipper2Lib;
    out.clear();
    PathsD region, cells;
    if (!buildRegion(f, N, cell, minX, minY, maxX, maxY, T, smooth, chaikin, region, cells)) return false;

    std::vector<PathsD> rings;
    rings.push_back(region);
    for (size_t k = 1; k < insets.size(); ++k)
    {
        PathsD r = InflatePaths(region, -insets[k], JoinType::Round, EndType::Polygon, 2.0, 2, 0.6);
        if (r.empty()) break;   // Blob ist an dieser Stelle zu duenn fuer weitere Ringe
        rings.push_back(std::move(r));
    }

    auto key = [](const PointD& p) { return std::make_pair(std::llround(p.x * 100.0), std::llround(p.y * 100.0)); };
    auto addTris = [&](const PathsD& tris, int ringA, const std::set<std::pair<long long, long long>>* inner, int ringB) {
        for (const PathD& t : tris)
        {
            if (t.size() != 3) continue;
            ColTri c;
            for (int i = 0; i < 3; ++i)
            {
                c.p[i]    = t[i];
                c.ring[i] = (inner && inner->count(key(t[i]))) ? ringB : ringA;
            }
            out.push_back(c);
        }
    };

    const int last = (int)rings.size() - 1;
    for (int k = 0; k < last; ++k)
    {
        PathsD band = Difference(rings[k], rings[k + 1], FillRule::NonZero, 2);
        PathsD tris;
        if (band.empty() || Triangulate(band, 2, tris, true) != TriangulateResult::success) continue;
        std::set<std::pair<long long, long long>> inner;
        for (const PathD& p : rings[k + 1])
            for (const PointD& q : p) inner.insert(key(q));
        addTris(tris, k, &inner, k + 1);
    }
    PathsD core;
    if (Triangulate(rings[last], 2, core, true) == TriangulateResult::success)
        addTris(core, last, nullptr, last);

    if (out.empty())   // Fallback: flache Zellen mit Ring 0
    {
        for (const PathD& c : cells)
            for (size_t i = 1; i + 1 < c.size(); ++i)
            {
                ColTri t;
                t.p[0] = c[0]; t.p[1] = c[i]; t.p[2] = c[i + 1];
                t.ring[0] = t.ring[1] = t.ring[2] = 0;
                out.push_back(t);
            }
    }
    return !out.empty();
}

}  // namespace mb
