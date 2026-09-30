#include "HomesteadDoor.h"

#include <algorithm>
#include <cmath>

namespace Homestead
{
namespace Door
{
namespace
{
Point Offset(Point origin, Point local, double yaw)
{
    const Point turned = RotateYaw(local, yaw);
    return {origin.x + turned.x, origin.y + turned.y};
}
}

bool HasLeaf(const Structure& piece)
{
    return piece.kind == Piece::Doorway && piece.heritage && piece.skin == StructureSkin::Stone;
}

Leaf LeafFor(const State& state, const Structure& piece)
{
    const Point centre = StructureCenter(state, piece);
    const double yaw = StructureYaw(state, piece);
    Leaf leaf;
    leaf.hinge = Offset(centre, {-OpeningHalfWidthCm, HingeYCm}, yaw);
    leaf.closedYaw = yaw;
    leaf.opening = Offset(centre, {0.0, OuterFaceCm}, yaw);
    leaf.outward = RotateYaw({0.0, 1.0}, yaw);
    return leaf;
}

double LeafYaw(const Leaf& leaf, double angleDeg) { return leaf.closedYaw + angleDeg; }

Point LatchEdge(const Leaf& leaf, double angleDeg)
{
    return Offset(leaf.hinge, {LeafWidthCm, -LeafThicknessCm * 0.5}, LeafYaw(leaf, angleDeg));
}

bool WantsOpen(bool wasWanted, double distanceCm)
{
    return distanceCm < (wasWanted ? CloseBeyondCm : OpenWithinCm);
}

double Step(double openness, bool wanted, double seconds)
{
    const double rate = seconds / (wanted ? OpenSeconds : CloseSeconds);
    return std::clamp(openness + (wanted ? rate : -rate), 0.0, 1.0);
}

double Angle(double openness)
{
    const double t = std::clamp(openness, 0.0, 1.0);
    return MaxOpenDeg * t * t * (3.0 - 2.0 * t);
}
}
}
