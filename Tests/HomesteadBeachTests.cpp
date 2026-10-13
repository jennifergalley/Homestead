#include "HomesteadBeach.h"
#include "HomesteadEstate.h"
#include "HomesteadItems.h"
#include "HomesteadRecipes.h"
#include "HomesteadShops.h"
#include "HomesteadSimulation.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

// add-beach-gathering: beach finds that come back each morning, their meals and sale, and the beach campfire.
namespace
{
int BeachChecks = 0;
void Check(bool condition, const char* expression, int line)
{
    ++BeachChecks;
    if (condition) return;
    std::cerr << "FAILED: " << expression << " (line " << line << ")\n";
    std::exit(1);
}
#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)

using namespace Homestead;

const ResourceKind Finds[] = {ResourceKind::Seashells, ResourceKind::Driftwood, ResourceKind::BeachPebbles,
    ResourceKind::MusselBed, ResourceKind::LaverRocks};
constexpr int FirstId = EstatePlacementIdBase + 40000;
constexpr int CampfireId = FirstId + 10;

Point Spot(int index)
{
    const Point spawn = ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {});
    return {spawn.x + 2500.0 + index * 600.0, spawn.y + 2500.0};
}

EstatePlacements Table()
{
    EstatePlacements placements;
    placements.bakeVersion = 9;
    for (int i = 0; i < 5; ++i) placements.placements.push_back({FirstId + i, Finds[i], Spot(i), 0, 0, 1, 0});
    placements.placements.push_back({CampfireId, ResourceKind::BeachCampfire, Spot(6), 0, 0, 1, 0});
    return placements;
}

const ResourceNode* Node(const Simulation& sim, int id)
{
    for (const auto& node : sim.GetState().resources) if (node.id == id) return &node;
    return nullptr;
}

void Catalogue()
{
    const Item raw[] = {Item::Seashell, Item::Driftwood, Item::Mussels, Item::Laver};
    const int prices[] = {5, 3, 6, 4};
    for (int i = 0; i < 4; ++i)
    {
        CHECK(BasePrice(raw[i]) == prices[i] && SellPrice(raw[i]) == prices[i]);
        CHECK(ShopBuys(ShopKind::GeneralStore, raw[i]));
        CHECK(!IsEdible(raw[i]));
        CHECK(ItemFromKey(ItemKey(raw[i])) == raw[i]);
    }
    CHECK(CountedName(Item::Seashell, 2) == "2 seashells" && CountedName(Item::Mussels, 3) == "3 mussels");
    // Balance 2026-10-10: Meals by round(12 + 0.6 x coins forgone), kindling (3) included.
    const Recipe meals[] = {Recipe::SteamedMussels, Recipe::MusselPotatoBroth, Recipe::FriedLaver};
    const double energy[] = {25.0, 35.0, 19.0};
    for (int i = 0; i < 3; ++i)
    {
        const auto* meal = FindBeachMeal(meals[i]);
        CHECK(meal && FindMeal(meals[i]) == meal && meal->cooking && IsRecipeAvailable(meals[i]));
        std::int64_t forgone = 0;
        for (const auto& ingredient : meal->ingredients)
            if (ingredient.item != Item::Count) forgone += SellPrice(ingredient.item) * ingredient.count;
        const ItemInfo& info = GetItemInfo(meal->output);
        CHECK(info.basePriceCoins == forgone && info.buyers == 0);
        CHECK(FoodClassOf(meal->output) == FoodClass::Meal);
        CHECK(info.energy == energy[i] && CropMealEnergyForCost(forgone) == energy[i]);
        CHECK(std::string(RecipeName(meals[i])) == ItemName(meal->output));
        CHECK(MealChange(meals[i])[static_cast<int>(meal->output)] == 1);
    }
    CHECK(!FindBeachMeal(Recipe::GrilledTrout) && !FindBeachMeal(Recipe::Count));
}

