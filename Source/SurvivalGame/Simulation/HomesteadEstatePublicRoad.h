#pragma once

#include "HomesteadEstate.h"
#include "HomesteadSimulation.h"

#include <string>
#include <vector>

// The public road from the manor forecourt to town (Scripts/Terrain/public_road.py generates its data
// from estate_layout.json into HomesteadEstatePublicRoad.inc). Distances along it ("chainage", metres
// from the manor end) are what fast travel, the road signs and the roadside forage use, so a walk's
// length is measured along the road, not straight across the hills.
//
// World frame as HomesteadEstate.h: Unreal centimetres, +X north, +Y east.
namespace Homestead
{
struct PublicRoadStop
{
    std::string name;       // "Manor", "Gateway", "Town"
    double chainage = 0.0;  // metres from the manor end
    Point position;         // on the road's centreline (cm)
    double z = 0.0;         // ground height there (cm)
};

struct PublicRoadSign
{
    std::string name;       // "ManorRoadSign", "GatewayRoadSign", "TownRoadSign"
    double chainage = 0.0;
    Point position;         // on the verge, off the road bed (cm)
    double z = 0.0;
    double yaw = 0.0;       // degrees; the sign's face points along this heading
};

// The road bridge over the river (Scripts/Terrain/road_grade.py holds the road level across it; public_road.py
// measures the deck). The deck's walking surface is at deckZ, level along its length; it runs halfLength
// either way of centre along yaw and is 2 * halfWidth wide between its railings.
struct PublicRoadBridge
{
    bool valid = false;
    Point centre;               // where the road crosses the river (cm)
    double yaw = 0.0;           // along the road, toward town (degrees)
    double deckZ = 0.0;         // walking surface (cm)
    double halfLength = 0.0;    // cm
    double halfWidth = 0.0;     // clear, between the railings (cm)
    double waterZ = 0.0;        // the river's surface under it (cm)
    double bedZ = 0.0;          // the river bed under it (cm)
    // The deck's ends (cm), toward the manor and toward town.
    Point End(double side) const;
    // True over the walking slab (the deck's length plus DeckSlabOverrunCm each end, its clear width), grown by
    // marginCm all round.
    bool Covers(Point world, double marginCm = 0.0) const;
    // The nearest point on the walking slab at least insetCm inside its edges (the point itself if it's there).
    Point OntoDeck(Point world, double insetCm) const;
    // Where a downward probe for footing should start: at least clearanceCm above the walking surface where
    // the deck covers the point (grown by marginCm), else z. A save made at the old ford sits under the deck.
    double ProbeStartZ(Point world, double z, double clearanceCm, double marginCm) const;
    // What a dropped thing rests on: the walking surface where the deck covers it, else the ground.
    double RestZ(Point world, double groundZ) const;
    static constexpr double DeckSlabOverrunCm = 20.0;   // HomesteadWorldRoadBridge.cpp: slab 40 cm longer than the deck
};

struct PublicRoad
{
    std::vector<Point> points;       // centreline (cm)
    std::vector<double> groundZ;     // graded road-bed height at each point (cm)
    std::vector<double> chainage;    // metres from the manor end at each point
    double bridgeChainage = 0.0;     // where the road crosses the river (m)
    double bridgeHalfAlong = 0.0;    // keep-out half length along the road (m)
    double bridgeHalfAcross = 0.0;   // keep-out half width across it (m)
    std::vector<PublicRoadStop> stops;
    std::vector<PublicRoadSign> signs;
    PublicRoadBridge deck;

    double Length() const { return chainage.empty() ? 0.0 : chainage.back(); }
    // Point on the centreline at a chainage (clamped to the road's ends).
    Point At(double metres) const;
    // Nearest point: its chainage (m) and the distance from the centreline (cm).
    struct Nearest
    {
        double chainage = 0.0;
        double distanceCm = 0.0;
    };
    Nearest NearestTo(Point world) const;
    // Metres walked along the road between two chainages.
    double WalkMetres(double fromChainage, double toChainage) const;
    const PublicRoadStop* FindStop(const char* name) const;
    const PublicRoadSign* FindSign(const char* name) const;
    // True inside the bridge's deck-and-ramps keep-out.
    bool InBridgeKeepOut(Point world) const;
};

const PublicRoad& EstatePublicRoad();

// The public roadside exception to the estate boundary: forage she may pick on the verges of the road to
// town (ids 581000-581099, Scripts/Terrain/forage.py). True only for those ids, forage kinds, 3.2-12 m
// from the road's centreline and outside the bridge keep-out; every other placement stays on the estate.
constexpr int PublicRoadsideFirstId = 581000;
constexpr int PublicRoadsideEndId = 581100;
bool IsPublicRoadsidePlacement(const EstatePlacement& placement);
// Where a placement may stand: on the estate, or the narrow public-roadside exception.
bool EstatePlacementAllowed(const EstateLayout& layout, const EstatePlacement& placement);
}
