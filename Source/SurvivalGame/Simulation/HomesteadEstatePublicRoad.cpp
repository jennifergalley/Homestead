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
        return road;
    }();
    return Road;
}
}
