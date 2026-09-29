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
    Count
};
constexpr int ItemCount = static_cast<int>(Item::Count);

enum class ItemCategory : int { Tool, Material, Forage, Food, Salvage, Supply, Count };

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
    std::int64_t basePriceCents;
    ShopMask buyers;         // Shops that buy it from her.
    double hunger = 0.0;     // Eating restores these; zero hunger means not edible.
    double energy = 0.0;
    const char* source = ""; // Where to find it, for recipe requirements.
    bool hiddenFromNewGames = false;
    const char* plural = nullptr; // For counts above one; the name when unset ("12 Stone").
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
bool IsEdible(Item item);
bool IsTool(Item item);
// Pack sort order: tools, then materials and salvage, forage and food, then supplies.
int ItemSortRank(Item item);
std::int64_t BasePrice(Item item);
bool ShopBuys(ShopKind shop, Item item);
}
