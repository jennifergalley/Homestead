#include "HomesteadMapGeometry.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace HomesteadMap;

namespace
{
int checks = 0;
void Check(bool condition, const char* expression, int line)
{
    ++checks;
    if (!condition)
    {
        std::cerr << "FAIL line " << line << ": " << expression << '\n';
        std::exit(1);
    }
}
#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)
bool Near(Vec a, Vec b, double tolerance = 1e-6) { return std::abs(a.x - b.x) <= tolerance && std::abs(a.y - b.y) <= tolerance; }
double Area(const std::vector<Vec>& ring, const std::vector<int>& triangles)
{
    double total = 0;
    for (std::size_t i = 0; i + 2 < triangles.size(); i += 3)
        total += std::abs(SignedArea({ring[triangles[i]], ring[triangles[i + 1]], ring[triangles[i + 2]]}));
    return total;
}

void Transforms()
{
    const MapTransform map;
    // North-west corner is the texture's top-left; the origin is its middle.
    CHECK(Near(WorldToUV(map, {201600, -201600}), {0, 0}));
    CHECK(Near(WorldToUV(map, {-201600, 201600}), {1, 1}));
    CHECK(Near(WorldToUV(map, {0, 0}), {0.5, 0.5}));
    CHECK(Near(UVToWorld(map, WorldToUV(map, {-25350, -64050})), {-25350, -64050}, 1e-6));
    // The cropped sheet (shrink-estate-map): X -92000..60000, Y -136000..16000; nothing assumes 4 km.
    const MapTransform sheet{-92000.0, -136000.0, 152000.0, 152000.0};
    CHECK(Near(WorldToUV(sheet, {60000, -136000}), {0, 0}));
    CHECK(Near(WorldToUV(sheet, {-92000, 16000}), {1, 1}));
    CHECK(Near(UVToWorld(sheet, WorldToUV(sheet, {-7119, 11588})), {-7119, 11588}, 1e-6));

    // North-up: north is screen-up, east is screen-right.
    View north{{1000, 2000}, {100, 100}, 0.01, 0.0};
    CHECK(Near(WorldToScreen(north, {1000, 2000}), {100, 100}));
    CHECK(Near(WorldToScreen(north, {2000, 2000}), {100, 90}));
    CHECK(Near(WorldToScreen(north, {1000, 3000}), {110, 100}));
    CHECK(Near(ScreenDirection(north, 0), {0, -1}));
    CHECK(Near(ScreenDirection(north, 90), {1, 0}));
    // Rotating with a camera that faces east puts east at the top and north on the left.
    View east = north;
    east.headingDegrees = 90;
    CHECK(Near(WorldToScreen(east, {1000, 3000}), {100, 90}));
    CHECK(Near(WorldToScreen(east, {2000, 2000}), {90, 100}));
    CHECK(Near(ScreenDirection(east, 0), {-1, 0}));
    CHECK(Near(ScreenDirection(east, 90), {0, -1}));
    for (double heading : {0.0, 33.0, 90.0, 211.0})
    {
        View view = north;
        view.headingDegrees = heading;
        const Vec world{-4321, 8765};
        CHECK(Near(ScreenToWorld(view, WorldToScreen(view, world)), world, 1e-6));
    }
}

void Clipping()
{
    CHECK(Near(ClampToRadius({30, 40}, 10), {6, 8}));
    CHECK(Near(ClampToRadius({3, 4}, 10), {3, 4}));
    Vec a{-20, 0}, b{20, 0};
    CHECK(ClipSegmentToCircle(a, b, {0, 0}, 10));
    CHECK(Near(a, {-10, 0}) && Near(b, {10, 0}));
    a = {-20, 20}; b = {20, 20};
    CHECK(!ClipSegmentToCircle(a, b, {0, 0}, 10));
    a = {1, 1}; b = {2, 2};
    CHECK(ClipSegmentToCircle(a, b, {0, 0}, 10) && Near(a, {1, 1}) && Near(b, {2, 2}));
    a = {-5, 5}; b = {15, 5};
    CHECK(ClipSegmentToRect(a, b, {0, 0}, {10, 10}) && Near(a, {0, 5}) && Near(b, {10, 5}));
    a = {-5, -5}; b = {-1, 20};
    CHECK(!ClipSegmentToRect(a, b, {0, 0}, {10, 10}));
    // A polygon clipped to the unit square: a diamond poking out of every side keeps an octagon of area 7/8.
    const std::vector<Vec> diamond{{0.5, -0.25}, {1.25, 0.5}, {0.5, 1.25}, {-0.25, 0.5}};
    const std::vector<Vec> clipped = ClipPolygonToRect(diamond, {0, 0}, {1, 1});
    CHECK(clipped.size() == 8 && std::abs(std::abs(SignedArea(clipped)) - 0.875) < 1e-9);
    for (const Vec& point : clipped) CHECK(point.x >= -1e-12 && point.x <= 1 + 1e-12 && point.y >= -1e-12 && point.y <= 1 + 1e-12);
    const std::vector<Vec> inside{{0.2, 0.2}, {0.8, 0.2}, {0.5, 0.7}};
    CHECK(ClipPolygonToRect(inside, {0, 0}, {1, 1}).size() == 3);
    CHECK(ClipPolygonToRect({{2, 2}, {3, 2}, {3, 3}}, {0, 0}, {1, 1}).empty());
}

void DashesAndHatching()
{
    const std::vector<Vec> square{{0, 0}, {10, 0}, {10, 10}, {0, 10}};
    const auto dashes = DashRing(square, 3, 2);
    // A 40-unit perimeter holds eight 5-unit periods, each drawing three units of line.
    double drawn = 0;
    for (const auto& dash : dashes)
        drawn += std::hypot(dash.second.x - dash.first.x, dash.second.y - dash.first.y);
    CHECK(std::abs(drawn - 24.0) < 1e-9);
    CHECK(DashRing(square, 0, 2).empty() && DashRing({{0, 0}}, 3, 2).empty());
    // Far-off screen rings with awkward fractional periods still terminate (this once hung).
    const std::vector<Vec> far{{-21873.37, 9021.113}, {15022.7, -31000.9}, {40011.1, 20000.3}, {-3.3, 44444.4}};
    const auto farDashes = DashRing(far, 8.123456789, 5.0000001);
    CHECK(!farDashes.empty() && farDashes.size() < 100000);
    // Hatching stays inside, even for a concave notch.
    const std::vector<Vec> ell{{0, 0}, {10, 0}, {10, 4}, {4, 4}, {4, 10}, {0, 10}};
    const auto lines = HatchPolygon(ell, 1.0, 45.0);
    CHECK(!lines.empty());
    for (const auto& line : lines)
    {
        const Vec middle{(line.first.x + line.second.x) * 0.5, (line.first.y + line.second.y) * 0.5};
        CHECK(Contains(ell, middle));
    }
    CHECK(HatchPolygon(ell, 0, 45).empty());
}

void Triangulation()
{
    const std::vector<Vec> ell{{0, 0}, {10, 0}, {10, 4}, {4, 4}, {4, 10}, {0, 10}};
    const auto triangles = Triangulate(ell);
    CHECK(triangles.size() == 12);
    CHECK(std::abs(Area(ell, triangles) - 64.0) < 1e-9);
    std::vector<Vec> clockwise(ell.rbegin(), ell.rend());
    CHECK(std::abs(Area(clockwise, Triangulate(clockwise)) - 64.0) < 1e-9);
    // Collinear points are dropped without losing area.
    const std::vector<Vec> strip{{0, 0}, {5, 0}, {10, 0}, {10, 5}, {0, 5}};
    CHECK(std::abs(Area(strip, Triangulate(strip)) - 50.0) < 1e-9);
    CHECK(Triangulate({{0, 0}, {1, 1}}).empty());
    CHECK(Near(Centroid({{0, 0}, {10, 0}, {10, 10}, {0, 10}}), {5, 5}));
    CHECK(std::abs(DistanceToRing({{0, 0}, {10, 0}, {10, 10}, {0, 10}}, {5, 3}) - 3.0) < 1e-9);
    CHECK(Contains({{0, 0}, {10, 0}, {10, 10}, {0, 10}}, {5, 5}) && !Contains({{0, 0}, {10, 0}, {10, 10}}, {9, 9.5}));
}

void Legibility()
{
    // 720p: 0.667 physical px per HUD unit, so an 8.5-unit badge is under 6 px until it's raised.
    CHECK(std::abs(AtLeastPhysical(8.5, 0.667, 9.0) * 0.667 - 9.0) < 1e-9);
    CHECK(AtLeastPhysical(11.0, 1.5, 9.0) == 11.0);
    CHECK(AtLeastPhysical(4.0, 0.0, 9.0) == 4.0);
    CHECK(std::abs(SnapToPixel(10.3, 1.5) * 1.5 - 15.0) < 1e-9);
    CHECK(std::abs(SnapToPixel(-2.9, 2.0) * 2.0 - -6.0) < 1e-9);
    CHECK(SnapToPixel(7.25, 0.0) == 7.25);
    // Priority order wins: the second overlaps the first and waits; the third is clear.
    const auto kept = SpacedCircles({{0, 0}, {15, 0}, {40, 0}}, {10, 10, 10}, 4);
    CHECK(kept.size() == 2 && kept[0] == 0 && kept[1] == 2);
    CHECK(SpacedCircles({{0, 0}, {24, 0}}, {10, 10}, 4).size() == 2);
    CHECK(SpacedCircles({{0, 0}}, {}, 4).empty());
}

void Compass()
{
    // +X is north (0), +Y east (90), -X south (180), -Y west (270).
    CHECK(std::abs(BearingDegrees({0, 0}, {10, 0}) - 0.0) < 1e-9);
    CHECK(std::abs(BearingDegrees({0, 0}, {0, 10}) - 90.0) < 1e-9);
    CHECK(std::abs(BearingDegrees({0, 0}, {-10, 0}) - 180.0) < 1e-9);
    CHECK(std::abs(BearingDegrees({0, 0}, {0, -10}) - 270.0) < 1e-9);
    CHECK(std::abs(BearingDegrees({5, 5}, {15, 15}) - 45.0) < 1e-9);
    CHECK(BearingDegrees({3, 3}, {3, 3}) == 0.0);
    CHECK(std::abs(RelativeDegrees(10, 350) - 20.0) < 1e-9);
    CHECK(std::abs(RelativeDegrees(350, 10) - -20.0) < 1e-9);
    CHECK(std::abs(RelativeDegrees(180, 0) - 180.0) < 1e-9);
    CHECK(std::abs(RelativeDegrees(0, 180) - 180.0) < 1e-9);
    CHECK(std::abs(RelativeDegrees(-90, 720) - -90.0) < 1e-9);
    // Facing east, north is a quarter-field to the left and south a quarter to the right.
    double offset = 0;
    CHECK(CompassOffset(0, 90, 90, 200, offset) && std::abs(offset - -200.0) < 1e-9);
    CHECK(CompassOffset(180, 90, 90, 200, offset) && std::abs(offset - 200.0) < 1e-9);
    CHECK(CompassOffset(90, 90, 90, 200, offset) && std::abs(offset) < 1e-9);
    CHECK(CompassOffset(135, 90, 90, 200, offset) && std::abs(offset - 100.0) < 1e-9);
    // Behind her is off the strip.
    CHECK(!CompassOffset(270, 90, 90, 200, offset));
    CHECK(!CompassOffset(0, 0, 0, 200, offset));
}
}

int main()
{
    Transforms();
    Clipping();
    DashesAndHatching();
    Triangulation();
    Legibility();
    Compass();
    std::cout << "Map geometry: " << checks << " checks passed.\n";
    return 0;
}
