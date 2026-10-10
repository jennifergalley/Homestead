#include "HomesteadPlayableBounds.h"
#include "HomesteadEstate.h"

#include <algorithm>
#include <cmath>

namespace Homestead
{
namespace
{
constexpr double RoadEndMinSegmentCm = 1.0; // shorter steps can't give the road a heading
constexpr double RoadEndDegreesPerRadian = 57.29577951308232;
}

PlayableRect EstatePlayableRect(const EstateLayout& layout)
{
    PlayableRect rect;
    const LandmarkPolygon* bounds = layout.FindPolygon(Anchor::PlayableBounds);
    if (!bounds || bounds->points.size() < 3) return rect;
    rect.minX = rect.maxX = bounds->points.front().x;
    rect.minY = rect.maxY = bounds->points.front().y;
    for (const Point& point : bounds->points)
    {
        rect.minX = std::min(rect.minX, point.x);
        rect.maxX = std::max(rect.maxX, point.x);
        rect.minY = std::min(rect.minY, point.y);
        rect.maxY = std::max(rect.maxY, point.y);
    }
    rect.valid = rect.maxX > rect.minX && rect.maxY > rect.minY;
    return rect;
}

RoadEndGate EstateRoadEndGate(const EstateLayout& layout, const std::vector<Point>& roadCentreline)
{
    RoadEndGate gate;
    if (const Landmark* landmark = layout.FindLandmark(RoadEndGateAnchor))
    {
        gate.valid = true;
        gate.fromLandmark = true;
        gate.position = landmark->position;
        gate.yaw = landmark->yaw;
        return gate;
    }
    if (roadCentreline.size() < 2) return gate;
    const Point end = roadCentreline.back();
    for (std::size_t i = roadCentreline.size() - 1; i-- > 0;)
    {
        const double dx = end.x - roadCentreline[i].x, dy = end.y - roadCentreline[i].y;
        const double length = std::hypot(dx, dy);
        if (length < RoadEndMinSegmentCm) continue;
        const double inset = std::min(RoadEndGateInsetCm, length);
        gate.valid = true;
        gate.position = {end.x - dx / length * inset, end.y - dy / length * inset};
        gate.yaw = std::atan2(dy, dx) * RoadEndDegreesPerRadian;
        return gate;
    }
    return gate;
}

bool RoadEndRefusalDue(double nowSeconds, double lastShownSeconds)
{
    return lastShownSeconds < 0.0 || nowSeconds - lastShownSeconds >= RoadEndRefusalCooldownSeconds
        || nowSeconds < lastShownSeconds;
}
}
