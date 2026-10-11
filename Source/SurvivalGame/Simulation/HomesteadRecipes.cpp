#include "HomesteadRecipes.h"

#include <cmath>

namespace Homestead
{
namespace CropMeals
{
constexpr MealRecipeInfo Table[] = {
    {Recipe::RoastedTurnips, Item::RoastedTurnips, {{{Item::Turnip, 1}, {Item::Kindling, 1}, {}, {}}}},
    {Recipe::StewedCarrots, Item::StewedCarrots, {{{Item::Carrot, 2}, {Item::Kindling, 1}, {}, {}}}},
    {Recipe::BakedPotatoes, Item::BakedPotatoes, {{{Item::Potato, 2}, {Item::Kindling, 1}, {}, {}}}},
    {Recipe::HerbedBroadBeans, Item::HerbedBroadBeans,
        {{{Item::BroadBeans, 3}, {Item::Flowers, 1}, {Item::Kindling, 1}, {}}}},
    {Recipe::CabbagePotatoStew, Item::CabbagePotatoStew,
        {{{Item::Cabbage, 1}, {Item::Potato, 1}, {Item::Flowers, 1}, {Item::Kindling, 1}}}},
    {Recipe::BerryCompote, Item::BerryCompote, {{{Item::Berries, 3}, {Item::Kindling, 1}, {}, {}}}},
    {Recipe::StrawberryCompote, Item::StrawberryCompote, {{{Item::Strawberries, 2}, {Item::Kindling, 1}, {}, {}}}},
    {Recipe::RootVegetableHotpot, Item::RootVegetableHotpot,
        {{{Item::Roots, 2}, {Item::Turnip, 1}, {Item::Flowers, 1}, {Item::Kindling, 1}}}},
};
static_assert(sizeof(Table) / sizeof(Table[0]) == 8, "The selected crop-food batch has eight meals.");

constexpr bool InOrder()
{
    for (int i = 0; i < 8; ++i)
        if (static_cast<int>(Table[i].recipe) != static_cast<int>(Recipe::RoastedTurnips) + i
            || static_cast<int>(Table[i].output) != static_cast<int>(Item::RoastedTurnips) + i)
            return false;
    return true;
}
static_assert(InOrder(), "Crop-meal rows must follow the append-only enums.");
}

namespace FishMeals
{
constexpr MealRecipeInfo Table[] = {
    {Recipe::RawFishSlices, Item::RawFishSlices, {{{Item::SeaMackerel, 1}, {}, {}, {}}}, false},
    {Recipe::GrilledTrout, Item::GrilledTrout, {{{Item::RiverTrout, 1}, {Item::Kindling, 1}, {}, {}}}},
    {Recipe::GrilledPerch, Item::GrilledPerch, {{{Item::LakePerch, 1}, {Item::Kindling, 1}, {}, {}}}},
    {Recipe::GrilledMackerel, Item::GrilledMackerel, {{{Item::SeaMackerel, 1}, {Item::Kindling, 1}, {}, {}}}},
    {Recipe::FishSoup, Item::FishSoup,
        {{{Item::RiverSalmon, 1}, {Item::Potato, 1}, {Item::Flowers, 1}, {Item::Kindling, 1}}}},
    {Recipe::FishAndPotatoes, Item::FishAndPotatoes, {{{Item::SeaBass, 1}, {Item::Potato, 2}, {Item::Kindling, 1}, {}}}},
    {Recipe::HerbedCarp, Item::HerbedCarp, {{{Item::LakeCarp, 1}, {Item::Flowers, 1}, {Item::Kindling, 1}, {}}}},
    {Recipe::MackerelChowder, Item::MackerelChowder,
        {{{Item::SeaMackerel, 1}, {Item::Potato, 1}, {Item::BroadBeans, 2}, {Item::Kindling, 1}}}},
};
constexpr bool InOrder()
{
    for (int i = 0; i < 8; ++i)
        if (static_cast<int>(Table[i].recipe) != static_cast<int>(Recipe::RawFishSlices) + i
            || static_cast<int>(Table[i].output) != static_cast<int>(Item::RawFishSlices) + i)
            return false;
    return true;
}
static_assert(sizeof(Table) / sizeof(Table[0]) == 8 && InOrder(), "Fish meals follow the append-only enums.");
}

// add-beach-gathering (Balance 2026-10-10).
namespace BeachMeals
{
constexpr MealRecipeInfo Table[] = {
    {Recipe::SteamedMussels, Item::SteamedMussels, {{{Item::Mussels, 3}, {Item::Kindling, 1}, {}, {}}}},
    {Recipe::MusselPotatoBroth, Item::MusselPotatoBroth,
        {{{Item::Mussels, 3}, {Item::Potato, 1}, {Item::Laver, 1}, {Item::Kindling, 1}}}},
    {Recipe::FriedLaver, Item::FriedLaver, {{{Item::Laver, 2}, {Item::Kindling, 1}, {}, {}}}},
};
constexpr int Count = sizeof(Table) / sizeof(Table[0]);
constexpr bool InOrder()
{
    for (int i = 0; i < Count; ++i)
        if (static_cast<int>(Table[i].recipe) != static_cast<int>(Recipe::SteamedMussels) + i
            || static_cast<int>(Table[i].output) != static_cast<int>(Item::SteamedMussels) + i)
            return false;
    return true;
}
static_assert(Count == 3 && InOrder(), "Beach meals follow the append-only enums.");
}

const MealRecipeInfo* FindCropMeal(Recipe recipe)
{
    if (recipe < Recipe::RoastedTurnips || recipe > Recipe::RootVegetableHotpot) return nullptr;
    const int index = static_cast<int>(recipe) - static_cast<int>(Recipe::RoastedTurnips);
    return &CropMeals::Table[index];
}

const MealRecipeInfo* FindFishMeal(Recipe recipe)
{
    if (recipe < Recipe::RawFishSlices || recipe > Recipe::MackerelChowder) return nullptr;
    return &FishMeals::Table[static_cast<int>(recipe) - static_cast<int>(Recipe::RawFishSlices)];
}

const MealRecipeInfo* FindBeachMeal(Recipe recipe)
{
    if (recipe < Recipe::SteamedMussels || recipe > Recipe::FriedLaver) return nullptr;
    return &BeachMeals::Table[static_cast<int>(recipe) - static_cast<int>(Recipe::SteamedMussels)];
}

const MealRecipeInfo* FindMeal(Recipe recipe)
{
    if (const auto* meal = FindCropMeal(recipe)) return meal;
    if (const auto* meal = FindFishMeal(recipe)) return meal;
    return FindBeachMeal(recipe);
}

bool IsRecipeAvailable(Recipe recipe)
{
    // Jenny's Oct 4 playtest admits eleven original meals; retain the five deferred enum/save ids.
    return recipe >= Recipe::HaftAxe && recipe < Recipe::Count
        && !(recipe >= Recipe::GrilledMackerel && recipe <= Recipe::MackerelChowder);
}

namespace MealRecipes
{
Inventory Change(const MealRecipeInfo* meal)
{
    Inventory change{};
    if (!meal) return change;
    for (const auto& ingredient : meal->ingredients)
        if (ingredient.item != Item::Count)
            change[static_cast<int>(ingredient.item)] -= ingredient.count;
    change[static_cast<int>(meal->output)] = 1;
    return change;
}
}

Inventory CropMealChange(Recipe recipe)
{
    return MealRecipes::Change(FindCropMeal(recipe));
}
Inventory FishMealChange(Recipe recipe) { return MealRecipes::Change(FindFishMeal(recipe)); }
Inventory MealChange(Recipe recipe) { return MealRecipes::Change(FindMeal(recipe)); }

double CropMealEnergyForCost(std::int64_t saleCoins)
{
    constexpr double CookingEnergyBenefit = 12.0;
    constexpr double EnergyPerForegoneCoin = 0.6;
    return std::round(CookingEnergyBenefit + EnergyPerForegoneCoin * static_cast<double>(saleCoins));
}
}
