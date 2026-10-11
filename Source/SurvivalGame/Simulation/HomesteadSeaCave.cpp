#include "HomesteadSeaCave.h"

#include "HomesteadEstate.h"

#include <cmath>
#include <cstring>

namespace Homestead
{
namespace
{
constexpr double SeaCavePi = 3.14159265358979323846;

struct SeaCaveTable
{
    double ax = 0.0, ay = 0.0, az = 0.0, yaw = 0.0;
    double waterZ = 0.0, floorMin = 0.0, roofMax = 0.0;
    double lx = 0.0, ly = 0.0, lz = 0.0;
    std::vector<Point> footprint, shore;
    std::vector<SeaCaveProp> props;     // local position and yaw
};

SeaCaveTable LoadSeaCaveTable()
{
    SeaCaveTable t;
#define SEA_CAVE_ANCHOR(X, Y, Z, YAW) t.ax = X; t.ay = Y; t.az = Z; t.yaw = YAW;
#define SEA_CAVE_WATER_Z(Z) t.waterZ = Z;
#define SEA_CAVE_FLOOR_MIN(Z) t.floorMin = Z;
#define SEA_CAVE_ROOF_MAX(Z) t.roofMax = Z;
#define SEA_CAVE_LIGHT(X, Y, Z) t.lx = X; t.ly = Y; t.lz = Z;
#define SEA_CAVE_FOOTPRINT(X, Y) t.footprint.push_back({X, Y});
#define SEA_CAVE_SHORE(X, Y) t.shore.push_back({X, Y});
#define SEA_CAVE_PROP(MESH, X, Y, Z, YAW, SCALE) t.props.push_back({MESH, {X, Y}, Z, YAW, SCALE});
#include "HomesteadSeaCave.inc"
#undef SEA_CAVE_PROP
#undef SEA_CAVE_ANCHOR
#undef SEA_CAVE_WATER_Z
#undef SEA_CAVE_FLOOR_MIN
#undef SEA_CAVE_ROOF_MAX
#undef SEA_CAVE_LIGHT
#undef SEA_CAVE_FOOTPRINT
#undef SEA_CAVE_SHORE
    return t;
}

SeaCave BuildSeaCave()
{
    const SeaCaveTable t = LoadSeaCaveTable();
    SeaCave cave;
    cave.anchor = {t.ax, t.ay};
    cave.floorZ = t.az;
    cave.yaw = t.yaw;
    cave.waterZ = t.az + t.waterZ;
    cave.floorMinZ = t.az + t.floorMin;
    cave.roofMaxZ = t.az + t.roofMax;
    const Point light = cave.ToWorld({t.lx, t.ly});
    cave.lightX = light.x;
    cave.lightY = light.y;
    cave.lightZ = t.az + t.lz;
    for (const Point& p : t.footprint) cave.footprint.push_back(cave.ToWorld(p));
    for (const Point& p : t.shore) cave.shore.push_back(cave.ToWorld(p));
    for (SeaCaveProp prop : t.props)
    {
        prop.position = cave.ToWorld(prop.position);
        prop.z += t.az;
        prop.yaw += t.yaw;
        cave.props.push_back(prop);
    }
    return cave;
}
}

Point SeaCave::ToWorld(Point local) const
{
    const double c = std::cos(yaw * SeaCavePi / 180.0), s = std::sin(yaw * SeaCavePi / 180.0);
    return {anchor.x + c * local.x - s * local.y, anchor.y + s * local.x + c * local.y};
}

Point SeaCave::ToLocal(Point world) const
{
    const double c = std::cos(yaw * SeaCavePi / 180.0), s = std::sin(yaw * SeaCavePi / 180.0);
    const double dx = world.x - anchor.x, dy = world.y - anchor.y;
    return {c * dx + s * dy, -s * dx + c * dy};
}

const SeaCave& EstateSeaCave()
{
    static const SeaCave cave = BuildSeaCave();
    return cave;
}

const std::vector<CoveCliffProp>& EstateCoveCliff()
{
    static const std::vector<CoveCliffProp> props = []
    {
        std::vector<CoveCliffProp> out;
#define COVE_CLIFF_PROP(MESH, X, Y, Z, YAW, UX, UY, UZ, SCALE, SCALEZ) out.push_back({MESH, {X, Y}, Z, YAW, UX, UY, UZ, SCALE, SCALEZ});
#include "HomesteadCoveCliff.inc"
#undef COVE_CLIFF_PROP
        return out;
    }();
    return props;
}

bool CoveCliffPropBlocks(const CoveCliffProp& prop)
{
    return std::strstr(prop.mesh, "/Granite") != nullptr;
}

bool InSeaCave(const SeaCave& cave, Point point, double z, double surfaceZ)
{
    if (!cave.valid() || !std::isfinite(z) || !std::isfinite(surfaceZ)) return false;
    if (z > surfaceZ - SeaCaveBelowSurfaceCm || z < cave.floorMinZ - SeaCaveBelowFloorCm) return false;
    return PointInPolygon(cave.footprint, point);
}

double SeaCaveAwareGround(const SeaCave& cave, Point point, double z, double surfaceZ)
{
    return InSeaCave(cave, point, z, surfaceZ) ? cave.floorMinZ : surfaceZ;
}
}
