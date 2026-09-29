#include "HomesteadEstate.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <tuple>
#include <utility>

// The derelict farm and the neglected grounds (add-derelict-farm-and-estate-disrepair): clearable
// overgrowth rows 550000+ in ProvisionalEstatePlacements. The Unreal set dressing (fence, ridges,
// plough, debris) is AHomesteadDerelictFarm and AHomesteadManorRuin; neither has Simulation state.
namespace Homestead
{
namespace
{
// Stable 0..1 noise from integers, identical on every compiler (no <random> distributions).
double DisrepairHash01(int a, int b, int salt)
{
    std::uint32_t h = static_cast<std::uint32_t>(a) * 73856093u ^ static_cast<std::uint32_t>(b) * 19349663u
        ^ static_cast<std::uint32_t>(salt) * 83492791u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return static_cast<double>(h & 0xFFFFFFu) / 16777216.0;
}

struct DisrepairPicker
{
    EstatePlacements& table;
    int next = 550001;
    // Keep new nodes this far from every placement already in the table, so each keeps its focus.
    double spacing = 150.0;

    bool Clear(Point at, double gap) const
    {
        // Pickable brambles (540000+) keep 3 m to themselves, so they stay easy to find and pick.
        for (const auto& placement : table.placements)
            if (std::hypot(placement.position.x - at.x, placement.position.y - at.y)
                < (placement.kind == ResourceKind::BerryBush && placement.id >= 540000 && placement.id < 550000 ? std::max(gap, 300.0) : gap))
                return false;
        return true;
    }
    bool Add(ResourceKind kind, Point at, double gap = 0.0)
    {
        if (!Clear(at, gap > 0.0 ? gap : spacing)) return false;
        table.placements.push_back({next++, kind, at, 0.0, 0.0, 1.0, 0});
        return true;
    }
};
}

DerelictFarmPlan EstateDerelictFarm(const EstateLayout& layout)
{
    DerelictFarmPlan plan;
    const LandmarkPolygon* field = layout.FindPolygon(Anchor::DerelictFarm);
    if (!field || field->points.size() < 3) return plan;
    double minX = field->points.front().x, maxX = minX, minY = field->points.front().y, maxY = minY;
    for (const Point& corner : field->points)
    {
        minX = std::min(minX, corner.x);
        maxX = std::max(maxX, corner.x);
        minY = std::min(minY, corner.y);
        maxY = std::max(maxY, corner.y);
    }
    plan.valid = maxX - minX > 1000.0 && maxY - minY > 1000.0;
    plan.southWest = {minX, minY};
    plan.lengthU = maxX - minX;
    plan.lengthV = maxY - minY;
    const Point gate = layout.PointOr(Anchor::DerelictFarmGate, {minX, (minY + maxY) * 0.5});
    plan.gateV = std::clamp(gate.y - minY, plan.gateWidth, plan.lengthV - plan.gateWidth);
    // Two blocks of old ridges: the main west block and a narrower strip east of the gate path.
    const double u = plan.lengthU, v = plan.lengthV;
    plan.ridgeBlocks = {{u * 0.13, v * 0.10, u * 0.78, v * 0.57}, {u * 0.23, v * 0.66, u * 0.72, v * 0.90}};
    plan.plough = {u * 0.52, v * 0.38};
    return plan;
}

void AppendDerelictFarmAndDisrepair(EstatePlacements& table)
{
    DisrepairPicker pick{table};
    const DerelictFarmPlan farm = EstateDerelictFarm();
    if (farm.valid)
    {
        const double U = farm.lengthU, V = farm.lengthV;
        const auto inRidges = [&](double u, double v)
        {
            for (const auto& block : farm.ridgeBlocks)
                if (u >= block[0] && u <= block[2] && v >= block[1] && v <= block[3]) return true;
            return false;
        };
        const auto clearOfPlough = [&](double u, double v)
        { return std::hypot(u - farm.plough.x, v - farm.plough.y) > 260.0; };
        // The broken gate is choked with bramble, with a nettle-bed of weeds just inside.
        for (const auto& [kind, du, dv] : std::initializer_list<std::tuple<ResourceKind, double, double>>{
                 {ResourceKind::BrambleThin, -120.0, -60.0}, {ResourceKind::BrambleThin, 90.0, 120.0},
                 {ResourceKind::BrambleThin, 60.0, -190.0}, {ResourceKind::Weeds, 260.0, 30.0}})
            pick.Add(kind, farm.World(du, farm.gateV + dv), 120.0);
        // Old bramble banks in the far corners (steel-tier teases) and a thicket in the south-west.
        pick.Add(ResourceKind::BrambleBank, farm.World(U - 330.0, 330.0), 300.0);
        pick.Add(ResourceKind::BrambleBank, farm.World(U - 330.0, V - 330.0), 300.0);
        pick.Add(ResourceKind::BrambleThicket, farm.World(280.0, 300.0), 250.0);
        pick.Add(ResourceKind::BrambleThicket, farm.World(U * 0.5, V - 250.0), 250.0);
        // Bramble, thorn and elder seedlings have run along the whole fence line, both sides.
        const double perimeter = 2.0 * (U + V);
        int step = 0;
        for (double s = 190.0; s < perimeter; s += 380.0, ++step)
        {
            double u, v, inU, inV;
            if (s < V) { u = 0.0; v = s; inU = 1.0; inV = 0.0; }
            else if (s < V + U) { u = s - V; v = V; inU = 0.0; inV = -1.0; }
            else if (s < 2.0 * V + U) { u = U; v = V - (s - V - U); inU = -1.0; inV = 0.0; }
            else { u = U - (s - 2.0 * V - U); v = 0.0; inU = 0.0; inV = 1.0; }
            if (u == 0.0 && std::abs(v - farm.gateV) < farm.gateWidth) continue;
            if (DisrepairHash01(step, 1, 11) > 0.62) continue;
            const double roll = DisrepairHash01(step, 2, 11);
            const ResourceKind kind = roll < 0.70 ? ResourceKind::BrambleThin
                : roll < 0.86 ? ResourceKind::BrambleThicket : ResourceKind::Sapling;
            const double side = DisrepairHash01(step, 3, 11) < 0.62 ? 1.0 : -1.0;
            const double off = 100.0 + 70.0 * DisrepairHash01(step, 4, 11);
            pick.Add(kind, farm.World(u + inU * off * side, v + inV * off * side));
        }
        // The field itself: weeds, docks and rank grass over the old ridges; bramble and saplings
        // crowding the uncultivated headlands.
        const double cell = 430.0;
        const int count = static_cast<int>((std::min(U, V) - 520.0) / cell) + 1;
        for (int i = 0; i < count; ++i)
            for (int j = 0; j < count; ++j)
            {
                const double u = 260.0 + i * cell + (DisrepairHash01(i, j, 21) - 0.5) * 280.0;
                const double v = 260.0 + j * cell + (DisrepairHash01(i, j, 22) - 0.5) * 280.0;
                if (u < 150.0 || v < 150.0 || u > U - 150.0 || v > V - 150.0 || !clearOfPlough(u, v)) continue;
                const double roll = DisrepairHash01(i, j, 23);
                ResourceKind kind;
                if (inRidges(u, v))
                {
                    if (roll < 0.15) continue;
                    kind = roll < 0.55 ? ResourceKind::Weeds : roll < 0.85 ? ResourceKind::TallGrass : ResourceKind::BrambleThin;
                }
                else
                {
                    if (roll < 0.12) continue;
                    kind = roll < 0.44 ? ResourceKind::BrambleThin : roll < 0.62 ? ResourceKind::Weeds
                        : roll < 0.80 ? ResourceKind::TallGrass : roll < 0.93 ? ResourceKind::Sapling
                        : ResourceKind::BrambleThicket;
                }
                pick.Add(kind, farm.World(u, v));
            }
    }
    // The neglected grounds round the manor, between it and the field, and along the drive's verges
    // (generated off the terrain and road by Scripts/Terrain/estate_disrepair.py, clear of the drive,
    // the ruin's gaps and every earlier placement).
    for (const auto& [kind, at] : std::initializer_list<std::pair<ResourceKind, Point>>{
#include "HomesteadEstateDisrepairPlacements.inc"
         })
        pick.Add(kind, at, 120.0);
}
}
