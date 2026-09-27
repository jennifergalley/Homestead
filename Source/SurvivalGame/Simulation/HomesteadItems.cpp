#include "HomesteadItems.h"

#include <cstring>
namespace Homestead
{
namespace
{
constexpr ShopMask StoreBuys = ShopBit(ShopKind::GeneralStore);
constexpr ShopMask NoBuyers = 0;

// One row per Item, in enum order. Prices are in cents.
constexpr ItemInfo ItemCatalogue[] = {
    {Item::Knife, "knife", "Knife", "A plain belt knife for cutting cord, cloth and hide.",
        ItemCategory::Tool, "knife", 150, NoBuyers},
    {Item::Branch, "branch", "Branch", "A dry fallen branch. Burns, builds, and snaps into kindling.",
        ItemCategory::Material, "branch", 4, StoreBuys, 0.0, 0.0, "Fallen branches"},
    {Item::Stone, "stone", "Stone", "A fist-sized granite stone, handy for footings and hearths.",
        ItemCategory::Material, "stone", 5, StoreBuys, 0.0, 0.0, "Loose stones"},
    {Item::Fiber, "fiber", "Fiber", "Stripped reed fiber for binding and weaving.",
        ItemCategory::Material, "fiber", 3, NoBuyers, 0.0, 0.0, "Reeds near water"},
    {Item::Berries, "berries", "Berries", "A handful of wild berries. A quick bite on the go.",
        ItemCategory::Forage, "berries", 6, NoBuyers, 12.0, 6.0},
    {Item::Roots, "roots", "Roots", "Wild roots. Too tough to eat raw; roast them over a fire.",
        ItemCategory::Forage, "roots", 4, NoBuyers, 0.0, 0.0, "Wild roots"},
    {Item::Flowers, "flowers", "Meadow herb", "A fragrant meadow herb for seasoning and posies.",
        ItemCategory::Forage, "flowers", 10, StoreBuys, 0.0, 0.0, "Meadow herb patches"},
    {Item::Seeds, "seeds", "Seeds", "Root seeds for planting in tilled soil.",
        ItemCategory::Supply, "seeds", 2, NoBuyers},
    {Item::Hatchet, "hatchet", "Crude hatchet", "A stone-headed hatchet for felling and splitting.",
        ItemCategory::Tool, "hatchet", 120, NoBuyers},
    {Item::DiggingStick, "digging-stick", "Stone hoe", "A stone-bladed hoe for tilling and weeding.",
        ItemCategory::Tool, "digging-stick", 80, NoBuyers},
    {Item::WateringCan, "watering-can", "Watering can", "A light wooden can for carrying water to crops.",
        ItemCategory::Tool, "watering-can", 90, NoBuyers},
    {Item::Water, "water", "Water", "Fresh stream water for the garden.",
        ItemCategory::Supply, "water", 0, NoBuyers},
    {Item::RoastedRoots, "roasted-roots", "Roasted roots", "Wild roots softened over a cookfire.",
        ItemCategory::Food, "roasted-roots", 15, NoBuyers, 28.0, 12.0},
    {Item::HerbedRoots, "herbed-roots", "Herbed roots", "Roasted roots brightened with meadow herbs.",
        ItemCategory::Food, "herbed-roots", 25, NoBuyers, 38.0, 18.0},
    {Item::Timber, "timber", "Timber", "A sawn length of trunk for heavy building or splitting.",
        ItemCategory::Material, "timber", 40, NoBuyers, 0.0, 0.0, "Mature trees with Hatchet"},
    {Item::Firewood, "firewood", "Firewood", "Split, seasoned firewood. Every kitchen in town wants it.",
        ItemCategory::Material, "firewood", 15, StoreBuys},
    {Item::Machete, "machete", "Machete", "A long blade for hacking back undergrowth.",
        ItemCategory::Tool, "machete", 200, NoBuyers},
    {Item::Fur, "fur", "Fur", "A cured deer hide for warm clothing.",
        ItemCategory::Material, "fur", 60, NoBuyers, 0.0, 0.0, "Deer remains in the woods, with your knife"},
    {Item::Pasty, "pasty", "Cornish pasty", "Beef, potato, swede and onion in a crimped crust. A proper meal.",
        ItemCategory::Food, "pasty", 80, NoBuyers, 45.0, 25.0},
    {Item::Bread, "bread", "Bread", "A round loaf from the town bakehouse.",
        ItemCategory::Food, "bread", 40, NoBuyers, 20.0, 8.0},
    {Item::Cheese, "cheese", "Cheese", "A wedge of hard farmhouse cheese.",
        ItemCategory::Food, "cheese", 60, NoBuyers, 15.0, 12.0},
    {Item::Twine, "twine", "Twine", "A hank of hemp twine for binding and tying.",
        ItemCategory::Material, "twine", 20, NoBuyers},
};
static_assert(sizeof(ItemCatalogue) / sizeof(ItemCatalogue[0]) == ItemCount, "Every item needs exactly one ItemCatalogue row.");

constexpr bool CatalogueInOrder()
{
    for (int i = 0; i < ItemCount; ++i)
        if (static_cast<int>(ItemCatalogue[i].item) != i) return false;
    return true;
}
static_assert(CatalogueInOrder(), "ItemCatalogue rows must follow the Item enum order.");

constexpr bool CatalogueComplete()
{
    for (const auto& info : ItemCatalogue)
    {
        if (!info.key || !*info.key || !info.name || !*info.name || !info.description || !*info.description
            || !info.icon || !*info.icon || info.basePriceCents < 0
            || static_cast<int>(info.category) < 0 || info.category >= ItemCategory::Count
            || info.hunger < 0.0 || info.energy < 0.0 || !info.source)
            return false;
    }
    return true;
}
static_assert(CatalogueComplete(), "Every ItemCatalogue row needs a key, name, description, icon and valid values.");

constexpr ItemInfo UnknownItemInfo{Item::Count, "unknown", "Unknown item", "Unknown item",
    ItemCategory::Material, "pack", 0, NoBuyers};

bool ValidItem(Item item)
{
    return static_cast<int>(item) >= 0 && static_cast<int>(item) < ItemCount;
}
}

const ItemInfo& GetItemInfo(Item item) { return ValidItem(item) ? ItemCatalogue[static_cast<int>(item)] : UnknownItemInfo; }
const char* ItemName(Item item) { return GetItemInfo(item).name; }
const char* ItemKey(Item item) { return GetItemInfo(item).key; }
Item ItemFromKey(const char* key)
{
    if (!key) return Item::Count;
    for (const auto& info : ItemCatalogue)
        if (std::strcmp(info.key, key) == 0) return info.item;
    return Item::Count;
}
const char* ItemDescription(Item item) { return GetItemInfo(item).description; }
const char* ItemIcon(Item item) { return GetItemInfo(item).icon; }
const char* ItemSource(Item item) { return GetItemInfo(item).source; }
ItemCategory CategoryOf(Item item) { return GetItemInfo(item).category; }
bool IsEdible(Item item) { return GetItemInfo(item).hunger > 0.0; }
bool IsTool(Item item) { return ValidItem(item) && GetItemInfo(item).category == ItemCategory::Tool; }
int ItemSortRank(Item item)
{
    if (!ValidItem(item)) return 5;
    switch (GetItemInfo(item).category)
    {
    case ItemCategory::Tool: return 0;
    case ItemCategory::Material:
    case ItemCategory::Salvage: return 1;
    case ItemCategory::Forage:
    case ItemCategory::Food: return 2;
    case ItemCategory::Supply: return 3;
    default: return 5;
    }
}
std::int64_t BasePrice(Item item) { return GetItemInfo(item).basePriceCents; }
bool ShopBuys(ShopKind shop, Item item) { return ValidItem(item) && (GetItemInfo(item).buyers & ShopBit(shop)) != 0; }
}
