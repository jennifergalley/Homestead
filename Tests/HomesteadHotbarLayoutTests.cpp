#include "HomesteadEstate.h"
#include "HomesteadHotbarLayout.h"
#include "HomesteadSimulation.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
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

constexpr int V(Item item) { return static_cast<int>(item); }

// The layout a new game starts with (AHomesteadController::ResetHotbar).
HotbarLayout NewGameLayout()
{
    return {V(Item::Billhook), V(Item::Hatchet), V(Item::Scythe), V(Item::Pickaxe), V(Item::DiggingStick),
        V(Item::WateringCan), V(Item::Berries), V(Item::OilLamp), HotbarEmpty, HotbarEmpty};
}

bool NoDuplicates(const HotbarLayout& slots)
{
    for (int a = 0; a < HotbarSize; ++a)
        for (int b = a + 1; b < HotbarSize; ++b)
            if (slots[a] != HotbarEmpty && slots[a] == slots[b]) return false;
    return true;
}

void TenNumberedSlots()
{
    CHECK(HotbarSize == 10);
    const char* keys[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    for (int slot = 0; slot < HotbarSize; ++slot) CHECK(HotbarKeyLabel(slot) == keys[slot]);
    // What can ride on it: tools and the lamp, food, seed; never materials or water.
    for (Item item : {Item::Hatchet, Item::Billhook, Item::WateringCan, Item::OilLamp, Item::Berries, Item::Pasty,
        Item::Seeds, Item::TurnipSeed, Item::StrawberryRunner})
        CHECK(CanPinToHotbar(item));
    for (Item item : {Item::Stone, Item::Branch, Item::Water, Item::Timber, Item::OilFlask, Item::RustedAxeHead})
        CHECK(!CanPinToHotbar(item));
    CHECK(!CanPinToHotbar(Item::Count) && !CanPinToHotbar(static_cast<Item>(-1)));
}

void PackStackOntoASlot()
{
    // Onto an empty slot.
    HotbarLayout slots = NewGameLayout();
    auto edit = AssignHotbarSlot(slots, Item::TurnipSeed, 8, true);
    CHECK(edit.code == HotbarEditCode::Changed && edit.unpinned == HotbarEmpty && slots[8] == V(Item::TurnipSeed));
    // Onto a used slot: it replaces the binding; the old item is unpinned (still in her pack).
    edit = AssignHotbarSlot(slots, Item::Pasty, 6, true);
    CHECK(edit.code == HotbarEditCode::Changed && edit.unpinned == V(Item::Berries));
    CHECK(slots[6] == V(Item::Pasty) && HotbarSlotOf(slots, Item::Berries) == HotbarEmpty);
    // An item already on the hotbar moves, and what was in the slot swaps back into its old place.
    edit = AssignHotbarSlot(slots, Item::Hatchet, 3, true);
    CHECK(edit.code == HotbarEditCode::Changed && edit.unpinned == HotbarEmpty);
    CHECK(slots[3] == V(Item::Hatchet) && slots[1] == V(Item::Pickaxe) && NoDuplicates(slots));
    // Into an empty slot, it just moves.
    edit = AssignHotbarSlot(slots, Item::Hatchet, 9, true);
    CHECK(slots[9] == V(Item::Hatchet) && slots[3] == HotbarEmpty && NoDuplicates(slots));
    // Onto its own slot: nothing changes.
    const HotbarLayout before = slots;
    edit = AssignHotbarSlot(slots, Item::Hatchet, 9, true);
    CHECK(edit.code == HotbarEditCode::Unchanged && !edit.Refused() && slots == before);
}

void RefusalsChangeNothing()
{
    HotbarLayout slots = NewGameLayout();
    const HotbarLayout before = slots;
    // A stack in a chest has to come into her pack first.
    auto edit = AssignHotbarSlot(slots, Item::Pasty, 8, false);
    CHECK(edit.Refused() && edit.code == HotbarEditCode::NotInPack && slots == before);
    CHECK(edit.message == "Take it to your pack first, then put it on the hotbar.");
    // Even an item already on the hotbar can't be re-bound from a chest stack.
    CHECK(AssignHotbarSlot(slots, Item::Berries, 9, false).code == HotbarEditCode::NotInPack && slots == before);
    edit = AssignHotbarSlot(slots, Item::Stone, 8, true);
    CHECK(edit.Refused() && edit.message == "Only tools, food and seeds can go on the hotbar." && slots == before);
    edit = AssignHotbarSlot(slots, Item::Water, 8, true);
    CHECK(edit.code == HotbarEditCode::NotPinnable && slots == before);
    for (int bad : {-1, 10, 99})
    {
        CHECK(AssignHotbarSlot(slots, Item::Pasty, bad, true).code == HotbarEditCode::BadSlot);
        CHECK(MoveHotbarSlot(slots, 0, bad).code == HotbarEditCode::BadSlot);
        CHECK(MoveHotbarSlot(slots, bad, 0).code == HotbarEditCode::BadSlot);
    }
    CHECK(MoveHotbarSlot(slots, 8, 2).code == HotbarEditCode::EmptySource);
    CHECK(slots == before);
}

void SlotOntoSlot()
{
    HotbarLayout slots = NewGameLayout();
    // Into an empty slot it moves, and the slot it left is cleared.
    auto edit = MoveHotbarSlot(slots, 6, 9);
    CHECK(edit.code == HotbarEditCode::Changed && slots[9] == V(Item::Berries) && slots[6] == HotbarEmpty);
    // Onto a used slot the two swap.
    edit = MoveHotbarSlot(slots, 0, 1);
    CHECK(edit.code == HotbarEditCode::Changed && slots[0] == V(Item::Hatchet) && slots[1] == V(Item::Billhook));
    const HotbarLayout before = slots;
    CHECK(MoveHotbarSlot(slots, 4, 4).code == HotbarEditCode::Unchanged && slots == before);
    // Many moves never duplicate or lose a binding.
    int bound = 0;
    for (int slot : slots) bound += slot != HotbarEmpty;
    for (int step = 0; step < 40; ++step) MoveHotbarSlot(slots, (step * 7) % 10, (step * 3 + 1) % 10);
    int after = 0;
    for (int slot : slots) after += slot != HotbarEmpty;
    CHECK(after == bound && NoDuplicates(slots));
}

void EditsNeverTouchHerStock()
{
    Simulation sim;
    CHECK(sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()).ok);
    const std::string saved = sim.Serialize();
    const auto revision = sim.GetRevision();
    const auto money = sim.GetState().money;
    HotbarLayout slots = NewGameLayout();
    AssignHotbarSlot(slots, Item::Pasty, 8, true);
    AssignHotbarSlot(slots, Item::Hatchet, 8, true);
    MoveHotbarSlot(slots, 0, 9);
    AssignHotbarSlot(slots, Item::Stone, 2, true);
    CHECK(sim.Serialize() == saved && sim.GetRevision() == revision && sim.GetState().money == money);
}

