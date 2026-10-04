#pragma once

#include "HomesteadSimulation.h"

namespace Homestead
{
namespace SeedPackets
{
// Preserve the original group and its saved slot, allocating the other packets new identities.
bool SplitGroups(State& state, InventoryLayout& layout);
bool NormalizeSavedGroups(State& state);
}
}
