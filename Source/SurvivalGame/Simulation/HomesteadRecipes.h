#pragma once

#include "HomesteadSimulation.h"

#include <array>

namespace Homestead
{
struct MealIngredient
{
    Item item = Item::Count;
    int count = 0;
};

struct CropMealInfo
{
    Recipe recipe;
    Item output;
    std::array<MealIngredient, 4> ingredients;
};

const CropMealInfo* FindCropMeal(Recipe recipe);
Inventory CropMealChange(Recipe recipe);
// Nominal Energy, balanced against ingredient sale opportunity including fuel and seasoning.
double CropMealEnergyForCost(std::int64_t saleCoins);
}