void DailyFinds()
{
    // The morning rollover: strictly after the hour, never more than a day ahead.
    CHECK(Beach::NextMorning(6.0) == 30.0 && Beach::NextMorning(5.5) == 6.0 && Beach::NextMorning(29.99) == 30.0);
    CHECK(Beach::NextMorning(30.0) == 54.0);
    // A picked find refills at the first 06:00 at least 12 h later: any pick from 06:00 to 18:00 is back tomorrow
    // morning; an evening or night pick (or one at 05:00) skips the very next 06:00.
    CHECK(Beach::RefillHour(10.0) == 30.0 && Beach::RefillHour(12.0) == 30.0 && Beach::RefillHour(18.0) == 30.0);
    CHECK(Beach::RefillHour(18.5) == 54.0 && Beach::RefillHour(30.0) == 54.0);
    CHECK(Beach::RefillHour(29.0) == 54.0 && Beach::RefillHour(22.0) == 54.0 && Beach::RefillHour(9.99) == 30.0);
    for (double pick = 6.0; pick < 80.0; pick += 0.37)
    {
        const double refill = Beach::RefillHour(pick);
        CHECK(refill - pick >= Beach::MinRefillHours && refill - pick < Beach::MinRefillHours + 24.0);
        CHECK(std::fmod(refill - 6.0, 24.0) == 0.0);
    }
    const Inventory expected[] = {Beach::DailyYield(ResourceKind::Seashells), Beach::DailyYield(ResourceKind::Driftwood),
        Beach::DailyYield(ResourceKind::BeachPebbles), Beach::DailyYield(ResourceKind::MusselBed),
        Beach::DailyYield(ResourceKind::LaverRocks)};
    CHECK(expected[0][static_cast<int>(Item::Seashell)] == 2 && expected[1][static_cast<int>(Item::Driftwood)] == 3
        && expected[2][static_cast<int>(Item::Stone)] == 2 && expected[3][static_cast<int>(Item::Mussels)] == 3
        && expected[4][static_cast<int>(Item::Laver)] == 2);

    Simulation sim;
    CHECK(sim.NewEstateGame(ProvisionalEstateLayout(), Table()));
    // Late in the day: everything gathers once, then waits for 06:00 rather than a fixed regrowth.
    sim.AdvanceGameHours(15.0, Spot(0));
    for (int i = 0; i < 5; ++i)
    {
        CHECK(sim.SetEnergy(100.0));
        Inventory before{};
        for (int item = 0; item < ItemCount; ++item) before[item] = sim.Count(static_cast<Item>(item));
        CHECK(sim.CanHarvest(FirstId + i));
        CHECK(sim.Harvest(FirstId + i, Spot(i)));
        CHECK(std::abs(sim.GetState().energy - 99.5) < 1e-6);
        for (int item = 0; item < ItemCount; ++item)
            CHECK(sim.Count(static_cast<Item>(item)) - before[item] == expected[i][item]);
        CHECK(!sim.Harvest(FirstId + i, Spot(i)));
        CHECK(Node(sim, FirstId + i)->readyAtHour == Beach::RefillHour(sim.GetState().hour));
    }
    const double morning = Node(sim, FirstId)->readyAtHour;
    // Saved and loaded, the picked finds stay picked until the morning.
    const std::string saved = sim.Serialize();
    Simulation loaded;
    loaded.SetPlacements(Table());
    CHECK(loaded.Deserialize(saved));
    CHECK(loaded.Serialize() == saved);
    CHECK(Node(loaded, FirstId)->readyAtHour == morning && !loaded.CanHarvest(FirstId));
    sim.AdvanceGameHours(morning - sim.GetState().hour - 0.05, Spot(0));
    for (int i = 0; i < 5; ++i) CHECK(!sim.CanHarvest(FirstId + i));
    sim.AdvanceGameHours(0.1, Spot(0));
    for (int i = 0; i < 5; ++i) CHECK(sim.CanHarvest(FirstId + i));
    // Nothing on the beach can be cleared away for good.
    CHECK(!sim.Clear(FirstId, Spot(0)) && !Node(sim, FirstId)->cleared);
}

// A shore rock is solid, so she stops at its edge: gathering reaches past the centre by the edge allowance,
// and other finds keep the plain 300 cm reach.
void RockReach()
{
    CHECK(Beach::EdgeAllowanceCm(ResourceKind::MusselBed) > 0.0 && Beach::EdgeAllowanceCm(ResourceKind::LaverRocks) > 0.0);
    CHECK(Beach::EdgeAllowanceCm(ResourceKind::Seashells) == 0.0 && Beach::EdgeAllowanceCm(ResourceKind::Driftwood) == 0.0);
    const double reach = 300.0;
    const double edge = Beach::RockEdgeAllowanceCm;
    for (const int i : {3, 4})
    {
        Simulation sim;
        CHECK(sim.NewEstateGame(ProvisionalEstateLayout(), Table()));
        sim.AdvanceGameHours(15.0, Spot(0));
        CHECK(sim.SetEnergy(100.0));
        const Point centre = Spot(i);
        CHECK(!sim.Harvest(FirstId + i, {centre.x + reach + edge + 5.0, centre.y}));
        CHECK(sim.Harvest(FirstId + i, {centre.x + reach + edge - 5.0, centre.y}));
    }
    Simulation sim;
    CHECK(sim.NewEstateGame(ProvisionalEstateLayout(), Table()));
    sim.AdvanceGameHours(15.0, Spot(0));
    CHECK(sim.SetEnergy(100.0));
    const Point shells = Spot(0);
    CHECK(!sim.Harvest(FirstId, {shells.x + reach + 5.0, shells.y}));
    CHECK(sim.Harvest(FirstId, {shells.x + reach - 5.0, shells.y}));
}

