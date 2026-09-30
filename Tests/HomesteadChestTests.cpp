// Portable tests for her storage chests: auto-store onto matching stacks and custom names.
#include "HomesteadChests.h"
#include "HomesteadEstate.h"
#include "HomesteadItems.h"
#include "HomesteadPackRow.h"
#include "HomesteadSimulation.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

using namespace Homestead;

namespace
{
int checks = 0;
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

struct Estate
{
    Simulation sim;
    int chest = 0;
    Point at{};
};

// A new estate game standing at the standing room's chest (its pail, branches, pasties and bread).
Estate NewEstate()
{
    Estate estate;
    OK(estate.sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    for (const auto& piece : estate.sim.GetState().structures)
        if (piece.kind == Piece::Chest) { estate.chest = piece.id; estate.at = StructureCenter(estate.sim.GetState(), piece); break; }
    CHECK(estate.chest > 0);
    return estate;
}

int ChestCount(const Estate& estate, Item item)
{
    for (const auto& piece : estate.sim.GetState().structures)
        if (piece.id == estate.chest) return piece.storage[static_cast<int>(item)];
    return -1;
}

int GroupOf(const Simulation& sim, Item item)
{
    for (const auto& entry : sim.GetState().inventoryLayout)
        if (entry.wearableId == 0 && entry.item == item) return entry.groupId;
    return 0;
}

bool InRow(const Simulation& sim, int groupId)
{
    for (const auto& cell : sim.GetState().packRow) if (cell.groupId == groupId) return true;
    return false;
}

Result Store(Estate& estate) { return estate.sim.StoreMatching(estate.chest, estate.at, estate.sim.GetRevision()); }

// Tops the chest up with Stone until it holds `used` things.
void FillChestTo(Estate& estate, int used)
{
    auto& sim = estate.sim;
    if (ChestCount(estate, Item::Stone) == 0)
    {
        OK(sim.GrantItems(Item::Stone, 1));
        OK(sim.TransferGroup(estate.chest, GroupOf(sim, Item::Stone), 1, true, estate.at, sim.GetRevision()));
    }
    while (sim.ChestUsedCapacity(estate.chest) < used)
    {
        const int want = std::min(used - sim.ChestUsedCapacity(estate.chest), InventoryCapacity - sim.UsedCapacity());
        CHECK(want > 0);
        OK(sim.GrantItems(Item::Stone, want));
        OK(Store(estate));
    }
    CHECK(sim.ChestUsedCapacity(estate.chest) == used);
}

void StoresOnlyMatchingGoods()
{
    Estate estate = NewEstate();
    auto& sim = estate.sim;
    const int branchesBefore = ChestCount(estate, Item::Branch);
    CHECK(branchesBefore > 0 && ChestCount(estate, Item::Stone) == 0);
    OK(sim.GrantItems(Item::Branch, 5));
    OK(sim.GrantItems(Item::Stone, 3));
    const int branchGroup = GroupOf(sim, Item::Branch);
    const int stoneGroup = GroupOf(sim, Item::Stone);
    CHECK(InRow(sim, branchGroup) && InRow(sim, stoneGroup));
    const auto equipment = sim.GetState().equipment;
    const auto wearables = sim.GetState().wearables.size();
    const auto lampBefore = sim.Count(Item::OilLamp);
    const std::uint64_t revision = sim.GetRevision();
    const Result stored = Store(estate);
    OK(stored);
    CHECK(stored.message == "Stored 5 items onto matching stacks.");
    CHECK(sim.GetRevision() == revision + 1);
    CHECK(ChestCount(estate, Item::Branch) == branchesBefore + 5 && sim.Count(Item::Branch) == 0);
    // Unmatched goods, tools and clothes stay with her; the stone keeps its hotbar cell.
    CHECK(sim.Count(Item::Stone) == 3 && ChestCount(estate, Item::Stone) == 0 && InRow(sim, stoneGroup));
    CHECK(!InRow(sim, branchGroup));
    CHECK(sim.Count(Item::OilLamp) == lampBefore && sim.GetState().equipment == equipment);
    CHECK(sim.GetState().wearables.size() == wearables);
    // The chest's branches are still one stack.
    int branchStacks = 0;
    for (const auto& entry : *sim.GetLayout(estate.chest))
        branchStacks += entry.wearableId == 0 && entry.item == Item::Branch;
    CHECK(branchStacks == 1);
    // Saved and loaded the same.
    Simulation loaded;
    loaded.SetPlacements(ProvisionalEstatePlacements());
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(loaded.Serialize() == sim.Serialize());
}

void NothingToStoreChangesNothing()
{
    Estate estate = NewEstate();
    auto& sim = estate.sim;
    OK(sim.GrantItems(Item::Stone, 2));
    const std::string before = sim.Serialize();
    const std::uint64_t revision = sim.GetRevision();
    const Result none = Store(estate);
    CHECK(!none.ok && none.message == "Nothing in your pack matches what's already in this chest.");
    CHECK(sim.Serialize() == before && sim.GetRevision() == revision);
    // Stale, too far and not-a-chest are refused too.
    CHECK(sim.StoreMatching(estate.chest, estate.at, revision + 7).code == ResultCode::StaleRevision);
    CHECK(!sim.StoreMatching(estate.chest, {estate.at.x + 5000.0, estate.at.y}, revision).ok);
    CHECK(!sim.StoreMatching(0, estate.at, revision).ok);
    CHECK(sim.Serialize() == before);
    // Tools, the lamp and the pail's water never auto-store, whatever the chest holds.
    for (int i = 0; i < ItemCount; ++i)
    {
        const Item item = static_cast<Item>(i);
        if (IsTool(item) || item == Item::Water || item == Item::OilLamp) CHECK(!Chests::AutoStores(item));
    }
    CHECK(Chests::AutoStores(Item::Branch) && Chests::AutoStores(Item::Pasty));
}

void PartialWhenFullAndRowLast()
{
    Estate estate = NewEstate();
    auto& sim = estate.sim;
    FillChestTo(estate, ChestCapacity - 3);
    // A branch stack in the hotbar row and a split-off one below it.
    OK(sim.GrantItems(Item::Branch, 6));
    const int rowGroup = GroupOf(sim, Item::Branch);
    CHECK(InRow(sim, rowGroup));
    OK(sim.SplitGroup(0, rowGroup, 4, estate.at, sim.GetRevision()));
    int belowGroup = 0;
    for (const auto& entry : sim.GetState().inventoryLayout)
        if (entry.wearableId == 0 && entry.item == Item::Branch && entry.groupId != rowGroup) belowGroup = entry.groupId;
    CHECK(belowGroup > 0 && !InRow(sim, belowGroup));
    const Result partial = Store(estate);
    OK(partial);
    CHECK(partial.message == "Stored 3 of 6 matching items; the chest is full.");
    CHECK(sim.ChestUsedCapacity(estate.chest) == ChestCapacity && sim.Count(Item::Branch) == 3);
    // The stack below the row gave first; the row's stack is untouched.
    for (const auto& entry : sim.GetState().inventoryLayout)
    {
        if (entry.groupId == rowGroup) CHECK(entry.quantity == 2);
        if (entry.groupId == belowGroup) CHECK(entry.quantity == 1);
    }
    CHECK(InRow(sim, rowGroup));
    // Full: refused as a capacity problem, and nothing is lost.
    const std::string before = sim.Serialize();
    const Result full = Store(estate);
    CHECK(!full.ok && full.code == ResultCode::Capacity && full.message == "The chest is full.");
    CHECK(sim.Serialize() == before && sim.Count(Item::Branch) == 3);
}

void NamesPersistAndValidate()
{
    Estate estate = NewEstate();
    auto& sim = estate.sim;
    CHECK(Chests::DisplayName(sim.GetState(), estate.chest) == Chests::DefaultName);
    const std::string unnamed = sim.Serialize();
    // No section until a chest is named, so an unnamed game saves exactly as before.
    CHECK(unnamed.find(std::string("\n") + Chests::SaveTag + " ") == std::string::npos);
    const std::uint64_t revision = sim.GetRevision();
    const Result named = sim.RenameChest(estate.chest, "  Nan's linen \xE2\x80\x93 winter  ", estate.at, revision);
    OK(named);
    CHECK(sim.GetRevision() == revision + 1);
    CHECK(Chests::DisplayName(sim.GetState(), estate.chest) == "Nan's linen \xE2\x80\x93 winter");
    CHECK(named.message == "The chest is now called Nan's linen \xE2\x80\x93 winter.");
    const std::string saved = sim.Serialize();
    CHECK(saved.find(std::string("\n") + Chests::SaveTag + " 1 ") != std::string::npos);
    Simulation loaded;
    loaded.SetPlacements(ProvisionalEstatePlacements());
    OK(loaded.Deserialize(saved));
    CHECK(Chests::DisplayName(loaded.GetState(), estate.chest) == "Nan's linen \xE2\x80\x93 winter");
    CHECK(loaded.Serialize() == saved);
    // A save from before names loads with the default name.
    Simulation older;
    older.SetPlacements(ProvisionalEstatePlacements());
    OK(older.Deserialize(unnamed));
    CHECK(Chests::DisplayName(older.GetState(), estate.chest) == Chests::DefaultName);

    // Refusals change nothing.
    const std::uint64_t now = sim.GetRevision();
    CHECK(!sim.RenameChest(estate.chest, std::string(Chests::MaxNameLength + 1, 'a'), estate.at, now).ok);
    CHECK(sim.RenameChest(estate.chest, std::string(Chests::MaxNameLength, 'a'), estate.at, now).ok);
    const std::uint64_t after = sim.GetRevision();
    CHECK(!sim.RenameChest(estate.chest, "bad\x01name", estate.at, after).ok);
    CHECK(!sim.RenameChest(estate.chest, std::string(Chests::MaxNameLength, 'a'), estate.at, after).ok);
    CHECK(sim.RenameChest(estate.chest, "Seed", estate.at, after + 3).code == ResultCode::StaleRevision);
    CHECK(!sim.RenameChest(estate.chest, "Seed", {estate.at.x + 5000.0, estate.at.y}, after).ok);
    CHECK(sim.GetRevision() == after);
    // An empty name puts the default back and drops the section.
    OK(sim.RenameChest(estate.chest, "   ", estate.at, after));
    CHECK(Chests::DisplayName(sim.GetState(), estate.chest) == Chests::DefaultName);
    CHECK(sim.Serialize().find(std::string("\n") + Chests::SaveTag + " ") == std::string::npos);
    CHECK(!sim.RenameChest(estate.chest, "", estate.at, sim.GetRevision()).ok);

    // Corrupt sections are refused: an unknown id, a non-chest id, an untrimmed or empty name.
    const std::string payload = saved.substr(saved.find('\n') + 1);
    const auto start = payload.find(std::string("\n") + Chests::SaveTag + " ");
    const auto end = payload.find('\n', start + 1);
    const auto withLine = [&](const std::string& line)
        { return Reseal(saved, payload.substr(0, start + 1) + line + payload.substr(end + 1)); };
    int other = 0;
    for (const auto& piece : sim.GetState().structures) if (piece.kind != Piece::Chest) { other = piece.id; break; }
    for (const std::string& line : {std::string("chestnames 1 999999 6162\n"),
        "chestnames 1 " + std::to_string(other) + " 6162\n",
        "chestnames 1 " + std::to_string(estate.chest) + " 206162\n",
        "chestnames 1 " + std::to_string(estate.chest) + " 6g\n",
        "chestnames 2 " + std::to_string(estate.chest) + " 6162 " + std::to_string(estate.chest) + " 6162\n"})
    {
        Simulation bad;
        bad.SetPlacements(ProvisionalEstatePlacements());
        CHECK(!bad.Deserialize(withLine(line)).ok);
    }
    Simulation good;
    good.SetPlacements(ProvisionalEstatePlacements());
    OK(good.Deserialize(withLine("chestnames 1 " + std::to_string(estate.chest) + " 6162\n")));
    CHECK(Chests::DisplayName(good.GetState(), estate.chest) == "ab");
}
}

int main()
{
    StoresOnlyMatchingGoods();
    NothingToStoreChangesNothing();
    PartialWhenFullAndRowLast();
    NamesPersistAndValidate();
    std::cout << "Chests: " << checks << " checks passed.\n";
    return 0;
}
