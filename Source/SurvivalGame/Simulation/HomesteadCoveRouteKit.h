#pragma once

#include "HomesteadEstateCoveRoute.h"

#include <cstdint>
#include <vector>

// Props' add-cove-route-kit pieces placed along the cove route (HomesteadEstateCoveRoute): where each tread,
// landing slab, kerb, rail bay and fingerpost goes, in the kit's pivots, and the pawn-only blockers that stand
// on every railed edge (the rail meshes carry no collision). Plain C++ so the layout is native-tested; the
// runtime builder (HomesteadWorldCoveRoute.cpp) maps each piece to its mesh.
//
// World frame as HomesteadEstate.h: Unreal centimetres, +X north, +Y east; yaw in degrees from +X toward +Y.
namespace Homestead
{
enum class CoveKitPiece : std::uint8_t
{
    StepA, StepB, StepC,        // SM_CoveStep_A/B/C: one tread block; pivot on its top face at the front nosing
    LandingSlab,                // SM_CoveLandingSlab: 0.6 m; pivot on its top face at the downhill edge's centre
    Kerb,                       // SM_CoveKerb_Straight: 1 m; pivot on the path-side top edge, +Y to the drop
    RailLevel, Rail26, Rail28, Rail30,  // SM_CoveRail_*: a 1.70 m bay; pivot at the downhill post's foot
    Fingerpost,                 // SM_Fingerpost_ToTheCove: pivot at the post's foot, arm along +X
    Count
};

struct CoveKitPlacement
{
    CoveKitPiece piece = CoveKitPiece::StepA;
    Point position;             // the pivot (cm)
    double z = 0.0;             // the pivot (cm)
    double yaw = 0.0;           // degrees
    double scaleX = 1.0;        // a landing slab's stretch along X; a rail bay's plan length / 170 cm
    double scaleY = 1.0;        // -1: a mirrored rail bay (the drop on its -Y)
    double scaleZ = 1.0;        // a rail bay scales X and Z together so its pitch holds
};

// A pawn-only box along a railed edge (camera and visibility ignore it; no step-up).
struct CoveKitBlocker
{
    Point centre;               // cm
    double z = 0.0;             // centre (cm)
    double yaw = 0.0;
    double halfLength = 0.0;    // along yaw
    double halfThickness = 0.0;
    double halfHeight = 0.0;
};

struct CoveKitLayout
{
    std::vector<CoveKitPlacement> pieces;
    std::vector<CoveKitBlocker> blockers;
    int Count(CoveKitPiece piece) const;
};

constexpr double CoveKitLandingSlabCm = 60.0;   // cove_steps.py
constexpr double CoveKitRailBayCm = 170.0;      // cove_handrail.py BAY, in plan
constexpr double CoveKitBlockerThicknessCm = 10.0;
constexpr double CoveKitBlockerAboveCm = 120.0; // above the rail's pivot (the top rail is at 95 cm)
constexpr double CoveKitBlockerBelowCm = 20.0;

// The layout for a route (EstateCoveRoute() by default).
CoveKitLayout BuildCoveKitLayout(const CoveRoute& route);
const CoveKitLayout& EstateCoveRouteKit();
}