void SavedLayoutsLoadBack()
{
    // Round trip through what the save stores.
    HotbarLayout slots = NewGameLayout();
    AssignHotbarSlot(slots, Item::TurnipSeed, 8, true);
    MoveHotbarSlot(slots, 0, 5);
    const std::vector<int> stored(slots.begin(), slots.end());
    CHECK(SanitizeHotbarLayout(stored, 3) == slots);
    const HotbarLayout fresh = NewGameLayout();
    CHECK(SanitizeHotbarLayout(std::vector<int>(fresh.begin(), fresh.end()), 3) == fresh);
    // A seed she has none of keeps its slot (the book shows it; the world hotbar shows it empty),
    // so it comes back when she buys more.
    Simulation sim;
    CHECK(sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()).ok);
    CHECK(sim.Count(Item::TurnipSeed) == 0);
    CHECK(SanitizeHotbarLayout(stored, 3)[8] == V(Item::TurnipSeed));
    // Anything unsafe in a save becomes an empty slot: unknown values, materials and repeats.
    const std::vector<int> damaged = {V(Item::Hatchet), 9999, V(Item::Stone), V(Item::Hatchet), -7, V(Item::Pasty),
        HotbarEmpty, HotbarEmpty, V(Item::OilLamp), HotbarEmpty, V(Item::Berries), V(Item::Scythe)};
    const HotbarLayout clean = SanitizeHotbarLayout(damaged, 3);
    const HotbarLayout expected = {V(Item::Hatchet), HotbarEmpty, HotbarEmpty, HotbarEmpty, HotbarEmpty, V(Item::Pasty),
        HotbarEmpty, HotbarEmpty, V(Item::OilLamp), HotbarEmpty};
    CHECK(clean == expected);
    // A short (or empty) save fills the rest with empty slots.
    const HotbarLayout shortOne = SanitizeHotbarLayout({V(Item::Hatchet)}, 3);
    CHECK(shortOne[0] == V(Item::Hatchet));
    for (int slot = 1; slot < HotbarSize; ++slot) CHECK(shortOne[slot] == HotbarEmpty);
    // An old save with no hotbar array at all: current layout stays empty; layout 0 gains the defaults.
    const HotbarLayout none = SanitizeHotbarLayout({}, 3);
    for (int slot = 0; slot < HotbarSize; ++slot) CHECK(none[slot] == HotbarEmpty);
    const HotbarLayout aged = SanitizeHotbarLayout({}, 0);
    CHECK(aged[7] == V(Item::OilLamp) && aged[0] == V(Item::Billhook) && aged[1] == V(Item::Scythe)
        && aged[2] == V(Item::Pickaxe) && aged[3] == V(Item::Berries) && aged[4] == HotbarEmpty);
}

