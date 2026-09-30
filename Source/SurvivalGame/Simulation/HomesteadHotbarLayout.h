#pragma once

#include "HomesteadCrops.h"
#include "HomesteadItems.h"
#include "HomesteadOvergrowth.h"
#include "HomesteadSimulation.h"

#include <array>
#include <string>
#include <vector>

// Which item each of her ten hotbar slots points at, and how she rearranges them in the field book.
// A binding is only a pointer to a kind of thing she carries: editing it never moves, spends or
// creates stock, so none of this touches the Simulation's state or revision. An item is bound to at
// most one slot. Slot 9 is the key labelled 0.
namespace Homestead
{
constexpr int HotbarSize = 10;
constexpr int HotbarEmpty = -1;
using HotbarLayout = std::array<int, HotbarSize>;

// Tools (and the lamp), food and seed can ride on the hotbar; materials can't.
inline bool CanPinToHotbar(Item item)
{
    if (static_cast<int>(item) < 0 || static_cast<int>(item) >= ItemCount) return false;
    return ToolForItem(item) != ToolKind::Count || item == Item::OilLamp || IsEdible(item) || CropForSeed(item) != nullptr;
}

inline int HotbarSlotOf(const HotbarLayout& slots, Item item)
{
    for (int index = 0; index < HotbarSize; ++index)
        if (slots[index] == static_cast<int>(item)) return index;
    return HotbarEmpty;
}

inline std::string HotbarKeyLabel(int slot) { return slot == 9 ? "0" : std::to_string(slot + 1); }

enum class HotbarEditCode { Changed, Unchanged, BadSlot, NotPinnable, NotInPack, EmptySource };

struct HotbarEdit
{
    HotbarEditCode code = HotbarEditCode::Unchanged;
    std::string message;           // Why it was refused; empty for a change or a no-op.
    int unpinned = HotbarEmpty;    // An item that lost its slot to this one (inventory -> occupied slot).
    bool Refused() const { return code != HotbarEditCode::Changed && code != HotbarEditCode::Unchanged; }
};

// Dragging a stack from her pack onto a slot. An item already on the hotbar moves there, and whatever
// held the slot swaps back into the slot it left; otherwise the slot's old item is unpinned (it stays
// in her pack). Things in a chest have to come into her pack first.
inline HotbarEdit AssignHotbarSlot(HotbarLayout& slots, Item item, int target, bool inPack)
{
    HotbarEdit edit;
    if (target < 0 || target >= HotbarSize) { edit.code = HotbarEditCode::BadSlot; edit.message = "Choose one of the ten hotbar slots."; return edit; }
    if (!CanPinToHotbar(item)) { edit.code = HotbarEditCode::NotPinnable; edit.message = "Only tools, food and seeds can go on the hotbar."; return edit; }
    if (!inPack) { edit.code = HotbarEditCode::NotInPack; edit.message = "Take it to your pack first, then put it on the hotbar."; return edit; }
    const int value = static_cast<int>(item);
    if (slots[target] == value) return edit;
    const int current = HotbarSlotOf(slots, item);
    if (current != HotbarEmpty) slots[current] = slots[target];
    else edit.unpinned = slots[target];
    slots[target] = value;
    edit.code = HotbarEditCode::Changed;
    return edit;
}

// Dragging one hotbar slot onto another: into an empty slot it moves (the old slot clears); onto a
// used one the two swap.
inline HotbarEdit MoveHotbarSlot(HotbarLayout& slots, int from, int to)
{
    HotbarEdit edit;
    if (from < 0 || from >= HotbarSize || to < 0 || to >= HotbarSize) { edit.code = HotbarEditCode::BadSlot; edit.message = "Choose one of the ten hotbar slots."; return edit; }
    if (slots[from] == HotbarEmpty) { edit.code = HotbarEditCode::EmptySource; edit.message = "That slot is empty."; return edit; }
    if (from == to) return edit;
    const int moving = slots[from];
    slots[from] = slots[to];
    slots[to] = moving;
    edit.code = HotbarEditCode::Changed;
    return edit;
}

// The hotbar as a save stores it (any length, item values or -1), made safe to use: out-of-range,
// unpinnable and repeated items become empty slots, and older layouts gain the things later versions
// added once, in free slots. `layout` is the save's HotbarLayout; 3 is current.
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
