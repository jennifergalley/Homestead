#pragma once

#include "HomesteadSimulation.h"

namespace Homestead
{
namespace SeedStacks
{
// Saves from the afternoon build keep every seed packet as a group of one. Same-crop packets stack now
// (one Item per crop), so loading folds each crop's groups, in her pack and in every chest, into one:
// the group first in her pack's fill order (hotbar cells left to right, then below) keeps its identity,
// hotbar cell and square, takes the others' units, and the others' references (cells, squares, parked
// hotbar rows) are retired. Units and totals are unchanged. Returns false only on arithmetic overflow.
bool MergeSavedPackets(State& state);
}
}
