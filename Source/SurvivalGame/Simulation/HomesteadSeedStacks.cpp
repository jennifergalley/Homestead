#include "HomesteadSeedStacks.h"
#include "HomesteadPackRow.h"

#include <algorithm>
#include <array>
#include <limits>
#include <set>

namespace Homestead
{
namespace SeedStacks
{
namespace
{
// Folds one layout's single-packet seed groups (the shape older saves wrote) into the first of each crop in
// `order` (layout indices), collecting the retired group ids. Groups of more than one are a stack she
// arranged (a split included), so they are left as saved.
bool Fold(InventoryLayout& layout, const std::vector<int>& order, std::set<int>& retired)
{
    std::array<int, ItemCount> survivor;
    survivor.fill(-1);
    for (const int index : order)
    {
        auto& entry = layout[index];
        if (entry.wearableId != 0 || !IsSeedPacket(entry.item) || entry.quantity != 1) continue;
        int& keep = survivor[static_cast<int>(entry.item)];
        if (keep < 0) { keep = index; continue; }
        if (entry.quantity > std::numeric_limits<int>::max() - layout[keep].quantity) return false;
        layout[keep].quantity += entry.quantity;
        retired.insert(entry.groupId);
        entry.quantity = 0;
    }
    layout.erase(std::remove_if(layout.begin(), layout.end(), [&retired](const LayoutEntry& entry)
        { return entry.wearableId == 0 && retired.count(entry.groupId) != 0; }), layout.end());
    return true;
}
}

bool MergeSavedPackets(State& state)
{
    std::set<int> retired;
    const std::vector<int> order = PackRowRules::FillOrder(state.packRow, state.inventoryLayout);
    if (!Fold(state.inventoryLayout, order, retired)) return false;
    for (auto& piece : state.structures)
    {
        if (piece.kind != Piece::Chest) continue;
        std::vector<int> chestOrder(piece.layout.size());
        for (int index = 0; index < static_cast<int>(chestOrder.size()); ++index) chestOrder[index] = index;
        if (!Fold(piece.layout, chestOrder, retired)) return false;
    }
    if (retired.empty()) return true;

    const auto retire = [&retired](PackRowCell& cell)
    {
        if (cell.groupId != 0 && retired.count(cell.groupId)) cell = {};
    };
    for (auto& cell : state.packRow) retire(cell);
    for (auto& row : state.parkedRows)
        for (auto& cell : row) retire(cell);
    for (auto& cell : state.packSlots) retire(cell);
    if (!state.packSlots.empty()) state.packSlots = PackRowRules::GridCells(state);
    return true;
}
}
}
