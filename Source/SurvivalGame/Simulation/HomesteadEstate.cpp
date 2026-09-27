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
    // Estate about 1 km across in the south-west quarter, sloping south to its cove; the road runs
    // east-north-east about 1.5 km to the town at the head of an estuary.
    static const EstateLayout Layout = []
    {
        EstateLayout layout;
        layout.version = 1;
        layout.landmarks = {
            {Anchor::StandingRoomOrigin, {-55000.0, -40000.0}, 0.0, 0.0},
            {Anchor::StandingRoomSpawn, {-54700.0, -39700.0}, 0.0, 0.0},
            {Anchor::EstateGateway, {-15000.0, 5000.0}, 0.0, 45.0},
            {Anchor::CoveBeach, {-125000.0, -40000.0}, 0.0, 180.0},
            {Anchor::MineEntrance, {-105000.0, -80000.0}, 0.0, 0.0},
            {Anchor::MillSite, {-85000.0, -8000.0}, 0.0, 0.0},
            {Anchor::RoadEstateEnd, {-15000.0, 5000.0}, 0.0, 60.0},
            {Anchor::RoadTownEnd, {-38000.0, 128000.0}, 0.0, 90.0},
            {Anchor::TownSquare, {-45000.0, 140000.0}, 0.0, 0.0},
            {Anchor::GeneralStoreDoor, {-45000.0, 146000.0}, 0.0, 90.0},
            {Anchor::GeneralStoreCounter, {-45000.0, 147400.0}, 0.0, -90.0},
        };
        layout.polygons = {
            {Anchor::EstateBoundary, {{-10000.0, -95000.0}, {-10000.0, 12000.0}, {-60000.0, 18000.0},
                {-118000.0, 10000.0}, {-128000.0, -45000.0}, {-115000.0, -100000.0}}},
            {Anchor::ManorFootprint, {{-50000.0, -42000.0}, {-50000.0, -39000.0}, {-53000.0, -39000.0},
                {-53000.0, -36000.0}, {-68000.0, -36000.0}, {-68000.0, -42000.0}}},
            {std::string(Anchor::ForSaleParcelPrefix) + "Woodland",
                {{-10000.0, 12000.0}, {20000.0, 15000.0}, {15000.0, 60000.0}, {-20000.0, 50000.0}}},
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
        add(500001, ResourceKind::Branches, 900.0, 400.0);
        add(500002, ResourceKind::Branches, 1200.0, -700.0);
        add(500003, ResourceKind::Branches, 1600.0, 900.0);
        add(500004, ResourceKind::Stones, 1400.0, 200.0);
        add(500005, ResourceKind::Flowers, 1800.0, -300.0);
        add(500006, ResourceKind::ForestTree, 3000.0, 1500.0);
        add(500007, ResourceKind::ForestTree, 3400.0, -1800.0);
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
