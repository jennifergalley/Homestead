#pragma once

#include "HomesteadSimulation.h"

#include <vector>

// The playable area's edge (shrink-estate-map decision 7): the layout's PlayableBounds rectangle is the map
// sheet and the world's edge, and the public road ends inside it at a field gate she can't pass. The world
// spawns pawn-only walls along the rectangle and the gate (HomesteadWorldEdge.cpp, HomesteadRoadEndGate.cpp).
namespace Homestead
{
struct EstateLayout;

// Optional layout landmark for the field gate across the road's far end; its yaw is the road heading out.
constexpr const char* RoadEndGateAnchor = "RoadEndGate";
// Walking into the gate (agreed with Balance). UTF-8: the dash is an en dash.
constexpr const char* RoadEndRefusalMessage = "The road to Marlbury \xE2\x80\x93 another day.";
// The refusal shows at most this often, s: long enough that leaning on the gate doesn't repeat it.
constexpr double RoadEndRefusalCooldownSeconds = 12.0;
// Without the landmark the gate stands this far back along the road from its last centreline point, cm,
// so its posts are on the graded road bed rather than past its end.
constexpr double RoadEndGateInsetCm = 150.0;

struct PlayableRect
{
    bool valid = false;
    double minX = 0.0;
    double minY = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
    bool Contains(Point point) const
    {
        return valid && point.x >= minX && point.x <= maxX && point.y >= minY && point.y <= maxY;
    }
};

// The bounding box of the layout's PlayableBounds polygon; invalid when it's missing or has no area.
PlayableRect EstatePlayableRect(const EstateLayout& layout);

struct RoadEndGate
{
    bool valid = false;
    bool fromLandmark = false;
    Point position;      // the middle of the gate, on the road's centreline (cm)
    double yaw = 0.0;    // the road's heading through the gate, out toward Marlbury (Unreal yaw, degrees)
};

// The RoadEndGate landmark when the layout has one, else RoadEndGateInsetCm back from the centreline's last
// point, facing along its last segment. Invalid without a landmark or two distinct road points.
RoadEndGate EstateRoadEndGate(const EstateLayout& layout, const std::vector<Point>& roadCentreline);

// Whether the refusal may show at `nowSeconds` when it last showed at `lastShownSeconds` (negative: never).
bool RoadEndRefusalDue(double nowSeconds, double lastShownSeconds);
}
