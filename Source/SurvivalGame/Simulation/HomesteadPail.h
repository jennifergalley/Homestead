#pragma once

#include "HomesteadSimulation.h"

#include <cmath>
#include <string>

// How the pail and its water are presented (a presentation rule only: the Simulation still keeps
// water as ordinary Item::Water stock in the pack, filled by FillWater and spent by Water).
// With her one pail carried, its water shows as a gauge on the pail (0 to PailCapacity) and the
// pack's Water tile is hidden. Anything unusual - no pail, two or more, or more water than one pail
// holds (older saves) - shows the Water tile as ordinary stock so none of it is out of reach.
// Water in a chest or set down is always shown where it is.
namespace Homestead
{
constexpr int PailCapacity = 6;

struct PailPresentation
{
    bool gauge = false;         // A pail slot shows a fill bar.
    int charge = 0;             // What the bar shows, 0 to PailCapacity.
    bool hidePackWater = false; // The pack's Water tile is folded into the pail.
};

inline PailPresentation PresentPail(const State& state)
{
    PailPresentation result;
    const int pails = state.inventory[static_cast<int>(Item::WateringCan)];
    const int water = state.inventory[static_cast<int>(Item::Water)];
    if (pails <= 0) return result;
    result.gauge = true;
    result.charge = water < 0 ? 0 : water > PailCapacity ? PailCapacity : water;
    result.hidePackWater = pails == 1 && water <= PailCapacity;
    return result;
}

// The pail's charge in words, for the pack's selected-item line and its tooltip: "Water 5 / 6".
inline std::string PailChargeLabel(const PailPresentation& pail)
{
    if (!pail.gauge) return {};
    return "Water " + std::to_string(pail.charge) + " / " + std::to_string(PailCapacity);
}

// Standing in the water (Jenny, 2026-09-30: fill the pail in the shallows, not from the bank): where her pail goes in.
// `reachForward`/`reachRight` is her pail's reach from where she stands, at her heading `yaw` (degrees);
// `edgeDistance(point)` is the signed distance to the waterline (cm, negative in the water). Her reach ahead when
// that's at least `inside` cm into the water; otherwise whichever of eight headings reaches deepest, so it never
// goes back onto the bank.
template <typename EdgeDistance>
Point InWaterDipPoint(Point position, double yaw, double reachForward, double reachRight, double inside,
    EdgeDistance&& edgeDistance)
{
    const auto reachAt = [&](double heading) {
        const double radians = heading * 3.14159265358979323846 / 180.0;
        const double c = std::cos(radians), s = std::sin(radians);
        return Point{position.x + reachForward * c - reachRight * s, position.y + reachForward * s + reachRight * c};
    };
    Point best = reachAt(yaw);
    double bestEdge = edgeDistance(best);
    if (bestEdge <= -inside) return best;
    for (int step = 1; step < 8; ++step)
    {
        const Point candidate = reachAt(yaw + step * 45.0);
        const double edge = edgeDistance(candidate);
        if (edge < bestEdge) { best = candidate; bestEdge = edge; }
    }
    return best;
}
}
