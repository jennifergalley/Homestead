#pragma once

#include "HomesteadSimulation.h"

#include <vector>

// The estate mine (openspec change open-estate-mine-shallow-level): a stone mine house on the surface
// with a trapdoor, and a dark tunnel level far below it that she mines with the pickaxe by lamplight.
// Everything here is plain C++17 with no Unreal types. The layout (house, trapdoor, ladder, node
// sockets, walkable floors) is published by the Mine Art Agent in HomesteadMineLayout.inc; until that
// lands the greybox HomesteadMineLayoutGreybox.inc stands in. World units are cm, yaw is degrees from
// +X toward +Y.
namespace Homestead::Mine
{
// Estate placement ids: 588000-588099 are the mine's fixed pieces, 588100-588199 are the gameplay lane's.
constexpr int FirstPlacementId = 588000;
constexpr int LastPlacementId = 588199;
constexpr int PileId = 588000;
constexpr int TrapdoorId = 588001;
constexpr int LadderFootId = 588002;

// The one-time pile upstairs.
constexpr int PileStone = 25;

// Underground rules, all in cm.
constexpr double ReachZCm = 220.0;                // Vertical reach to a node's socket.
constexpr double UndergroundCeilingAboveFloorCm = 1500.0; // Above this over the lowest floor she is "in the house".
constexpr double FootprintMarginCm = 600.0;       // Margin around the walkable floors that still counts as the mine.
constexpr double StuckSlackCm = 220.0;            // How far past a floor's edge before the stuck rule moves her.
constexpr double StuckBelowFloorCm = 300.0;       // Below the lowest floor by this much she has fallen out of the level.
// The mine house's outer footprint is 900 x 600 cm (yaw 0) about MINE_HOUSE; its level runs from just under the
// ground floor (boards sit 18 cm above the house z) up through the roof.
constexpr double HouseHalfLengthCm = 450.0;
constexpr double HouseHalfWidthCm = 300.0;
// The doorstep boards run past the walls over holed landscape, so the settle treats a margin round the footprint
// as the house too (the threshold at the east gable sat 10 cm outside the bare footprint and held a teleport 80 s).
constexpr double HouseSettleMarginCm = 120.0;
constexpr double HouseBelowCm = 100.0;
constexpr double HouseAboveCm = 600.0;
// Too tired to swing even the cheapest node (a worn-pickaxe coal seam costs 2.0 on top of the Reserve): she is
// "out of energy" for the mine, and wakes at the house (WakeAtMine).
constexpr double ExhaustedBelow = Exertion::Reserve + 2.0;
inline bool IsExhausted(double energy) { return energy < ExhaustedBelow; }

enum class NodeKind { Rock, Coal, Copper };

struct Spot
{
    double x = 0.0, y = 0.0, z = 0.0, yaw = 0.0;
};

struct Trapdoor
{
    double hingeX = 0.0, hingeY = 0.0, hingeZ = 0.0, yaw = 0.0;
    double lidLength = 0.0, lidWidth = 0.0;
};

// One mineable socket. The wall normal points out of the rock toward the gallery.
struct Node
{
    int id = 0;
    NodeKind kind = NodeKind::Rock;
    double x = 0.0, y = 0.0, z = 0.0, yaw = 0.0, normalX = 0.0, normalY = 0.0;
};

// A walkable stretch of tunnel: a centreline with a half-width, at floor height z.
struct FloorSegment
{
    Point a, b;
    double halfWidth = 0.0;
    double z = 0.0;
};

struct Layout
{
    Spot house;
    Spot door;
    Spot houseFloor;     // Where she wakes in the ground-floor room.
    Trapdoor trapdoor;
    Spot ladderTop;      // Where she stands to go down.
    Spot ladderFoot;     // Where she arrives below, and where the stuck rule returns her.
    Spot pile;
    Spot deskLamp;
    Spot daylight;       // The shaft of daylight at the ladder foot.
    std::vector<Node> nodes;
    std::vector<FloorSegment> floors;
};

const Layout& GetLayout();

ResourceKind ResourceFor(NodeKind kind);
bool IsMineNodeKind(ResourceKind kind);   // Rock face, coal seam or copper vein.
bool IsMinePlacementId(int id);
const Node* FindNode(int id);

// Swings, energy (at the worn tier) and the hours after the next 06:00 that a mined node stays bare.
struct NodeInfo
{
    int swings[ToolTierCount] = {};
    double energy = 0.0;
    int minYield = 0, maxYield = 0;
    double extraRefillHours = 0.0;
    int gemChancePercent = 0;   // Chance a mined node also gives one gem (quartz, rarely amethyst).
};
const NodeInfo& InfoFor(NodeKind kind);

// Whether the point at height z is in the tunnel level.
bool IsUnderground(Point p, double z);
// Whether the point is inside the mine house's footprint, at the house's own level (not the tunnels under it).
bool InHouse(Point p, double z);
// Distance beyond the nearest floor's edge (negative when on it), over all segments at z's level.
double OutsideFloorBy(Point p);
// The stuck rule: underground, below the level or off every floor by StuckSlackCm.
bool IsStuck(Point p, double z);
// Where the stuck rule and a ladder climb put her.
Spot StuckTarget();
}
