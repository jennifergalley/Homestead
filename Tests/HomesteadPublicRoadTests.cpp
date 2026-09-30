// Native tests for the public road (Simulation/HomesteadEstatePublicRoad).
#include "../Source/SurvivalGame/Simulation/HomesteadEstate.h"
#include "../Source/SurvivalGame/Simulation/HomesteadEstatePublicRoad.h"
#include "../Source/SurvivalGame/Simulation/HomesteadSimulation.h"

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
}

int main()
{
    const PublicRoad& road = EstatePublicRoad();
    Check(road.points.size() == 486, "486 centreline points", static_cast<double>(road.points.size()));
    Check(std::abs(road.Length() - 1940.0) < 1.0, "1.94 km", road.Length());
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
        Check(manor->chainage < gateway->chainage && gateway->chainage < town->chainage, "stops in order");
        Check(road.NearestTo(manor->position).distanceCm < 1.0 && road.NearestTo(town->position).distanceCm < 1.0, "stops on the road");
        const EstateLayout& layout = ProvisionalEstateLayout();
        const Landmark* gate = layout.FindLandmark(Anchor::EstateGateway);
        const Landmark* townEnd = layout.FindLandmark(Anchor::RoadTownEnd);
        Check(gate && Distance(gateway->position, gate->position) < 1500.0, "gateway stop at the estate gateway",
            gate ? Distance(gateway->position, gate->position) : -1.0);
        Check(townEnd && Distance(town->position, townEnd->position) < 1000.0, "town stop at the road's town end",
            townEnd ? Distance(town->position, townEnd->position) : -1.0);
        Check(std::abs(road.WalkMetres(manor->chainage, town->chainage) - 1922.0) < 1.0, "manor to town walk",
            road.WalkMetres(manor->chainage, town->chainage));
        Check(road.WalkMetres(town->chainage, manor->chainage) == road.WalkMetres(manor->chainage, town->chainage), "walk is symmetric");
        Check(manor->z > 0.0 && town->z > 0.0, "stop heights above the sea");
    }

    // The signs stand on the verge, not the bed, and none in the bridge keep-out.
    Check(road.signs.size() == 3, "three signs", static_cast<double>(road.signs.size()));
    for (const PublicRoadSign& sign : road.signs)
    {
        const double off = road.NearestTo(sign.position).distanceCm;
        Check(off > 320.0 && off < 600.0, "sign on the verge", off);
        Check(!road.InBridgeKeepOut(sign.position), "sign clear of the bridge");
        Check(sign.yaw >= 0.0 && sign.yaw < 360.0, "sign yaw normalised", sign.yaw);
    }

    // The bridge keep-out covers the crossing and its ramps and nothing far off it.
    const Point crossing = road.At(road.bridgeChainage);
    Check(road.InBridgeKeepOut(crossing), "crossing in the keep-out");
    Check(road.InBridgeKeepOut(road.At(road.bridgeChainage + road.bridgeHalfAlong - 1.0)), "ramp end in the keep-out");
    Check(!road.InBridgeKeepOut(road.At(road.bridgeChainage + road.bridgeHalfAlong + 5.0)), "past the ramp is clear");
    Check(road.bridgeChainage > 600.0 && road.bridgeChainage < 760.0, "bridge between the manor and the gateway", road.bridgeChainage);

    // The roadside exception: only its ids and forage kinds, on the verge, clear of the bridge.
    const EstateLayout& estateLayout = ProvisionalEstateLayout();
    const Point verge = road.At(1500.0);
    const PublicRoad::Nearest at1500 = road.NearestTo(verge);
    (void)at1500;
    auto offset = [&](double metres, double cm) {
        const Point a = road.At(metres - 1.0), b = road.At(metres + 1.0);
        const double dx = b.x - a.x, dy = b.y - a.y, n = std::hypot(dx, dy);
        const Point c = road.At(metres);
        return Point{c.x - dy / n * cm, c.y + dx / n * cm};
    };
    const EstatePlacement roadside{581050, ResourceKind::BerryBush, offset(1500.0, 450.0), 0, 0, 1, 0};
    Check(IsPublicRoadsidePlacement(roadside) && EstatePlacementAllowed(estateLayout, roadside), "roadside bramble allowed");
    EstatePlacement wrong = roadside;
    wrong.id = 500123;
    Check(!EstatePlacementAllowed(estateLayout, wrong), "ordinary id off the estate rejected");
    wrong = roadside;
    wrong.position = offset(1500.0, 20000.0);
    Check(!IsPublicRoadsidePlacement(wrong), "reserved id 200 m off the road rejected");
    wrong = roadside;
    wrong.position = offset(1500.0, 150.0);
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
    Check(roadsideNodes >= 12 && roadsideBrambles >= 8, "roadside forage stops", roadsideNodes);
    Check(farthest > 1800.0, "roadside forage reaches the town end", farthest);
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
    // Coverage: a pickable bramble within 120 m of the road at every 400 m mark (0-1940 m).
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
                && road.NearestTo(placement.position).chainage > 1300.0)
                far = &placement;
        Check(far != nullptr, "a roadside bramble well off the estate");
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
            if (picked.size() < 3 && (row.id == 582128 || row.id == 581005 || row.id == 582100))
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

    std::printf("forage: %d new estate brambles, %d roadside nodes (%d brambles, to %.0f m), %zu brambles in all\n",
        estateBrambles, roadsideNodes, roadsideBrambles, farthest, brambles.size());
    std::printf("public road %.1f m; manor %.1f, bridge %.1f, gateway %.1f, town %.1f m\n", road.Length(),
        manor ? manor->chainage : -1.0, road.bridgeChainage, gateway ? gateway->chainage : -1.0, town ? town->chainage : -1.0);
    std::printf(Failures ? "%d failure(s)\n" : "public road: all checks passed\n", Failures);
    return Failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
