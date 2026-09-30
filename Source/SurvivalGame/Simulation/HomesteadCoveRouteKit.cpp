#include "HomesteadCoveRouteKit.h"

#include <algorithm>
#include <cmath>

namespace Homestead
{
namespace
{
constexpr double CoveKitPi = 3.14159265358979323846;
Point CoveKitAlong(Point p, double yaw, double cm)
{
    const double a = yaw * CoveKitPi / 180.0;
    return {p.x + std::cos(a) * cm, p.y + std::sin(a) * cm};
}
}

int CoveKitLayout::Count(CoveKitPiece piece) const
{
    return static_cast<int>(std::count_if(pieces.begin(), pieces.end(), [piece](const CoveKitPlacement& p) { return p.piece == piece; }));
}

CoveKitLayout BuildCoveKitLayout(const CoveRoute& route)
{
    CoveKitLayout layout;
    // Treads: tread i of a flight at its foot nosing + i x (going, rise); the three dressed variants in a
    // fixed shuffle so neighbours differ.
    for (size_t f = 0; f < route.flights.size(); ++f)
    {
        const CoveRouteFlight& flight = route.flights[f];
        for (int i = 0; i < flight.treads; ++i)
        {
            const unsigned variant = static_cast<unsigned>(f * 7 + static_cast<size_t>(i) * 5) % 3u;
            layout.pieces.push_back({static_cast<CoveKitPiece>(static_cast<int>(CoveKitPiece::StepA) + static_cast<int>(variant)),
                flight.TreadPivot(i), flight.TreadZ(i), flight.yaw});
        }
    }
    // Landings: whole 0.6 m slabs laid up the landing from its downhill edge, the run stretched to fit.
    for (const CoveRouteLanding& landing : route.landings)
    {
        // The whole number of slabs that stretches least (either way).
        const int fewer = std::max(1, static_cast<int>(std::floor(landing.length / CoveKitLandingSlabCm)));
        const auto stretchFor = [&](int n) { return landing.length / (n * CoveKitLandingSlabCm); };
        const int slabs = std::fabs(std::log(stretchFor(fewer))) <= std::fabs(std::log(stretchFor(fewer + 1))) ? fewer : fewer + 1;
        const double stretch = stretchFor(slabs);
        for (int k = 0; k < slabs; ++k)
            layout.pieces.push_back({CoveKitPiece::LandingSlab, CoveKitAlong(landing.position, landing.yaw, k * stretch * CoveKitLandingSlabCm),
                landing.z, landing.yaw, stretch, 1.0, 1.0});
    }
    for (const CoveRouteKerb& kerb : route.kerbs)
        layout.pieces.push_back({CoveKitPiece::Kerb, kerb.position, kerb.z, kerb.yaw});
    // Rail bays, scaled to their plan length in X and Z together (the pitch holds), mirrored where the drop
    // is on the bay's -Y; each with a pawn-only blocker along its rail line, tall enough for the rake.
    for (const CoveRouteRail& rail : route.rails)
    {
        CoveKitPiece piece = CoveKitPiece::RailLevel;
        if (rail.pitch >= 29.0) piece = CoveKitPiece::Rail30;
        else if (rail.pitch >= 27.0) piece = CoveKitPiece::Rail28;
        else if (rail.pitch > 1.0) piece = CoveKitPiece::Rail26;
        const double scale = rail.length / CoveKitRailBayCm;
        layout.pieces.push_back({piece, rail.position, rail.z, rail.yaw, scale, rail.mirrored ? -1.0 : 1.0, scale});
        const double rise = rail.length * std::tan(rail.pitch * CoveKitPi / 180.0);
        const double bottom = rail.z - CoveKitBlockerBelowCm, top = rail.z + rise + CoveKitBlockerAboveCm;
        layout.blockers.push_back({CoveKitAlong(rail.position, rail.yaw, rail.length * 0.5), 0.5 * (bottom + top), rail.yaw,
            rail.length * 0.5, CoveKitBlockerThicknessCm * 0.5, 0.5 * (top - bottom)});
    }
    for (const CoveRouteFingerpost& post : route.fingerposts)
        layout.pieces.push_back({CoveKitPiece::Fingerpost, post.position, post.z, post.yaw});
    return layout;
}

const CoveKitLayout& EstateCoveRouteKit()
{
    static const CoveKitLayout Layout = BuildCoveKitLayout(EstateCoveRoute());
    return Layout;
}
}
