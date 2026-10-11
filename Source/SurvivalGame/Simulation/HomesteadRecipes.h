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

struct MealRecipeInfo
{
    Recipe recipe;
    Item output;
    std::array<MealIngredient, 4> ingredients;
    bool cooking = true;
};

const MealRecipeInfo* FindCropMeal(Recipe recipe);
const MealRecipeInfo* FindFishMeal(Recipe recipe);
const MealRecipeInfo* FindBeachMeal(Recipe recipe);
// Any cooked or prepared meal recipe (crop, fish or beach); null for tools and the legacy recipes.
const MealRecipeInfo* FindMeal(Recipe recipe);
bool IsRecipeAvailable(Recipe recipe);
Inventory CropMealChange(Recipe recipe);
Inventory FishMealChange(Recipe recipe);
// What any meal recipe (FindMeal) takes and makes; empty for anything else.
Inventory MealChange(Recipe recipe);
// Nominal Energy, balanced against ingredient sale opportunity including fuel and seasoning.
double CropMealEnergyForCost(std::int64_t saleCoins);
}
