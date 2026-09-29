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
