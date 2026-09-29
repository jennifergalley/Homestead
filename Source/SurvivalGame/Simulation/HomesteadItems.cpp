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
    // The knife, fibre, machete and fur are retired from new games (add-overgrown-estate-clearing).
    {Item::Knife, "knife", "Knife", "A plain belt knife for cutting cord, cloth and hide.",
        ItemCategory::Tool, "knife", 150, NoBuyers, 0.0, 0.0, "", true},
    {Item::Branch, "branch", "Branch", "A dry fallen branch. Burns, builds, and snaps into kindling.",
        ItemCategory::Material, "branch", 4, StoreBuys, 0.0, 0.0, "Fallen branches"},
    {Item::Stone, "stone", "Stone", "A fist-sized granite stone, handy for footings and hearths.",
        ItemCategory::Material, "stone", 5, StoreBuys, 0.0, 0.0, "Loose stones"},
    {Item::Fiber, "fiber", "Fiber", "Stripped reed fiber for binding and weaving.",
        ItemCategory::Material, "fiber", 3, NoBuyers, 0.0, 0.0, "Reeds near water", true},
    {Item::Berries, "berries", "Berries", "A handful of wild berries. A quick bite on the go.",
        ItemCategory::Forage, "berries", 6, NoBuyers, 12.0, 6.0},
    {Item::Roots, "roots", "Roots", "Wild roots. Too tough to eat raw; roast them over a fire.",
        ItemCategory::Forage, "roots", 4, NoBuyers, 0.0, 0.0, "Wild roots"},
    {Item::Flowers, "flowers", "Meadow herb", "A fragrant meadow herb for seasoning and posies.",
        ItemCategory::Forage, "flowers", 10, StoreBuys, 0.0, 0.0, "Meadow herb patches"},
    {Item::Seeds, "seeds", "Seeds", "Root seeds for planting in tilled soil. Matures in about 2 days if watered.",
        ItemCategory::Supply, "seeds", 2, NoBuyers},
    // The axe, hoe and pail keep their original keys and enum names.
    {Item::Hatchet, "hatchet", "Axe", "A salvaged iron axe head on a new haft. Fells trees and clears stumps and fallen timber.",
        ItemCategory::Tool, "hatchet", 120, NoBuyers},
    {Item::DiggingStick, "digging-stick", "Hoe", "A salvaged iron hoe blade on a new handle, for tilling and weeding.",
        ItemCategory::Tool, "digging-stick", 80, NoBuyers},
    {Item::WateringCan, "watering-can", "Pail", "A wooden pail for carrying stream water to crops.",
        ItemCategory::Tool, "watering-can", 90, NoBuyers},
    {Item::Water, "water", "Water", "Fresh stream water for the garden.",
        ItemCategory::Supply, "water", 0, NoBuyers},
    {Item::RoastedRoots, "roasted-roots", "Roasted roots", "Wild roots softened over a cookfire.",
        ItemCategory::Food, "roasted-roots", 15, NoBuyers, 28.0, 12.0},
    {Item::HerbedRoots, "herbed-roots", "Herbed roots", "Roasted roots brightened with meadow herbs.",
        ItemCategory::Food, "herbed-roots", 25, NoBuyers, 38.0, 18.0},
    {Item::Timber, "timber", "Timber", "A sawn length of trunk for heavy building or splitting.",
        ItemCategory::Material, "timber", 40, NoBuyers, 0.0, 0.0, "Mature trees, large stumps and fallen logs, with the axe"},
    {Item::Firewood, "firewood", "Firewood", "Split, seasoned firewood. Every kitchen in town wants it.",
        ItemCategory::Material, "firewood", 15, StoreBuys},
    {Item::Machete, "machete", "Machete", "A long blade for hacking back undergrowth.",
        ItemCategory::Tool, "machete", 200, NoBuyers, 0.0, 0.0, "", true},
    {Item::Fur, "fur", "Fur", "A cured deer hide.",
        ItemCategory::Material, "fur", 60, NoBuyers, 0.0, 0.0, "", true},
    {Item::Pasty, "pasty", "Cornish pasty", "Beef, potato, swede and onion in a crimped crust. A proper meal.",
        ItemCategory::Food, "pasty", 80, NoBuyers, 45.0, 25.0, "", false, "Cornish pasties"},
    {Item::Bread, "bread", "Bread", "A round loaf from the town bakehouse.",
        ItemCategory::Food, "bread", 40, NoBuyers, 20.0, 8.0, "", false, "loaves of bread"},
    {Item::Cheese, "cheese", "Cheese", "A wedge of hard farmhouse cheese.",
        ItemCategory::Food, "cheese", 60, NoBuyers, 15.0, 12.0, "", false, "wedges of cheese"},
    {Item::Twine, "twine", "Twine", "A hank of hemp twine for binding and tying.",
        ItemCategory::Material, "twine", 20, NoBuyers, 0.0, 0.0, "", false, "hanks of twine"},
    // Estate clearing: the hafted tools, the rusted heads salvaged from the ruin, what clearing
    // yields, and the spring flowers she can sell in town. Hay, scrap and flowers sell at the store.
    {Item::Scythe, "scythe", "Scythe", "A salvaged scythe blade on a new snath. Mows grass and weeds in a wide sweep.",
        ItemCategory::Tool, "scythe", 250, NoBuyers},
    {Item::Billhook, "billhook", "Billhook", "A salvaged West Country billhook on a new handle. Hacks through bramble and saplings.",
        ItemCategory::Tool, "billhook", 180, NoBuyers},
    {Item::Pickaxe, "pickaxe", "Pickaxe", "A salvaged pick head on a new haft. Breaks rubble and rocks.",
        ItemCategory::Tool, "pickaxe", 220, NoBuyers},
    {Item::RustedAxeHead, "rusted-axe-head", "Rusted axe head", "An old axe head from the ruin. Craft an axe from it and two branches.",
        ItemCategory::Salvage, "rusted-axe-head", 30, NoBuyers, 0.0, 0.0, "Salvage piles around the manor"},
    {Item::RustedHoeBlade, "rusted-hoe-blade", "Rusted hoe blade", "An old hoe blade from the ruin. Craft a hoe from it and two branches.",
        ItemCategory::Salvage, "rusted-hoe-blade", 25, NoBuyers, 0.0, 0.0, "Salvage piles around the manor"},
    {Item::RustedScytheBlade, "rusted-scythe-blade", "Rusted scythe blade", "An old scythe blade from the ruin. Craft a scythe from it and two branches.",
        ItemCategory::Salvage, "rusted-scythe-blade", 35, NoBuyers, 0.0, 0.0, "Salvage piles around the manor"},
    {Item::RustedBillhookHead, "rusted-billhook-head", "Rusted billhook head", "An old billhook head from the ruin. Craft a billhook from it and two branches.",
        ItemCategory::Salvage, "rusted-billhook-head", 25, NoBuyers, 0.0, 0.0, "Salvage piles around the manor"},
    {Item::RustedPickHead, "rusted-pick-head", "Rusted pick head", "An old pick head from the ruin. Craft a pickaxe from it and two branches.",
        ItemCategory::Salvage, "rusted-pick-head", 30, NoBuyers, 0.0, 0.0, "Salvage piles around the manor"},
    {Item::Hay, "hay", "Hay", "Mown grass, dried for fodder. Carters and stables in town buy it.",
        ItemCategory::Material, "hay", 6, StoreBuys, 0.0, 0.0, "Tall grass, with the scythe"},
    {Item::Weeds, "weeds", "Weeds", "Pulled dock, nettle and thistle. Good for nothing yet but the compost heap.",
        ItemCategory::Material, "weeds", 1, NoBuyers, 0.0, 0.0, "Weeds, with the scythe"},
    {Item::BrambleCanes, "bramble-canes", "Bramble canes", "Long, prickly canes, stripped for wattle and lashing.",
        ItemCategory::Material, "bramble-canes", 3, NoBuyers, 0.0, 0.0, "Bramble, with the billhook"},
    {Item::Kindling, "kindling", "Kindling", "Dry twigs and splinters for starting a fire.",
        ItemCategory::Material, "kindling", 3, StoreBuys, 0.0, 0.0, "Saplings and fallen boughs"},
    {Item::ScrapIron, "scrap-iron", "Scrap iron", "Rusted nails, hinges and hoops. The smith and the rag-and-bone men buy it.",
        ItemCategory::Salvage, "scrap-iron", 25, StoreBuys, 0.0, 0.0, "Rubble, with the pickaxe, and salvage piles"},
    {Item::ScrapLead, "scrap-lead", "Scrap lead", "Old roof flashing and pipe from the ruin, worth more than iron.",
        ItemCategory::Salvage, "scrap-lead", 40, StoreBuys, 0.0, 0.0, "Rubble, with the pickaxe"},
    {Item::Primroses, "primroses", "Primroses", "A posy of pale yellow primroses from the hedge banks.",
        ItemCategory::Forage, "primroses", 12, StoreBuys, 0.0, 0.0, "Spring flowers in the verges and valley"},
    {Item::Bluebells, "bluebells", "Bluebells", "An armful of bluebells from under the trees.",
        ItemCategory::Forage, "bluebells", 15, StoreBuys, 0.0, 0.0, "Spring flowers in the verges and valley"},
    {Item::WildDaffodils, "wild-daffodils", "Wild daffodils", "Small wild Lent lilies, the first gold of the year.",
        ItemCategory::Forage, "wild-daffodils", 15, StoreBuys, 0.0, 0.0, "Spring flowers in the verges and valley"},
    {Item::WildGarlic, "wild-garlic", "Wild garlic", "Ramsons leaves and flowers from the damp valley floor.",
        ItemCategory::Forage, "wild-garlic", 10, StoreBuys, 0.0, 0.0, "Spring flowers in the verges and valley"},
    // add-oil-lamp: her first light, and the oil for it.
    {Item::OilLamp, "oil-lamp", "Oil lamp", "A tin and brass hurricane lantern with a glass chimney. Select it to carry it lit; set it down to light the ground around it.",
        ItemCategory::Tool, "oil-lamp", 150, NoBuyers},
    {Item::OilFlask, "oil-flask", "Oil flask", "A stoppered tin flask of lamp oil. Fills the lamp for about six hours.",
        ItemCategory::Supply, "oil-flask", 12, NoBuyers, 0.0, 0.0, "The general store", false, "oil flasks"},
    // improve-crops-and-harvest. Seed prices are base; the store sells at 125%. Growing times match
    // HomesteadCrops.cpp (checked by the native tests).
    {Item::TurnipSeed, "turnip-seed", "Turnip seed", "A paper of white-globe turnip seed. Matures in about 4 days if watered.",
        ItemCategory::Supply, "seeds", 16, NoBuyers, 0.0, 0.0, "Pascoe's general store", false, "papers of turnip seed"},
    {Item::CarrotSeed, "carrot-seed", "Carrot seed", "A paper of long orange carrot seed. Matures in about 5 days if watered.",
        ItemCategory::Supply, "seeds", 20, NoBuyers, 0.0, 0.0, "Pascoe's general store", false, "papers of carrot seed"},
    {Item::SeedPotato, "seed-potato", "Seed potato", "A chitted seed potato, sprouting from its eyes. Matures in about 6 days if watered.",
        ItemCategory::Supply, "seeds", 24, NoBuyers, 0.0, 0.0, "Pascoe's general store", false, "seed potatoes"},
    {Item::CabbageSeed, "cabbage-seed", "Cabbage seed", "A paper of drumhead cabbage seed. Matures in about 9 days if watered.",
        ItemCategory::Supply, "seeds", 32, NoBuyers, 0.0, 0.0, "Pascoe's general store", false, "papers of cabbage seed"},
    {Item::BroadBeanSeed, "broad-bean-seed", "Broad bean seed", "A twist of broad beans for sowing. Matures in about 7 days if watered, then crops every 3 days.",
        ItemCategory::Supply, "seeds", 36, NoBuyers, 0.0, 0.0, "Pascoe's general store", false, "twists of broad bean seed"},
    {Item::StrawberryRunner, "strawberry-runner", "Strawberry runner", "A rooted strawberry runner. Fruits in about 8 days if watered, then every 3 days.",
        ItemCategory::Supply, "seeds", 48, NoBuyers, 0.0, 0.0, "Pascoe's general store", false, "strawberry runners"},
    {Item::Turnip, "turnip", "Turnip", "A white turnip with a purple shoulder. Crisp and peppery raw.",
        ItemCategory::Food, "roots", 20, StoreBuys, 8.0, 4.0, "Grown from turnip seed", false, "turnips"},
    {Item::Carrot, "carrot", "Carrot", "A sweet orange carrot, earth still on it.",
        ItemCategory::Food, "roots", 16, StoreBuys, 6.0, 4.0, "Grown from carrot seed", false, "carrots"},
    {Item::Potato, "potato", "Potato", "A floury potato. Best sold, or cooked once there's a pot to boil it in.",
        ItemCategory::Food, "roots", 14, StoreBuys, 0.0, 0.0, "Grown from seed potatoes", false, "potatoes"},
    {Item::Cabbage, "cabbage", "Cabbage", "A firm drumhead cabbage. It fetches a good price in town.",
        ItemCategory::Food, "wild-garlic", 90, StoreBuys, 14.0, 6.0, "Grown from cabbage seed", false, "cabbages"},
    {Item::BroadBeans, "broad-beans", "Broad bean pods", "Fat green pods of young broad beans.",
        ItemCategory::Food, "wild-garlic", 8, StoreBuys, 5.0, 3.0, "Picked from broad bean plants", false, "broad bean pods"},
    {Item::Strawberries, "strawberries", "Strawberries", "Sweet red strawberries, warm from the sun.",
        ItemCategory::Food, "berries", 13, StoreBuys, 8.0, 8.0, "Picked from strawberry plants", false, "strawberries"},
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
std::string CountedName(Item item, int quantity)
{
    const ItemInfo& info = GetItemInfo(item);
    return std::to_string(quantity) + " " + (quantity != 1 && info.plural ? info.plural : info.name);
}
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