void Campfire()
{
    Simulation sim;
    CHECK(sim.NewEstateGame(ProvisionalEstateLayout(), Table()));
    const Point fire = Spot(6);
    const auto* node = Node(sim, CampfireId);
    CHECK(node && !Beach::CampfireLit(*node, sim.GetState().hour) && sim.CanHarvest(CampfireId));
    CHECK(!sim.IsNearFire(fire));
    // Not enough driftwood: nothing changes.
    CHECK(sim.GrantItems(Item::Driftwood, 2));
    const auto refused = sim.Harvest(CampfireId, fire);
    CHECK(!refused && refused.message == "Needs 3 driftwood.");
    CHECK(sim.Count(Item::Driftwood) == 2 && !sim.IsNearFire(fire));
    CHECK(!sim.Harvest(CampfireId, {fire.x + 2000.0, fire.y}));
    // Lit: three driftwood burn for four hours, and it cooks like a cookfire.
    CHECK(sim.GrantItems(Item::Driftwood, 1));
    CHECK(sim.SetEnergy(100.0));
    const double litAt = sim.GetState().hour;
    CHECK(sim.Harvest(CampfireId, fire));
    CHECK(sim.Count(Item::Driftwood) == 0 && std::abs(sim.GetState().energy - (100.0 - Exertion::FuelEnergy)) < 1e-6);
    CHECK(Beach::CampfireLit(*Node(sim, CampfireId), sim.GetState().hour));
    CHECK(Node(sim, CampfireId)->readyAtHour == litAt + Beach::CampfireBurnHours);
    CHECK(sim.IsNearFire(fire) && !sim.CanHarvest(CampfireId));
    CHECK(sim.GrantItems(Item::Driftwood, 3));
    CHECK(!sim.Harvest(CampfireId, fire) && sim.Count(Item::Driftwood) == 3);
    CHECK(sim.GrantItems(Item::Mussels, 3) && sim.GrantItems(Item::Kindling, 1));
    CHECK(sim.Craft(Recipe::SteamedMussels, fire));
    CHECK(sim.Count(Item::SteamedMussels) == 1 && sim.Count(Item::Mussels) == 0);
    // Saved mid-burn, it loads still burning.
    const std::string saved = sim.Serialize();
    Simulation loaded;
    loaded.SetPlacements(Table());
    CHECK(loaded.Deserialize(saved));
    CHECK(loaded.IsNearFire(fire) && Node(loaded, CampfireId)->readyAtHour == litAt + Beach::CampfireBurnHours);
    // Burnt out: no fire to cook at, and she can light it again.
    sim.AdvanceGameHours(Beach::CampfireBurnHours + 0.01, fire);
    CHECK(!Beach::CampfireLit(*Node(sim, CampfireId), sim.GetState().hour) && !sim.IsNearFire(fire));
    CHECK(sim.GrantItems(Item::Mussels, 3) && sim.GrantItems(Item::Kindling, 1));
    CHECK(!sim.Craft(Recipe::SteamedMussels, fire));
    CHECK(sim.SetEnergy(100.0));
    CHECK(sim.Harvest(CampfireId, fire) && sim.Count(Item::Driftwood) == 0 && sim.IsNearFire(fire));
    CHECK(!sim.Clear(CampfireId, fire));
}
// Balance 2026-10-10: forty beach finds round the cove (two reserved for the sea cave's mouth) and one campfire, in
// 586000-586099, generated by Scripts/Terrain/beach_detritus.py.
void EstateBeach()
{
    int finds = 0, campfires = 0;
    for (const auto& placement : ProvisionalEstatePlacements().placements)
    {
        if (placement.id < 586000 || placement.id >= 586100) continue;
        CHECK(Beach::IsBeachKind(placement.kind));
        if (placement.kind == ResourceKind::BeachCampfire) { ++campfires; CHECK(placement.id == 586000); }
        else ++finds;
    }
    CHECK(campfires == 1 && finds >= 36 && finds <= 40);
    Simulation sim;
    CHECK(sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    const auto* fire = Node(sim, 586000);
    CHECK(fire && !fire->cleared && !Beach::CampfireLit(*fire, sim.GetState().hour));
}
}

int main()
{
    Catalogue();
    DailyFinds();
    RockReach();
    Campfire();
    EstateBeach();
    std::cout << "HomesteadBeachTests passed (" << BeachChecks << " checks)\n";
    return 0;
}
