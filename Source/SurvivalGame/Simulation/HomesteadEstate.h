#pragma once

#include "HomesteadSimulation.h"

#include <array>
#include <string>
#include <vector>

// The fixed estate map's shared round-1 interface. `estate_layout.json` is mirrored manually in
// ProvisionalEstateLayout(), which the controller currently installs for every Estate simulation.
// Every other lane reads anchors by name only, so keep that C++ table in step when terrain scripts
// move an anchor.
//
// World frame: Unreal centimetres, +X north, +Y east, the map centred on the origin (a 4033 m
// Landscape spans about -201600..201600 on both axes). The sea lies to the south.
namespace Homestead
{
namespace Anchor
{
// Points (position, ground height, facing yaw).
constexpr const char* StandingRoomSpawn = "StandingRoomSpawn"; // Where she wakes; yaw faces the door.
constexpr const char* StandingRoomOrigin = "StandingRoomOrigin"; // Centre of the heritage room; yaw is its grid heading.
constexpr const char* EstateGateway = "EstateGateway";
constexpr const char* CoveBeach = "CoveBeach";
constexpr const char* MineEntrance = "MineEntrance";
constexpr const char* MillSite = "MillSite";
constexpr const char* RoadEstateEnd = "RoadEstateEnd";
constexpr const char* RoadTownEnd = "RoadTownEnd";
constexpr const char* TownSquare = "TownSquare";
constexpr const char* GeneralStoreDoor = "GeneralStoreDoor"; // Outside the door; yaw faces into the shop.
constexpr const char* GeneralStoreCounter = "GeneralStoreCounter"; // Where the shopkeeper stands; yaw faces the customer.
// Polygons (world XY rings, not closed: the last point joins the first).
constexpr const char* EstateBoundary = "EstateBoundary";
constexpr const char* ManorFootprint = "ManorFootprint";
constexpr const char* ForSaleParcelPrefix = "ForSale."; // ForSale.Woodland, ForSale.MoorField, ...
// The derelict farm behind the manor: an axis-aligned field (a 4-point ring) and the gap of its
// broken field gate on the fence (yaw faces out of the field).
constexpr const char* DerelictFarm = "DerelictFarm";
constexpr const char* DerelictFarmGate = "DerelictFarmGate";
}

struct Landmark
{
    std::string name;
    Point position;
    double z = 0.0;
    double yaw = 0.0;
};

struct LandmarkPolygon
{
    std::string name;
    std::vector<Point> points;
};

struct EstateLayout
{
    int version = 1;
    std::vector<Landmark> landmarks;
    std::vector<LandmarkPolygon> polygons;

    const Landmark* FindLandmark(const char* name) const;
    const LandmarkPolygon* FindPolygon(const char* name) const;
    // Position of a named landmark, or `fallback` when the layout lacks it.
    Point PointOr(const char* name, Point fallback) const;
};

// One interactive, Simulation-tracked thing baked from the Estate level (trees, rocks, forage,
// overgrowth, salvage piles). `id` is stable across bakes of the same version and is the
// resource's identity in saves.
struct EstatePlacement
{
    int id = 0;
    ResourceKind kind = ResourceKind::Branches;
    Point position;
    double z = 0.0;
    double yaw = 0.0;
    double scale = 1.0;
    int minTier = 0; // Tool tier needed (0 = worn); see add-overgrown-estate-clearing.
};

struct EstatePlacements
{
    int bakeVersion = 1;
    std::vector<EstatePlacement> placements;
};

// The current Estate anchor source. The name remains provisional from the early development phase;
// AHomesteadController installs this table directly when preparing an Estate simulation.
const EstateLayout& ProvisionalEstateLayout();

// Provisional interactive placements until the world lane bakes DA_EstatePlacements from the
// Estate level. Each lane appends its own section. Id ranges (registry: docs/handoff/round-<n>.md):
// world 500000+, overgrowth 510000+, salvage 520000+, town 530000+ (reserved), berry brambles
// 540000-540043 (clearing), derelict farm and estate disrepair 550000+ (manor), MVP woodland biome 560000-569999 (scatter.py / mvp_woodland.py),
// clear-out near the manor 570000-579999 (clearing), field mushrooms 580000-580999 (seasons). Order matters: later sections yield to earlier
// ones (keep clear of what's already placed), so add sections in id order: berries, then the farm,
// then the clear-out. Farm-first skipped a quarter of the brambles and failed the simulation tests.
const EstatePlacements& ProvisionalEstatePlacements();

// The gap in the ruin's fallen front door on its south front, 10.5 m east of the ManorFootprint's
// west end: the way she walks out of the ruin.
Point EstateManorFrontDoor(const EstateLayout& layout = ProvisionalEstateLayout());
// The derelict farm's field in its own frame: u runs north from the field's south fence, v east
// from its west fence (cm). The Simulation's overgrowth and the Unreal set dressing both read it.
struct DerelictFarmPlan
{
    bool valid = false;
    Point southWest;        // world XY of the field's south-west fence corner
    double lengthU = 0.0;   // north-south fence length
    double lengthV = 0.0;   // east-west fence length
    double gateV = 0.0;     // centre of the gate gap along the south fence
    double gateWidth = 310.0;
    // Where the old crop ridges still show (u0, v0, u1, v1), rows running along u (downhill).
    std::vector<std::array<double, 4>> ridgeBlocks;
    Point plough; // (u, v) where the plough was left in the ridges
    Point World(double u, double v) const { return {southWest.x + u, southWest.y + v}; }
};
DerelictFarmPlan EstateDerelictFarm(const EstateLayout& layout = ProvisionalEstateLayout());
// Appends the derelict farm's and the grounds' clearable overgrowth (550000+) to a placement table.
void AppendDerelictFarmAndDisrepair(EstatePlacements& table);
// Even-odd point-in-polygon test for simple rings in world XY.
bool PointInPolygon(const std::vector<Point>& ring, Point point);
}
