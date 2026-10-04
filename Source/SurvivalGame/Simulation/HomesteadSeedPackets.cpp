#include "HomesteadSeedPackets.h"
#include "HomesteadPackRow.h"

#include <limits>

namespace Homestead
{
namespace SeedPackets
{
std::size_t CountPackets(const State& state)
{
    std::size_t total = 0;
    const auto count = [&total](const InventoryLayout& layout)
    {
        for (const auto& entry : layout)
            if (entry.wearableId == 0 && IsSeedPacket(entry.item) && entry.quantity > 0)
                total += static_cast<std::size_t>(entry.quantity);
    };
    count(state.inventoryLayout);
    for (const auto& piece : state.structures)
        if (piece.kind == Piece::Chest) count(piece.layout);
    return total;
}

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
    if (CountPackets(state) > MaxPacketGroups) return false;
    if (!SplitGroups(state, state.inventoryLayout)) return false;
    for (auto& piece : state.structures)
        if (piece.kind == Piece::Chest && !SplitGroups(state, piece.layout)) return false;
    if (!state.packSlots.empty()) state.packSlots = PackRowRules::GridCells(state);
    return true;
}
}
}
