#pragma once

#include "HomesteadCrops.h"
#include "HomesteadSimulation.h"

// How the pail and its water are presented (a presentation rule only: the Simulation still keeps
// water as ordinary Item::Water stock in the pack, filled by FillWater and spent by Water).
// With her one pail carried, its water shows as a gauge on the pail (0 to PailCapacity) and the
// pack's Water tile is hidden. Anything unusual - no pail, two or more, or more water than one pail
// holds (older saves) - shows the Water tile as ordinary stock so none of it is out of reach.
// Water in a chest or set down is always shown where it is.
namespace Homestead
{
constexpr int PailCapacity = PailPortions;   // HomesteadCrops.h

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

// The pail's charge in words, for the pack's selected-item line and its tooltip: "Water 5 / 15".
inline std::string PailChargeLabel(const PailPresentation& pail)
{
    if (!pail.gauge) return {};
    return "Water " + std::to_string(pail.charge) + " / " + std::to_string(PailCapacity);
}
}
