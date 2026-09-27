#pragma once

#include "HomesteadSimulation.h"

// Internal helpers HomesteadSimulation.cpp shares with the other Simulation translation units.
namespace Homestead
{
namespace Detail
{
// Records a resource node's cleared/regrowth state by its stable key; false at the edit limit.
bool SaveResourceEdit(State& candidate, const ResourceNode& node);
// Forgets a node's edit, so it returns to its authored or generated state.
void EraseResourceEdit(State& candidate, const Generation::GeneratedEntityKey& key);
// Items and carried garments in her pack.
int PackUsed(const State& state);
// Leaves `quantity` of `item` on the ground at `position`, merging into a nearby matching drop.
// False when the world drop limit or id space is exhausted.
bool AddWorldDrop(State& candidate, Point position, Item item, int quantity);
}
}
