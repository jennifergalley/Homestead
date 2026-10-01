#include "HomesteadBed.h"

#include <algorithm>
#include <cmath>

namespace Homestead
{
int ReachableBed(const State& state, Point player, Point facing)
{
    if (!std::isfinite(player.x) || !std::isfinite(player.y)
        || !std::isfinite(facing.x) || !std::isfinite(facing.y)) return -1;
    const double facingLength = std::hypot(facing.x, facing.y);
    if (facingLength < 1e-6) return -1;

    int nearest = -1;
    double best = BedEdgeReachCm;
    for (const Structure& structure : state.structures)
    {
        if (structure.kind != Piece::Bed) continue;
        const Footprint box = StructureFootprint(state, structure);
        const Point local = RotateYaw({player.x - box.center.x, player.y - box.center.y}, -box.yaw);
        const Point edge = RotateYaw({
            std::clamp(local.x, -box.half.x, box.half.x),
            std::clamp(local.y, -box.half.y, box.half.y)}, box.yaw);
        Point toward = {box.center.x + edge.x - player.x, box.center.y + edge.y - player.y};
        double distance = std::hypot(toward.x, toward.y);
        if (distance < 1e-6)
        {
            toward = {box.center.x - player.x, box.center.y - player.y};
            distance = std::hypot(toward.x, toward.y);
        }
        if (distance > 1e-6 && (facing.x * toward.x + facing.y * toward.y)
            < 0.8660254037844386 * facingLength * distance) continue;
        const double outside = std::hypot(
            std::max(0.0, std::abs(local.x) - box.half.x),
            std::max(0.0, std::abs(local.y) - box.half.y));
        if (outside <= best) { best = outside; nearest = structure.id; }
    }
    return nearest;
}

int BedFocusCandidate(const State& state, Point player, Point facing, bool otherFocus)
{
    return otherFocus ? -1 : ReachableBed(state, player, facing);
}
}
