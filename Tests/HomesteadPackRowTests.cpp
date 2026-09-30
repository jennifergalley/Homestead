// Portable tests for the hotbar as the first row of her pack (HomesteadPackRow.h), and for migrating
// the old pinned hotbar a save stored (HomesteadHotbarLayout.h).
#include "HomesteadEstate.h"
#include "HomesteadHotbarLayout.h"
#include "HomesteadPackRow.h"
#include "HomesteadSimulation.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace Homestead;

namespace
{
int checks = 0;
int cases = 0;
void Check(bool condition, const char* expression, int line)
{
    ++checks;
    if (!condition)
    {
        std::cerr << "FAIL line " << line << ": " << expression << '\n';
        std::exit(1);
    }
}
#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)

void Okay(const Result& result, int line)
{
    ++checks;
    if (!result.ok)
    {
        std::cerr << "FAIL line " << line << ": " << result.message << '\n';
        std::exit(1);
    }
}
#define OK(expression) Okay(expression, __LINE__)

constexpr int V(Item item) { return static_cast<int>(item); }

// The layout a new game used to start with (AHomesteadController::ResetHotbar).
HotbarLayout NewGameLayout()
{
    return {V(Item::Billhook), V(Item::Hatchet), V(Item::Scythe), V(Item::Pickaxe), V(Item::DiggingStick),
        V(Item::WateringCan), V(Item::Berries), V(Item::OilLamp), HotbarEmpty, HotbarEmpty};
}

// A save's header with `payload` in place of its own.
std::string Reseal(const std::string& saved, const std::string& payload)
{
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned char c : payload) { hash ^= c; hash *= UINT64_C(1099511628211); }
    std::istringstream header(saved.substr(0, saved.find('\n')));
    std::string magic, version;
    header >> magic >> version;
    return magic + " " + version + " " + std::to_string(payload.size()) + " " + std::to_string(hash) + "\n" + payload;
}

// The save with its "packrow" line replaced by `line` (empty to drop the section, as older builds wrote).
std::string WithRowLine(const std::string& saved, const std::string& line)
{
    std::string payload = saved.substr(saved.find('\n') + 1);
    const auto start = payload.find("\npackrow ");
    CHECK(start != std::string::npos);
    const auto end = payload.find('\n', start + 1);
    payload = payload.substr(0, start + 1) + (line.empty() ? "" : line + "\n") + payload.substr(end + 1);
    return Reseal(saved, payload);
}

Simulation Estate()
{
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    return sim;
}

int FirstEmpty(const Simulation& sim)
{
    for (int cell = 0; cell < PackRowSize; ++cell) if (sim.GetState().packRow[cell].Empty()) return cell;
    return -1;
}

// The one carried stack of `item` (group id); fails the test when there isn't exactly one.
int Group(const Simulation& sim, Item item)
{
    int found = 0, count = 0;
    for (const auto& entry : sim.GetState().inventoryLayout)
        if (entry.wearableId == 0 && entry.item == item) { found = entry.groupId; ++count; }
    CHECK(count == 1);
    return found;
}

Item CellItem(const Simulation& sim, int cell)
{
    const auto* entry = PackRowRules::RowEntry(sim.GetState(), cell);
    return entry && entry->wearableId == 0 ? entry->item : Item::Count;
}

int CellQuantity(const Simulation& sim, int cell)
{
    const auto* entry = PackRowRules::RowEntry(sim.GetState(), cell);
    return entry ? entry->quantity : 0;
}

// What lies below the row, in order (item ids; garments as -wearable id).
std::vector<int> Below(const Simulation& sim)
{
    std::vector<int> result;
    const auto& state = sim.GetState();
    for (const int index : PackRowRules::BelowRow(state.packRow, state.inventoryLayout))
    {
        const auto& entry = state.inventoryLayout[index];
        result.push_back(entry.wearableId ? -entry.wearableId : V(entry.item));
    }
    return result;
}

// Empties the row (everything drops below it).
void ClearRow(Simulation& sim)
{
    std::array<int, PackRowSize> none;
    none.fill(-1);
    OK(sim.ArrangePackRow(none));
    for (const auto& cell : sim.GetState().packRow) CHECK(cell.Empty());
}

const Structure* StarterChest(const Simulation& sim)
{
    for (const auto& piece : sim.GetState().structures)
        if (piece.kind == Piece::Chest) return &piece;
    return nullptr;
}

