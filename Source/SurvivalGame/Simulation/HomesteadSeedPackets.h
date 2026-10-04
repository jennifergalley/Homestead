#pragma once

#include "HomesteadSimulation.h"

namespace Homestead
{
namespace SeedPackets
{
// Eight full seed chests and an upgraded pack already exceed realistic estate playtests.
constexpr std::size_t MaxPacketGroups = 8 * ChestCapacity + MaxPackCapacity;
std::size_t CountPackets(const State& state);
// Preserve the original group and its saved slot, allocating the other packets new identities.
bool SplitGroups(State& state, InventoryLayout& layout);
bool NormalizeSavedGroups(State& state);
}
}
