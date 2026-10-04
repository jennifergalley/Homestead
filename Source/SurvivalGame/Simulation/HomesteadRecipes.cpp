#include "HomesteadRecipes.h"

#include <cmath>

namespace Homestead
{
namespace CropMeals
{
constexpr CropMealInfo Table[] = {
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

const CropMealInfo* FindCropMeal(Recipe recipe)
{
    if (recipe < Recipe::RoastedTurnips || recipe > Recipe::RootVegetableHotpot) return nullptr;
    const int index = static_cast<int>(recipe) - static_cast<int>(Recipe::RoastedTurnips);
    return &CropMeals::Table[index];
}

Inventory CropMealChange(Recipe recipe)
{
    Inventory change{};
    if (const auto* meal = FindCropMeal(recipe))
    {
        for (const auto& ingredient : meal->ingredients)
            if (ingredient.item != Item::Count)
                change[static_cast<int>(ingredient.item)] -= ingredient.count;
        change[static_cast<int>(meal->output)] = 1;
    }
    return change;
}

double CropMealEnergyForCost(std::int64_t saleCoins)
{
    constexpr double CookingEnergyBenefit = 12.0;
    constexpr double EnergyPerForegoneCoin = 0.6;
    return std::round(CookingEnergyBenefit + EnergyPerForegoneCoin * static_cast<double>(saleCoins));
}
}
