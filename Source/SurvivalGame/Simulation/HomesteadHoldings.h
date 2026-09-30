#pragma once

#include "HomesteadSimulation.h"

// What she holds, for the "+3 Berries" pickup line (UI/SHomesteadPickups): a presentation rule only.
// A gain raises both her pack and everything she owns (pack, chests and things set down); taking
// from a chest or picking her own drop back up raises only the pack, so moves never count.
namespace Homestead
{
struct Holdings
{
    Inventory pack{};
    Inventory owned{};
};

inline Holdings CountHoldings(const State& state)
{
    Holdings result;
    result.pack = state.inventory;
    result.owned = state.inventory;
    for (const auto& piece : state.structures)
        for (int index = 0; index < ItemCount; ++index) result.owned[index] += piece.storage[index];
    for (const auto& drop : state.worldDrops)
        if (static_cast<int>(drop.item) >= 0 && static_cast<int>(drop.item) < ItemCount)
            result.owned[static_cast<int>(drop.item)] += drop.quantity;
    return result;
}

// How many of `item` she has newly gained between two counts; 0 for moves, spends and losses.
// Pail water shows on the pail's gauge (HomesteadPail.h), never as a pickup.
inline int PickupGain(const Holdings& before, const Holdings& after, Item item)
{
    const int index = static_cast<int>(item);
    if (index < 0 || index >= ItemCount || item == Item::Water) return 0;
    const int pack = after.pack[index] - before.pack[index];
    const int owned = after.owned[index] - before.owned[index];
    const int gain = pack < owned ? pack : owned;
    return gain > 0 ? gain : 0;
}
}
