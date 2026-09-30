#pragma once

#include "HomesteadSimulation.h"

// The standing room's door (Jenny, 2026-09-29): an oak leaf in the heritage stone doorway that swings
// out when she comes near and shuts behind her. Presentation only: its swing isn't game state, nothing
// is saved, and it never blocks her (AHomesteadWorld gives it no pawn collision). The geometry matches
// SM_StoneDoorway (Scripts/Blender/Recipes/stone_doorway.py) in the piece's own frame: opening
// X -65..65 cm, wall faces at Y 130 (in) and 158 (out); +Y points out of the room.
namespace Homestead
{
namespace Door
{
constexpr double OpeningHalfWidthCm = 65.0;
constexpr double OuterFaceCm = 158.0;
// The hinge pin stands on the -X jamb, 3 cm in from the outer face, so the shut leaf sits in the reveal.
constexpr double HingeYCm = OuterFaceCm - 3.0;
constexpr double LeafWidthCm = 129.0;            // 1 cm short of the jamb it shuts against
constexpr double LeafThicknessCm = 5.0;
constexpr double MaxOpenDeg = 100.0;             // swung out past square, back towards the outside wall
// She is near when within OpenWithinCm of the opening; it waits until she is CloseBeyondCm away to shut.
constexpr double OpenWithinCm = 220.0;
constexpr double CloseBeyondCm = 300.0;
constexpr double OpenSeconds = 0.9;
constexpr double CloseSeconds = 1.4;

struct Leaf
{
    Point hinge;              // world XY of the hinge pin (cm)
    double closedYaw = 0.0;   // Unreal yaw of the leaf, hinge towards latch, when shut
    Point opening;            // centre of the opening at the outer face
    Point outward;            // unit vector out of the room
};

// Heritage stone doorways (the standing room's) get a leaf; a timber doorway keeps its tied-back cloth.
bool HasLeaf(const Structure& piece);
Leaf LeafFor(const State& state, const Structure& piece);
// The leaf's yaw when swung `angleDeg` open (positive swings it outward).
double LeafYaw(const Leaf& leaf, double angleDeg);
// World XY of the latch edge's midline at that swing.
Point LatchEdge(const Leaf& leaf, double angleDeg);
// Whether she's near enough for it to be open, with hysteresis between OpenWithinCm and CloseBeyondCm.
bool WantsOpen(bool wasWanted, double distanceCm);
// Openness 0 (shut) .. 1 (fully open), moved towards `wanted` at the open or close rate.
double Step(double openness, bool wanted, double seconds);
// Swing angle for an openness, eased at both ends (degrees, 0 .. MaxOpenDeg).
double Angle(double openness);
}
}
