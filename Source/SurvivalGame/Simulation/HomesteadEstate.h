#pragma once

#include "HomesteadSimulation.h"

#include <string>
#include <vector>

// The fixed estate map's shared round-1 interface. The world lane owns the positions (authored in
// the Estate level's DA_EstateLandmarks); every other lane reads anchors by name only, so the
// coordinates may move freely as the terrain is reshaped.
//
// World frame: Unreal centimetres, +X north, +Y east, the map centred on the origin (a 4033 m
// Landscape spans about -201600..201600 on both axes). The sea lies to the south.
namespace Homestead
{
namespace Anchor
{
// Points (position, ground height, facing yaw).
constexpr const char* StandingRoomSpawn = "StandingRoomSpawn"; // Where she wakes; yaw faces the door.
constexpr const char* StandingRoomOrigin = "StandingRoomOrigin"; // Cell (0,0) of the heritage room; yaw is its grid heading.
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

// Rough v1 anchors so every lane can develop and test before the Estate level is authored. The
// Unreal game replaces these with the level's DA_EstateLandmarks when it loads the Estate map.
const EstateLayout& ProvisionalEstateLayout();

// Provisional interactive placements until the world lane bakes DA_EstatePlacements from the
// Estate level. Each lane appends its own section (ids: world 500000+, overgrowth 510000+,
// salvage 520000+, town 530000+).
const EstatePlacements& ProvisionalEstatePlacements();

// Even-odd point-in-polygon test for simple rings in world XY.
bool PointInPolygon(const std::vector<Point>& ring, Point point);
}
