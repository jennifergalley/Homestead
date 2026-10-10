#include "HomesteadEstatePublicRoad.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace Homestead
{
Point PublicRoad::At(double metres) const
{
    if (points.empty()) return {};
    if (metres <= chainage.front()) return points.front();
    if (metres >= chainage.back()) return points.back();
    const auto upper = std::upper_bound(chainage.begin(), chainage.end(), metres);
    const size_t i = static_cast<size_t>(upper - chainage.begin());
    const double span = std::max(chainage[i] - chainage[i - 1], 1e-9);
    const double t = (metres - chainage[i - 1]) / span;
    return {points[i - 1].x + (points[i].x - points[i - 1].x) * t, points[i - 1].y + (points[i].y - points[i - 1].y) * t};
}

PublicRoad::Nearest PublicRoad::NearestTo(Point world) const
{
    Nearest best{0.0, 1e300};
    for (size_t i = 1; i < points.size(); ++i)
    {
        const Point a = points[i - 1], b = points[i];
        const double ex = b.x - a.x, ey = b.y - a.y;
        const double lengthSquared = std::max(ex * ex + ey * ey, 1e-9);
        const double t = std::clamp(((world.x - a.x) * ex + (world.y - a.y) * ey) / lengthSquared, 0.0, 1.0);
        const double dx = world.x - (a.x + ex * t), dy = world.y - (a.y + ey * t);
        const double distance = std::sqrt(dx * dx + dy * dy);
        if (distance < best.distanceCm)
            best = {chainage[i - 1] + (chainage[i] - chainage[i - 1]) * t, distance};
    }
    return best;
}

double PublicRoad::WalkMetres(double fromChainage, double toChainage) const
{
    const double a = std::clamp(fromChainage, 0.0, Length()), b = std::clamp(toChainage, 0.0, Length());
    return std::abs(b - a);
}

const PublicRoadStop* PublicRoad::FindStop(const char* name) const
{
    for (const PublicRoadStop& stop : stops)
        if (stop.name == name) return &stop;
    return nullptr;
}

const PublicRoadSign* PublicRoad::FindSign(const char* name) const
{
    for (const PublicRoadSign& sign : signs)
        if (sign.name == name) return &sign;
    return nullptr;
}

bool PublicRoad::InBridgeKeepOut(Point world) const
{
    const Nearest nearest = NearestTo(world);
    return std::abs(nearest.chainage - bridgeChainage) <= bridgeHalfAlong && nearest.distanceCm <= bridgeHalfAcross * 100.0;
}

Point PublicRoadBridge::End(double side) const
{
    const double radians = yaw * 3.14159265358979323846 / 180.0;
    const double along = (side < 0.0 ? -1.0 : 1.0) * halfLength;
    return {centre.x + std::cos(radians) * along, centre.y + std::sin(radians) * along};
}

namespace
{
// The point in the deck's frame: (along toward town, across to the right), cm from its centre.
void DeckFrame(const PublicRoadBridge& deck, Point world, double& along, double& across)
{
    const double radians = deck.yaw * 3.14159265358979323846 / 180.0;
    const double dx = world.x - deck.centre.x, dy = world.y - deck.centre.y;
    along = dx * std::cos(radians) + dy * std::sin(radians);
    across = -dx * std::sin(radians) + dy * std::cos(radians);
}
}

bool PublicRoadBridge::Covers(Point world, double marginCm) const
{
    if (!valid) return false;
    double along = 0.0, across = 0.0;
    DeckFrame(*this, world, along, across);
    return std::abs(along) <= halfLength + DeckSlabOverrunCm + marginCm && std::abs(across) <= halfWidth + marginCm;
}

Point PublicRoadBridge::OntoDeck(Point world, double insetCm) const
{
    if (!valid) return world;
    double along = 0.0, across = 0.0;
    DeckFrame(*this, world, along, across);
    const double maxAlong = std::max(0.0, halfLength + DeckSlabOverrunCm - insetCm);
    const double maxAcross = std::max(0.0, halfWidth - insetCm);
    along = std::clamp(along, -maxAlong, maxAlong);
    across = std::clamp(across, -maxAcross, maxAcross);
    const double radians = yaw * 3.14159265358979323846 / 180.0;
    return {centre.x + along * std::cos(radians) - across * std::sin(radians),
            centre.y + along * std::sin(radians) + across * std::cos(radians)};
}

double PublicRoadBridge::ProbeStartZ(Point world, double z, double clearanceCm, double marginCm) const
{
    return Covers(world, marginCm) ? std::max(z, deckZ + clearanceCm) : z;
}

double PublicRoadBridge::RestZ(Point world, double groundZ) const
{
    return Covers(world) ? std::max(groundZ, deckZ) : groundZ;
}

const PublicRoad& EstatePublicRoad()
{
    static const PublicRoad Road = [] {
        PublicRoad road;
        auto roadPoint = [&](double x, double y, double z, double metres) {
            road.points.push_back({x, y});
            road.groundZ.push_back(z);
            road.chainage.push_back(metres);
        };
        auto bridge = [&](double metres, double halfAlong, double halfAcross) {
            road.bridgeChainage = metres;
            road.bridgeHalfAlong = halfAlong;
            road.bridgeHalfAcross = halfAcross;
        };
        std::vector<std::pair<std::string, double>> stopChainages;
        auto stop = [&](const char* name, double metres) { stopChainages.emplace_back(name, metres); };
        auto sign = [&](const char* name, double metres, double x, double y, double z, double yaw) {
            road.signs.push_back({name, metres, {x, y}, z, yaw});
        };
        auto deck = [&](double x, double y, double yaw, double z, double halfLength, double halfWidth, double water, double bed) {
            road.deck = {true, {x, y}, yaw, z, halfLength, halfWidth, water, bed};
        };
        struct Arrival { std::string stop; Point at; double z, yaw; };
        std::vector<Arrival> arrivals;
        auto arrival = [&](const char* name, double x, double y, double z, double yaw) { arrivals.push_back({name, {x, y}, z, yaw}); };
#define road roadPoint
#include "HomesteadEstatePublicRoad.inc"
#undef road
        for (const auto& [name, metres] : stopChainages)
        {
            const Point at = road.At(metres);
            // Ground height: linear along the nearest segment's two samples.
            const auto upper = std::upper_bound(road.chainage.begin(), road.chainage.end(), metres);
            const size_t i = std::clamp<size_t>(static_cast<size_t>(upper - road.chainage.begin()), 1, road.points.size() - 1);
            const double span = std::max(road.chainage[i] - road.chainage[i - 1], 1e-9);
            const double t = std::clamp((metres - road.chainage[i - 1]) / span, 0.0, 1.0);
            road.stops.push_back({name, metres, at, road.groundZ[i - 1] + (road.groundZ[i] - road.groundZ[i - 1]) * t});
        }
        for (const Arrival& off : arrivals)
            for (PublicRoadStop& endpoint : road.stops)
                if (endpoint.name == off.stop)
                {
                    endpoint.hasArrival = true;
                    endpoint.arrival = off.at;
                    endpoint.arrivalZ = off.z;
                    endpoint.arrivalYaw = off.yaw;
                }
        return road;
    }();
    return Road;
}

bool IsPublicRoadsidePlacement(const EstatePlacement& placement)
{
    if (placement.id < PublicRoadsideFirstId || placement.id >= PublicRoadsideEndId) return false;
    switch (placement.kind)
    {
    case ResourceKind::BerryBush:
    case ResourceKind::Flowers:
    case ResourceKind::Roots:
    case ResourceKind::Primroses:
    case ResourceKind::WildDaffodils:
        break;
    default:
        return false;
    }
    const PublicRoad& road = EstatePublicRoad();
    const PublicRoad::Nearest nearest = road.NearestTo(placement.position);
    return nearest.distanceCm >= 320.0 && nearest.distanceCm <= 1200.0 && !road.InBridgeKeepOut(placement.position);
}

bool EstatePlacementAllowed(const EstateLayout& layout, const EstatePlacement& placement)
{
    const LandmarkPolygon* boundary = layout.FindPolygon(Anchor::EstateBoundary);
    return (boundary && PointInPolygon(boundary->points, placement.position)) || IsPublicRoadsidePlacement(placement)
        || IsDriveVergeOvergrowth(placement);
}

bool IsDriveVergeOvergrowth(const EstatePlacement& placement)
{
    // The drive's verge overgrowth (550000+) past where the compact map's smaller estate ends (shrink-estate-map):
    // it stays where it was, on the road's verge, so the walk to the village looks as it did.
    if (placement.id < DriveVergeFirstId || placement.id > DriveVergeLastId) return false;
    const PublicRoad& road = EstatePublicRoad();
    return road.NearestTo(placement.position).distanceCm <= DriveVergeReachCm && !road.InBridgeKeepOut(placement.position);
}
}