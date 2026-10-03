#include "HomesteadPackRow.h"
#include "HomesteadPail.h"

#include <algorithm>
#include <istream>
#include <ostream>
#include <set>
#include <string>
#include <utility>

namespace Homestead
{
namespace PackRowRules
{
void WriteSaveSection(std::ostream& output, const State& state)
{
    output << SaveTag << ' ' << PackRowSize;
    for (const auto& cell : state.packRow) output << ' ' << cell.groupId << ' ' << cell.wearableId;
    output << '\n';
}

bool ReadSaveSection(std::istream& input, State& state)
{
    int count = 0;
    if (!(input >> count) || count != PackRowSize) return false;
    for (auto& cell : state.packRow)
        if (!(input >> cell.groupId >> cell.wearableId) || cell.groupId < 0 || cell.wearableId < 0) return false;
    // Whether each cell names a carried entry is checked with the rest of the inventory.
    return true;
}

// The most rows she could park: every carried stack in a row of its own.
constexpr int MaxParkedRows = 1024;

void WriteParkedSection(std::ostream& output, const State& state)
{
    if (state.parkedRows.empty()) return;
    output << ParkedSaveTag << ' ' << state.parkedRows.size() << ' ' << PackRowSize;
    for (const auto& row : state.parkedRows)
        for (const auto& cell : row) output << ' ' << cell.groupId << ' ' << cell.wearableId;
    output << '\n';
}

bool ReadParkedSection(std::istream& input, State& state)
{
    int rows = 0, width = 0;
    if (!(input >> rows >> width) || rows < 0 || rows > MaxParkedRows || width != PackRowSize) return false;
    state.parkedRows.assign(static_cast<size_t>(rows), PackRow{});
    for (auto& row : state.parkedRows)
        for (auto& cell : row)
            if (!(input >> cell.groupId >> cell.wearableId) || cell.groupId < 0 || cell.wearableId < 0) return false;
    // Stale cells (stacks since used up or moved) are dropped as a row comes back (RotatePackRow).
    return true;
}

void WriteSlotsSection(std::ostream& output, const State& state)
{
    if (state.packSlots.empty()) return;
    output << SlotsSaveTag << ' ' << state.packSlots.size();
    for (const auto& cell : state.packSlots) output << ' ' << cell.groupId << ' ' << cell.wearableId;
    output << '\n';
}

bool ReadSlotsSection(std::istream& input, State& state)
{
    int count = 0;
    if (!(input >> count) || count < 0 || count > MaxPackSlots) return false;
    state.packSlots.assign(static_cast<size_t>(count), PackRowCell{});
    for (auto& cell : state.packSlots)
        if (!(input >> cell.groupId >> cell.wearableId) || cell.groupId < 0 || cell.wearableId < 0
            || (cell.groupId != 0 && cell.wearableId != 0))
            return false;
    // References are validated after all save sections have been read.
    return true;
}

bool ValidSlots(const State& state)
{
    if (state.packSlots.size() > MaxPackSlots) return false;
    std::set<int> groups, garments;
    const bool hideWater = PresentPail(state).hidePackWater;
    for (const auto& cell : state.packSlots)
    {
        if (cell.Empty()) continue;
        if (cell.groupId < 0 || cell.wearableId < 0 || (cell.groupId != 0) == (cell.wearableId != 0))
            return false;
        const int index = FindEntry(state.inventoryLayout, cell);
        if (index < 0 || CellOf(state.packRow, state.inventoryLayout[index]) >= 0) return false;
        const auto& entry = state.inventoryLayout[index];
        if (hideWater && entry.wearableId == 0 && entry.item == Item::Water) return false;
        if (cell.groupId != 0 ? !groups.insert(cell.groupId).second : !garments.insert(cell.wearableId).second)
            return false;
    }
    return true;
}

std::vector<int> Grid(const State& state)
{
    const auto& layout = state.inventoryLayout;
    const bool hideWater = PresentPail(state).hidePackWater;
    std::vector<int> below;
    for (const int index : BelowRow(state.packRow, layout))
        if (!(hideWater && layout[index].wearableId == 0 && layout[index].item == Item::Water)) below.push_back(index);
    std::vector<bool> placed(layout.size(), false);
    std::vector<int> grid;
    grid.reserve(std::max(state.packSlots.size(), below.size()));
    for (const auto& cell : state.packSlots)
    {
        int found = -1;
        if (!cell.Empty())
            for (const int index : below)
                if (!placed[index] && CellFor(layout[index]) == cell) { found = index; break; }
        if (found >= 0) placed[found] = true;
        grid.push_back(found);
    }
    for (const int index : below)
    {
        if (placed[index]) continue;
        const auto gap = std::find(grid.begin(), grid.end(), -1);
        if (gap != grid.end()) *gap = index;
        else grid.push_back(index);
    }
    while (!grid.empty() && grid.back() < 0) grid.pop_back();
    return grid;
}

std::vector<PackRowCell> GridCells(const State& state)
{
    std::vector<PackRowCell> cells;
    for (const int index : Grid(state)) cells.push_back(index < 0 ? PackRowCell{} : CellFor(state.inventoryLayout[index]));
    return cells;
}

void ReplaceSlot(State& state, const PackRowCell& from, const PackRowCell& to)
{
    for (auto& cell : state.packSlots)
        if (!from.Empty() && cell == from) { cell = to; return; }
}

void OrderLayoutByGrid(State& state)
{
    const auto& layout = state.inventoryLayout;
    InventoryLayout ordered;
    ordered.reserve(layout.size());
    std::vector<bool> taken(layout.size(), false);
    const auto take = [&](int index) { if (!taken[index]) { ordered.push_back(layout[index]); taken[index] = true; } };
    for (int index = 0; index < static_cast<int>(layout.size()); ++index)
        if (CellOf(state.packRow, layout[index]) >= 0) take(index);
    for (const int index : Grid(state)) if (index >= 0) take(index);
    for (int index = 0; index < static_cast<int>(layout.size()); ++index) take(index);
    state.inventoryLayout = std::move(ordered);
}

PackRowCell CellFor(const LayoutEntry& entry)
{
    return entry.wearableId != 0 ? PackRowCell{0, entry.wearableId} : PackRowCell{entry.groupId, 0};
}

int CellOf(const PackRow& row, const LayoutEntry& entry)
{
    const PackRowCell key = CellFor(entry);
    for (int cell = 0; cell < PackRowSize; ++cell)
        if (!row[cell].Empty() && row[cell] == key) return cell;
    return -1;
}

int FindEntry(const InventoryLayout& layout, const PackRowCell& cell)
{
    if (cell.Empty()) return -1;
    for (int index = 0; index < static_cast<int>(layout.size()); ++index)
        if (CellFor(layout[index]) == cell) return index;
    return -1;
}

const LayoutEntry* RowEntry(const State& state, int cell)
{
    if (cell < 0 || cell >= PackRowSize) return nullptr;
    const int index = FindEntry(state.inventoryLayout, state.packRow[cell]);
    return index < 0 ? nullptr : &state.inventoryLayout[index];
}

int RowCellOf(const State& state, int groupId, int wearableId)
{
    const PackRowCell key = wearableId != 0 ? PackRowCell{0, wearableId} : PackRowCell{groupId, 0};
    if (key.Empty()) return -1;
    for (int cell = 0; cell < PackRowSize; ++cell)
        if (state.packRow[cell] == key) return cell;
    return -1;
}

std::vector<int> FillOrder(const PackRow& row, const InventoryLayout& layout)
{
    std::vector<int> order;
    order.reserve(layout.size());
    for (const auto& cell : row)
        if (const int index = FindEntry(layout, cell); index >= 0) order.push_back(index);
    for (const int index : BelowRow(row, layout)) order.push_back(index);
    return order;
}

std::vector<int> BelowRow(const PackRow& row, const InventoryLayout& layout)
{
    std::vector<int> below;
    below.reserve(layout.size());
    for (int index = 0; index < static_cast<int>(layout.size()); ++index)
        if (CellOf(row, layout[index]) < 0) below.push_back(index);
    return below;
}

void Prune(PackRow& row, const InventoryLayout& layout)
{
    for (auto& cell : row)
        if (!cell.Empty() && FindEntry(layout, cell) < 0) cell = {};
}

bool TakeFirstEmpty(PackRow& row, const LayoutEntry& entry)
{
    if (CellOf(row, entry) >= 0) return true;
    for (auto& cell : row)
        if (cell.Empty()) { cell = CellFor(entry); return true; }
    return false;
}

bool Valid(const PackRow& row, const InventoryLayout& layout)
{
    for (int cell = 0; cell < PackRowSize; ++cell)
    {
        const auto& value = row[cell];
        if (value.Empty()) continue;
        if ((value.groupId != 0) == (value.wearableId != 0) || FindEntry(layout, value) < 0) return false;
        for (int other = cell + 1; other < PackRowSize; ++other)
            if (row[other] == value) return false;
    }
    return true;
}
}

namespace
{
Result RowBad(const std::string& text) { return {false, text, ResultCode::Invalid}; }
bool Stackable(const LayoutEntry& left, const LayoutEntry& right)
{
    return left.wearableId == 0 && right.wearableId == 0 && left.item == right.item;
}
std::string SlotName(int cell) { return "hotbar slot " + std::to_string(PackRowRules::KeyNumber(cell)); }
int FindCarried(const InventoryLayout& layout, int groupId, int wearableId)
{
    if (wearableId != 0) return PackRowRules::FindEntry(layout, {0, wearableId});
    return groupId > 0 ? PackRowRules::FindEntry(layout, {groupId, 0}) : -1;
}
}

Result Simulation::MoveToPackRow(int groupId, int wearableId, int cell, std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    if (cell < 0 || cell >= PackRowSize) return RowBad("Choose one of the ten hotbar slots.");
    State candidate = state_;
    auto& layout = candidate.inventoryLayout;
    auto& row = candidate.packRow;
    const int source = FindCarried(layout, groupId, wearableId);
    if (source < 0) return RowBad("Choose something in your pack. Take stored things into your pack first.");
    const int from = PackRowRules::CellOf(row, layout[source]);
    if (from == cell) return {true, "", ResultCode::None, revision_};
    const PackRowCell occupant = row[cell];
    const int held = PackRowRules::FindEntry(layout, occupant);
    if (held >= 0 && Stackable(layout[held], layout[source]))
    {
        layout[held].quantity += layout[source].quantity;
        layout.erase(layout.begin() + source);
        if (from >= 0) row[from] = {};
        return CommitInventory(std::move(candidate), ("Added to the stack in " + SlotName(cell) + ".").c_str());
    }
    row[cell] = PackRowRules::CellFor(layout[source]);
    // Within the row the two cells trade places; from below, whatever was in the cell takes her
    // stack's old place in the pack.
    if (from >= 0) row[from] = held >= 0 ? occupant : PackRowCell{};
    else if (held >= 0)
    {
        PackRowRules::ReplaceSlot(candidate, row[cell], occupant);
        std::swap(layout[source], layout[held]);
    }
    return CommitInventory(std::move(candidate),
        ((held >= 0 ? "Swapped into " : "Moved to ") + SlotName(cell) + ".").c_str());
}

Result Simulation::MoveFromPackRow(int cell, int targetGroupId, int targetWearableId, std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    if (cell < 0 || cell >= PackRowSize) return RowBad("Choose one of the ten hotbar slots.");
    State candidate = state_;
    auto& layout = candidate.inventoryLayout;
    auto& row = candidate.packRow;
    const int source = PackRowRules::FindEntry(layout, row[cell]);
    if (source < 0) return RowBad("That hotbar slot is empty.");
    if (targetGroupId == 0 && targetWearableId == 0)
    {
        const LayoutEntry entry = layout[source];
        layout.erase(layout.begin() + source);
        layout.push_back(entry);
        row[cell] = {};
        return CommitInventory(std::move(candidate), "Moved into your pack, below the hotbar.");
    }
    const int target = FindCarried(layout, targetGroupId, targetWearableId);
    if (target < 0 || PackRowRules::CellOf(row, layout[target]) >= 0)
        return RowBad("Choose a place in your pack below the hotbar.");
    if (Stackable(layout[target], layout[source]))
    {
        layout[target].quantity += layout[source].quantity;
        layout.erase(layout.begin() + source);
        row[cell] = {};
        return CommitInventory(std::move(candidate), "Added to the stack in your pack.");
    }
    const PackRowCell moving = row[cell];
    row[cell] = PackRowRules::CellFor(layout[target]);
    PackRowRules::ReplaceSlot(candidate, row[cell], moving);
    std::swap(layout[source], layout[target]);
    return CommitInventory(std::move(candidate), ("Swapped with " + SlotName(cell) + ".").c_str());
}

Result Simulation::MoveToPackSlot(int groupId, int wearableId, int slot, std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    if (slot < 0 || slot >= PackRowRules::MaxPackSlots) return RowBad("Choose a square in your pack.");
    State candidate = state_;
    auto& layout = candidate.inventoryLayout;
    auto& row = candidate.packRow;
    const int source = FindCarried(layout, groupId, wearableId);
    if (source < 0) return RowBad("Choose something in your pack. Take stored things into your pack first.");
    const PackRowCell key = PackRowRules::CellFor(layout[source]);
    // From here on every stack keeps its square (State::packSlots).
    auto slots = PackRowRules::GridCells(candidate);
    if (static_cast<int>(slots.size()) <= slot) slots.resize(static_cast<size_t>(slot) + 1);
    const int fromCell = PackRowRules::CellOf(row, layout[source]);
    const auto found = std::find(slots.begin(), slots.end(), key);
    const int fromSlot = found == slots.end() ? -1 : static_cast<int>(found - slots.begin());
    if (fromCell < 0 && fromSlot < 0) return RowBad("That can't go in a square of your pack.");
    if (fromSlot == slot) return {true, "", ResultCode::None, revision_};
    const PackRowCell occupant = slots[slot];
    const int held = PackRowRules::FindEntry(layout, occupant);
    slots[slot] = key;
    if (fromCell >= 0) row[fromCell] = held >= 0 ? occupant : PackRowCell{};
    else slots[fromSlot] = held >= 0 ? occupant : PackRowCell{};
    while (!slots.empty() && slots.back().Empty()) slots.pop_back();
    candidate.packSlots = std::move(slots);
    PackRowRules::OrderLayoutByGrid(candidate);
    return CommitInventory(std::move(candidate), "");
}

Result Simulation::RotatePackRow(std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    State candidate = state_;
    const auto& layout = candidate.inventoryLayout;
    PackRow& row = candidate.packRow;
    auto& parked = candidate.parkedRows;
    const auto named = [&layout](const PackRowCell& cell) { return !cell.Empty() && PackRowRules::FindEntry(layout, cell) >= 0; };
    const auto inRow = [](const PackRow& in, const PackRowCell& cell)
    {
        for (const auto& each : in) if (!cell.Empty() && each == cell) return true;
        return false;
    };
    // Stacks below the hotbar that no parked row names come up first, ten at a time in pack order;
    // once every stack belongs to a row, the parked rows come back in turn, gaps and all.
    PackRow incoming{};
    bool found = false;
    int cell = 0;
    for (const int index : PackRowRules::Grid(candidate))
    {
        if (index < 0) continue;
        const PackRowCell key = PackRowRules::CellFor(layout[index]);
        bool isParked = false;
        for (const auto& other : parked) isParked = isParked || inRow(other, key);
        if (isParked) continue;
        incoming[cell++] = key;
        found = true;
        if (cell == PackRowSize) break;
    }
    while (!found && !parked.empty())
    {
        incoming = parked.front();
        parked.erase(parked.begin());
        // A cell whose stack was used up, or now sits in the hotbar going down, stays empty.
        for (auto& each : incoming) if (!named(each) || inRow(row, each)) each = {};
        for (const auto& each : incoming) found = found || !each.Empty();
    }
    if (!found) return RowBad("Nothing else in your pack.");
    // Each stack belongs to one row: the incoming row's stacks leave any other parked row.
    for (auto& other : parked)
        for (auto& each : other) if (inRow(incoming, each)) each = {};
    bool rowUsed = false;
    for (const auto& each : row) rowUsed = rowUsed || named(each);
    if (rowUsed) parked.push_back(row);
    parked.erase(std::remove_if(parked.begin(), parked.end(), [&named](const PackRow& other)
        {
            for (const auto& each : other) if (named(each)) return false;
            return true;
        }), parked.end());
    row = incoming;
    return CommitInventory(std::move(candidate), "");
}

Result Simulation::ArrangePackRow(const std::array<int, PackRowSize>& items)
{
    State candidate = state_;
    PackRow row{};
    for (int cell = 0; cell < PackRowSize; ++cell)
    {
        const int value = items[cell];
        if (value < 0 || value >= ItemCount) continue;
        for (const auto& entry : candidate.inventoryLayout)
            if (entry.wearableId == 0 && static_cast<int>(entry.item) == value
                && PackRowRules::CellOf(row, entry) < 0)
            {
                row[cell] = PackRowRules::CellFor(entry);
                break;
            }
    }
    candidate.packRow = row;
    return CommitInventory(std::move(candidate), "Hotbar arranged.");
}
}
