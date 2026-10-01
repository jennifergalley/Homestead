#pragma once

#include "HomesteadSimulation.h"

namespace Homestead
{
// She must be within 90 cm of the bed's edge and facing within 30 degrees of it.
constexpr double BedEdgeReachCm = 90.0;
constexpr double BedFacingCosine = 0.8660254037844386;
int ReachableBed(const State& state, Point player, Point facing,
    double edgeReach = BedEdgeReachCm, double coneCosine = BedFacingCosine);
int BedFocusCandidate(const State& state, Point player, Point facing, bool otherFacing, bool heldFocus = false);
}
