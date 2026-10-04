#include "HomesteadFlowerCover.h"

#include <algorithm>
#include <cmath>

namespace Homestead
{
namespace FlowerCoverDetail
{
std::uint64_t CellKey(int x, int y)
{
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) << 32)
        | static_cast<std::uint32_t>(y);
}

bool TouchesPolygon(const std::vector<Point>& ring, Point centre, double radius)
{
    if (PointInPolygon(ring, centre)) return true;
    for (std::size_t i = 0; i < ring.size(); ++i)
    {
        const Point a = ring[i], b = ring[(i + 1) % ring.size()];
        const double dx = b.x - a.x, dy = b.y - a.y;
        const double length2 = dx * dx + dy * dy;
        const double t = length2 > 0.0
            ? std::clamp(((centre.x - a.x) * dx + (centre.y - a.y) * dy) / length2, 0.0, 1.0) : 0.0;
        const double ex = centre.x - (a.x + t * dx), ey = centre.y - (a.y + t * dy);
        if (ex * ex + ey * ey <= radius * radius) return true;
    }
    return false;
}
}

FlowerCoverMask::FlowerCoverMask(const State& state, const EstateLayout& layout)
{
    for (const auto& plot : state.plots)
        plots_.insert(FlowerCoverDetail::CellKey(plot.cellX, plot.cellY));
    for (const char* name : {Anchor::DerelictFarm, Anchor::ManorFootprint})
        if (const auto* polygon = layout.FindPolygon(name))
            landmarks_.push_back(polygon->points);
}

bool FlowerCoverMask::Excludes(Point centre, double radiusCm) const
{
    for (const auto& ring : landmarks_)
        if (FlowerCoverDetail::TouchesPolygon(ring, centre, radiusCm)) return true;
    if (plots_.empty()) return false;
    // Closed square edges: a clump tangent to an upper edge must also inspect the preceding cell.
    const int minX = GardenCell(centre.x - radiusCm) - 1, maxX = GardenCell(centre.x + radiusCm);
    const int minY = GardenCell(centre.y - radiusCm) - 1, maxY = GardenCell(centre.y + radiusCm);
    for (int x = minX; x <= maxX; ++x)
        for (int y = minY; y <= maxY; ++y)
        {
            if (plots_.count(FlowerCoverDetail::CellKey(x, y)) == 0) continue;
            const Point square = GardenCellCenter(x, y);
            const double dx = std::max(0.0, std::abs(centre.x - square.x) - GardenCellSize * 0.5);
            const double dy = std::max(0.0, std::abs(centre.y - square.y) - GardenCellSize * 0.5);
            if (dx * dx + dy * dy <= radiusCm * radiusCm) return true;
        }
    return false;
}
}
