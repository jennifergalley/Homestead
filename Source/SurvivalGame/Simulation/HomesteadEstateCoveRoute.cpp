#include "HomesteadEstateCoveRoute.h"

#include <cmath>

namespace Homestead
{
namespace
{
constexpr double CoveRoutePi = 3.14159265358979323846;
double CoveRouteRadians(double degrees) { return degrees * CoveRoutePi / 180.0; }
}

Point CoveRouteFlight::TreadPivot(int i) const
{
    const double a = CoveRouteRadians(yaw);
    return {start.x + std::cos(a) * going * i, start.y + std::sin(a) * going * i};
}

double CoveRouteFlight::PitchDegrees() const
{
    return going > 0.0 ? std::atan2(rise, going) * 180.0 / CoveRoutePi : 0.0;
}

int CoveRoute::Steps() const
{
    int steps = 0;
    for (const CoveRouteFlight& flight : flights) steps += flight.treads;
    return steps;
}

CoveRoute::Nearest CoveRoute::NearestTo(Point world) const
{
    Nearest best;
    double bestSq = 1e300;
    for (size_t i = 0; i < stations.size(); ++i)
    {
        const double dx = stations[i].position.x - world.x, dy = stations[i].position.y - world.y;
        const double sq = dx * dx + dy * dy;
        if (sq < bestSq)
        {
            bestSq = sq;
            best.station = static_cast<int>(i);
        }
    }
    best.distanceCm = best.station >= 0 ? std::sqrt(bestSq) : 0.0;
    return best;
}

const CoveRoute& EstateCoveRoute()
{
    static const CoveRoute Route = []
    {
        CoveRoute route;
        auto station = [&](double x, double y, double walk, double bed, double metres, int steps) {
            route.stations.push_back({{x, y}, walk, bed, metres, steps != 0});
        };
        auto ground = [&](double metres, double left, double centre, double right) {
            route.ground.push_back({metres, left, centre, right});
        };
        auto flight = [&](double x, double y, double z, double yaw, double rise, double going, int treads, double pitch) {
            route.flights.push_back({{x, y}, z, yaw, rise, going, treads, pitch});
        };
        auto flightEnds = [&](double footLeft, double foot, double footRight, double headLeft, double head, double headRight) {
            route.flightEnds.push_back({{footLeft, foot, footRight}, {headLeft, head, headRight}});
        };
        auto corner = [&](double x, double y, double z, double landingYaw, double otherYaw) {
            route.corners.push_back({{x, y}, z, landingYaw, otherYaw});
        };
        auto landing = [&](double x, double y, double z, double yaw, double length) {
            route.landings.push_back({{x, y}, z, yaw, length});
        };
        auto kerb = [&](double x, double y, double z, double yaw) { route.kerbs.push_back({{x, y}, z, yaw}); };
        auto rail = [&](double x, double y, double z, double yaw, double pitch, double length, int mirrored) {
            route.rails.push_back({{x, y}, z, yaw, pitch, length, mirrored != 0});
        };
        auto fingerpost = [&](double x, double y, double z, double yaw) { route.fingerposts.push_back({{x, y}, z, yaw}); };
#include "HomesteadEstateCoveRoute.inc"
        return route;
    }();
    return Route;
}
}
