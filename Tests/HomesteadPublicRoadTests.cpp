// Native tests for the public road (Simulation/HomesteadEstatePublicRoad).
#include "../Source/SurvivalGame/Simulation/HomesteadEstate.h"
#include "../Source/SurvivalGame/Simulation/HomesteadEstatePublicRoad.h"
#include "../Source/SurvivalGame/Simulation/HomesteadSimulation.h"
#include "../Source/SurvivalGame/Simulation/HomesteadTravel.h"

#include <algorithm>
#include <string>
#include <string>
#include <vector>

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace Homestead;

namespace
{
int Failures = 0;
void Check(bool ok, const char* what, double value = 0.0)
{
    if (!ok)
    {
        std::printf("FAIL: %s (%g)\n", what, value);
        ++Failures;
    }
}
double Distance(Point a, Point b) { return std::hypot(a.x - b.x, a.y - b.y); }
double PolylineDistance(const std::vector<Point>& line, Point p, bool closed = false)
{
    double best = 1e300;
    const size_t n = line.size();
    for (size_t i = 0; i + (closed ? 0 : 1) < n; ++i)
    {
        const Point a = line[i], b = line[(i + 1) % n];
        const double dx = b.x - a.x, dy = b.y - a.y, len2 = dx * dx + dy * dy;
        const double t = len2 > 0.0 ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / len2, 0.0, 1.0) : 0.0;
        best = std::min(best, Distance(p, {a.x + t * dx, a.y + t * dy}));
    }
    return best;
}
bool NearBox(const std::vector<Point>& ring, Point p, double marginCm)
{
    double x0 = 1e300, x1 = -1e300, y0 = 1e300, y1 = -1e300;
    for (const Point& q : ring)
    {
        x0 = std::min(x0, q.x); x1 = std::max(x1, q.x);
        y0 = std::min(y0, q.y); y1 = std::max(y1, q.y);
    }
    return p.x > x0 - marginCm && p.x < x1 + marginCm && p.y > y0 - marginCm && p.y < y1 + marginCm;
}
bool IsFood(ResourceKind kind) { return kind == ResourceKind::BerryBush || kind == ResourceKind::Roots; }
// forage.py's abundance passes (Jenny's playtest, 2026-09-29): estate rows from here up, roadside infill from here up.
constexpr int MoreEstateFoodFirstId = 582144;
constexpr int MoreRoadsideFoodFirstId = 581015;
bool IsMoreFood(int id)
{
    return (id >= MoreEstateFoodFirstId && id < 582300) || (id >= MoreRoadsideFoodFirstId && id < PublicRoadsideEndId);
}
}

