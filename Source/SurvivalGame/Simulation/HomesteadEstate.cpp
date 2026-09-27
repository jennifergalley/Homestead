#include "HomesteadEstate.h"

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
        table.bakeVersion = 1;
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
        // Overgrowth lane (510000+).
        // Salvage lane (520000+).
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
