#include "HomesteadBed.h"
#include "HomesteadFood.h"

#include <algorithm>
#include <cmath>

namespace Homestead
{
std::optional<SleepOption> BedSleepOption(double hour, double energy, double sunrise, double sunset)
{
    if (!std::isfinite(hour) || !std::isfinite(energy) || hour < 0.0 || energy < 0.0 || energy > 100.0
        || !std::isfinite(sunrise) || !std::isfinite(sunset) || sunrise <= 0.0 || sunset >= 24.0
        || sunrise >= sunset)
        return std::nullopt;
    const double morning = std::min(Daylight::MorningSleepHour, sunrise);
    const double evening = std::min(Daylight::EveningSleepHour, sunset);
    double current = std::fmod(hour, 24.0);
    if (std::abs(current - morning) < Exertion::MinDawnSleepHours) current = morning;
    if (current >= evening || current < morning)
    {
        const double hours = std::fmod(morning - current + 48.0, 24.0);
        if (hours < Exertion::MinDawnSleepHours) return std::nullopt;
        return SleepOption{SleepChoice::UntilMorning, hours, morning};
    }
    if (energy >= Food::FullEnergyAt) return std::nullopt;
    const double hours = std::clamp(std::ceil((100.0 - energy) / Exertion::SleepPerHour * 4.0 - 1e-9) / 4.0,
        Exertion::MinRestHours, Exertion::MaxRestHours);
    return SleepOption{SleepChoice::UntilRested, hours, std::fmod(current + hours, 24.0)};
}

namespace BedAim
{
constexpr double CoarseReachCm = 300.0; // The 90 cm edge reach plus the bed's offset and half extents.
constexpr double FocusGraceCm = 5.0;
constexpr double FocusGraceCosine = 0.8386705679454240; // 33 degrees while retaining focus.
bool RayIntersects(const Footprint& box, Point origin, Point direction)
{
    double first = 0.0, last = 1e30;
    const double coordinates[] = {origin.x, origin.y};
    const double axes[] = {direction.x, direction.y};
    const double extents[] = {box.half.x, box.half.y};
    for (int axis = 0; axis < 2; ++axis)
    {
        if (std::abs(axes[axis]) < 1e-9)
        {
            if (std::abs(coordinates[axis]) > extents[axis]) return false;
            continue;
        }
        double entryT = (-extents[axis] - coordinates[axis]) / axes[axis];
        double exitT = (extents[axis] - coordinates[axis]) / axes[axis];
        if (entryT > exitT) std::swap(entryT, exitT);
        first = std::max(first, entryT);
        last = std::min(last, exitT);
        if (first > last) return false;
    }
    return true;
}
}

int ReachableBed(const State& state, Point player, Point facing, double edgeReach, double coneCosine)
{
    if (!std::isfinite(player.x) || !std::isfinite(player.y)
        || !std::isfinite(facing.x) || !std::isfinite(facing.y)) return -1;
    const double facingLength = std::hypot(facing.x, facing.y);
    if (facingLength < 1e-6) return -1;

    int nearest = -1;
    double best = edgeReach;
    for (const Structure& structure : state.structures)
    {
        if (structure.kind != Piece::Bed) continue;
        const Point cell = StructureCenter(state, structure);
        if (std::hypot(player.x - cell.x, player.y - cell.y) > BedAim::CoarseReachCm) continue;
        const Footprint box = StructureFootprint(state, structure);
        const Point local = RotateYaw({player.x - box.center.x, player.y - box.center.y}, -box.yaw);
        const double outside = std::hypot(
            std::max(0.0, std::abs(local.x) - box.half.x),
            std::max(0.0, std::abs(local.y) - box.half.y));
        if (outside > best) continue;
        const Point localFacing = RotateYaw(facing, -box.yaw);
        const auto inCone = [&](Point target)
        {
            const Point toward{target.x - local.x, target.y - local.y};
            const double length = std::hypot(toward.x, toward.y);
            return length < 1e-6
                || localFacing.x * toward.x + localFacing.y * toward.y >= coneCosine * facingLength * length;
        };
        bool aiming = BedAim::RayIntersects(box, local, localFacing)
            || inCone({std::clamp(local.x, -box.half.x, box.half.x),
                std::clamp(local.y, -box.half.y, box.half.y)});
        for (double x : {-box.half.x, box.half.x})
            for (double y : {-box.half.y, box.half.y})
                aiming |= inCone({x, y});
        if (aiming) { best = outside; nearest = structure.id; }
    }
    return nearest;
}

int BedFocusCandidate(const State& state, Point player, Point facing, bool otherFacing, bool heldFocus)
{
    // Keeping a previously focused bed gets 5 cm / 3 degrees of tolerance against frame-to-frame flicker.
    return otherFacing ? -1 : heldFocus
        ? ReachableBed(state, player, facing, BedEdgeReachCm + BedAim::FocusGraceCm, BedAim::FocusGraceCosine)
        : ReachableBed(state, player, facing);
}
}
