#include "HomesteadEstate.h"
#include "HomesteadRuinDebris.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <utility>

namespace Homestead
{
const Landmark* EstateLayout::FindLandmark(const char* name) const
{
    for (const auto& landmark : landmarks)
        if (landmark.name == name) return &landmark;
    return nullptr;
}

const LandmarkPolygon* EstateLayout::FindPolygon(const char* name) const
{
    for (const auto& polygon : polygons)
        if (polygon.name == name) return &polygon;
    return nullptr;
}

Point EstateLayout::PointOr(const char* name, Point fallback) const
{
    const Landmark* landmark = FindLandmark(name);
    return landmark ? landmark->position : fallback;
}

const EstateLayout& ProvisionalEstateLayout()
{
    // Fitted to the reshaped LIDAR master heightmap (Scripts/Terrain/estate_layout.json). The estate
    // is the sunny west bank of a wooded valley whose river runs south-west into its own cove; the
    // road fords the river by the mill, climbs the east bank and runs about 1.7 km east to the town
    // on the spur above the estuary. Positions and z are in world centimetres.
    static const EstateLayout Layout = []
    {
        EstateLayout layout;
        layout.version = 2;
        layout.landmarks = {
            // The standing room is 2 x 2 building cells (6 x 6 m) flush with the ruin's south-east
            // corner; she wakes in line with its doorway on the west side, facing it.
            {Anchor::StandingRoomOrigin, {-25600.0, -63800.0}, 8652.6, 0.0},
            {Anchor::StandingRoomSpawn, {-25750.0, -63800.0}, 8652.6, -90.0},
            {Anchor::EstateGateway, {-5500.0, 9000.0}, 5632.0, 60.0},
            {Anchor::CoveBeach, {-65000.0, -51500.0}, 178.0, 215.0},
            {Anchor::MineEntrance, {-40500.0, -100000.0}, 7224.0, 0.0},
            {Anchor::MillSite, {-11800.0, -9200.0}, 3113.1, 0.0},
            {Anchor::RoadEstateEnd, {-5500.0, 9000.0}, 5632.0, 60.0},
            {Anchor::RoadTownEnd, {-50000.0, 109000.0}, 9241.0, 45.0},
            {Anchor::TownSquare, {-54000.0, 115000.0}, 9109.0, 0.0},
            {Anchor::GeneralStoreDoor, {-54000.0, 117950.0}, 9175.0, 90.0},
            {Anchor::GeneralStoreCounter, {-54000.0, 118550.0}, 9188.0, -90.0},
            // The derelict farm's broken gate, on its south fence facing the ruin's rear-wall gap.
            {Anchor::DerelictFarmGate, {-22200.0, -65700.0}, 8720.6, 180.0},
        };
        layout.polygons = {
            {Anchor::EstateBoundary,
                {{16000.0, -115000.0}, {16000.0, -25000.0}, {6000.0, -3000.0}, {-4000.0, 11000.0},
                 {-20000.0, -5000.0}, {-35000.0, -21000.0}, {-52000.0, -32000.0}, {-76000.0, -43000.0},
                 {-76000.0, -115000.0}}},
            // The ruin is 18 x 30 m; the 6 x 6 m standing room is its carved-out south-east corner.
            {Anchor::ManorFootprint,
                {{-24100.0, -66500.0}, {-24100.0, -63500.0}, {-25300.0, -63500.0}, {-25300.0, -64100.0},
                 {-25900.0, -64100.0}, {-25900.0, -66500.0}}},
            // The old walled field 19 m behind the ruin: 60 x 60 m of gentle south-facing pasture
            // (about 4 degrees, 5.5 m of relief), 25 m or more off the road.
            {Anchor::DerelictFarm,
                {{-22200.0, -70500.0}, {-22200.0, -64500.0}, {-16200.0, -64500.0}, {-16200.0, -70500.0}}},
            {std::string(Anchor::ForSaleParcelPrefix) + "Woodland",
                {{16000.0, -90000.0}, {60000.0, -90000.0}, {60000.0, -25000.0}, {16000.0, -25000.0}}},
            {std::string(Anchor::ForSaleParcelPrefix) + "MoorField",
                {{60000.0, -150000.0}, {100000.0, -150000.0}, {100000.0, -90000.0}, {60000.0, -90000.0}}},
            {std::string(Anchor::ForSaleParcelPrefix) + "WestCove",
                {{16000.0, -160000.0}, {16000.0, -115000.0}, {-76000.0, -115000.0}, {-76000.0, -160000.0}}},
        };
        return layout;
    }();
    return Layout;
}

Point EstateManorFrontDoor(const EstateLayout& layout)
{
    const LandmarkPolygon* manor = layout.FindPolygon(Anchor::ManorFootprint);
    if (!manor || manor->points.empty()) return layout.PointOr(Anchor::StandingRoomSpawn, {});
    double south = manor->points.front().x, west = manor->points.front().y;
    for (const Point& corner : manor->points)
    {
        south = std::min(south, corner.x);
        west = std::min(west, corner.y);
    }
    return {south, west + 1050.0};
}

const EstatePlacements& ProvisionalEstatePlacements()
{
    static const EstatePlacements Placements = []
    {
        EstatePlacements table;
        table.bakeVersion = 2;
        const Point room = ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {});
        auto add = [&](int id, ResourceKind kind, double dx, double dy, int minTier = 0)
        {
            table.placements.push_back({id, kind, {room.x + dx, room.y + dy}, 0.0, 0.0, 1.0, minTier});
        };
        // World lane: loose forage around the manor, so branches exist before the first axe.
        add(500001, ResourceKind::Branches, -900.0, 400.0);
        add(500002, ResourceKind::Branches, -1200.0, -700.0);
        add(500003, ResourceKind::Branches, -1600.0, 900.0);
        add(500004, ResourceKind::Stones, -1400.0, 1200.0);
        add(500005, ResourceKind::Flowers, -1800.0, -300.0);
        add(500006, ResourceKind::ForestTree, -3000.0, 1500.0);
        add(500007, ResourceKind::ForestTree, -3400.0, -1800.0);
        // World lane: the estate's interactive woodland and forage, baked by Scripts/Terrain/scatter.py.
        auto world = [&](int id, ResourceKind kind, double x, double y)
        {
            table.placements.push_back({id, kind, {x, y}, 0.0, 0.0, 1.0, 0});
        };
#include "HomesteadEstateWorldPlacements.inc"
        // Overgrowth lane (510000+). Offsets are from the spawn (x north, y east). The standing room's
        // door opens west into the ruin's south range, and she leaves the ruin through the fallen front
        // door on its south front (x = -25900). The drive follows the road north-east toward the gateway and the
        // valley falls south toward the cove. Everything stays outside ManorFootprint.
        int next = 510001;
        auto grow = [&](ResourceKind kind, double dx, double dy, int minTier = 0) { add(next++, kind, dx, dy, minTier); };
        const Point cove = ProvisionalEstateLayout().PointOr(Anchor::CoveBeach, {room.x - 1.0, room.y});
        const auto unit = [&](Point to) {
            const double x = to.x - room.x, y = to.y - room.y, length = std::sqrt(x * x + y * y);
            return length > 0 ? Point{x / length, y / length} : Point{1.0, 0.0};
        };
        const Point valley = unit(cove);
        // `distance` along a heading, `side` metres-in-cm to its left (negative: right).
        auto along = [&](ResourceKind kind, Point heading, double distance, double side, int minTier = 0)
        {
            grow(kind, heading.x * distance - heading.y * side, heading.y * distance + heading.x * side, minTier);
        };
        // Thin bramble chokes the 3 m gap outside the fallen front door (Y -65600..-65300), so she
        // meets it before the open pasture. The billhook's salvage pile is indoors.
        const Point frontDoor = EstateManorFrontDoor();
        for (const Point p : {Point{-100, 0}, Point{-150, -180}, Point{-150, 180}, Point{-300, -90}, Point{-300, 110},
                 Point{-480, -190}, Point{-480, 170}})
            grow(ResourceKind::BrambleThin, frontDoor.x + p.x - room.x, frontDoor.y + p.y - room.y);
        // The forecourt is a meadow of tall grass with weeds through it.
        for (int row = 0; row < 4; ++row)
            for (int column = 0; column < 6; ++column)
                grow(ResourceKind::TallGrass, -1100.0 + column * 110.0 + (row % 2) * 45.0, 1500.0 + row * 120.0);
        for (const Point p : {Point{-700, 1400}, Point{-200, 1700}, Point{300, 1550}, Point{600, 1900}, Point{-900, 2000}, Point{100, 2100}})
            grow(ResourceKind::Weeds, p.x, p.y);
        // Rubble and rocks shed from the ruin.
        for (const Point p : {Point{1900, -1500}, Point{1800, -500}, Point{-800, -2000}, Point{-800, -1000}, Point{0, -3100}})
            grow(ResourceKind::Rubble, p.x, p.y);
        for (const Point p : {Point{600, -2800}, Point{1700, -2400}, Point{-1100, 1800}})
            grow(ResourceKind::SmallRock, p.x, p.y);
        // The drive: bramble, saplings and fallen boughs crowd both verges of the road. Absolute points
        // follow the road polyline (Scripts/Terrain/estate_layout.json) from its estate start at
        // (-23200, -62800): the same arc distances as before, 4.5 m either side (grass 2.5 m, boughs
        // and stumps 7-8 m), clear of the graded road and its shoulders (1.45 m).
        auto place = [&](ResourceKind kind, Point at) { grow(kind, at.x - room.x, at.y - room.y); };
        for (const auto& [kind, at] : std::initializer_list<std::pair<ResourceKind, Point>>{
                 {ResourceKind::BrambleThin, {-21422, -60838}}, {ResourceKind::BrambleThin, {-20563, -61319}},
                 {ResourceKind::BrambleThin, {-20700, -60299}}, {ResourceKind::Weeds, {-19841, -60780}},
                 {ResourceKind::Sapling, {-19981, -59760}}, {ResourceKind::BrambleThin, {-19118, -60234}},
                 {ResourceKind::BrambleThin, {-19273, -59217}}, {ResourceKind::Weeds, {-18402, -59678}},
                 {ResourceKind::BrambleThin, {-18575, -58664}}, {ResourceKind::BrambleThin, {-17695, -59106}},
                 {ResourceKind::Sapling, {-17890, -58095}}, {ResourceKind::Weeds, {-17002, -58518}},
                 {ResourceKind::BrambleThin, {-17223, -57515}}, {ResourceKind::BrambleThin, {-16319, -57910}},
                 {ResourceKind::BrambleThin, {-16566, -56912}}, {ResourceKind::Weeds, {-15654, -57287}},
                 {ResourceKind::FallenBranch, {-20413, -61519}}, {ResourceKind::FallenBranch, {-18330, -59940}},
                 {ResourceKind::FallenBranch, {-16149, -58094}},
                 {ResourceKind::StumpSmall, {-20351, -59600}}, {ResourceKind::StumpSmall, {-17970, -57703}},
                 {ResourceKind::TallGrass, {-20442, -60980}}, {ResourceKind::TallGrass, {-20181, -60160}},
                 {ResourceKind::TallGrass, {-19321, -60137}}, {ResourceKind::TallGrass, {-19071, -59314}},
                 {ResourceKind::TallGrass, {-18212, -59270}}, {ResourceKind::TallGrass, {-17988, -58440}}})
            place(kind, at);
        // Down the valley the overgrowth thickens, with the iron and steel teases off to the sides.
        for (int step = 0; step < 6; ++step)
        {
            const double distance = 3500.0 + step * 1200.0;
            along(ResourceKind::BrambleThin, valley, distance, 350.0);
            along(step % 2 ? ResourceKind::Sapling : ResourceKind::Weeds, valley, distance + 500.0, -400.0);
        }
        along(ResourceKind::FallenBranch, valley, 4000.0, -900.0);
        along(ResourceKind::StumpSmall, valley, 5200.0, 900.0);
        along(ResourceKind::BrambleThicket, valley, 3000.0, 1200.0);
        along(ResourceKind::BrambleThicket, valley, 6800.0, -1300.0);
        place(ResourceKind::BrambleThicket, {-19173, -57925}); // 14 m off the road.
        along(ResourceKind::BrambleBank, valley, 9500.0, 900.0);
        along(ResourceKind::StumpLarge, valley, 7600.0, 1500.0);
        along(ResourceKind::FallenLog, valley, 8800.0, -1400.0);
        along(ResourceKind::StumpAncient, valley, 11000.0, -1000.0);
        along(ResourceKind::GiantLog, valley, 12000.0, 700.0);
        grow(ResourceKind::Boulder, 2400.0, -3200.0);
        along(ResourceKind::Boulder, valley, 10200.0, -300.0);
        // Spring flowers in the verges and the valley, sold at the general store.
        place(ResourceKind::Primroses, {-21451, -60297});
        place(ResourceKind::Primroses, {-17408, -59453});
        grow(ResourceKind::Primroses, -1300.0, 2600.0);
        along(ResourceKind::Bluebells, valley, 5800.0, 1100.0);
        along(ResourceKind::Bluebells, valley, 8200.0, -900.0);
        place(ResourceKind::WildDaffodils, {-18867, -60734});
        grow(ResourceKind::WildDaffodils, 900.0, 2500.0);
        along(ResourceKind::WildGarlic, valley, 4600.0, 1400.0);
        along(ResourceKind::WildGarlic, valley, 7000.0, 1000.0);
        // Appended so earlier ids stay stable: thin bramble outside the rear (north-wall) gap at
        // (-24100, -65150), flanking salvage pile 520004 so the pile keeps its own focus.
        for (const Point at : {Point{-24040, -65290}, Point{-24040, -65010}, Point{-23830, -65020}})
            place(ResourceKind::BrambleThin, at);
        // Salvage (520000+), placed by add-ruined-manor-and-arrival in and around the ruin. Each pile
        // yields the next rusted head she's missing, so the one just outside the standing room's door
        // gives the billhook. Positions use HomesteadManorRuin's frame: u east from the footprint's
        // west end, v north from its south front (cm), clear of the wall runs and rubble.
        double manorX = room.x, manorY = room.y;
        if (const LandmarkPolygon* manor = ProvisionalEstateLayout().FindPolygon(Anchor::ManorFootprint))
            for (const Point& corner : manor->points)
            {
                manorX = std::min(manorX, corner.x);
                manorY = std::min(manorY, corner.y);
            }
        auto salvage = [&](int id, double u, double v)
        {
            table.placements.push_back({id, ResourceKind::SalvagePile, {manorX + v, manorY + u}, 0.0, 0.0, 1.0, 0});
        };
        salvage(520001, 2150.0, 300.0);  // In the south range, 2.5 m west of the standing room's door.
        salvage(520002, 1250.0, 450.0);  // Inside the fallen front door, beside its rubble.
        salvage(520003, 300.0, 1300.0);  // In the west rooms, north of the chimney breast.
        salvage(520004, 1350.0, 1950.0); // Outside the gap in the fallen rear wall.
        salvage(520005, 400.0, -200.0);  // Under the collapsed south-west corner, outside.
        // Pickable blackberry brambles (540000+, Scripts/Terrain/berries.py): round the manor, along the
        // drive and at the woodland edges. A row within 3 m of an earlier placement is skipped (its id
        // stays unused), so other lanes' additions never renumber them.
        auto berry = [&](int id, double x, double y)
        {
            for (const EstatePlacement& other : table.placements)
                if ((other.position.x - x) * (other.position.x - x) + (other.position.y - y) * (other.position.y - y) < 300.0 * 300.0)
                    return;
            table.placements.push_back({id, ResourceKind::BerryBush, {x, y}, 0.0, 0.0, 1.0, 0});
        };
#include "HomesteadEstateBerryPlacements.inc"
        // The derelict farm and neglected grounds (550000+) go after the brambles and keep 3 m from them.
        AppendDerelictFarmAndDisrepair(table);
        // The manor clear-out (570000+, Scripts/Terrain/clearout.py, add-coral-island-clearout): the
        // ground round the ruin littered with weeds, nettles, stumps, rocks and rubbish to clear before
        // she can dig or build there. A row within 1.5 m of an earlier placement (3 m of a blackberry
        // bramble) is skipped, so other lanes' additions never collide with it or renumber it.
        auto clearout = [&](int id, ResourceKind kind, double x, double y)
        {
            for (const EstatePlacement& other : table.placements)
            {
                const double gap = other.kind == ResourceKind::BerryBush ? 300.0 : 150.0;
                if ((other.position.x - x) * (other.position.x - x) + (other.position.y - y) * (other.position.y - y) < gap * gap)
                    return;
            }
            table.placements.push_back({id, kind, {x, y}, 0.0, 0.0, 1.0, 0});
        };
#include "HomesteadEstateClearoutPlacements.inc"
        // Forage on the verges of the public road to town (581000-581099, Scripts/Terrain/forage.py), off
        // the estate by the narrow IsPublicRoadsidePlacement exception. Same 3 m rule as the brambles.
        auto roadside = [&](int id, ResourceKind kind, double x, double y)
        {
            for (const EstatePlacement& other : table.placements)
                if ((other.position.x - x) * (other.position.x - x) + (other.position.y - y) * (other.position.y - y) < 300.0 * 300.0)
                    return;
            table.placements.push_back({id, kind, {x, y}, 0.0, 0.0, 1.0, 0});
        };
#include "HomesteadEstateRoadsidePlacements.inc"
        // Clearable manor ruin debris (582000-582099, HomesteadRuinDebris.h): the slate and loose
        // granite heaps the ruin used to draw as scenery, at the same spots in its frame. A heap an
        // earlier section has since grown into steps 1.5 m aside (never onto another placement).
        auto debris = [&](const RuinDebris::Spot& spot)
        {
            const Point steps[] = {{0, 0}, {150, 0}, {-150, 0}, {0, 150}, {0, -150}};
            Point at{manorX + spot.v, manorY + spot.u};
            for (const Point& step : steps)
            {
                const Point candidate{manorX + spot.v + step.x, manorY + spot.u + step.y};
                const bool crowded = std::any_of(table.placements.begin(), table.placements.end(), [&](const EstatePlacement& other)
                    { return std::hypot(other.position.x - candidate.x, other.position.y - candidate.y) <= 120.0; });
                if (!crowded) { at = candidate; break; }
            }
            table.placements.push_back({spot.id, spot.kind, at, 0.0, spot.yaw, spot.scale, 0});
        };
        for (const RuinDebris::Spot& spot : RuinDebris::Spots)
            if (spot.kind != ResourceKind::RuinTimbers) debris(spot);
        // More brambles in the estate's woods and field hedges (582100-582299, Scripts/Terrain/forage.py),
        // after everything else; a row within 3 m of an earlier placement is skipped.
        auto forage = [&](int id, ResourceKind kind, double x, double y)
        {
            for (const EstatePlacement& other : table.placements)
                if ((other.position.x - x) * (other.position.x - x) + (other.position.y - y) * (other.position.y - y) < 300.0 * 300.0)
                    return;
            table.placements.push_back({id, kind, {x, y}, 0.0, 0.0, 1.0, 0});
        };
#include "HomesteadEstateForagePlacements.inc"
        // The ruin's fallen roof timbers (582012-582013) joined the clearable debris after the forage was
        // placed: they come after it, so no forage row's 3 m skip can change because of them.
        for (const RuinDebris::Spot& spot : RuinDebris::Spots)
            if (spot.kind == ResourceKind::RuinTimbers) debris(spot);
        // Father's tool rack, where the arrival journal says his garden tools hung: by the chimney in
        // the west rooms. Added after the first playtests (Jenny couldn't find a hoe), so it waits
        // unsearched in every older save and gives whichever rusted head she is still missing. It goes
        // last, so no earlier section's proximity skips or sequential ids can shift because of it.
        salvage(520006, 230.0, 1010.0);
        // Town lane (530000+).
        return table;
    }();
    return Placements;
}

bool PointInPolygon(const std::vector<Point>& ring, Point point)
{
    bool inside = false;
    const size_t count = ring.size();
    for (size_t i = 0, j = count ? count - 1 : 0; i < count; j = i++)
    {
        const Point& a = ring[i];
        const Point& b = ring[j];
        if ((a.y > point.y) != (b.y > point.y)
            && point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x)
            inside = !inside;
    }
    return inside;
}
}
