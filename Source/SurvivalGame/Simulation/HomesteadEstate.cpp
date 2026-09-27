#include "HomesteadEstate.h"

#include <cmath>

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
            {Anchor::StandingRoomOrigin, {-25500.0, -63900.0}, 8652.6, 0.0},
            {Anchor::StandingRoomSpawn, {-25350.0, -64050.0}, 8652.6, 90.0},
            {Anchor::EstateGateway, {-5500.0, 9000.0}, 4916.0, 60.0},
            {Anchor::CoveBeach, {-65000.0, -51500.0}, 178.0, 215.0},
            {Anchor::MineEntrance, {-40500.0, -100000.0}, 7224.0, 0.0},
            {Anchor::MillSite, {-11800.0, -9200.0}, 3113.1, 0.0},
            {Anchor::RoadEstateEnd, {-5500.0, 9000.0}, 4916.0, 60.0},
            {Anchor::RoadTownEnd, {-50000.0, 109000.0}, 9241.0, 45.0},
            {Anchor::TownSquare, {-54000.0, 115000.0}, 9109.0, 0.0},
            {Anchor::GeneralStoreDoor, {-54000.0, 117000.0}, 9153.0, 90.0},
            {Anchor::GeneralStoreCounter, {-54000.0, 117600.0}, 9166.0, -90.0},
        };
        layout.polygons = {
            {Anchor::EstateBoundary,
                {{16000.0, -115000.0}, {16000.0, -25000.0}, {6000.0, -3000.0}, {-4000.0, 11000.0},
                 {-20000.0, -5000.0}, {-35000.0, -21000.0}, {-52000.0, -32000.0}, {-76000.0, -43000.0},
                 {-76000.0, -115000.0}}},
            // The ruin is 18 x 30 m; the 8 x 8 m standing room is its carved-out south-east corner.
            {Anchor::ManorFootprint,
                {{-24100.0, -66500.0}, {-24100.0, -63500.0}, {-25100.0, -63500.0}, {-25100.0, -64300.0},
                 {-25900.0, -64300.0}, {-25900.0, -66500.0}}},
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
        // door faces east onto the forecourt; the drive climbs north-east toward the gateway and the
        // valley falls south toward the cove. Everything stays outside ManorFootprint.
        int next = 510001;
        auto grow = [&](ResourceKind kind, double dx, double dy, int minTier = 0) { add(next++, kind, dx, dy, minTier); };
        const Point gateway = ProvisionalEstateLayout().PointOr(Anchor::EstateGateway, {room.x + 1.0, room.y + 1.0});
        const Point cove = ProvisionalEstateLayout().PointOr(Anchor::CoveBeach, {room.x - 1.0, room.y});
        const auto unit = [&](Point to) {
            const double x = to.x - room.x, y = to.y - room.y, length = std::sqrt(x * x + y * y);
            return length > 0 ? Point{x / length, y / length} : Point{1.0, 0.0};
        };
        const Point drive = unit(gateway), valley = unit(cove);
        // `distance` along a heading, `side` metres-in-cm to its left (negative: right).
        auto along = [&](ResourceKind kind, Point heading, double distance, double side, int minTier = 0)
        {
            grow(kind, heading.x * distance - heading.y * side, heading.y * distance + heading.x * side, minTier);
        };
        // Thin bramble chokes the doorway.
        for (const Point p : {Point{-300, 800}, Point{0, 850}, Point{300, 780}, Point{-150, 1050}, Point{200, 1100},
                 Point{450, 950}, Point{-450, 1000}})
            grow(ResourceKind::BrambleThin, p.x, p.y);
        // The forecourt is a meadow of tall grass with weeds through it.
        for (int row = 0; row < 4; ++row)
            for (int column = 0; column < 6; ++column)
                grow(ResourceKind::TallGrass, -1100.0 + column * 110.0 + (row % 2) * 45.0, 1500.0 + row * 120.0);
        for (const Point p : {Point{-700, 1400}, Point{-200, 1700}, Point{300, 1550}, Point{600, 1900}, Point{-900, 2000}, Point{100, 2100}})
            grow(ResourceKind::Weeds, p.x, p.y);
        // Rubble and rocks shed from the ruin.
        for (const Point p : {Point{1500, -1500}, Point{1450, -500}, Point{-800, -2000}, Point{-800, -1000}, Point{0, -2700}})
            grow(ResourceKind::Rubble, p.x, p.y);
        for (const Point p : {Point{600, -2800}, Point{1700, -2400}, Point{-1100, 1800}})
            grow(ResourceKind::SmallRock, p.x, p.y);
        // The drive: bramble, saplings and fallen boughs crowd both verges.
        for (int step = 0; step < 8; ++step)
        {
            const double distance = 2600.0 + step * 900.0;
            along(step % 3 == 2 ? ResourceKind::Sapling : ResourceKind::BrambleThin, drive, distance, 450.0);
            along(step % 2 ? ResourceKind::Weeds : ResourceKind::BrambleThin, drive, distance + 400.0, -450.0);
        }
        for (const double distance : {3000.0, 5600.0, 8400.0}) along(ResourceKind::FallenBranch, drive, distance, -700.0);
        for (const double distance : {4200.0, 7300.0}) along(ResourceKind::StumpSmall, drive, distance, 800.0);
        for (int step = 0; step < 6; ++step) along(ResourceKind::TallGrass, drive, 3300.0 + step * 700.0, step % 2 ? 250.0 : -250.0);
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
        along(ResourceKind::BrambleThicket, drive, 6200.0, 1400.0);
        along(ResourceKind::BrambleBank, valley, 9500.0, 900.0);
        along(ResourceKind::StumpLarge, valley, 7600.0, 1500.0);
        along(ResourceKind::FallenLog, valley, 8800.0, -1400.0);
        along(ResourceKind::StumpAncient, valley, 11000.0, -1000.0);
        along(ResourceKind::GiantLog, valley, 12000.0, 700.0);
        grow(ResourceKind::Boulder, 2400.0, -3200.0);
        along(ResourceKind::Boulder, valley, 10200.0, -300.0);
        // Spring flowers in the verges and the valley, sold at the general store.
        along(ResourceKind::Primroses, drive, 2900.0, 900.0);
        along(ResourceKind::Primroses, drive, 6600.0, -900.0);
        grow(ResourceKind::Primroses, -1300.0, 2600.0);
        along(ResourceKind::Bluebells, valley, 5800.0, 1100.0);
        along(ResourceKind::Bluebells, valley, 8200.0, -900.0);
        along(ResourceKind::WildDaffodils, drive, 4700.0, -1000.0);
        grow(ResourceKind::WildDaffodils, 900.0, 2500.0);
        along(ResourceKind::WildGarlic, valley, 4600.0, 1400.0);
        along(ResourceKind::WildGarlic, valley, 7000.0, 1000.0);
        // Salvage lane (520000+). Provisional piles beside the ruin until add-ruined-manor-and-arrival
        // places its own; each yields the next rusted head she's missing (billhook first).
        add(520001, ResourceKind::SalvagePile, 1600.0, 300.0);
        add(520002, ResourceKind::SalvagePile, -900.0, -600.0);
        add(520003, ResourceKind::SalvagePile, -900.0, -1800.0);
        add(520004, ResourceKind::SalvagePile, 1500.0, -2800.0);
        add(520005, ResourceKind::SalvagePile, 400.0, -3000.0);
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
