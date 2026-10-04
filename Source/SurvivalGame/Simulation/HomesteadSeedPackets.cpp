#include "HomesteadSeedPackets.h"
#include "HomesteadPackRow.h"

#include <limits>

namespace Homestead
{
namespace SeedPackets
{
bool SplitGroups(State& state, InventoryLayout& layout)
{
    InventoryLayout packets;
    for (auto& entry : layout)
    {
        if (entry.wearableId != 0 || !IsSeedPacket(entry.item) || entry.quantity <= 1) continue;
        const int extras = entry.quantity - 1;
        if (state.nextGroupId <= 0 || state.nextGroupId >= std::numeric_limits<int>::max() - extras - 1)
            return false;
        for (int packet = 0; packet < extras; ++packet)
            packets.push_back({state.nextGroupId++, entry.item, 1, 0});
        entry.quantity = 1;
    }
    layout.insert(layout.end(), packets.begin(), packets.end());
    return true;
}

bool NormalizeSavedGroups(State& state)
{
    if (!SplitGroups(state, state.inventoryLayout)) return false;
    for (auto& piece : state.structures)
        if (piece.kind == Piece::Chest && !SplitGroups(state, piece.layout)) return false;
    if (!state.packSlots.empty()) state.packSlots = PackRowRules::GridCells(state);
    return true;
}
}
}
