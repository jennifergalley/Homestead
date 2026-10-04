#include "HomesteadBackpack.h"
#include "HomesteadEstate.h"
#include "HomesteadFood.h"
#include "HomesteadRecipes.h"
#include "HomesteadSimulation.h"
#include "../Source/SurvivalGame/HomesteadOriginalItemArt.h"
#include "../Source/SurvivalGame/HomesteadFishingPresentationRules.h"
#include "../Source/SurvivalGame/HomesteadFishingPresentation.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>

namespace FishingTests
{
int Checks = 0;
void Check(bool value, int line)
{
    ++Checks;
    if (!value) { std::cerr << "Fishing check failed at line " << line << '\n'; std::exit(1); }
}
#define CHECK(value) FishingTests::Check(static_cast<bool>(value), __LINE__)
void Okay(Homestead::Result result, int line)
{
    if (!result.ok) std::cerr << result.message << '\n';
    Check(result.ok, line);
}
#define OK(value) FishingTests::Okay(value, __LINE__)

Homestead::Simulation Fisher(Homestead::FishingWater water)
{
    Homestead::Simulation sim;
    OK(sim.GrantItems(Homestead::Item::FishingPole, 1));
    sim.SetFishingWaterProbe([water](Homestead::Point p) { return p.x < 50.0 ? water : Homestead::FishingWater::None; });
    return sim;
}

void Hook(Homestead::Simulation& sim)
{
    OK(sim.BeginFishing({0, 0}));
    CHECK(sim.FishingCast().phase == Homestead::FishingPhase::Casting);
    OK(sim.AdvanceFishing(Homestead::Fishing::CastSplashSeconds, {0, 0}));
    OK(sim.FishingAnimationContact(Homestead::FishingContact::CastSplash, sim.FishingCast().token, {0, 0}));
    OK(sim.AdvanceFishing(sim.FishingCast().biteAfter, {0, 0}));
    CHECK(sim.FishingCast().phase == Homestead::FishingPhase::Bite);
    OK(sim.FishingPress({0, 0}));
    CHECK(sim.FishingCast().phase == Homestead::FishingPhase::Landing);
}

void LandingBeat(Homestead::Simulation& sim, double cueOffset = Homestead::Fishing::StrikeWindowSeconds * 0.5)
{
    OK(sim.AdvanceFishing(sim.FishingCast().strikeAfter + cueOffset, {0, 0}));
    OK(sim.FishingPress({0, 0}));
}

Homestead::Result Lift(Homestead::Simulation& sim)
{
    OK(sim.AdvanceFishing(Homestead::Fishing::CatchLiftSeconds, {0, 0}));
    return sim.FishingAnimationContact(Homestead::FishingContact::CatchLift, sim.FishingCast().token, {0, 0});
}

void PoleAndSales()
{
    using namespace Homestead;
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    const auto* shop = sim.FindShop(ShopKind::GeneralStore);
    CHECK(shop);
    const Point counter{shop->counterX, shop->counterY};
    const int shopId = shop->id;
    sim.SkipToHourOfDay(10.0);
    OK(sim.GrantMoney(10000));
    const auto& goods = ShopGoods(ShopKind::GeneralStore);
    CHECK(std::find(goods.begin(), goods.end(), Item::FishingPole) != goods.end());
    CHECK(Backpack::Offered(sim.GetState(), ShopKind::GeneralStore));
    CHECK(BuyPrice(Item::FishingPole) == 1500 && IsTool(Item::FishingPole));
    Simulation shortOfMoney = sim;
    OK(shortOfMoney.GrantMoney(1499 - shortOfMoney.GetState().money));
    const auto unaffordable = shortOfMoney.Serialize();
    CHECK(!shortOfMoney.Buy(shopId, Item::FishingPole, 1, false, counter));
    CHECK(shortOfMoney.Serialize() == unaffordable);
    const auto money = sim.GetState().money;
    OK(sim.Buy(shopId, Item::FishingPole, 1, false, counter));
    CHECK(sim.GetState().money == money - 1500 && sim.Count(Item::FishingPole) == 1);
    CHECK(!sim.GetState().leatherBackpack);
    const auto pole = std::find_if(sim.GetState().inventoryLayout.begin(), sim.GetState().inventoryLayout.end(),
        [](const LayoutEntry& entry) { return entry.item == Item::FishingPole; });
    CHECK(pole != sim.GetState().inventoryLayout.end() && pole->quantity == 1);
    const int group = pole->groupId;
    OK(sim.MoveToPackRow(group, 0, 3, sim.GetRevision()));
    CHECK(sim.GetState().packRow[3].groupId == group);
    OK(sim.BuyBackpack(shopId, counter));
    CHECK(std::find(goods.begin(), goods.end(), Item::FishingPole) != goods.end());
    for (Item fish : {Item::RiverTrout, Item::RiverSalmon, Item::LakePerch, Item::LakeCarp, Item::SeaMackerel, Item::SeaBass})
    {
        CHECK(ShopBuys(ShopKind::GeneralStore, fish) && !IsEdible(fish));
        OK(sim.GrantItems(fish, 1));
        const auto raw = sim.Serialize();
        CHECK(!sim.Eat(fish) && sim.Serialize() == raw);
        const auto before = sim.GetState().money;
        OK(sim.Sell(shopId, fish, 1, counter));
        CHECK(sim.GetState().money == before + SellPrice(fish) && sim.Count(fish) == 0);
    }
    Simulation loaded = sim;
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(loaded.Count(Item::FishingPole) == 1 && loaded.GetState().packRow[3].groupId == group);
    CHECK(loaded.Serialize() == sim.Serialize());
    OK(sim.GrantItems(Item::Stone, sim.PackCapacity() - sim.UsedCapacity()));
    const auto full = sim.Serialize();
    CHECK(!sim.Buy(shopId, Item::FishingPole, 1, false, counter));
    CHECK(sim.Serialize() == full);
}

void HabitatsAndReplay()
{
    using namespace Homestead;
    std::set<Item> all;
    for (FishingWater water : {FishingWater::River, FishingWater::Lake, FishingWater::Ocean})
    {
        std::set<Item> pool;
        for (std::uint64_t seed = 0; seed < 128; ++seed) pool.insert(Fishing::CatchFor(water, seed));
        CHECK(pool.size() == 2);
        for (Item fish : pool) CHECK(all.insert(fish).second);
        Simulation sim = Fisher(water);
        const auto before = sim.Serialize();
        const auto revision = sim.GetRevision();
        CHECK(sim.CheckFishing({0, 0}).ok && sim.Serialize() == before && sim.GetRevision() == revision);
        Simulation replay = sim;
        Hook(sim);
        Hook(replay);
        CHECK(sim.FishingCast().catchItem == replay.FishingCast().catchItem);
        CHECK(sim.FishingCast().biteAfter == replay.FishingCast().biteAfter);
        const Item caught = sim.FishingCast().catchItem;
        CHECK(pool.count(caught) == 1 && sim.Count(caught) == 0);
        CHECK(std::abs(sim.GetState().energy - (100.0 - Fishing::CastEnergy)) < 1e-9);
        LandingBeat(sim);
        CHECK(sim.Count(caught) == 0 && sim.FishingCast().landedBeats == 1);
        LandingBeat(sim);
        LandingBeat(replay);
        LandingBeat(replay);
        CHECK(sim.FishingCast().phase == FishingPhase::Catching && sim.Count(caught) == 0);
        OK(Lift(sim));
        OK(Lift(replay));
        CHECK(sim.Count(caught) == 1 && sim.FishingCast().phase == FishingPhase::Idle);
        CHECK(sim.Serialize() == replay.Serialize());
        const auto saved = sim.Serialize();
        OK(sim.Deserialize(saved));
        CHECK(sim.Serialize() == saved);
        CHECK(!sim.AdvanceFishing(1.0, {0, 0}));
    }
    CHECK(all.size() == 6 && Fishing::CatchFor(FishingWater::None, 1) == Item::Count);
    CHECK(Fishing::CatchFor(FishingWater::Count, 1) == Item::Count);
}

void TimingAndCancellation()
{
    using namespace Homestead;
    Simulation sim;
    const auto empty = sim.Serialize();
    CHECK(!sim.BeginFishing({0, 0}) && sim.Serialize() == empty);
    sim = Fisher(FishingWater::River);
    const auto before = sim.Serialize();
    CHECK(!sim.BeginFishing({60, 0}) && sim.Serialize() == before);
    CHECK(sim.FishingWaterAt({MaxWorldCoordinate + 1.0, 0}) == FishingWater::None);
    OK(sim.BeginFishing({0, 0}));
    const auto spent = sim.Serialize();
    const auto revision = sim.GetRevision();
    const auto token = sim.FishingCast().token;
    const double delay = sim.FishingCast().biteAfter;
    CHECK(delay >= 2.0 && delay < 4.0);
    CHECK(!sim.FishingAnimationContact(FishingContact::CastSplash, token, {0, 0}));
    CHECK(!sim.FishingAnimationContact(FishingContact::CatchLift, token, {0, 0}));
    OK(sim.AdvanceFishing(Fishing::CastSplashSeconds, {0, 0}));
    CHECK(sim.GetRevision() == revision && sim.Serialize() == spent);
    const double elapsed = sim.FishingCast().elapsed;
    CHECK(!sim.AdvanceFishing(std::numeric_limits<double>::quiet_NaN(), {0, 0}));
    CHECK(!sim.AdvanceFishing(-1, {0, 0}));
    CHECK(!sim.AdvanceFishing(61, {0, 0}));
    CHECK(!sim.AdvanceFishing(0, {std::numeric_limits<double>::infinity(), 0}));
    CHECK(sim.FishingCast().elapsed == elapsed && sim.Serialize() == spent);
    Simulation restored = sim;
    OK(restored.Deserialize(spent));
    CHECK(restored.FishingCast().phase == FishingPhase::Idle && restored.Serialize() == spent);
    OK(sim.FishingAnimationContact(FishingContact::CastSplash, token, {0, 0}));
    CHECK(!sim.FishingAnimationContact(FishingContact::CastSplash, token, {0, 0}));
    OK(sim.FishingPress({0, 0}));
    CHECK(sim.FishingCast().phase == FishingPhase::Idle && sim.Serialize() == spent);
    OK(sim.BeginFishing({0, 0}));
    CHECK(sim.FishingCast().token != token && sim.FishingCast().biteAfter != delay);
    CHECK(!sim.FishingAnimationContact(FishingContact::CastSplash, token, {0, 0}));
    OK(sim.AdvanceFishing(Fishing::CastContactTimeoutSeconds + 0.001, {0, 0}));
    CHECK(sim.FishingCast().phase == FishingPhase::Idle);
    OK(sim.BeginFishing({0, 0}));
    OK(sim.AdvanceFishing(Fishing::CastSplashSeconds, {0, 0}));
    OK(sim.FishingAnimationContact(FishingContact::CastSplash, sim.FishingCast().token, {0, 0}));
    Simulation noHook = sim;
    OK(noHook.AdvanceFishing(noHook.FishingCast().biteAfter + Fishing::HookWindowSeconds + 0.001, {0, 0}));
    CHECK(noHook.FishingCast().phase == FishingPhase::Idle);
    OK(sim.AdvanceFishing(sim.FishingCast().biteAfter + Fishing::HookWindowSeconds, {0, 0}));
    OK(sim.FishingPress({0, 0}));
    CHECK(sim.FishingCast().phase == FishingPhase::Landing);
    OK(sim.CancelFishing());
    Hook(sim);
    const double firstStrike = sim.FishingCast().strikeAfter;
    CHECK(firstStrike >= Fishing::MinStrikeSeconds && firstStrike < Fishing::MinStrikeSeconds + Fishing::StrikeVariationSeconds);
    for (double offset : {0.0, Fishing::StrikeWindowSeconds, -0.001, Fishing::StrikeWindowSeconds + 0.001})
    {
        Simulation trial = sim;
        LandingBeat(trial, offset);
        const bool inBand = offset >= 0.0 && offset <= Fishing::StrikeWindowSeconds;
        CHECK((trial.FishingCast().phase == FishingPhase::Landing) == inBand);
        CHECK(trial.Count(sim.FishingCast().catchItem) == 0);
    }
    Simulation late = sim;
    OK(late.AdvanceFishing(firstStrike + Fishing::StrikeWindowSeconds + 0.001, {0, 0}));
    CHECK(late.FishingCast().phase == FishingPhase::Idle);
    OK(sim.AdvanceFishing(0.0, {101, 0}));
    CHECK(sim.FishingCast().phase == FishingPhase::Idle);
    Hook(sim);
    const auto fishingSave = sim.Serialize();
    CHECK(!sim.Deserialize("incomplete save"));
    CHECK(sim.FishingCast().phase == FishingPhase::Landing && sim.Serialize() == fishingSave);
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    CHECK(sim.FishingCast().phase == FishingPhase::Idle);
    sim = Fisher(FishingWater::River);
    Hook(sim);
    OK(sim.CancelFishing());
    CHECK(sim.FishingCast().phase == FishingPhase::Idle && !sim.CancelFishing());
    Hook(sim);
    OK(sim.NewGame());
    CHECK(sim.FishingCast().phase == FishingPhase::Idle);
    sim = Fisher(FishingWater::Lake);
    OK(sim.GrantItems(Item::Stone, sim.PackCapacity() - sim.UsedCapacity()));
    const auto full = sim.Serialize();
    CHECK(!sim.BeginFishing({0, 0}) && sim.Serialize() == full);
    sim = Fisher(FishingWater::Ocean);
    Hook(sim);
    const Item catchItem = sim.FishingCast().catchItem;
    LandingBeat(sim);
    CHECK(sim.FishingCast().strikeAfter != firstStrike);
    LandingBeat(sim);
    const auto catchToken = sim.FishingCast().token;
    CHECK(!sim.FishingAnimationContact(FishingContact::CatchLift, catchToken, {0, 0}));
    Simulation noContact = sim;
    OK(noContact.AdvanceFishing(Fishing::CatchContactTimeoutSeconds + 0.001, {0, 0}));
    CHECK(noContact.FishingCast().phase == FishingPhase::Idle && noContact.Count(catchItem) == 0);
    Simulation cancelled = sim;
    OK(cancelled.CancelFishing());
    CHECK(!cancelled.FishingAnimationContact(FishingContact::CatchLift, catchToken, {0, 0}));
    Simulation success = sim;
    OK(Lift(success));
    const auto caughtSave = success.Serialize();
    CHECK(success.Count(catchItem) == 1);
    CHECK(!success.FishingAnimationContact(FishingContact::CatchLift, catchToken, {0, 0}));
    CHECK(success.Serialize() == caughtSave);
    OK(sim.GrantItems(Item::Stone, sim.PackCapacity() - sim.UsedCapacity()));
    CHECK(!Lift(sim));
    CHECK(sim.FishingCast().phase == FishingPhase::Idle && sim.Count(catchItem) == 0);
    sim = Fisher(FishingWater::River);
    Hook(sim);
    LandingBeat(sim);
    LandingBeat(sim);
    sim.SetFishingWaterProbe([](Point) { return FishingWater::Lake; });
    OK(sim.AdvanceFishing(Fishing::CatchLiftSeconds, {0, 0}));
    CHECK(sim.FishingCast().phase == FishingPhase::Idle && sim.Count(Item::RiverTrout) == 0 && sim.Count(Item::RiverSalmon) == 0);
}

void PresentationCompletionAndRecast()
{
    using namespace Homestead;
    using HomesteadFishingPresentationRules::FinishedMiss;
    using HomesteadFishingPresentationRules::NewCast;
    auto sim = Fisher(FishingWater::River);
    OK(sim.BeginFishing({0, 0}));
    const auto first = sim.FishingCast().token;
    CHECK(NewCast(sim.FishingCast(), 0));
    CHECK(!NewCast(sim.FishingCast(), first));
    OK(sim.CancelFishing());
    CHECK(!NewCast(sim.FishingCast(), first));
    OK(sim.BeginFishing({0, 0}));
    CHECK(sim.FishingCast().phase == FishingPhase::Casting && NewCast(sim.FishingCast(), first));
    CHECK(sim.FishingCast().elapsed == 0.0);
    CHECK(!sim.FishingAnimationContact(FishingContact::CastSplash, first, {0, 0}));
    CHECK(!sim.FishingAnimationContact(FishingContact::CastSplash, sim.FishingCast().token, {0, 0}));
    OK(sim.AdvanceFishing(Fishing::CastSplashSeconds, {0, 0}));
    OK(sim.FishingAnimationContact(FishingContact::CastSplash, sim.FishingCast().token, {0, 0}));
    CHECK(!NewCast(sim.FishingCast(), first));
    constexpr float lastSample = HomesteadFishingTiming::MissEnd - HomesteadFishingTiming::ClipFrameSeconds;
    constexpr double endMargin = HomesteadFishingTiming::ClipFrameSeconds + 0.0001;
    CHECK(!FinishedMiss(true, true, HomesteadFishingTiming::MissStart, HomesteadFishingTiming::MissEnd, endMargin));
    CHECK(!FinishedMiss(true, true, lastSample - HomesteadFishingTiming::ClipFrameSeconds,
        HomesteadFishingTiming::MissEnd, endMargin));
    CHECK(FinishedMiss(true, true, lastSample, HomesteadFishingTiming::MissEnd, endMargin));
    CHECK(FinishedMiss(true, true, HomesteadFishingTiming::MissEnd, HomesteadFishingTiming::MissEnd, endMargin));
    CHECK(FinishedMiss(true, false, -1.0, HomesteadFishingTiming::MissEnd, endMargin));
    CHECK(!FinishedMiss(false, true, lastSample, HomesteadFishingTiming::MissEnd, endMargin));
    CHECK(std::abs(HomesteadFishingTiming::CastSplashSeconds - Fishing::CastSplashSeconds) < 0.0001);
    CHECK(std::abs(HomesteadFishingTiming::CatchLiftSeconds - Fishing::CatchLiftSeconds) < 0.0001);
}

void Preparations()
{
    using namespace Homestead;
    Simulation base;
    OK(base.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    Point fire{};
    bool found = false;
    for (const auto& structure : base.GetState().structures)
        if (structure.kind == Piece::Hearth) { fire = base.StructureCenter(structure); found = true; break; }
    CHECK(found);
    std::set<Item> usedFish;
    int available = 0, deferred = 0;
    for (int index = static_cast<int>(Recipe::RawFishSlices); index <= static_cast<int>(Recipe::MackerelChowder); ++index)
    {
        const auto recipe = static_cast<Recipe>(index);
        const auto* meal = FindFishMeal(recipe);
        CHECK(meal);
        Simulation sim = base;
        Simulation noFuel = base;
        std::int64_t cost = 0;
        for (const auto& ingredient : meal->ingredients)
            if (ingredient.item != Item::Count)
            {
                OK(sim.GrantItems(ingredient.item, ingredient.count));
                if (ingredient.item != Item::Kindling) OK(noFuel.GrantItems(ingredient.item, ingredient.count));
                cost += SellPrice(ingredient.item) * ingredient.count;
                if (ingredient.item >= Item::RiverTrout && ingredient.item <= Item::SeaBass)
                    usedFish.insert(ingredient.item);
            }
        const double energy = GetItemInfo(meal->output).energy;
        CHECK(energy == CropMealEnergyForCost(cost));
        CHECK(FoodClassOf(meal->output) == FoodClass::Meal && energy <= 100.0);
        CHECK(FishMealChange(recipe)[static_cast<int>(Item::Kindling)] == (meal->cooking ? -1 : 0));
        const auto inspection = sim.AssessRecipe(recipe, fire);
        if (!IsRecipeAvailable(recipe))
        {
            ++deferred;
            const auto before = sim.Serialize();
            const auto revision = sim.GetRevision();
            CHECK(!inspection.craftable && inspection.output == Item::Count);
            CHECK(inspection.blocker == "This preparation is deferred from this playtest.");
            CHECK(!sim.Craft(recipe, fire) && sim.Serialize() == before && sim.GetRevision() == revision);
            continue;
        }
        ++available;
        CHECK(inspection.craftable && inspection.stationRequired == meal->cooking);
        CHECK(inspection.output == meal->output && inspection.outputCount == 1);
        if (meal->cooking)
        {
            const auto before = sim.Serialize(), fuelBefore = noFuel.Serialize();
            CHECK(!sim.Craft(recipe, {50000, 50000}) && sim.Serialize() == before);
            CHECK(!noFuel.Craft(recipe, fire) && noFuel.Serialize() == fuelBefore);
        }
        const auto stock = sim.GetState().inventory;
        OK(sim.Craft(recipe, meal->cooking ? fire : Point{50000, 50000}));
        const auto change = FishMealChange(recipe);
        for (int item = 0; item < ItemCount; ++item) CHECK(sim.GetState().inventory[item] == stock[item] + change[item]);
        const auto beforeSecond = sim.Serialize();
        CHECK(!sim.Craft(recipe, fire) && sim.Serialize() == beforeSecond);
        Simulation restored = base;
        OK(restored.Deserialize(sim.Serialize()));
        CHECK(restored.Serialize() == sim.Serialize());
        OK(sim.SetEnergy(10.0));
        const double beforeEnergy = sim.GetState().energy;
        OK(sim.Eat(meal->output));
        CHECK(sim.Count(meal->output) == 0 && sim.IsWellFed());
        CHECK(sim.GetState().energy == std::min(100.0, beforeEnergy + energy));
    }
    CHECK(usedFish.size() == 6);
    CHECK(available == 3 && deferred == 5);
    CHECK(IsRecipeAvailable(Recipe::RootVegetableHotpot) && IsRecipeAvailable(Recipe::GrilledPerch));
    CHECK(!IsRecipeAvailable(Recipe::Count)
        && !IsRecipeAvailable(static_cast<Recipe>(std::numeric_limits<int>::min())));
    CHECK(!FindFishMeal(Recipe::RoastedRoots) && !FindFishMeal(Recipe::Count));
    CHECK(!FindFishMeal(static_cast<Recipe>(std::numeric_limits<int>::min())));
}

void OriginalArtMappings()
{
    using namespace Homestead;
    std::set<Item> items;
    std::set<std::string> icons;
    int meals = 0, fish = 0;
    for (const auto& art : HomesteadOriginalItemArt::Entries)
    {
        CHECK(items.insert(art.item).second);
        CHECK(icons.insert(art.icon).second);
        CHECK(std::string(ItemIcon(art.item)) == art.icon);
        CHECK(art.serving && *art.serving);
        if (art.portion)
        {
            ++meals;
            CHECK(FoodClassOf(art.item) == FoodClass::Meal);
            CHECK(std::string(art.folder) == "PreparedFood");
        }
        if (art.fish)
        {
            ++fish;
            CHECK(!art.portion && art.item >= Item::RiverTrout && art.item <= Item::SeaBass);
            CHECK(std::string(art.folder) == "CaughtFish");
        }
    }
    CHECK(meals == 11 && fish == 6 && items.size() == 18);
    for (int index = static_cast<int>(Item::GrilledMackerel); index <= static_cast<int>(Item::MackerelChowder); ++index)
    {
        const Item item = static_cast<Item>(index);
        CHECK(!HomesteadOriginalItemArt::Find(item));
        CHECK(std::string(ItemIcon(item)) == "deferred-meal");
    }
    CHECK(!HomesteadOriginalItemArt::Find(Item::Count));
    CHECK(!HomesteadOriginalItemArt::FindIcon("fish"));
}
}

int main()
{
    FishingTests::PoleAndSales();
    FishingTests::HabitatsAndReplay();
    FishingTests::TimingAndCancellation();
    FishingTests::PresentationCompletionAndRecast();
    FishingTests::Preparations();
    FishingTests::OriginalArtMappings();
    std::cout << "Fishing price, habitats, timing, cancellation, food and persistence: "
        << FishingTests::Checks << " checks passed.\n";
}