int ChestGroup(const Simulation& sim, int chest, Item item)
{
    for (const auto& entry : *sim.GetLayout(chest)) if (entry.wearableId == 0 && entry.item == item) return entry.groupId;
    return 0;
}

void KeysAndTheOldPinRule()
{
    CHECK(HotbarSize == 10 && PackRowSize == 10);
    const char* keys[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    for (int slot = 0; slot < HotbarSize; ++slot)
    {
        CHECK(HotbarKeyLabel(slot) == keys[slot]);
        CHECK(std::to_string(PackRowRules::KeyNumber(slot)) == keys[slot]);
    }
    // The old pinned hotbar only took tools, food and seed; that rule now only cleans old saves.
    for (Item item : {Item::Hatchet, Item::WateringCan, Item::OilLamp, Item::Berries, Item::Pasty, Item::TurnipSeed})
        CHECK(CanPinToHotbar(item));
    for (Item item : {Item::Stone, Item::Branch, Item::Water, Item::OilFlask})
        CHECK(!CanPinToHotbar(item));
}

void NewThingsFillTheFirstEmptyCell()
{
    Simulation sim = Estate();
    ClearRow(sim);
    // Any carried thing can sit in the row, materials included.
    OK(sim.GrantItems(Item::Stone, 3));
    CHECK(CellItem(sim, 0) == Item::Stone && CellQuantity(sim, 0) == 3);
    // More of something she carries tops up that stack; it doesn't take another cell.
    OK(sim.GrantItems(Item::Stone, 2));
    CHECK(CellQuantity(sim, 0) == 5 && sim.GetState().packRow[1].Empty());
    OK(sim.GrantItems(Item::Branch, 4));
    CHECK(CellItem(sim, 1) == Item::Branch);
    // A full row sends the next new thing below it.
    const Item fill[] = {Item::Fiber, Item::Seeds, Item::Berries, Item::Roots, Item::Timber, Item::Pasty, Item::Bread, Item::TurnipSeed};
    for (const Item item : fill) OK(sim.GrantItems(item, 1));
    CHECK(FirstEmpty(sim) == -1 && CellItem(sim, 9) == Item::TurnipSeed);
    OK(sim.GrantItems(Item::Cheese, 1));
    const auto before = Below(sim);
    CHECK(PackRowRules::RowCellOf(sim.GetState(), Group(sim, Item::Cheese), 0) == -1);
    CHECK(!before.empty() && before.back() == V(Item::Cheese));
    CHECK(PackRowRules::Valid(sim.GetState().packRow, sim.GetState().inventoryLayout));
    // The pail's water never takes a cell: it shows on the pail.
    ClearRow(sim);
    if (sim.GrantItems(Item::Water, 2).ok)
    {
        CHECK(FirstEmpty(sim) == 0);
        CHECK(PackRowRules::RowCellOf(sim.GetState(), Group(sim, Item::Water), 0) == -1);
    }
}

void UsesDrawFromTheRowAndLeaveItEmpty()
{
    Simulation sim = Estate();
    ClearRow(sim);
    const auto* chest = StarterChest(sim);
    CHECK(chest != nullptr);
    const int chestId = chest->id;
    const Point at = sim.StructureCenter(*chest);
    OK(sim.GrantItems(Item::Berries, 4));
    const int berries = Group(sim, Item::Berries);
    CHECK(CellItem(sim, 0) == Item::Berries);
    // A second stack below the row: split, and the split half goes below.
    OK(sim.SplitGroup(0, berries, 1, {}, sim.GetRevision()));
    CHECK(CellQuantity(sim, 0) == 3 && Below(sim).back() == V(Item::Berries));
    // Gains top up the row's stack first; uses (here, storing by item) draw from it first.
    OK(sim.GrantItems(Item::Berries, 1));
    CHECK(CellQuantity(sim, 0) == 4);
    OK(sim.Transfer(chestId, Item::Berries, 4, at));
    // Used up: the cell is empty, with nothing held for it; the stack below is untouched.
    CHECK(sim.GetState().packRow[0].Empty() && sim.Count(Item::Berries) == 1);
    // The next new thing takes the first empty cell, whatever it is.
    OK(sim.GrantItems(Item::Stone, 1));
    CHECK(CellItem(sim, 0) == Item::Stone);
    // More berries top up the stack she still has below; they don't come back into the row.
    OK(sim.GrantItems(Item::Berries, 2));
    CHECK(sim.Count(Item::Berries) == 3 && CellItem(sim, 1) == Item::Count);
    // Eating a chosen stack (the hotbar's selected cell) spends that stack.
    OK(sim.MoveToPackRow(Group(sim, Item::Berries), 0, 4, sim.GetRevision()));
    OK(sim.EatGroup(sim.GetState().packRow[4].groupId, sim.GetRevision()));
    CHECK(CellQuantity(sim, 4) == 2);
}

void MovesMergeOrSwap()
{
    Simulation sim = Estate();
    ClearRow(sim);
    for (const Item item : {Item::Stone, Item::Branch, Item::Fiber}) OK(sim.GrantItems(item, 2));
    CHECK(CellItem(sim, 0) == Item::Stone && CellItem(sim, 1) == Item::Branch && CellItem(sim, 2) == Item::Fiber);
    // Row onto an empty cell moves; onto a used cell the two swap.
    OK(sim.MoveToPackRow(Group(sim, Item::Stone), 0, 7, sim.GetRevision()));
    CHECK(sim.GetState().packRow[0].Empty() && CellItem(sim, 7) == Item::Stone);
    OK(sim.MoveToPackRow(Group(sim, Item::Stone), 0, 1, sim.GetRevision()));
    CHECK(CellItem(sim, 1) == Item::Stone && CellItem(sim, 7) == Item::Branch);
    // From below onto a used cell: they swap, and the displaced stack takes her stack's old place.
    ClearRow(sim);
    const auto before = Below(sim);
    OK(sim.MoveToPackRow(Group(sim, Item::Branch), 0, 3, sim.GetRevision()));
    OK(sim.MoveToPackRow(Group(sim, Item::Fiber), 0, 3, sim.GetRevision()));
    CHECK(CellItem(sim, 3) == Item::Fiber);
    std::vector<int> expected;
    for (const int value : before) if (value != V(Item::Fiber)) expected.push_back(value);
    CHECK(Below(sim).size() == expected.size());
    // The branch now sits where the fiber was.
    std::vector<int> swapped = before;
    for (int& value : swapped) if (value == V(Item::Fiber)) value = V(Item::Branch);
    swapped.erase(std::find(swapped.begin(), swapped.end(), V(Item::Branch)));
    CHECK(Below(sim) == swapped);
    // Onto the same item they merge into the cell's stack.
    OK(sim.GrantItems(Item::Stone, 1));
    OK(sim.SplitGroup(0, Group(sim, Item::Stone), 1, {}, sim.GetRevision()));
    int splitStone = 0;
    for (const auto& entry : sim.GetState().inventoryLayout)
        if (entry.item == Item::Stone && entry.wearableId == 0 && entry.quantity == 1) splitStone = entry.groupId;
    int mainStone = 0;
    for (const auto& entry : sim.GetState().inventoryLayout)
        if (entry.item == Item::Stone && entry.wearableId == 0 && entry.groupId != splitStone) mainStone = entry.groupId;
    OK(sim.MoveToPackRow(mainStone, 0, 5, sim.GetRevision()));
    OK(sim.MoveToPackRow(splitStone, 0, 5, sim.GetRevision()));
    CHECK(CellItem(sim, 5) == Item::Stone && CellQuantity(sim, 5) == sim.Count(Item::Stone));
    // Out of the row: to the end, onto the same item (merge) or onto another stack (swap).
    OK(sim.MoveFromPackRow(5, 0, 0, sim.GetRevision()));
    CHECK(sim.GetState().packRow[5].Empty() && Below(sim).back() == V(Item::Stone));
    OK(sim.MoveToPackRow(Group(sim, Item::Branch), 0, 0, sim.GetRevision()));
    OK(sim.MoveFromPackRow(3, Group(sim, Item::Stone), 0, sim.GetRevision()));
    CHECK(CellItem(sim, 3) == Item::Stone && Below(sim).back() == V(Item::Fiber));
    OK(sim.GrantItems(Item::Fiber, 1));
    CHECK(sim.Count(Item::Fiber) == 3);
    // Refusals change nothing.
    const std::string saved = sim.Serialize();
    const auto revision = sim.GetRevision();
    CHECK(!sim.MoveToPackRow(Group(sim, Item::Fiber), 0, 10, revision));
    CHECK(!sim.MoveToPackRow(Group(sim, Item::Fiber), 0, -1, revision));
    CHECK(!sim.MoveToPackRow(999999, 0, 2, revision));
    CHECK(!sim.MoveFromPackRow(9, 0, 0, revision));
    CHECK(!sim.MoveFromPackRow(0, Group(sim, Item::Stone), 0, revision));
    CHECK(sim.MoveToPackRow(Group(sim, Item::Fiber), 0, 2, revision + 1).code == ResultCode::StaleRevision);
    // Moving a stack onto its own cell is a quiet no-op.
    OK(sim.MoveToPackRow(Group(sim, Item::Stone), 0, 3, revision));
    CHECK(sim.Serialize() == saved && sim.GetRevision() == revision);
}

void GarmentsTakeCellsToo()
{
    Simulation sim = Estate();
    ClearRow(sim);
    int worn = 0;
    for (const auto& item : sim.GetState().wearables) if (item.owner == WearableOwner::Equipped) { worn = item.id; break; }
    CHECK(worn != 0);
    OK(sim.UnequipWearable(worn, sim.GetRevision()));
    const int cell = PackRowRules::RowCellOf(sim.GetState(), 0, worn);
    CHECK(cell == 0);
    OK(sim.MoveToPackRow(0, worn, 6, sim.GetRevision()));
    CHECK(PackRowRules::RowCellOf(sim.GetState(), 0, worn) == 6);
    // Worn again, it leaves the pack and its cell empties.
    OK(sim.EquipWearable(worn, sim.GetRevision()));
    CHECK(sim.GetState().packRow[6].Empty());
}

void SortingLeavesTheRowAlone()
{
    Simulation sim = Estate();
    ClearRow(sim);
    for (const Item item : {Item::Timber, Item::Stone, Item::Berries, Item::Branch}) OK(sim.GrantItems(item, 2));
    OK(sim.MoveToPackRow(Group(sim, Item::Stone), 0, 4, sim.GetRevision()));
    OK(sim.MoveToPackRow(Group(sim, Item::Timber), 0, 0, sim.GetRevision()));
    // A second berry stack in the row and one below: sorting doesn't merge them.
    OK(sim.SplitGroup(0, Group(sim, Item::Berries), 1, {}, sim.GetRevision()));
    int belowBerries = 0;
    for (const int index : PackRowRules::BelowRow(sim.GetState().packRow, sim.GetState().inventoryLayout))
        if (sim.GetState().inventoryLayout[index].item == Item::Berries) belowBerries = sim.GetState().inventoryLayout[index].groupId;
    OK(sim.MoveToPackRow(belowBerries, 0, 8, sim.GetRevision()));
    const PackRow row = sim.GetState().packRow;
    OK(sim.SortPack(sim.GetRevision()));
    CHECK(sim.GetState().packRow == row);
    CHECK(CellItem(sim, 0) == Item::Timber && CellItem(sim, 4) == Item::Stone && CellItem(sim, 8) == Item::Berries);
    CHECK(sim.Count(Item::Berries) == 2 && CellQuantity(sim, 8) == 1);
    // Below the row is in sort order, and sorting again reports it.
    const auto& state = sim.GetState();
    const auto below = PackRowRules::BelowRow(state.packRow, state.inventoryLayout);
    for (std::size_t i = 1; i < below.size(); ++i)
    {
        const auto& left = state.inventoryLayout[below[i - 1]];
        const auto& right = state.inventoryLayout[below[i]];
        if (left.wearableId == 0 && right.wearableId == 0)
            CHECK(ItemSortRank(left.item) < ItemSortRank(right.item)
                || (ItemSortRank(left.item) == ItemSortRank(right.item) && V(left.item) < V(right.item)));
    }
    const auto sorted = sim.SortPack(sim.GetRevision());
    CHECK(sorted.ok && sorted.message == "Pack is already sorted.");
}

void ChestsTradeWithTheRow()
{
    Simulation sim = Estate();
    const auto* chest = StarterChest(sim);
    CHECK(chest != nullptr);
    const int chestId = chest->id;
    const Point at = sim.StructureCenter(*chest);
    ClearRow(sim);
    OK(sim.GrantItems(Item::Stone, 2));
    const int pasty = ChestGroup(sim, chestId, Item::Pasty);
    int stored = 0;
    for (const auto& entry : *sim.GetLayout(chestId)) if (entry.item == Item::Pasty) stored += entry.quantity;
    CHECK(pasty != 0 && stored >= 2);
    // Straight from the chest onto a used cell: it becomes that cell's stack, the stone goes below.
    OK(sim.TransferGroupToPackRow(chestId, pasty, 2, 0, at, sim.GetRevision()));
    CHECK(CellItem(sim, 0) == Item::Pasty && CellQuantity(sim, 0) == 2 && sim.Count(Item::Pasty) == 2);
    CHECK(PackRowRules::RowCellOf(sim.GetState(), Group(sim, Item::Stone), 0) == -1);
    // Onto the same item it merges, and the chest loses exactly what she gained.
    if (stored > 2)
    {
        OK(sim.TransferGroupToPackRow(chestId, ChestGroup(sim, chestId, Item::Pasty), 1, 0, at, sim.GetRevision()));
        CHECK(CellQuantity(sim, 0) == 3 && sim.Count(Item::Pasty) == 3);
    }
    int chestPasties = 0;
    for (const auto& entry : *sim.GetLayout(chestId)) if (entry.item == Item::Pasty) chestPasties += entry.quantity;
    CHECK(chestPasties + sim.Count(Item::Pasty) == stored);
    // Storing the whole row stack empties its cell; taking it back (the ordinary way) fills the first empty cell.
    OK(sim.TransferGroup(chestId, sim.GetState().packRow[0].groupId, sim.Count(Item::Pasty), true, at, sim.GetRevision()));
    CHECK(sim.GetState().packRow[0].Empty() && sim.Count(Item::Pasty) == 0);
    OK(sim.MoveToPackRow(Group(sim, Item::Stone), 0, 2, sim.GetRevision()));
    OK(sim.TransferGroup(chestId, ChestGroup(sim, chestId, Item::Pasty), 1, false, at, sim.GetRevision()));
    CHECK(CellItem(sim, 0) == Item::Pasty);
    // Refused (too far, bad cell, too many, a full pack) with nothing changed.
    const std::string saved = sim.Serialize();
    const auto revision = sim.GetRevision();
    const int bread = ChestGroup(sim, chestId, Item::Bread);
    CHECK(bread != 0);
    CHECK(!sim.TransferGroupToPackRow(chestId, bread, 1, 3, {at.x + 5000.0, at.y}, revision));
    CHECK(!sim.TransferGroupToPackRow(chestId, bread, 1, 10, at, revision));
    CHECK(!sim.TransferGroupToPackRow(chestId, bread, 999, 3, at, revision));
    CHECK(!sim.TransferGroupToPackRow(0, bread, 1, 3, at, revision));
    CHECK(sim.Serialize() == saved && sim.GetRevision() == revision);
    while (sim.GrantItems(Item::Stone, 1).ok) {}
    const std::string full = sim.Serialize();
    const auto fullRevision = sim.GetRevision();
    const auto refused = sim.TransferGroupToPackRow(chestId, bread, 1, 3, at, fullRevision);
    CHECK(!refused && refused.code == ResultCode::Capacity);
    CHECK(sim.Serialize() == full && sim.GetRevision() == fullRevision);
}

void TheRowIsSaved()
{
    Simulation sim = Estate();
    ClearRow(sim);
    for (const Item item : {Item::Stone, Item::Branch, Item::Berries}) OK(sim.GrantItems(item, 2));
    OK(sim.MoveToPackRow(Group(sim, Item::Stone), 0, 9, sim.GetRevision()));
    OK(sim.MoveToPackRow(Group(sim, Item::Berries), 0, 4, sim.GetRevision()));
    const PackRow row = sim.GetState().packRow;
    const std::string saved = sim.Serialize();
    CHECK(saved.find("\npackrow 10 ") != std::string::npos);
    Simulation loaded = Estate();
    OK(loaded.Deserialize(saved));
    CHECK(loaded.GetState().packRow == row && loaded.Serialize() == saved);
    // A damaged row makes the save unreadable rather than guessed at.
    Simulation strict = Estate();
    const std::string before = strict.Serialize();
    const auto missing = strict.Deserialize(WithRowLine(saved, "packrow 10 999999 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"));
    CHECK(!missing && missing.code == ResultCode::CorruptSave);
    const int stone = row[9].groupId;
    const std::string twice = "packrow 10 " + std::to_string(stone) + " 0 " + std::to_string(stone) + " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0";
    CHECK(!strict.Deserialize(WithRowLine(saved, twice)));
    CHECK(!strict.Deserialize(WithRowLine(saved, "packrow 9 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0")));
    CHECK(!strict.Deserialize(WithRowLine(saved, "packrow 10 " + std::to_string(stone) + " 7 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0")));
    CHECK(strict.Serialize() == before);
}

void OldPinnedHotbarsMigrateOnce()
{
    // A save from before the row: it loads with an empty row, and the game then arranges it once
    // from the old pinned slots it kept (AHomesteadController::ApplySave).
    Simulation sim = Estate();
    ClearRow(sim);
    for (const Item item : {Item::Hatchet, Item::Berries, Item::OilLamp, Item::Stone}) OK(sim.GrantItems(item, 1));
    OK(sim.GrantItems(Item::Berries, 3));
    const std::string legacy = WithRowLine(sim.Serialize(), "");
    CHECK(legacy.find("packrow") == std::string::npos);
    Simulation loaded = Estate();
    OK(loaded.Deserialize(legacy));
    for (const auto& cell : loaded.GetState().packRow) CHECK(cell.Empty());
    // Pins: the hatchet, a turnip seed she has none of, the berries, the lamp; layout 3.
    const std::vector<int> pins = {V(Item::Hatchet), V(Item::TurnipSeed), HotbarEmpty, HotbarEmpty, HotbarEmpty,
        HotbarEmpty, V(Item::Berries), V(Item::OilLamp), HotbarEmpty, HotbarEmpty};
    const HotbarLayout clean = SanitizeHotbarLayout(pins, 3);
    std::array<int, PackRowSize> items;
    for (int cell = 0; cell < PackRowSize; ++cell) items[cell] = clean[cell];
    const Inventory stock = loaded.GetState().inventory;
    OK(loaded.ArrangePackRow(items));
    CHECK(CellItem(loaded, 0) == Item::Hatchet && CellItem(loaded, 6) == Item::Berries && CellQuantity(loaded, 6) == 4
        && CellItem(loaded, 7) == Item::OilLamp);
    // A pin she has none of becomes an ordinary empty cell (no promise is kept for it)...
    CHECK(loaded.GetState().packRow[1].Empty());
    // ...so the next new thing she picks up lands there, not a turnip seed.
    OK(loaded.GrantItems(Item::Timber, 1));
    CHECK(CellItem(loaded, 1) == Item::Timber);
    OK(loaded.GrantItems(Item::TurnipSeed, 1));
    CHECK(CellItem(loaded, 2) == Item::TurnipSeed);
    // Migrating never changes what she owns; unpinned stacks (the stone) sit below the row.
    Inventory after = loaded.GetState().inventory;
    after[V(Item::Timber)] -= 1;
    after[V(Item::TurnipSeed)] -= 1;
    CHECK(after == stock);
    CHECK(PackRowRules::RowCellOf(loaded.GetState(), Group(loaded, Item::Stone), 0) == -1);
    // Saved again it carries the row, and old pins no longer matter.
    Simulation again = Estate();
    OK(again.Deserialize(loaded.Serialize()));
    CHECK(again.GetState().packRow == loaded.GetState().packRow);
    // A pinned item repeated in an old list (sanitize drops repeats) or stored in a chest stays empty.
    std::array<int, PackRowSize> repeats;
    repeats.fill(-1);
    repeats[0] = V(Item::Pasty);
    OK(again.ArrangePackRow(repeats));
    CHECK(again.GetState().packRow[0].Empty());
}

void SavedLayoutsSanitize()
{
    const HotbarLayout fresh = NewGameLayout();
    CHECK(SanitizeHotbarLayout(std::vector<int>(fresh.begin(), fresh.end()), 3) == fresh);
    // Anything unsafe in a save becomes an empty slot: unknown values, materials and repeats.
    const std::vector<int> damaged = {V(Item::Hatchet), 9999, V(Item::Stone), V(Item::Hatchet), -7, V(Item::Pasty),
        HotbarEmpty, HotbarEmpty, V(Item::OilLamp), HotbarEmpty, V(Item::Berries), V(Item::Scythe)};
    const HotbarLayout expected = {V(Item::Hatchet), HotbarEmpty, HotbarEmpty, HotbarEmpty, HotbarEmpty, V(Item::Pasty),
        HotbarEmpty, HotbarEmpty, V(Item::OilLamp), HotbarEmpty};
    CHECK(SanitizeHotbarLayout(damaged, 3) == expected);
    const HotbarLayout shortOne = SanitizeHotbarLayout({V(Item::Hatchet)}, 3);
    CHECK(shortOne[0] == V(Item::Hatchet));
    for (int slot = 1; slot < HotbarSize; ++slot) CHECK(shortOne[slot] == HotbarEmpty);
    // An old save with no hotbar array at all: layout 3 stays empty; layout 0 gains the defaults.
    const HotbarLayout none = SanitizeHotbarLayout({}, 3);
    for (int slot = 0; slot < HotbarSize; ++slot) CHECK(none[slot] == HotbarEmpty);
    const HotbarLayout aged = SanitizeHotbarLayout({}, 0);
    CHECK(aged[7] == V(Item::OilLamp) && aged[0] == V(Item::Billhook) && aged[1] == V(Item::Scythe)
        && aged[2] == V(Item::Pickaxe) && aged[3] == V(Item::Berries) && aged[4] == HotbarEmpty);
    // Layout 1 (before the estate tools): the lamp in the first free slot from 8, then the billhook,
    // scythe, pickaxe and berries in free slots; the retired knife no longer rides on it.
    const std::vector<int> old = {V(Item::Knife), V(Item::Hatchet), V(Item::DiggingStick), V(Item::WateringCan),
        HotbarEmpty, HotbarEmpty, HotbarEmpty, HotbarEmpty, HotbarEmpty, HotbarEmpty};
    const HotbarLayout migrated = SanitizeHotbarLayout(old, 1);
    CHECK(migrated[0] == V(Item::Billhook) && migrated[1] == V(Item::Hatchet) && migrated[2] == V(Item::DiggingStick)
        && migrated[3] == V(Item::WateringCan) && migrated[4] == V(Item::Scythe) && migrated[5] == V(Item::Pickaxe)
        && migrated[6] == V(Item::Berries) && migrated[7] == V(Item::OilLamp));
    // Layout 2 only gains the lamp; one she already pinned stays; with 8-10 full it wraps round.
    const std::vector<int> two = {V(Item::Billhook), V(Item::Hatchet), HotbarEmpty, HotbarEmpty, HotbarEmpty,
        HotbarEmpty, HotbarEmpty, HotbarEmpty, HotbarEmpty, HotbarEmpty};
    CHECK(SanitizeHotbarLayout(two, 2)[7] == V(Item::OilLamp) && SanitizeHotbarLayout(two, 2)[2] == HotbarEmpty);
    std::vector<int> lampPinned = two;
    lampPinned[3] = V(Item::OilLamp);
    const HotbarLayout kept = SanitizeHotbarLayout(lampPinned, 2);
    CHECK(kept[3] == V(Item::OilLamp) && kept[7] == HotbarEmpty);
    std::vector<int> full = two;
    full[7] = V(Item::Pasty); full[8] = V(Item::Bread); full[9] = V(Item::Cheese);
    CHECK(SanitizeHotbarLayout(full, 2)[2] == V(Item::OilLamp));
}

const char* filter = nullptr;
void Run(const char* name, void (*test)())
{
    if (filter && !std::strstr(name, filter)) return;
    test();
    ++cases;
    std::cout << "PASS " << name << '\n';
}
}

int main(int argc, char** argv)
{
    if (argc > 1) filter = argv[1];
    Run("ten keyed cells; the old pin rule", KeysAndTheOldPinRule);
    Run("new things fill the first empty cell", NewThingsFillTheFirstEmptyCell);
    Run("uses draw from the row and leave it empty", UsesDrawFromTheRowAndLeaveItEmpty);
    Run("moves merge or swap", MovesMergeOrSwap);
    Run("garments take cells too", GarmentsTakeCellsToo);
    Run("sorting leaves the row alone", SortingLeavesTheRowAlone);
    Run("chests trade with the row", ChestsTradeWithTheRow);
    Run("the row is saved", TheRowIsSaved);
    Run("old pinned hotbars migrate once", OldPinnedHotbarsMigrateOnce);
    Run("saved pin lists sanitize", SavedLayoutsSanitize);
    std::cout << cases << " scenarios, " << checks << " explicit checks passed.\n";
    return 0;
}
