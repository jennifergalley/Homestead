#pragma once

#include "HomesteadSimulation.h"

#include <iosfwd>
#include <vector>

// The hotbar is the first row of her pack, as in Coral Island: ten cells (keys 1-0) holding real
// carried stacks and garments, not pins. State::packRow names which of her pack's layout entries
// sits in each cell; every other carried entry is "below the row", in layout order.
//
// - A new stack or garment arriving in her pack takes the first empty cell, else goes below.
//   (The pail's water never does: it shows on the pail, HomesteadPail.h.)
// - Gains top up an existing stack, and uses draw from one, row cells first (left to right), then
//   the stacks below in order.
// - A stack used up leaves its cell empty; nothing is held for it.
// - Moving onto an empty hotbar cell moves, onto the same item merges, onto anything else swaps.
// - Below the row her pack is a grid of squares (State::packSlots): once she drops something on a
//   square, every stack keeps its square, gaps included, through saves; new stacks take the first gap.
//   Dropping onto an occupied square swaps the two entries, even for matching items.
// - Sort Pack sorts only below the row, and packs the grid again.
namespace Homestead
{
namespace PackRowRules
{
// Optional trailing save section (tag "packrow"). Saves without it (from before the row) load with
// an empty row; the game arranges it once from the save's old pinned hotbar (ArrangePackRow).
constexpr const char* SaveTag = "packrow";
void WriteSaveSection(std::ostream& output, const State& state);
bool ReadSaveSection(std::istream& input, State& state);
// Optional trailing section (tag "packrowsparked") for the rows rotated out of the hotbar; written only
// when there are any, so saves without it load with none.
constexpr const char* ParkedSaveTag = "packrowsparked";
void WriteParkedSection(std::ostream& output, const State& state);
bool ReadParkedSection(std::istream& input, State& state);
// Optional trailing section (tag "packslots") for where each stack sits in the grid below the row
// (State::packSlots); written only once she has placed something, so older saves load packed.
constexpr const char* SlotsSaveTag = "packslots";
void WriteSlotsSection(std::ostream& output, const State& state);
bool ReadSlotsSection(std::istream& input, State& state);
bool ValidSlots(const State& state);
// After reading all sections: retire old consumed-stack references without relaxing corrupt-save checks.
bool RestoreSlots(State& state);
// The most squares her pack's grid may span.
constexpr int MaxPackSlots = 1024;

// Her pack's grid below the row, square by square: the layout index in each, or -1 for a gap (never
// a trailing one). Placed stacks keep their squares; the rest fill the first gaps, then follow in
// layout order. The pail's water, while it shows on the pail (HomesteadPail.h), takes no square.
std::vector<int> Grid(const State& state);
// Grid() as cells: what State::packSlots holds once she places something.
std::vector<PackRowCell> GridCells(const State& state);
// Renames a placed square (a stack swapped in for another), when she has placed any.
void ReplaceSlot(State& state, const PackRowCell& from, const PackRowCell& to);
// Puts the layout in the order she sees: the row's entries, then the grid's, then anything hidden.
void OrderLayoutByGrid(State& state);

// The cell naming `entry`.
PackRowCell CellFor(const LayoutEntry& entry);
// Which cell holds `entry`, or -1 when it lies below the row.
int CellOf(const PackRow& row, const LayoutEntry& entry);
// The layout index `cell` names, or -1.
int FindEntry(const InventoryLayout& layout, const PackRowCell& cell);
// The carried entry in `cell`, or nullptr when the cell is empty.
const LayoutEntry* RowEntry(const State& state, int cell);
// The cell holding her stack `groupId` (or garment `wearableId`), or -1.
int RowCellOf(const State& state, int groupId, int wearableId);
// Layout indices in the order gains and uses visit them: row cells left to right, then below.
std::vector<int> FillOrder(const PackRow& row, const InventoryLayout& layout);
// Layout indices below the row, in layout order.
std::vector<int> BelowRow(const PackRow& row, const InventoryLayout& layout);
// Empties cells naming entries no longer in the layout.
void Prune(PackRow& row, const InventoryLayout& layout);
// Puts `entry` in the first empty cell; false when the row is full.
bool TakeFirstEmpty(PackRow& row, const LayoutEntry& entry);
// Every non-empty cell names exactly one carried entry, and no entry sits in two cells.
bool Valid(const PackRow& row, const InventoryLayout& layout);
// The key that selects `cell`: "1".."9", then "0".
inline int KeyNumber(int cell) { return cell == PackRowSize - 1 ? 0 : cell + 1; }
}
}