void OlderLayoutsMigrate()
{
    // Layout 1 (before the estate tools): the lamp goes in the first free slot from 8, then the
    // billhook, scythe, pickaxe and berries fill free slots in order.
    const std::vector<int> old = {V(Item::Knife), V(Item::Hatchet), V(Item::DiggingStick), V(Item::WateringCan),
        HotbarEmpty, HotbarEmpty, HotbarEmpty, HotbarEmpty, HotbarEmpty, HotbarEmpty};
    const HotbarLayout migrated = SanitizeHotbarLayout(old, 1);
    CHECK(migrated[1] == V(Item::Hatchet) && migrated[2] == V(Item::DiggingStick) && migrated[3] == V(Item::WateringCan));
    CHECK(migrated[7] == V(Item::OilLamp));
    // The retired knife no longer rides on the hotbar, so the billhook takes its slot.
    CHECK(migrated[0] == V(Item::Billhook) && migrated[4] == V(Item::Scythe) && migrated[5] == V(Item::Pickaxe)
        && migrated[6] == V(Item::Berries));
    CHECK(NoDuplicates(migrated));
    // Layout 2 only gains the lamp; one she already pinned stays where it is.
    const std::vector<int> two = {V(Item::Billhook), V(Item::Hatchet), HotbarEmpty, HotbarEmpty, HotbarEmpty,
        HotbarEmpty, HotbarEmpty, HotbarEmpty, HotbarEmpty, HotbarEmpty};
    CHECK(SanitizeHotbarLayout(two, 2)[7] == V(Item::OilLamp));
    CHECK(HotbarSlotOf(SanitizeHotbarLayout(two, 2), Item::Scythe) == HotbarEmpty);
    std::vector<int> lampPinned = two;
    lampPinned[3] = V(Item::OilLamp);
    const HotbarLayout kept = SanitizeHotbarLayout(lampPinned, 2);
    CHECK(kept[3] == V(Item::OilLamp) && kept[7] == HotbarEmpty);
    // With slots 8-10 full, the lamp wraps round to the first free slot.
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
    Run("ten numbered slots take tools, food and seed", TenNumberedSlots);
    Run("a pack stack onto a slot", PackStackOntoASlot);
    Run("refusals change nothing", RefusalsChangeNothing);
    Run("a slot onto a slot moves or swaps", SlotOntoSlot);
    Run("editing the hotbar never touches her stock", EditsNeverTouchHerStock);
    Run("saved hotbars load back", SavedLayoutsLoadBack);
    Run("older hotbars migrate", OlderLayoutsMigrate);
    std::cout << cases << " scenarios, " << checks << " explicit checks passed.\n";
    return 0;
}
