#pragma once

#include "HomesteadCrops.h"
#include "HomesteadItems.h"
#include "HomesteadOvergrowth.h"
#include "HomesteadSimulation.h"

#include <array>
#include <string>
#include <vector>

// The old pinned hotbar, kept only to migrate saves from before the hotbar became the first row of
// her pack (HomesteadPackRow.h). Those saves stored which item each of the ten slots pointed at
// (UHomesteadSave::HotbarSlots, layout 3 or older); SanitizeHotbarLayout makes that list safe, and
// the game then moves her first carried stack of each item into that cell once
// (Simulation::ArrangePackRow). Slot 9 is the key labelled 0.
namespace Homestead
{
constexpr int HotbarSize = 10;
constexpr int HotbarEmpty = -1;
using HotbarLayout = std::array<int, HotbarSize>;
static_assert(HotbarSize == PackRowSize, "The old pinned slots map one to one onto the pack row's cells.");

// The old pinned hotbar took tools (and the lamp), food and seed, never materials.
inline bool CanPinToHotbar(Item item)
{
    if (static_cast<int>(item) < 0 || static_cast<int>(item) >= ItemCount) return false;
    return ToolForItem(item) != ToolKind::Count || item == Item::OilLamp || IsEdible(item) || CropForSeed(item) != nullptr;
}

inline std::string HotbarKeyLabel(int slot) { return slot == 9 ? "0" : std::to_string(slot + 1); }

// The hotbar as a save stores it (any length, item values or -1), made safe to use: out-of-range,
// unpinnable and repeated items become empty slots, and older layouts gain the things later versions
// added once, in free slots. `layout` is the save's HotbarLayout; 3 was the last pinned one.
inline HotbarLayout SanitizeHotbarLayout(const std::vector<int>& saved, int layout)
{
    HotbarLayout slots;
    slots.fill(HotbarEmpty);
    const auto Pinned = [&slots](int value)
    {
        for (int slot : slots) if (slot == value) return true;
        return false;
    };
    for (int index = 0; index < HotbarSize && index < static_cast<int>(saved.size()); ++index)
    {
        const int value = saved[index];
        if (value >= 0 && value < ItemCount && CanPinToHotbar(static_cast<Item>(value)) && !Pinned(value))
            slots[index] = value;
    }
    // Layout 3 added the oil lamp, in the first free slot from 8.
    if (layout < 3 && !Pinned(static_cast<int>(Item::OilLamp)))
        for (int step = 0; step < HotbarSize; ++step)
            if (const int index = (step + 7) % HotbarSize; slots[index] == HotbarEmpty)
            {
                slots[index] = static_cast<int>(Item::OilLamp);
                break;
            }
    // Layout 2 added the estate tools and pinned food.
    if (layout < 2)
        for (const Item item : {Item::Billhook, Item::Scythe, Item::Pickaxe, Item::Berries})
        {
            if (Pinned(static_cast<int>(item))) continue;
            for (int& slot : slots)
                if (slot == HotbarEmpty) { slot = static_cast<int>(item); break; }
        }
    return slots;
}
}
