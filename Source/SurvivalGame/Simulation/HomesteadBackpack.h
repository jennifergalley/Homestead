#pragma once

#include "HomesteadShops.h"
#include "HomesteadSimulation.h"

#include <iosfwd>

// The leather backpack: a one-time General Store upgrade that doubles her pack from
// InventoryCapacity to MaxPackCapacity. It isn't an item (it can't be sold, dropped or stored), and
// whether it shows on her back is a separate look choice. Presentation lives in the Unreal module;
// the worn mesh is Props' original asset.
namespace Homestead
{
namespace Backpack
{
// Tentative (round-2.md): above the 1,000-coin start, about thirty cabbage harvests.
constexpr Cents Price = 1500;
constexpr const char* Name = "Leather backpack";
constexpr const char* Description = "A sturdy leather rucksack. Doubles what you can carry.";
static_assert(MaxPackCapacity == 2 * InventoryCapacity, "The backpack doubles her pack.");

// Whether a shop offers it: the General Store, until she owns one.
bool Offered(const State& state, ShopKind kind);

// Optional trailing save section (tag "backpack"): owned and shown. Written only once she owns one
// or has hidden it, so older games save exactly as before.
constexpr const char* SaveTag = "backpack";
bool HasSaveSection(const State& state);
void WriteSaveSection(std::ostream& output, const State& state);
// Reads the section after its tag.
bool ReadSaveSection(std::istream& input, State& state);
}
}
