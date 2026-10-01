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

CoveKitGroup CoveKitGroupOf(CoveKitPiece piece)
{
    switch (piece)
    {
    case CoveKitPiece::Kerb: return CoveKitGroup::Kerbs;
    case CoveKitPiece::RailLevel:
    case CoveKitPiece::Rail26:
    case CoveKitPiece::Rail28:
    case CoveKitPiece::Rail30:
    case CoveKitPiece::RailEndPost: return CoveKitGroup::Rails;
    case CoveKitPiece::Fingerpost: return CoveKitGroup::Fingerposts;
    default: return CoveKitGroup::Steps;   // treads, landing slabs, corner wedges
    }
}

const char* CoveKitGroupName(CoveKitGroup group)
{
    switch (group)
    {
    case CoveKitGroup::Steps: return "CoveSteps";
    case CoveKitGroup::Kerbs: return "CoveKerb";
    case CoveKitGroup::Rails: return "CoveRail";
    case CoveKitGroup::Fingerposts: return "Fingerpost";
    default: return "?";
    }
}

CoveKitGroupsPlaced CoveKitPlaceableGroups(const CoveKitMeshesLoaded& loaded)
{
    CoveKitGroupsPlaced placed;
    placed.fill(true);
    for (std::size_t index = 0; index < loaded.size(); ++index)
        if (!loaded[index]) placed[static_cast<std::size_t>(CoveKitGroupOf(static_cast<CoveKitPiece>(index)))] = false;
    return placed;
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
    // Landings: 0.75 m and 0.6 m slabs laid up the landing from its downhill edge (the long ones first), the
    // combination that stretches least, the run stretched to fit.
    for (const CoveRouteLanding& landing : route.landings)
    {
        int bestLong = 0, bestShort = 1;
        double bestError = 1e300;
        for (int longs = 0; longs <= 4; ++longs)
            for (int shorts = 0; shorts + longs <= 4; ++shorts)
            {
                if (longs + shorts == 0) continue;
                const double error = std::fabs(std::log(landing.length / (longs * CoveKitLandingSlab75Cm + shorts * CoveKitLandingSlabCm)));
                if (error < bestError - 1e-12) { bestError = error; bestLong = longs; bestShort = shorts; }
            }
        const double stretch = landing.length / (bestLong * CoveKitLandingSlab75Cm + bestShort * CoveKitLandingSlabCm);
        double at = 0.0;
        for (int k = 0; k < bestLong + bestShort; ++k)
        {
            const bool isLong = k < bestLong;
            layout.pieces.push_back({isLong ? CoveKitPiece::LandingSlab75 : CoveKitPiece::LandingSlab,
                CoveKitAlong(landing.position, landing.yaw, at), landing.z, landing.yaw, stretch, 1.0, 1.0});
            at += stretch * (isLong ? CoveKitLandingSlab75Cm : CoveKitLandingSlabCm);
        }
    }
    // Corner wedges: close the outside of each turn between a corner landing's uphill end edge and the other
    // leg's first tread. Apex at the edge's inner corner, +X along the edge to the outer corner, opening toward
    // the other leg (+Y, mirrored where that's the edge's -Y), scaled in Y to the turn.
    for (const CoveRouteCorner& corner : route.corners)
    {
        const double a = corner.landingYaw * CoveKitPi / 180.0, b = corner.otherYaw * CoveKitPi / 180.0;
        const Point u{std::cos(a), std::sin(a)}, n{-std::sin(a), std::cos(a)};
        const Point u1{std::cos(b), std::sin(b)}, n1{-std::sin(b), std::cos(b)};
        const double half = 0.5 * CoveRouteTreadWidthCm, reach = CoveRouteCornerHalfCm;
        auto edge = [&](double side) { return Point{corner.position.x + u.x * reach + n.x * half * side, corner.position.y + u.y * reach + n.y * half * side}; };
        auto tread = [&](double side) { return Point{corner.position.x + u1.x * reach + n1.x * half * side, corner.position.y + u1.y * reach + n1.y * half * side}; };
        // The outside of the turn is where the other leg's first tread stands furthest beyond the edge.
        auto gap = [&](double side) { const Point e = edge(side), t = tread(side); return (t.x - e.x) * u.x + (t.y - e.y) * u.y; };
        const double outer = gap(1.0) >= gap(-1.0) ? 1.0 : -1.0;
        const Point apex = edge(-outer), far = edge(outer);
        const double yaw = std::atan2(far.y - apex.y, far.x - apex.x) * 180.0 / CoveKitPi;
        const double yr = yaw * CoveKitPi / 180.0;
        const double opensUp = -std::sin(yr) * u.x + std::cos(yr) * u.y;   // the wedge's +Y . the landing's up
        const double turn = std::acos(std::clamp(u.x * u1.x + u.y * u1.y, -1.0, 1.0));
        const double scaleY = (opensUp >= 0.0 ? 1.0 : -1.0) * std::tan(turn) / std::tan(CoveKitWedgeDegrees * CoveKitPi / 180.0);
        layout.pieces.push_back({CoveKitPiece::LandingWedge, apex, corner.z, yaw, 1.0, scaleY, 1.0});
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
    // Each bay carries a post at its pivot. Its far end needs one too unless another bay's post stands there;
    // two bays meeting end to end (a landing's rail laid from its uphill end) share one end post.
    const size_t firstEndPost = layout.pieces.size();
    for (const CoveRouteRail& rail : route.rails)
    {
        const Point end = CoveKitAlong(rail.position, rail.yaw, rail.length);
        const double endZ = rail.z + rail.length * std::tan(rail.pitch * CoveKitPi / 180.0);
        const auto near = [&](Point p, double z) {
            return std::hypot(p.x - end.x, p.y - end.y) < CoveKitRailJoinCm && std::fabs(z - endZ) < CoveKitRailJoinZCm;
        };
        const bool posted = std::any_of(route.rails.begin(), route.rails.end(), [&](const CoveRouteRail& other) {
            return &other != &rail && near(other.position, other.z);
        });
        const bool shared = std::any_of(layout.pieces.begin() + static_cast<std::ptrdiff_t>(firstEndPost), layout.pieces.end(),
            [&](const CoveKitPlacement& post) { return near(post.position, post.z); });
        if (!posted && !shared) layout.pieces.push_back({CoveKitPiece::RailEndPost, end, endZ, rail.yaw});
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