int main()
{
    const PublicRoad& road = EstatePublicRoad();
    Check(road.points.size() == 214, "214 centreline points", static_cast<double>(road.points.size()));
    Check(std::abs(road.Length() - 852.0) < 1.0, "0.85 km", road.Length());
    for (size_t i = 1; i < road.chainage.size(); ++i)
        Check(road.chainage[i] > road.chainage[i - 1], "chainage increases", static_cast<double>(i));

    // At and NearestTo agree along the whole road.
    for (double metres = 0.0; metres <= road.Length(); metres += 37.0)
    {
        const PublicRoad::Nearest nearest = road.NearestTo(road.At(metres));
        Check(std::abs(nearest.chainage - metres) < 0.5 && nearest.distanceCm < 1.0, "At/NearestTo round trip", metres);
    }

    // The endpoints: on the road bed, in order, and where the layout's anchors are.
    const PublicRoadStop* manor = road.FindStop("Manor");
    const PublicRoadStop* gateway = road.FindStop("Gateway");
    const PublicRoadStop* town = road.FindStop("Town");
    Check(manor && gateway && town, "three stops");
    if (manor && gateway && town)
    {
        Check(manor->chainage < town->chainage && town->chainage < gateway->chainage, "stops in order (the village junction comes before the gateway)");
        Check(road.NearestTo(manor->position).distanceCm < 1.0 && road.NearestTo(town->position).distanceCm < 1.0, "stops on the road");
        const EstateLayout& layout = ProvisionalEstateLayout();
        const Landmark* gate = layout.FindLandmark(Anchor::EstateGateway);
        const Landmark* townEnd = layout.FindLandmark(Anchor::RoadTownEnd);
        Check(gate && Distance(gateway->position, gate->position) < 1500.0, "gateway stop at the estate gateway",
            gate ? Distance(gateway->position, gate->position) : -1.0);
        Check(townEnd && Distance(town->position, townEnd->position) < 1000.0, "town stop at the road's town end",
            townEnd ? Distance(town->position, townEnd->position) : -1.0);
        Check(std::abs(road.WalkMetres(manor->chainage, town->chainage) - 248.0) < 1.0, "manor to town walk",
            road.WalkMetres(manor->chainage, town->chainage));
        Check(road.WalkMetres(town->chainage, manor->chainage) == road.WalkMetres(manor->chainage, town->chainage), "walk is symmetric");
        Check(manor->z > 0.0 && town->z > 0.0, "stop heights above the sea");
    }

    // The road follows the ground (Scripts/Terrain/road_grade.py): never steeper than 1 in 5 between its
    // 4 m points, and no long straight earthwork (the old 1 in 9 clamp ran 250-530 m at one exact grade).
    {
        double steepest = 0.0;
        int straightRun = 0, longestStraight = 0;
        for (size_t i = 1; i < road.points.size(); ++i)
        {
            const double mid = (road.chainage[i] + road.chainage[i - 1]) * 0.5;
            const double grade = (road.groundZ[i] - road.groundZ[i - 1]) / 100.0 / (road.chainage[i] - road.chainage[i - 1]);
            if (std::abs(mid - road.bridgeChainage) > road.bridgeHalfAlong) steepest = std::max(steepest, std::abs(grade));
            const double previous = i > 1 ? (road.groundZ[i - 1] - road.groundZ[i - 2]) / 100.0 / (road.chainage[i - 1] - road.chainage[i - 2]) : 1e9;
            straightRun = std::abs(grade) > 0.03 && std::abs(grade - previous) < 0.002 ? straightRun + 1 : 0;
            longestStraight = std::max(longestStraight, straightRun);
        }
        Check(steepest <= 0.2005, "road no steeper than 1 in 5", steepest);
        Check(longestStraight < 20, "no long straight-graded earthwork", longestStraight);
        // Over the river it holds one level (the bridge deck) either side of the crossing.
        double lowest = 1e300, highest = -1e300;
        for (size_t i = 0; i < road.points.size(); ++i)
            if (std::abs(road.chainage[i] - road.bridgeChainage) <= 5.0)
            {
                lowest = std::min(lowest, road.groundZ[i]);
                highest = std::max(highest, road.groundZ[i]);
            }
        Check(highest - lowest < 1.0, "the road is level over the river", highest - lowest);
    }

    // The road bridge: over the river where the road crosses it, on the road's line and level, clear of
    // the water, spanning the channel into the keep-out's ramps, a cart's width between its railings.
    {
        const PublicRoadBridge& deck = road.deck;
        Check(deck.valid, "the road has a bridge");
        const PublicRoad::Nearest onRoad = road.NearestTo(deck.centre);
        Check(onRoad.distanceCm < 50.0 && std::abs(onRoad.chainage - road.bridgeChainage) < 3.0, "bridge on the road at the crossing",
            onRoad.distanceCm);
        const Point ahead = road.At(onRoad.chainage + 2.0), behind = road.At(onRoad.chainage - 2.0);
        const double roadYaw = std::atan2(ahead.y - behind.y, ahead.x - behind.x) * 180.0 / 3.14159265358979323846;
        const double turn = std::remainder(deck.yaw - roadYaw, 360.0);
        Check(std::abs(turn) < 5.0, "bridge along the road toward town", turn);
        Check(deck.deckZ - deck.waterZ >= 80.0, "deck clear of the water", deck.deckZ - deck.waterZ);
        Check(deck.waterZ > deck.bedZ, "water over the bed under the bridge", deck.waterZ - deck.bedZ);
        Check(deck.halfLength >= 400.0 && deck.halfLength <= road.bridgeHalfAlong * 100.0 - 200.0, "deck spans the channel inside the keep-out",
            deck.halfLength);
        Check(deck.halfWidth >= 160.0 && deck.halfWidth <= road.bridgeHalfAcross * 100.0, "a cart's width", deck.halfWidth);
        for (const double side : {-1.0, 1.0})
        {
            const Point end = deck.End(side);
            Check(road.InBridgeKeepOut(end), "deck end in the keep-out");
            const PublicRoad::Nearest at = road.NearestTo(end);
            Check(at.distanceCm < 60.0, "deck end on the road's line", at.distanceCm);
            // The road meets the deck: its graded level at each end is within a small step of the deck.
            const auto upper = std::upper_bound(road.chainage.begin(), road.chainage.end(), at.chainage);
            const size_t i = std::clamp<size_t>(static_cast<size_t>(upper - road.chainage.begin()), 1, road.points.size() - 1);
            const double t = (at.chainage - road.chainage[i - 1]) / (road.chainage[i] - road.chainage[i - 1]);
            const double level = road.groundZ[i - 1] + (road.groundZ[i] - road.groundZ[i - 1]) * t;
            Check(std::abs(level - deck.deckZ) < 25.0, "road meets the deck without a step", level - deck.deckZ);
        }
        std::printf("bridge: %.1f m long, %.1f m wide, deck %.2f m, %.2f m over the water\n", deck.halfLength / 50.0,
            deck.halfWidth / 50.0, deck.deckZ / 100.0, (deck.deckZ - deck.waterZ) / 100.0);

        // Saves from the old ford (review, 2026-09-30): a position 1.7 m either way of the channel's centre,
        // at or below the deck, settles from above the deck's walking slab; dropped things rest on its
        // planks, not on the river bed; away from the bridge nothing changes.
        constexpr double HalfHeightCm = 90.0, RadiusCm = 34.0, MarginCm = 30.0;
        const double radians = deck.yaw * 3.14159265358979323846 / 180.0;
        for (const double along : {-170.0, 0.0, 170.0})
            for (const double across : {-120.0, 0.0, 120.0, deck.halfWidth + 40.0})
            {
                const Point at{deck.centre.x + std::cos(radians) * along - std::sin(radians) * across,
                               deck.centre.y + std::sin(radians) * along + std::cos(radians) * across};
                Check(deck.Covers(at, RadiusCm + MarginCm), "the ford's saves are over the deck", along);
                const Point on = deck.OntoDeck(at, RadiusCm + 5.0);
                Check(deck.Covers(on) && Distance(on, at) <= std::max(0.0, std::abs(across) - (deck.halfWidth - RadiusCm - 5.0)) + 0.01,
                      "settles onto the slab, moved only across", Distance(on, at));
                for (const double savedZ : {deck.bedZ + 100.0, deck.deckZ - 25.0, deck.deckZ - 30.0 + HalfHeightCm})
                    Check(deck.ProbeStartZ(on, savedZ, HalfHeightCm + MarginCm, 0.0) >= deck.deckZ + HalfHeightCm,
                          "the settle probe starts above the slab", savedZ);
                if (std::abs(across) <= deck.halfWidth)
                    Check(deck.RestZ(at, deck.bedZ) == deck.deckZ, "a drop on the bridge rests on its planks", along);
            }
        const Point away = road.At(road.bridgeChainage + road.bridgeHalfAlong + 30.0);
        Check(!deck.Covers(away, RadiusCm + MarginCm), "off the bridge is not over it");
        Check(deck.ProbeStartZ(away, 1234.0, HalfHeightCm + MarginCm, 0.0) == 1234.0, "off the bridge the probe is unchanged");
        Check(deck.RestZ(away, 1234.0) == 1234.0, "off the bridge a drop rests on the ground");
    }

    // The town (Scripts/Terrain/town_layout.py; Jenny, 2026-09-29: "bunched too tightly"): an open 60 x 45 m
    // square, buildings either sharing a party wall or 3 m or more apart, a 5.5 m street from the main road's
    // end into the square, and the general store's door and counter where the layout's anchors put them.
    {
        struct Footprint { std::string name; std::vector<Point> corners; };
        std::vector<Footprint> footprints;
        std::vector<Point> street;
        Point square{};
        double halfX = 0.0, halfY = 0.0, streetHalf = 0.0;
        auto townSquare = [&](double x, double y, double hx, double hy) { square = {x * 100.0, y * 100.0}; halfX = hx * 100.0; halfY = hy * 100.0; };
        auto streetHalfWidth = [&](double metres) { streetHalf = metres * 100.0; };
        auto streetPoint = [&](double x, double y) { street.push_back({x * 100.0, y * 100.0}); };
        auto footprint = [&](const char* name, double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3) {
            footprints.push_back({name, {{x0 * 100.0, y0 * 100.0}, {x1 * 100.0, y1 * 100.0}, {x2 * 100.0, y2 * 100.0}, {x3 * 100.0, y3 * 100.0}}});
        };
#define street streetPoint
#include "Data/HomesteadTownLayout.inc"
#undef street
        const EstateLayout& townLayout = ProvisionalEstateLayout();
        Check(footprints.size() >= 13 && footprints[0].name == "GeneralStore", "town footprints, the store first", static_cast<double>(footprints.size()));
        Check(std::abs(2.0 * halfY - 6000.0) < 1.0 && std::abs(2.0 * halfX - 4500.0) < 1.0, "a 60 x 45 m square");
        const Landmark* squareAnchor = townLayout.FindLandmark(Anchor::TownSquare);
        Check(squareAnchor && Distance(squareAnchor->position, square) < 1.0, "the square round the TownSquare anchor");
        auto inSquare = [&](Point p, double margin) { return std::abs(p.x - square.x) < halfX - margin && std::abs(p.y - square.y) < halfY - margin; };
        for (const Footprint& building : footprints)
        {
            for (const Point& corner : building.corners) Check(!inSquare(corner, 1.0), "a building stands in the square");
        }
        // Nothing bunched: every pair shares a party wall (<= 15 cm) or leaves a 3 m lane or more.
        for (size_t i = 0; i < footprints.size(); ++i)
            for (size_t j = i + 1; j < footprints.size(); ++j)
            {
                double nearest = 1e300;
                for (size_t a = 0; a < 4; ++a)
                    for (size_t b = 0; b < 4; ++b)
                    {
                        nearest = std::min(nearest, PolylineDistance({footprints[j].corners[b], footprints[j].corners[(b + 1) % 4]}, footprints[i].corners[a]));
                        nearest = std::min(nearest, PolylineDistance({footprints[i].corners[a], footprints[i].corners[(a + 1) % 4]}, footprints[j].corners[b]));
                    }
                Check(nearest <= 15.0 || nearest >= 300.0, "buildings bunched (0.15-3 m apart)", nearest);
            }
        // The street: from the main road's last point into the square, clear of every wall.
        const PublicRoadStop* junction = road.FindStop("Town");
        Check(street.size() > 20 && junction && road.NearestTo(street.front()).distanceCm < 100.0, "the street leaves the main road at the village junction", road.NearestTo(street.front()).distanceCm);
        Check(inSquare(street.back(), 50.0), "the street ends in the square");
        for (const Point& p : street)
            for (const Footprint& building : footprints)
            {
                std::vector<Point> ring = building.corners;
                const bool inside = PointInPolygon(ring, p);
                Check(!inside && PolylineDistance(ring, p, true) > streetHalf + 70.0, "the street passes a wall", PolylineDistance(ring, p, true));
            }
        // The store: its door on the square's edge facing in, the counter 6 m inside its footprint, and a clear
        // walk from where the street arrives to the door.
        const Landmark* door = townLayout.FindLandmark(Anchor::GeneralStoreDoor);
        const Landmark* counter = townLayout.FindLandmark(Anchor::GeneralStoreCounter);
        Check(door && counter, "store anchors");
        if (door && counter)
        {
            Check(std::abs(std::abs(door->position.y - square.y) - halfY) < 100.0 && inSquare({door->position.x, square.y}, 0.0), "store door on the square's edge");
            Check(std::abs(Distance(door->position, counter->position) - 600.0) < 1.0, "counter 6 m in from the door");
            Check(PointInPolygon(footprints[0].corners, counter->position), "counter inside the store");
            Check(!PointInPolygon(footprints[0].corners, door->position), "door anchor outside the store");
            const double facing = std::atan2(counter->position.y - door->position.y, counter->position.x - door->position.x) * 180.0 / 3.14159265358979323846;
            Check(std::abs(std::remainder(door->yaw - facing, 360.0)) < 1.0 && std::abs(std::remainder(counter->yaw - facing - 180.0, 360.0)) < 1.0,
                "door faces the counter and the counter the customer");
            for (int step = 0; step <= 50; ++step)
            {
                const double t = step / 50.0;
                const Point p{street.back().x + (door->position.x - street.back().x) * t, street.back().y + (door->position.y - street.back().y) * t};
                for (const Footprint& building : footprints) Check(!PointInPolygon(building.corners, p), "a building between the street and the store door");
            }
        }
        std::printf("town: %zu buildings and the store round a %.0f x %.0f m square, street %.0f m\n", footprints.size() - 1,
            2.0 * halfY / 100.0, 2.0 * halfX / 100.0, static_cast<double>(street.size()));
    }

    // Every sign stands on the verge; the existing town-bound sign is past the derelict farm, not the manor.
    Check(road.signs.size() == 3, "three signs", static_cast<double>(road.signs.size()));
    for (const PublicRoadSign& sign : road.signs)
    {
        const double off = road.NearestTo(sign.position).distanceCm;
        Check(off > 320.0 && off < 600.0, "sign on the verge", off);
        Check(!road.InBridgeKeepOut(sign.position), "sign clear of the bridge");
        Check(sign.yaw >= 0.0 && sign.yaw < 360.0, "sign yaw normalised", sign.yaw);
    }
    const PublicRoadSign* farmSign = road.FindSign("ManorRoadSign");
    const LandmarkPolygon* farm = ProvisionalEstateLayout().FindPolygon(Anchor::DerelictFarm);
    Check(farmSign && farm, "original sign and derelict farm identities retained");
    if (farmSign && farm)
    {
        double farmEnd = 0.0;
        for (Point corner : farm->points) farmEnd = std::max(farmEnd, road.NearestTo(corner).chainage);
        Check(farmSign->chainage > farmEnd + 10.0 && farmSign->chainage < farmEnd + 30.0, "sign beyond the farm toward town", farmSign->chainage - farmEnd);
        Check(!PointInPolygon(farm->points, farmSign->position), "sign outside the farm");
        const Point beforeSign = road.At(farmSign->chainage - 1.0), afterSign = road.At(farmSign->chainage + 1.0);
        const double heading = std::atan2(afterSign.y - beforeSign.y, afterSign.x - beforeSign.x) * 180.0 / 3.14159265358979323846;
        Check(std::abs(std::remainder(farmSign->yaw + 90.0 - heading, 360.0)) < 1.0, "local +Y sign arm points along road toward town");
        for (const PublicRoadSign& sign : road.signs)
            Check(Distance(sign.position, EstateManorFrontDoor()) > 5000.0, "no sign outside the manor", Distance(sign.position, EstateManorFrontDoor()));
    }

    // The bridge keep-out covers the crossing and its ramps and nothing far off it.
    const Point crossing = road.At(road.bridgeChainage);
    Check(road.InBridgeKeepOut(crossing), "crossing in the keep-out");
    Check(road.InBridgeKeepOut(road.At(road.bridgeChainage + road.bridgeHalfAlong - 1.0)), "ramp end in the keep-out");
    Check(!road.InBridgeKeepOut(road.At(road.bridgeChainage + road.bridgeHalfAlong + 5.0)), "past the ramp is clear");
    Check(road.bridgeChainage > 600.0 && road.bridgeChainage < 760.0, "bridge between the manor and the gateway", road.bridgeChainage);

    // The roadside exception: only its ids and forage kinds, on the verge, clear of the bridge.
    const EstateLayout& estateLayout = ProvisionalEstateLayout();
    const Point verge = road.At(835.0);
    const PublicRoad::Nearest at1500 = road.NearestTo(verge);
    (void)at1500;
    auto offset = [&](double metres, double cm) {
        const Point a = road.At(metres - 1.0), b = road.At(metres + 1.0);
        const double dx = b.x - a.x, dy = b.y - a.y, n = std::hypot(dx, dy);
        const Point c = road.At(metres);
        return Point{c.x - dy / n * cm, c.y + dx / n * cm};
    };
    const EstatePlacement roadside{581050, ResourceKind::BerryBush, offset(835.0, 450.0), 0, 0, 1, 0};
    Check(IsPublicRoadsidePlacement(roadside) && EstatePlacementAllowed(estateLayout, roadside), "roadside bramble allowed");
    EstatePlacement wrong = roadside;
    wrong.id = 500123;
    Check(!EstatePlacementAllowed(estateLayout, wrong), "ordinary id off the estate rejected");
    wrong = roadside;
    wrong.position = offset(835.0, 20000.0);
    Check(!IsPublicRoadsidePlacement(wrong), "reserved id 200 m off the road rejected");
    wrong = roadside;
    wrong.position = offset(835.0, 150.0);
    Check(!IsPublicRoadsidePlacement(wrong), "reserved id on the road bed rejected");
    wrong = roadside;
    wrong.position = offset(road.bridgeChainage, 450.0);
    Check(!IsPublicRoadsidePlacement(wrong), "reserved id in the bridge keep-out rejected");
    wrong = roadside;
    wrong.kind = ResourceKind::ForestTree;
    Check(!IsPublicRoadsidePlacement(wrong), "reserved id of a disallowed kind rejected");

    // The baked table: new forage is there, spread out, and reaches the far end of the road.
    int estateBrambles = 0, roadsideNodes = 0, roadsideBrambles = 0;
    double farthest = 0.0;
    std::vector<Point> brambles, added;
    for (const EstatePlacement& placement : ProvisionalEstatePlacements().placements)
    {
        if (placement.kind == ResourceKind::BerryBush) brambles.push_back(placement.position);
        if (placement.id >= 582100 && placement.id < 582300 && placement.kind == ResourceKind::BerryBush) ++estateBrambles;
        if ((placement.id >= 582100 && placement.id < 582300) || (placement.id >= PublicRoadsideFirstId && placement.id < PublicRoadsideEndId))
            added.push_back(placement.position);
        if (placement.id >= PublicRoadsideFirstId && placement.id < PublicRoadsideEndId)
        {
            ++roadsideNodes;
            roadsideBrambles += placement.kind == ResourceKind::BerryBush;
            farthest = std::max(farthest, road.NearestTo(placement.position).chainage);
            Check(EstatePlacementAllowed(estateLayout, placement), "roadside row allowed", placement.id);
        }
    }
    Check(estateBrambles >= 25, "at least 25 more estate brambles", estateBrambles);
    Check(roadsideNodes >= 4 && roadsideBrambles >= 1, "roadside forage stops", roadsideNodes);
    Check(farthest > 800.0, "roadside forage reaches the road's far end", farthest);
    Check(brambles.size() >= 125, "about 130 pickable brambles in all", static_cast<double>(brambles.size()));
    // The new rows keep 3 m from every placement (the table's skip rule) and 8 m from each other.
    for (size_t i = 0; i < added.size(); ++i)
    {
        for (size_t j = i + 1; j < added.size(); ++j)
            Check(Distance(added[i], added[j]) >= 800.0, "new forage at least 8 m apart", static_cast<double>(i));
        int crowded = 0;
        for (const EstatePlacement& other : ProvisionalEstatePlacements().placements)
            crowded += Distance(other.position, added[i]) < 300.0;
        Check(crowded == 1, "new forage 3 m clear of other placements", static_cast<double>(i));
    }
    // Coverage: a pickable bramble within 120 m of the road at every 400 m mark (0-850 m).
    for (double metres = 0.0; metres <= road.Length(); metres += 400.0)
    {
        double nearest = 1e300;
        for (const Point& b : brambles) nearest = std::min(nearest, Distance(b, road.At(metres)));
        Check(nearest < 12000.0, "a bramble near the road at this mark", metres);
    }

    // Gathering off the estate: a roadside bramble well outside the boundary spawns, is picked, saves,
    // stays picked after a reload and ripens again after its 36 hours.
    {
        const EstatePlacement* far = nullptr;
        for (const EstatePlacement& placement : ProvisionalEstatePlacements().placements)
            if (placement.id >= PublicRoadsideFirstId && placement.id < PublicRoadsideEndId && placement.kind == ResourceKind::BerryBush
                && !PointInPolygon(estateLayout.FindPolygon(Anchor::EstateBoundary)->points, placement.position)
                && road.NearestTo(placement.position).chainage > 800.0)
                far = &placement;
        Check(far != nullptr, "a roadside bramble off the estate");
        if (far)
        {
            Simulation sim;
            Check(sim.NewEstateGame(estateLayout, ProvisionalEstatePlacements()).ok, "new estate game");
            Check(sim.CanHarvest(far->id), "roadside bramble ready at the start");
            const int before = sim.Count(Item::Berries);
            Check(sim.Harvest(far->id, far->position).ok, "picked the roadside bramble");
            Check(sim.Count(Item::Berries) > before, "berries in the pack", sim.Count(Item::Berries));
            const std::string saved = sim.Serialize();
            Simulation loaded;
            loaded.SetLayout(estateLayout);
            loaded.SetPlacements(ProvisionalEstatePlacements());
            Check(loaded.Deserialize(saved).ok, "reloaded");
            Check(!loaded.CanHarvest(far->id), "still picked after the reload");
            loaded.AdvanceGameHours(37.0, far->position);
            Check(loaded.CanHarvest(far->id), "ripe again after 36 hours");
        }
    }

    // Wild roots (a cooked meal) in the woods round the manor: some within reach on day one, all at least
    // 12 m apart, and an old save made before they existed loads them fresh and ready.
    {
        const Point home = estateLayout.PointOr(Anchor::StandingRoomOrigin, {});
        int roots = 0, near = 0;
        const EstatePlacement* firstRoot = nullptr;
        for (const EstatePlacement& placement : ProvisionalEstatePlacements().placements)
            if (placement.id >= 582100 && placement.id < 582300 && placement.kind == ResourceKind::Roots)
            {
                ++roots;
                near += Distance(placement.position, home) < 11000.0;
                if (!firstRoot) firstRoot = &placement;
            }
        Check(roots >= 12, "wild roots in the woods round the manor", roots);
        Check(near >= 2, "wild roots within 110 m of the standing room", near);
        EstatePlacements old;
        old.bakeVersion = ProvisionalEstatePlacements().bakeVersion;
        for (const EstatePlacement& placement : ProvisionalEstatePlacements().placements)
            if (!(placement.id >= 581000 && placement.id < 581100) && !(placement.id >= 582100 && placement.id < 582300))
                old.placements.push_back(placement);
        Simulation before;
        Check(before.NewEstateGame(estateLayout, old).ok, "old-table game");
        const std::string saved = before.Serialize();
        Simulation after;
        after.SetLayout(estateLayout);
        after.SetPlacements(ProvisionalEstatePlacements());
        Check(after.Deserialize(saved).ok, "old save loads with the new forage");
        if (firstRoot)
        {
            Check(after.CanHarvest(firstRoot->id), "new roots ready in an old save");
            const int rootsBefore = after.Count(Item::Roots);
            Check(after.Harvest(firstRoot->id, firstRoot->position).ok, "dug the new roots");
            Check(after.Count(Item::Roots) > rootsBefore, "roots in the pack", after.Count(Item::Roots));
            // They grow back in 48 hours (her hunger would run out first if the test just waited it out).
            for (const auto& node : after.GetState().resources)
                if (node.id == firstRoot->id)
                    Check(std::abs(node.readyAtHour - after.GetState().hour - 48.0) < 0.01, "roots grow back after 48 hours", node.readyAtHour - after.GetState().hour);
        }
        // Every node the old save knew is still there, at the same place.
        for (const EstatePlacement& placement : old.placements)
        {
            bool found = false;
            for (const EstatePlacement& now : ProvisionalEstatePlacements().placements)
                if (now.id == placement.id && now.kind == placement.kind && Distance(now.position, placement.position) < 0.01) found = true;
            Check(found, "an old placement moved or vanished", placement.id);
        }
        std::printf("wild roots: %d new, %d within 110 m of the standing room\n", roots, near);
    }

    // More food (Jenny's playtest, 2026-09-29: "abundant live berry bushes"): live brambles and root patches
    // fill the estate's empty woods and fields and the road between the first stops. Counts by zone, coverage,
    // and every new row on open ground clear of the water, the lake trail, the road bed, the bridge, the ruin
    // and the farm.
    {
        std::vector<Point> riverPts, shorePts, pathPts;
        auto river = [&](double x, double y) { riverPts.push_back({x, y}); };
        auto lakeShore = [&](double x, double y) { shorePts.push_back({x, y}); };
        auto lakePath = [&](double x, double y) { pathPts.push_back({x, y}); };
#include "Data/HomesteadForageKeepOuts.inc"
        Check(riverPts.size() > 100 && shorePts.size() > 10 && pathPts.size() >= 2, "keep-out data", static_cast<double>(riverPts.size()));

        const std::vector<Point>& boundary = estateLayout.FindPolygon(Anchor::EstateBoundary)->points;
        const std::vector<Point>& manorRing = estateLayout.FindPolygon(Anchor::ManorFootprint)->points;
        const LandmarkPolygon* farm = estateLayout.FindPolygon(Anchor::DerelictFarm);
        const Point home = estateLayout.PointOr(Anchor::StandingRoomOrigin, {});
        const std::vector<EstatePlacement>& all = ProvisionalEstatePlacements().placements;

        int moreBrambles = 0, moreRoots = 0, moreRoadside = 0, moreRoadsideRoots = 0;
        int manorGrounds = 0, nearWoods = 0, farEstate = 0, alongRoad = 0;
        std::vector<Point> food;
        for (const EstatePlacement& placement : all)
        {
            if (!IsFood(placement.kind)) continue;
            food.push_back(placement.position);
            const bool onRoadside = placement.id >= PublicRoadsideFirstId && placement.id < PublicRoadsideEndId;
            if (onRoadside) ++alongRoad;
            else if (PointInPolygon(boundary, placement.position))
            {
                const double fromHome = Distance(placement.position, home);
                (fromHome < 15000.0 ? manorGrounds : fromHome < 45000.0 ? nearWoods : farEstate) += 1;
            }
            if (!IsMoreFood(placement.id)) continue;
            if (onRoadside)
            {
                ++moreRoadside;
                moreRoadsideRoots += placement.kind == ResourceKind::Roots;
            }
            else (placement.kind == ResourceKind::BerryBush ? moreBrambles : moreRoots) += 1;

            const Point p = placement.position;
            Check(EstatePlacementAllowed(estateLayout, placement), "new food row allowed", placement.id);
            Check(PolylineDistance(riverPts, p) > 1190.0, "new food 12 m off the river", placement.id);
            Check(!PointInPolygon(shorePts, p) && PolylineDistance(shorePts, p, true) > 100.0, "new food out of the lake", placement.id);
            Check(PolylineDistance(pathPts, p) > 160.0, "new food off the lake trail", placement.id);
            Check(!road.InBridgeKeepOut(p), "new food clear of the bridge", placement.id);
            int crowded = 0;
            for (const EstatePlacement& other : all)
                crowded += other.id != placement.id && Distance(other.position, p) < 400.0;
            Check(crowded == 0, "new food on open ground, 4 m from every other placement", placement.id);
            for (const EstatePlacement& other : all)
                if (other.id != placement.id && IsFood(other.kind))
                    Check(Distance(other.position, p) >= 2000.0, "new food 20 m from other food", placement.id);
            if (!onRoadside)
            {
                Check(PointInPolygon(boundary, p) && PolylineDistance(boundary, p, true) > 590.0, "new estate food inside the boundary", placement.id);
                Check(road.NearestTo(p).distanceCm > 590.0, "new estate food off the road", placement.id);
                Check(PolylineDistance(pathPts, p) > 390.0, "new estate food 4 m off the lake trail", placement.id);
                Check(!NearBox(manorRing, p, 3000.0), "new food clear of the ruin's grounds", placement.id);
                Check(!farm || !NearBox(farm->points, p, 800.0), "new food clear of the farm and its plots", placement.id);
            }
        }
        // The lake trail's forage (582300-582399, lake_path_plants.py; Jenny, 2026-09-30): every row placed, live
        // brambles and roots in the woods either side of the trail, beside it (not on its walk width) and deeper
        // in, out of the lake and clear of every other placement.
        {
            int lakeRows = 0, lakeBrambles = 0, lakeRoots = 0, besidePath = 0, deeper = 0, left = 0, right = 0;
            for (const EstatePlacement& placement : all)
            {
                if (placement.id < 582300 || placement.id >= 582400) continue;
                ++lakeRows;
                (placement.kind == ResourceKind::BerryBush ? lakeBrambles : lakeRoots) += 1;
                Check(IsFood(placement.kind), "lake-trail forage is a bramble or wild roots", placement.id);
                const Point p = placement.position;
                const double fromPath = PolylineDistance(pathPts, p);
                Check(fromPath >= 300.0, "lake-trail forage off the walk width", placement.id);
                Check(fromPath <= 3100.0, "lake-trail forage in the woods along the trail", placement.id);
                Check(!PointInPolygon(shorePts, p) && PolylineDistance(shorePts, p, true) > 290.0, "lake-trail forage out of the lake", placement.id);
                Check(EstatePlacementAllowed(estateLayout, placement), "lake-trail forage allowed", placement.id);
                for (const EstatePlacement& other : all)
                    if (other.id != placement.id) Check(Distance(other.position, p) >= 300.0, "lake-trail forage 3 m from every placement", placement.id);
                besidePath += fromPath <= 950.0;
                deeper += fromPath >= 1200.0;
                // Which side: the cross product with the trail's overall direction (farm to landing).
                const Point a = pathPts.front(), b = pathPts.back();
                ((b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x) > 0.0 ? left : right) += 1;
            }
            Check(lakeRows >= 20, "lake-trail forage rows all placed", lakeRows);
            Check(lakeBrambles >= 8 && lakeRoots >= 8, "lake-trail brambles and roots", lakeBrambles);
            Check(besidePath >= 10 && deeper >= 4, "lake-trail forage beside the path and deeper in", besidePath);
            Check(left >= 6 && right >= 6, "lake-trail forage on both sides", left);
        }
        Check(moreBrambles >= 45, "more estate brambles", moreBrambles);
        Check(moreRoots >= 22, "more estate root patches", moreRoots);
        (void)moreRoadsideRoots;
        // By zone, across every section of the table (the manor's berries, the MVP wood, the forage passes).
        Check(manorGrounds >= 38, "food round the manor (under 150 m)", manorGrounds);
        Check(nearWoods >= 180, "food in the near woods and fields (150-450 m)", nearWoods);
        Check(farEstate >= 55, "food in the far woods and fields (over 450 m)", farEstate);
        Check(alongRoad >= 1, "food along the public road", alongRoad);
        // Coverage: from almost anywhere on the estate, food within 150 m (it was 85% before this pass).
        int cells = 0, covered = 0;
        for (double x = -76000.0; x <= 16000.0; x += 2500.0)
            for (double y = -115000.0; y <= 11000.0; y += 2500.0)
            {
                if (!PointInPolygon(boundary, {x, y})) continue;
                ++cells;
                for (const Point& f : food)
                    if (Distance(f, {x, y}) < 15000.0)
                    {
                        ++covered;
                        break;
                    }
            }
        Check(cells > 1000 && covered >= 0.9 * cells, "food within 150 m of 90% of the estate", cells ? 100.0 * covered / cells : 0.0);

        // An old save from before these rows loads them fresh and ready, keeps its own edits, and they pick and grow.
        EstatePlacements old;
        old.bakeVersion = ProvisionalEstatePlacements().bakeVersion;
        const EstatePlacement* newBramble = nullptr;
        const EstatePlacement* newRoots = nullptr;
        for (const EstatePlacement& placement : all)
        {
            if (!IsMoreFood(placement.id)) old.placements.push_back(placement);
            else if (placement.kind == ResourceKind::BerryBush && !newBramble) newBramble = &placement;
            else if (placement.kind == ResourceKind::Roots && !newRoots) newRoots = &placement;
        }
        Check(newBramble && newRoots, "a new bramble and a new root patch");
        Simulation before;
        Check(before.NewEstateGame(estateLayout, old).ok, "pre-abundance game");
        const EstatePlacement* oldBramble = nullptr;
        for (const EstatePlacement& placement : old.placements)
            if (placement.id == 582100) oldBramble = &placement;
        Check(oldBramble && before.Harvest(oldBramble->id, oldBramble->position).ok, "picked an older bramble");
        Simulation after;
        after.SetLayout(estateLayout);
        after.SetPlacements(ProvisionalEstatePlacements());
        Check(after.Deserialize(before.Serialize()).ok, "pre-abundance save loads with the new food");
        Check(oldBramble && !after.CanHarvest(oldBramble->id), "the older bramble stays picked");
        if (newBramble && newRoots)
        {
            Check(after.CanHarvest(newBramble->id) && after.CanHarvest(newRoots->id), "new food ready in an old save");
            const int berries = after.Count(Item::Berries), roots = after.Count(Item::Roots);
            Check(after.Harvest(newBramble->id, newBramble->position).ok && after.Count(Item::Berries) > berries, "picked a new bramble");
            Check(after.Harvest(newRoots->id, newRoots->position).ok && after.Count(Item::Roots) > roots, "dug a new root patch");
        }
        std::printf("more food: %d estate brambles, %d estate root patches, %d roadside (%d roots); zones: manor %d, near %d, far %d, road %d; "
                    "coverage %.1f%% of %d cells\n",
            moreBrambles, moreRoots, moreRoadside, moreRoadsideRoots, manorGrounds, nearWoods, farEstate, alongRoad,
            cells ? 100.0 * covered / cells : 0.0, cells);
    }

    // Save identity: every frozen forage row (Tests/Data/HomesteadForageManifest.inc) is still in the table
    // with its kind and position, and a save that picked some of them keeps those edits on the same nodes.
    {
        struct Frozen { int id; ResourceKind kind; Point at; };
        std::vector<Frozen> frozen;
        auto manifest = [&](int id, ResourceKind kind, double x, double y) { frozen.push_back({id, kind, {x, y}}); };
#include "Data/HomesteadForageManifest.inc"
        Check(frozen.size() >= 59, "frozen forage rows", static_cast<double>(frozen.size()));
        for (const Frozen& row : frozen)
        {
            const EstatePlacement* now = nullptr;
            for (const EstatePlacement& placement : ProvisionalEstatePlacements().placements)
                if (placement.id == row.id) now = &placement;
            Check(now && now->kind == row.kind && Distance(now->position, row.at) < 0.05, "a frozen forage row moved, changed kind or vanished", row.id);
        }
        Simulation sim;
        Check(sim.NewEstateGame(estateLayout, ProvisionalEstatePlacements()).ok, "forage save game");
        std::vector<int> picked;
        for (const Frozen& row : frozen)
            if (picked.size() < 3 && (row.id == 582128 || row.id == 581000 || row.id == 582100))
                if (sim.Harvest(row.id, row.at).ok) picked.push_back(row.id);
        Check(picked.size() == 3, "picked a bramble, a root patch and a roadside stop", static_cast<double>(picked.size()));
        const std::string saved = sim.Serialize();
        Simulation loaded;
        loaded.SetLayout(estateLayout);
        loaded.SetPlacements(ProvisionalEstatePlacements());
        Check(loaded.Deserialize(saved).ok, "forage save reloads");
        for (const Frozen& row : frozen)
        {
            const bool wasPicked = std::find(picked.begin(), picked.end(), row.id) != picked.end();
            Check(loaded.CanHarvest(row.id) != wasPicked, "a picked edit landed on the wrong node", row.id);
            for (const auto& node : loaded.GetState().resources)
                if (node.id == row.id)
                    Check(node.kind == row.kind && Distance(node.position, row.at) < 0.05, "reloaded node moved", row.id);
        }
    }

    // shrink-estate-map: the village is close. The walk from the manor to the store is at most 288 m (60 s of
    // sprint at 4.8 m/s; balance.md section 9), well inside the 75 s cap, and the roadside sights bake is valid.
    {
        Simulation sim;
        Check(sim.NewEstateGame(estateLayout, ProvisionalEstatePlacements()).ok, "travel game");
        const PublicRoadStop* manorStop = road.FindStop("Manor");
        Check(sim.DiscoverTravel(TravelDestination::Town, estateLayout.PointOr(Anchor::TownSquare, {})).ok, "town discovered");
        Check(sim.DiscoverTravel(TravelDestination::Store, estateLayout.PointOr(Anchor::GeneralStoreDoor, {})).ok, "store door discovered");
        for (const TravelDestination destination : {TravelDestination::Town, TravelDestination::Store})
        {
            const TravelPlan plan = PlanTravel(sim.GetState(), manorStop->position, destination);
            Check(plan.ok && plan.totalMetres <= 360.0, "manor to the village is at most 360 m (75 s at a sprint)", plan.totalMetres);
            Check(plan.ok && plan.totalMetres >= 250.0, "manor to the village is a real walk, not a doorstep", plan.totalMetres);
        }
        for (const int id : {581032, 581033, 581034})
        {
            const EstatePlacement* found = nullptr;
            for (const EstatePlacement& placement : ProvisionalEstatePlacements().placements)
                if (placement.id == id) found = &placement;
            Check(found && IsPublicRoadsidePlacement(*found) && EstatePlacementAllowed(estateLayout, *found), "scenic roadside flowers are valid", id);
        }
    }
    std::printf("forage: %d new estate brambles, %d roadside nodes (%d brambles, to %.0f m), %zu brambles in all\n",
        estateBrambles, roadsideNodes, roadsideBrambles, farthest, brambles.size());
    std::printf("public road %.1f m; manor %.1f, bridge %.1f, gateway %.1f, town %.1f m\n", road.Length(),
        manor ? manor->chainage : -1.0, road.bridgeChainage, gateway ? gateway->chainage : -1.0, town ? town->chainage : -1.0);
    std::printf(Failures ? "%d failure(s)\n" : "public road: all checks passed\n", Failures);
    return Failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
