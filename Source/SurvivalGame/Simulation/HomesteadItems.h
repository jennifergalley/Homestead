#pragma once

#include <cstdint>
#include <string>

// The item catalogue: every item's identity and metadata in one table (HomesteadItems.cpp).
// Adding an item means one enum value here and one row there, in the same order; the build fails
// otherwise.
namespace Homestead
{
enum class Item : int
{
    Knife, Branch, Stone, Fiber, Berries, Roots, Flowers, Seeds,
    Hatchet, DiggingStick, WateringCan, Water, RoastedRoots, HerbedRoots,
    Timber, Firewood, Machete, Fur,
    // Round 1 general-store goods.
    Pasty, Bread, Cheese, Twine,
    // add-overgrown-estate-clearing: tools, salvaged heads, clearing yields and spring flowers.
    Scythe, Billhook, Pickaxe,
    RustedAxeHead, RustedHoeBlade, RustedScytheBlade, RustedBillhookHead, RustedPickHead,
    Hay, Weeds, BrambleCanes, Kindling, ScrapIron, ScrapLead,
    Primroses, Bluebells, WildDaffodils, WildGarlic,
    // add-oil-lamp.
    OilLamp, OilFlask,
    // improve-crops-and-harvest: period crop seed (sold at the general store) and the produce.
    TurnipSeed, CarrotSeed, SeedPotato, CabbageSeed, BroadBeanSeed, StrawberryRunner,
    Turnip, Carrot, Potato, Cabbage, BroadBeans, Strawberries,
    // add-basic-crop-cookfire-recipes.
    RoastedTurnips, StewedCarrots, BakedPotatoes, HerbedBroadBeans,
    CabbagePotatoStew, BerryCompote, StrawberryCompote, RootVegetableHotpot,
    FishingPole, RiverTrout, RiverSalmon, LakePerch, LakeCarp, SeaMackerel, SeaBass,
    RawFishSlices, GrilledTrout, GrilledPerch, GrilledMackerel,
    FishSoup, FishAndPotatoes, HerbedCarp, MackerelChowder,
    Count
};
constexpr int ItemCount = static_cast<int>(Item::Count);

enum class ItemCategory : int { Tool, Material, Forage, Food, Salvage, Supply, Count };

// What eating it does (rework-farming-calendar-and-period-crafting design §3a): a Snack restores
// Energy only; a Meal (a cooked dish, hearth-made or bought ready) also makes her Well fed.
enum class FoodClass : int { None, Snack, Meal };

enum class ShopKind : int { GeneralStore, Count };
using ShopMask = unsigned;
constexpr ShopMask ShopBit(ShopKind kind) { return 1u << static_cast<int>(kind); }

struct ItemInfo
{
    Item item;
    const char* key;         // Stable identifier, never shown.
    const char* name;
    const char* description;
    ItemCategory category;
    const char* icon;        // SHomesteadIcon glyph key.
    std::int64_t basePriceCoins;
    ShopMask buyers;         // Shops that buy it from her.
    double hunger = 0.0;     // Woodland only (the estate has no hunger); above zero exactly for food.
    double energy = 0.0;     // Energy one restores.
    const char* source = ""; // Where to find it, for recipe requirements.
    bool hiddenFromNewGames = false;
    const char* plural = nullptr; // For counts above one; the name when unset ("12 Stone").
    FoodClass food = FoodClass::None; // None means not edible.
};

// The catalogue row for an item; an "Unknown item" row for values outside the enum.
const ItemInfo& GetItemInfo(Item item);
const char* ItemName(Item item);
// "1 Cornish pasty", "3 Cornish pasties", "12 Stone".
std::string CountedName(Item item, int quantity);
const char* ItemKey(Item item);
// The item with this stable key, or Item::Count.
Item ItemFromKey(const char* key);
const char* ItemDescription(Item item);
const char* ItemIcon(Item item);
const char* ItemSource(Item item);
ItemCategory CategoryOf(Item item);
FoodClass FoodClassOf(Item item);
// Snacks and Meals; raw potatoes, roots and the like are cooked or sold instead.
bool IsEdible(Item item);
// The nominal Energy one of this food restores, rounded, from its catalogue row ("+40 Energy"); empty
// for anything that isn't food or restores no Energy. Shops and the pack show this before she eats.
std::string FoodEnergyLabel(Item item);
bool IsTool(Item item);
bool IsSeedPacket(Item item);
bool CanStackItem(Item item);
// Pack sort order: tools, then materials and salvage, forage and food, then supplies.
int ItemSortRank(Item item);
std::int64_t BasePrice(Item item);
bool ShopBuys(ShopKind shop, Item item);
}
