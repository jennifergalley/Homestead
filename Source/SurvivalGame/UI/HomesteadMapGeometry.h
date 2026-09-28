#pragma once

#include <utility>
#include <vector>

// Portable map geometry shared by the HUD minimap and the field-book Map tab (no engine types, so
// native tests cover it). World frame: Unreal centimetres, +X north, +Y east. Map UV: u runs east,
// v runs south, so north is up.
namespace HomesteadMap
{
struct Vec
{
    double x = 0.0;
    double y = 0.0;
};

// The baked map's world rectangle: its north-west corner is (maxX, minY).
struct MapTransform
{
    double minX = -201600.0;
    double minY = -201600.0;
    double sizeX = 403200.0; // North-south extent.
    double sizeY = 403200.0; // East-west extent.
};

Vec WorldToUV(const MapTransform& map, Vec world);
Vec UVToWorld(const MapTransform& map, Vec uv);

// A view onto the world, in screen pixels (y down): `center` in the world appears at `screenCenter`,
// `pixelsPerCm` scales, and `headingDegrees` (an Unreal yaw) points screen-up. North-up is 0.
struct View
{
    Vec center;
    Vec screenCenter;
    double pixelsPerCm = 1.0;
    double headingDegrees = 0.0;
};

Vec WorldToScreen(const View& view, Vec world);
Vec ScreenToWorld(const View& view, Vec screen);
// The screen direction (unit, y down) of a world heading, e.g. her facing or north (0).
Vec ScreenDirection(const View& view, double yawDegrees);

// Keeps an offset from a circle's centre within `radius` (for off-crop landmark glyphs).
Vec ClampToRadius(Vec offset, double radius);
// Clips the segment to the disc; false when nothing of it lies inside.
bool ClipSegmentToCircle(Vec& a, Vec& b, Vec center, double radius);
// Clips the segment to the rectangle [min, max]; false when nothing of it lies inside.
bool ClipSegmentToRect(Vec& a, Vec& b, Vec min, Vec max);

using Segment = std::pair<Vec, Vec>;
// Dashes along a closed ring (the last point joins the first), continuing the pattern across
// corners so the line reads evenly.
std::vector<Segment> DashRing(const std::vector<Vec>& ring, double dash, double gap);
// Parallel diagonal hatch lines clipped to a simple polygon (even-odd), `spacing` apart.
std::vector<Segment> HatchPolygon(const std::vector<Vec>& ring, double spacing, double angleDegrees);
// Ear-clipping triangulation of a simple polygon; indices into `ring`, three per triangle.
std::vector<int> Triangulate(const std::vector<Vec>& ring);
double SignedArea(const std::vector<Vec>& ring);
// Even-odd containment for a simple ring.
bool Contains(const std::vector<Vec>& ring, Vec point);
// Distance from a point to the nearest edge of a closed ring.
double DistanceToRing(const std::vector<Vec>& ring, Vec point);
Vec Centroid(const std::vector<Vec>& ring);
}
