#include "HomesteadMapGeometry.h"

#include <algorithm>
#include <cmath>

namespace HomesteadMap
{
namespace
{
constexpr double Pi = 3.14159265358979323846;
Vec Add(Vec a, Vec b) { return {a.x + b.x, a.y + b.y}; }
Vec Sub(Vec a, Vec b) { return {a.x - b.x, a.y - b.y}; }
Vec Mul(Vec a, double s) { return {a.x * s, a.y * s}; }
double Dot(Vec a, Vec b) { return a.x * b.x + a.y * b.y; }
double Cross(Vec a, Vec b) { return a.x * b.y - a.y * b.x; }
double Length(Vec a) { return std::sqrt(Dot(a, a)); }
Vec Forward(double degrees) { const double r = degrees * Pi / 180.0; return {std::cos(r), std::sin(r)}; }
Vec Right(double degrees) { const double r = degrees * Pi / 180.0; return {-std::sin(r), std::cos(r)}; }
Vec Rotate(Vec v, double degrees)
{
    const double r = degrees * Pi / 180.0, c = std::cos(r), s = std::sin(r);
    return {v.x * c - v.y * s, v.x * s + v.y * c};
}
double SegmentDistance(Vec p, Vec a, Vec b)
{
    const Vec ab = Sub(b, a);
    const double lengthSquared = Dot(ab, ab);
    const double t = lengthSquared > 0.0 ? std::clamp(Dot(Sub(p, a), ab) / lengthSquared, 0.0, 1.0) : 0.0;
    return Length(Sub(p, Add(a, Mul(ab, t))));
}
bool InTriangle(Vec p, Vec a, Vec b, Vec c)
{
    const double d1 = Cross(Sub(b, a), Sub(p, a));
    const double d2 = Cross(Sub(c, b), Sub(p, b));
    const double d3 = Cross(Sub(a, c), Sub(p, c));
    return d1 >= 0.0 && d2 >= 0.0 && d3 >= 0.0;
}
}

Vec WorldToUV(const MapTransform& map, Vec world)
{
    return {(world.y - map.minY) / map.sizeY, (map.minX + map.sizeX - world.x) / map.sizeX};
}

Vec UVToWorld(const MapTransform& map, Vec uv)
{
    return {map.minX + map.sizeX - uv.y * map.sizeX, map.minY + uv.x * map.sizeY};
}

Vec WorldToScreen(const View& view, Vec world)
{
    const Vec d = Sub(world, view.center);
    return {view.screenCenter.x + Dot(d, Right(view.headingDegrees)) * view.pixelsPerCm,
        view.screenCenter.y - Dot(d, Forward(view.headingDegrees)) * view.pixelsPerCm};
}

Vec ScreenToWorld(const View& view, Vec screen)
{
    const double across = (screen.x - view.screenCenter.x) / view.pixelsPerCm;
    const double ahead = -(screen.y - view.screenCenter.y) / view.pixelsPerCm;
    return Add(view.center, Add(Mul(Right(view.headingDegrees), across), Mul(Forward(view.headingDegrees), ahead)));
}

Vec ScreenDirection(const View& view, double yawDegrees)
{
    const Vec direction = Forward(yawDegrees);
    return {Dot(direction, Right(view.headingDegrees)), -Dot(direction, Forward(view.headingDegrees))};
}

Vec ClampToRadius(Vec offset, double radius)
{
    const double length = Length(offset);
    return length > radius && length > 0.0 ? Mul(offset, radius / length) : offset;
}

bool ClipSegmentToCircle(Vec& a, Vec& b, Vec center, double radius)
{
    const Vec d = Sub(b, a), f = Sub(a, center);
    const double qa = Dot(d, d), qb = 2.0 * Dot(f, d), qc = Dot(f, f) - radius * radius;
    if (qa <= 0.0) return qc <= 0.0;
    const double discriminant = qb * qb - 4.0 * qa * qc;
    if (discriminant <= 0.0) return false;
    const double root = std::sqrt(discriminant);
    const double t0 = std::max(0.0, (-qb - root) / (2.0 * qa));
    const double t1 = std::min(1.0, (-qb + root) / (2.0 * qa));
    if (t0 >= t1) return false;
    const Vec start = a;
    a = Add(start, Mul(d, t0));
    b = Add(start, Mul(d, t1));
    return true;
}

bool ClipSegmentToRect(Vec& a, Vec& b, Vec min, Vec max)
{
    double t0 = 0.0, t1 = 1.0;
    const Vec d = Sub(b, a);
    const double p[] = {-d.x, d.x, -d.y, d.y};
    const double q[] = {a.x - min.x, max.x - a.x, a.y - min.y, max.y - a.y};
    for (int i = 0; i < 4; ++i)
    {
        if (p[i] == 0.0)
        {
            if (q[i] < 0.0) return false;
            continue;
        }
        const double t = q[i] / p[i];
        if (p[i] < 0.0) t0 = std::max(t0, t);
        else t1 = std::min(t1, t);
        if (t0 > t1) return false;
    }
    const Vec start = a;
    a = Add(start, Mul(d, t0));
    b = Add(start, Mul(d, t1));
    return true;
}

std::vector<Vec> ClipPolygonToRect(const std::vector<Vec>& polygon, Vec min, Vec max)
{
    std::vector<Vec> current = polygon;
    // Each rectangle side as (axis, bound, keep-greater): x >= min.x, x <= max.x, y >= min.y, y <= max.y.
    for (int side = 0; side < 4 && !current.empty(); ++side)
    {
        const bool alongX = side < 2;
        const bool keepGreater = side % 2 == 0;
        const double bound = alongX ? (keepGreater ? min.x : max.x) : (keepGreater ? min.y : max.y);
        const auto Value = [alongX](Vec v) { return alongX ? v.x : v.y; };
        const auto Inside = [&](Vec v) { return keepGreater ? Value(v) >= bound : Value(v) <= bound; };
        std::vector<Vec> next;
        next.reserve(current.size() + 2);
        for (std::size_t i = 0; i < current.size(); ++i)
        {
            const Vec a = current[i], b = current[(i + 1) % current.size()];
            const bool aIn = Inside(a), bIn = Inside(b);
            if (aIn) next.push_back(a);
            if (aIn != bIn)
            {
                const double t = (bound - Value(a)) / (Value(b) - Value(a));
                next.push_back(Add(a, Mul(Sub(b, a), t)));
            }
        }
        current = std::move(next);
    }
    if (current.size() < 3) current.clear();
    return current;
}

std::vector<Segment> DashRing(const std::vector<Vec>& ring, double dash, double gap)
{
    std::vector<Segment> dashes;
    const std::size_t count = ring.size();
    if (count < 2 || !(dash > 0.0) || !(gap >= 0.0)) return dashes;
    const double period = dash + gap;
    // Each dash covers [k * period, k * period + dash] of the distance walked round the ring, so
    // the pattern carries across corners. Integer steps always terminate.
    double walked = 0.0;
    for (std::size_t i = 0; i < count; ++i)
    {
        const Vec a = ring[i], b = ring[(i + 1) % count];
        const double length = Length(Sub(b, a));
        if (!(length > 0.0) || !std::isfinite(length)) continue;
        const Vec step = Mul(Sub(b, a), 1.0 / length);
        const double start = walked, end = walked + length;
        const long long first = static_cast<long long>(std::floor(start / period));
        const long long last = static_cast<long long>(std::floor(end / period));
        for (long long k = first; k <= last && dashes.size() < 100000; ++k)
        {
            const double from = std::max(start, k * period), to = std::min(end, k * period + dash);
            if (to > from) dashes.push_back({Add(a, Mul(step, from - start)), Add(a, Mul(step, to - start))});
        }
        walked = end;
    }
    return dashes;
}

std::vector<Segment> HatchPolygon(const std::vector<Vec>& ring, double spacing, double angleDegrees)
{
    std::vector<Segment> lines;
    if (ring.size() < 3 || spacing <= 0.0) return lines;
    // Turn the polygon so the hatch lines are horizontal, scan, then turn the results back.
    std::vector<Vec> turned;
    turned.reserve(ring.size());
    double low = 1e300, high = -1e300;
    for (const Vec& point : ring)
    {
        turned.push_back(Rotate(point, -angleDegrees));
        low = std::min(low, turned.back().y);
        high = std::max(high, turned.back().y);
    }
    if ((high - low) / spacing > 20000.0) return lines;
    std::vector<double> crossings;
    for (double y = std::floor(low / spacing) * spacing + spacing * 0.5; y < high; y += spacing)
    {
        crossings.clear();
        for (std::size_t i = 0, j = turned.size() - 1; i < turned.size(); j = i++)
        {
            const Vec a = turned[i], b = turned[j];
            if ((a.y > y) != (b.y > y)) crossings.push_back(a.x + (y - a.y) * (b.x - a.x) / (b.y - a.y));
        }
        std::sort(crossings.begin(), crossings.end());
        for (std::size_t k = 0; k + 1 < crossings.size(); k += 2)
            lines.push_back({Rotate({crossings[k], y}, angleDegrees), Rotate({crossings[k + 1], y}, angleDegrees)});
    }
    return lines;
}

double SignedArea(const std::vector<Vec>& ring)
{
    double area = 0.0;
    for (std::size_t i = 0, j = ring.size() ? ring.size() - 1 : 0; i < ring.size(); j = i++)
        area += Cross(ring[j], ring[i]);
    return area * 0.5;
}

std::vector<int> Triangulate(const std::vector<Vec>& ring)
{
    std::vector<int> triangles;
    const int count = static_cast<int>(ring.size());
    if (count < 3) return triangles;
    std::vector<int> remaining(count);
    for (int i = 0; i < count; ++i) remaining[i] = i;
    // Work counter-clockwise (positive area) so convex corners have a positive cross product.
    if (SignedArea(ring) < 0.0) std::reverse(remaining.begin(), remaining.end());
    int guard = count * count + 8;
    while (remaining.size() > 3 && guard-- > 0)
    {
        bool clipped = false;
        const int n = static_cast<int>(remaining.size());
        for (int i = 0; i < n; ++i)
        {
            const int ia = remaining[(i + n - 1) % n], ib = remaining[i], ic = remaining[(i + 1) % n];
            const Vec a = ring[ia], b = ring[ib], c = ring[ic];
            const double corner = Cross(Sub(b, a), Sub(c, b));
            if (corner < 0.0) continue;
            if (corner == 0.0)
            {
                // A straight run adds no area; drop the middle point.
                remaining.erase(remaining.begin() + i);
                clipped = true;
                break;
            }
            bool ear = true;
            for (int k = 0; k < n && ear; ++k)
            {
                const int other = remaining[k];
                if (other == ia || other == ib || other == ic) continue;
                ear = !InTriangle(ring[other], a, b, c);
            }
            if (!ear) continue;
            triangles.insert(triangles.end(), {ia, ib, ic});
            remaining.erase(remaining.begin() + i);
            clipped = true;
            break;
        }
        if (!clipped) break; // Not simple; keep what we have rather than loop.
    }
    if (remaining.size() == 3 && Cross(Sub(ring[remaining[1]], ring[remaining[0]]), Sub(ring[remaining[2]], ring[remaining[1]])) != 0.0)
        triangles.insert(triangles.end(), {remaining[0], remaining[1], remaining[2]});
    return triangles;
}

bool Contains(const std::vector<Vec>& ring, Vec point)
{
    bool inside = false;
    for (std::size_t i = 0, j = ring.size() ? ring.size() - 1 : 0; i < ring.size(); j = i++)
    {
        const Vec a = ring[i], b = ring[j];
        if ((a.y > point.y) != (b.y > point.y) && point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x)
            inside = !inside;
    }
    return inside;
}

double DistanceToRing(const std::vector<Vec>& ring, Vec point)
{
    double best = 1e300;
    for (std::size_t i = 0, j = ring.size() ? ring.size() - 1 : 0; i < ring.size(); j = i++)
        best = std::min(best, SegmentDistance(point, ring[j], ring[i]));
    return best;
}

Vec Centroid(const std::vector<Vec>& ring)
{
    const double area = SignedArea(ring);
    if (std::abs(area) < 1e-9)
    {
        Vec sum;
        for (const Vec& point : ring) sum = Add(sum, point);
        return ring.empty() ? sum : Mul(sum, 1.0 / static_cast<double>(ring.size()));
    }
    Vec center;
    for (std::size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
    {
        const double f = Cross(ring[j], ring[i]);
        center = Add(center, Mul(Add(ring[j], ring[i]), f));
    }
    return Mul(center, 1.0 / (6.0 * area));
}

double AtLeastPhysical(double local, double physicalPerLocal, double minPhysical)
{
    if (physicalPerLocal <= 0.0) return local;
    return std::max(local, minPhysical / physicalPerLocal);
}

double SnapToPixel(double local, double physicalPerLocal)
{
    if (physicalPerLocal <= 0.0) return local;
    return std::round(local * physicalPerLocal) / physicalPerLocal;
}

std::vector<int> SpacedCircles(const std::vector<Vec>& centers, const std::vector<double>& radii, double gap)
{
    std::vector<int> kept;
    const std::size_t count = std::min(centers.size(), radii.size());
    for (std::size_t i = 0; i < count; ++i)
    {
        bool clear = true;
        for (int other : kept)
            clear = clear && Length(Sub(centers[i], centers[other])) >= radii[i] + radii[other] + gap;
        if (clear) kept.push_back(static_cast<int>(i));
    }
    return kept;
}

double BearingDegrees(Vec from, Vec to)
{
    const Vec d = Sub(to, from);
    if (Length(d) < 1e-9) return 0.0;
    const double degrees = std::atan2(d.y, d.x) * 180.0 / Pi;
    return degrees < 0.0 ? degrees + 360.0 : degrees;
}

double RelativeDegrees(double bearing, double heading)
{
    double relative = std::fmod(bearing - heading, 360.0);
    if (relative <= -180.0) relative += 360.0;
    if (relative > 180.0) relative -= 360.0;
    return relative;
}

bool CompassOffset(double bearing, double heading, double halfFieldDegrees, double halfWidth, double& offset)
{
    if (halfFieldDegrees <= 0.0) return false;
    const double relative = RelativeDegrees(bearing, heading);
    offset = relative / halfFieldDegrees * halfWidth;
    return std::abs(relative) <= halfFieldDegrees;
}
}
