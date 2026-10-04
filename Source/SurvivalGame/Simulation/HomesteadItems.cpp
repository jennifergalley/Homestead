#include "HomesteadItems.h"

#include <cmath>
#include <cstring>
namespace Homestead
{
namespace
{
constexpr ShopMask StoreBuys = ShopBit(ShopKind::GeneralStore);
constexpr ShopMask NoBuyers = 0;

// One row per Item, in enum order. Prices are in whole coins (HomesteadShops.h).
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
        ItemCategory::Forage, "berries", 6, StoreBuys, 12.0, 6.0, "", false, nullptr, FoodClass::Snack},
    {Item::Roots, "roots", "Roots", "Wild roots. Too tough to eat raw; roast them over a fire.",
        ItemCategory::Forage, "roots", 4, StoreBuys, 0.0, 0.0, "Wild roots"},
    {Item::Flowers, "flowers", "Meadow herb", "A fragrant meadow herb for seasoning and posies.",
        ItemCategory::Forage, "flowers", 10, StoreBuys, 0.0, 0.0, "Meadow herb patches"},
    {Item::Seeds, "seeds", "Roots seeds", "A packet of wild root seeds. Matures in about 2 days if watered.",
        ItemCategory::Supply, "roots-seeds", 2, NoBuyers},
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
        ItemCategory::Food, "roasted-roots", 15, NoBuyers, 28.0, 25.0, "", false, nullptr, FoodClass::Meal},
    {Item::HerbedRoots, "herbed-roots", "Herbed roots", "Roasted roots brightened with meadow herbs.",
        ItemCategory::Food, "herbed-roots", 25, NoBuyers, 38.0, 40.0, "", false, nullptr, FoodClass::Meal},
    {Item::Timber, "timber", "Timber", "A sawn length of trunk for heavy building or splitting.",
        ItemCategory::Material, "timber", 40, NoBuyers, 0.0, 0.0, "Mature trees, large stumps and fallen logs, with the axe"},
    {Item::Firewood, "firewood", "Firewood", "Split, seasoned firewood. Every kitchen in town wants it.",
        ItemCategory::Material, "firewood", 15, StoreBuys},
    {Item::Machete, "machete", "Machete", "A long blade for hacking back undergrowth.",
        ItemCategory::Tool, "machete", 200, NoBuyers, 0.0, 0.0, "", true},
    {Item::Fur, "fur", "Fur", "A cured deer hide.",
        ItemCategory::Material, "fur", 60, NoBuyers, 0.0, 0.0, "", true},
    {Item::Pasty, "pasty", "Cornish pasty", "Beef, potato, swede and onion in a crimped crust. A proper meal.",
        ItemCategory::Food, "pasty", 80, NoBuyers, 45.0, 40.0, "", false, "Cornish pasties", FoodClass::Meal},
    {Item::Bread, "bread", "Bread", "A round loaf from the town bakehouse.",
        ItemCategory::Food, "bread", 40, NoBuyers, 20.0, 12.0, "", false, "loaves of bread", FoodClass::Snack},
    {Item::Cheese, "cheese", "Cheese", "A wedge of hard farmhouse cheese.",
        ItemCategory::Food, "cheese", 60, NoBuyers, 15.0, 15.0, "", false, "wedges of cheese", FoodClass::Snack},
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
        ItemCategory::Material, "kindling", 3, StoreBuys, 0.0, 0.0, "Fallen branches, saplings and old boughs"},
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
    {Item::TurnipSeed, "turnip-seed", "Turnip seeds", "A packet of white-globe turnip seeds. Matures in about 4 days if watered.",
        ItemCategory::Supply, "turnip-seeds", 16, NoBuyers, 0.0, 0.0, "Trethewey's general store", false, "packets of turnip seeds"},
    {Item::CarrotSeed, "carrot-seed", "Carrot seeds", "A packet of long orange carrot seeds. Matures in about 5 days if watered.",
        ItemCategory::Supply, "carrot-seeds", 20, NoBuyers, 0.0, 0.0, "Trethewey's general store", false, "packets of carrot seeds"},
    {Item::SeedPotato, "seed-potato", "Potato seeds", "A packet of chitted seed potatoes. Matures in about 6 days if watered.",
        ItemCategory::Supply, "potato-seeds", 24, NoBuyers, 0.0, 0.0, "Trethewey's general store", false, "packets of potato seeds"},
    {Item::CabbageSeed, "cabbage-seed", "Cabbage seeds", "A packet of drumhead cabbage seeds. Matures in about 9 days if watered.",
        ItemCategory::Supply, "cabbage-seeds", 32, NoBuyers, 0.0, 0.0, "Trethewey's general store", false, "packets of cabbage seeds"},
    {Item::BroadBeanSeed, "broad-bean-seed", "Broad bean seeds", "A packet of broad beans for sowing. Matures in about 7 days if watered, then crops every 3 days.",
        ItemCategory::Supply, "broad-bean-seeds", 36, NoBuyers, 0.0, 0.0, "Trethewey's general store", false, "packets of broad bean seeds"},
    {Item::StrawberryRunner, "strawberry-runner", "Strawberry seeds", "A packet of strawberry planting stock. Fruits in about 8 days if watered, then every 3 days.",
        ItemCategory::Supply, "strawberry-seeds", 48, NoBuyers, 0.0, 0.0, "Trethewey's general store", false, "packets of strawberry seeds"},
    {Item::Turnip, "turnip", "Turnip", "A white turnip with a purple shoulder. Crisp and peppery raw.",
        ItemCategory::Food, "roots", 20, StoreBuys, 8.0, 6.0, "Grown from turnip seed", false, "turnips", FoodClass::Snack},
    {Item::Carrot, "carrot", "Carrot", "A sweet orange carrot, earth still on it.",
        ItemCategory::Food, "roots", 16, StoreBuys, 6.0, 6.0, "Grown from carrot seed", false, "carrots", FoodClass::Snack},
    {Item::Potato, "potato", "Potato", "A floury potato. Best sold, or cooked once there's a pot to boil it in.",
        ItemCategory::Food, "roots", 14, StoreBuys, 0.0, 0.0, "Grown from seed potatoes", false, "potatoes"},
    {Item::Cabbage, "cabbage", "Cabbage", "A firm drumhead cabbage. It fetches a good price in town.",
        ItemCategory::Food, "wild-garlic", 90, StoreBuys, 14.0, 10.0, "Grown from cabbage seed", false, "cabbages", FoodClass::Snack},
    {Item::BroadBeans, "broad-beans", "Broad bean pods", "Fat green pods of young broad beans.",
        ItemCategory::Food, "wild-garlic", 8, StoreBuys, 5.0, 4.0, "Picked from broad bean plants", false, "broad bean pods", FoodClass::Snack},
    {Item::Strawberries, "strawberries", "Strawberries", "Sweet red strawberries, warm from the sun.",
        ItemCategory::Food, "berries", 13, StoreBuys, 8.0, 8.0, "Picked from strawberry plants", false, "strawberries", FoodClass::Snack},
    // Energy: round(12 + 0.6 * consumed sale coins), including herbs and kindling.
    {Item::RoastedTurnips, "roasted-turnips", "Roasted turnips", "Tender turnip pieces roasted over the cookfire.",
        ItemCategory::Food, "roasted-turnips", 23, NoBuyers, 26.0, 26.0, "Cookfire", false, nullptr, FoodClass::Meal},
    {Item::StewedCarrots, "stewed-carrots", "Stewed carrots", "Sweet carrots gently stewed until tender.",
        ItemCategory::Food, "stewed-carrots", 35, NoBuyers, 33.0, 33.0, "Cookfire", false, nullptr, FoodClass::Meal},
    {Item::BakedPotatoes, "baked-potatoes", "Baked potatoes", "Floury potatoes baked in the cookfire's embers.",
        ItemCategory::Food, "baked-potatoes", 31, NoBuyers, 31.0, 31.0, "Cookfire", false, nullptr, FoodClass::Meal},
    {Item::HerbedBroadBeans, "herbed-broad-beans", "Herbed broad beans", "Tender broad beans seasoned with meadow herbs.",
        ItemCategory::Food, "herbed-broad-beans", 37, NoBuyers, 34.0, 34.0, "Cookfire", false, nullptr, FoodClass::Meal},
    {Item::CabbagePotatoStew, "cabbage-potato-stew", "Cabbage and potato stew", "A hearty cabbage and potato stew with meadow herbs.",
        ItemCategory::Food, "cabbage-potato-stew", 117, NoBuyers, 82.0, 82.0, "Cookfire", false, nullptr, FoodClass::Meal},
    {Item::BerryCompote, "berry-compote", "Berry compote", "Wild berries simmered in their own juices.",
        ItemCategory::Food, "berry-compote", 21, NoBuyers, 25.0, 25.0, "Cookfire", false, nullptr, FoodClass::Meal},
    {Item::StrawberryCompote, "strawberry-compote", "Strawberry compote", "Sweet strawberries gently simmered over the cookfire.",
        ItemCategory::Food, "strawberry-compote", 29, NoBuyers, 29.0, 29.0, "Cookfire", false, nullptr, FoodClass::Meal},
    {Item::RootVegetableHotpot, "root-vegetable-hotpot", "Root vegetable hotpot", "Wild roots and turnip stewed with meadow herbs.",
        ItemCategory::Food, "root-vegetable-hotpot", 41, NoBuyers, 37.0, 37.0, "Cookfire", false, nullptr, FoodClass::Meal},
    {Item::FishingPole, "fishing-pole", "Fishing pole", "Select this pole on the hotbar beside a river, lake or the sea. Cast, hook the bite, then follow the float's closing-ring cues to land the fish.",
        ItemCategory::Tool, "fishing-pole", 1200, NoBuyers, 0.0, 0.0, "General Store: 1500 coins", false, "fishing poles"},
    {Item::RiverTrout, "river-trout", "River trout", "A trout from running freshwater. Grill it or sell it at the General Store.",
        ItemCategory::Food, "river-trout", 40, StoreBuys, 0.0, 0.0, "River fishing", false, "river trout"},
    {Item::RiverSalmon, "river-salmon", "River salmon", "A silver salmon from the river. Fetches a good price.",
        ItemCategory::Food, "river-salmon", 54, StoreBuys, 0.0, 0.0, "River fishing", false, "river salmon"},
    {Item::LakePerch, "lake-perch", "Lake perch", "A freshwater perch from still water. Cook or sell it.",
        ItemCategory::Food, "lake-perch", 28, StoreBuys, 0.0, 0.0, "Lake fishing", false, "lake perch"},
    {Item::LakeCarp, "lake-carp", "Lake carp", "A carp from the lake. Sell it at the General Store.",
        ItemCategory::Food, "lake-carp", 22, StoreBuys, 0.0, 0.0, "Lake fishing", false, "lake carp"},
    {Item::SeaMackerel, "sea-mackerel", "Sea mackerel", "A saltwater mackerel. Slice it fresh or sell it.",
        ItemCategory::Food, "sea-mackerel", 30, StoreBuys, 0.0, 0.0, "Sea fishing", false, "sea mackerel"},
    {Item::SeaBass, "sea-bass", "Sea bass", "A sea bass from the coast. Fetches a good price.",
        ItemCategory::Food, "sea-bass", 52, StoreBuys, 0.0, 0.0, "Sea fishing", false, "sea bass"},
    {Item::RawFishSlices, "raw-fish-slices", "Fresh mackerel slices", "Mackerel sliced thin and eaten fresh. No fire needed.",
        ItemCategory::Food, "raw-fish-slices", 30, NoBuyers, 30.0, 30.0, "Prepare from mackerel", false, nullptr, FoodClass::Meal},
    {Item::GrilledTrout, "grilled-trout", "Grilled trout", "River trout grilled over the fire.",
        ItemCategory::Food, "grilled-trout", 43, NoBuyers, 38.0, 38.0, "Cookfire", false, nullptr, FoodClass::Meal},
    {Item::GrilledPerch, "grilled-perch", "Grilled perch", "Lake perch cooked until tender.",
        ItemCategory::Food, "grilled-perch", 31, NoBuyers, 31.0, 31.0, "Cookfire", false, nullptr, FoodClass::Meal},
    {Item::GrilledMackerel, "grilled-mackerel", "Grilled mackerel", "Mackerel browned over the cookfire.",
        ItemCategory::Food, "deferred-meal", 33, NoBuyers, 32.0, 32.0, "Deferred from this playtest", false, nullptr, FoodClass::Meal},
    {Item::FishSoup, "fish-soup", "Salmon and potato soup", "River salmon and potato simmered with meadow herbs.",
        ItemCategory::Food, "deferred-meal", 81, NoBuyers, 61.0, 61.0, "Deferred from this playtest", false, nullptr, FoodClass::Meal},
    {Item::FishAndPotatoes, "fish-and-potatoes", "Fish and ember potatoes", "Sea bass and potatoes roasted beside the coals, a simple fish-and-chips supper.",
        ItemCategory::Food, "deferred-meal", 83, NoBuyers, 62.0, 62.0, "Deferred from this playtest", false, nullptr, FoodClass::Meal},
    {Item::HerbedCarp, "herbed-carp", "Herbed carp", "Lake carp gently cooked with meadow herbs.",
        ItemCategory::Food, "deferred-meal", 35, NoBuyers, 33.0, 33.0, "Deferred from this playtest", false, nullptr, FoodClass::Meal},
    {Item::MackerelChowder, "mackerel-chowder", "Mackerel and bean chowder", "Mackerel, potato and broad beans in a warming pot of chowder.",
        ItemCategory::Food, "deferred-meal", 63, NoBuyers, 50.0, 50.0, "Deferred from this playtest", false, nullptr, FoodClass::Meal},
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
            || !info.icon || !*info.icon || info.basePriceCoins < 0
            || static_cast<int>(info.category) < 0 || info.category >= ItemCategory::Count
            || info.hunger < 0.0 || info.energy < 0.0 || !info.source
            || (info.food != FoodClass::None) != (info.hunger > 0.0))
            return false;
    }
    return true;
}
static_assert(CatalogueComplete(), "Every ItemCatalogue row needs a key, name, description, icon and valid values; food rows a food class.");

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
FoodClass FoodClassOf(Item item) { return GetItemInfo(item).food; }
bool IsEdible(Item item) { return FoodClassOf(item) != FoodClass::None; }
std::string FoodEnergyLabel(Item item)
{
    if (!ValidItem(item) || !IsEdible(item)) return {};
    const long energy = std::lround(GetItemInfo(item).energy);
    return energy > 0 ? "+" + std::to_string(energy) + " Energy" : std::string();
}
bool IsTool(Item item) { return ValidItem(item) && GetItemInfo(item).category == ItemCategory::Tool; }
bool IsSeedPacket(Item item)
{
    switch (item)
    {
    case Item::Seeds: case Item::TurnipSeed: case Item::CarrotSeed: case Item::SeedPotato:
    case Item::CabbageSeed: case Item::BroadBeanSeed: case Item::StrawberryRunner: return true;
    default: return false;
    }
}
bool CanStackItem(Item item) { return ValidItem(item) && !IsSeedPacket(item); }
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
std::int64_t BasePrice(Item item) { return GetItemInfo(item).basePriceCoins; }
bool ShopBuys(ShopKind shop, Item item) { return ValidItem(item) && (GetItemInfo(item).buyers & ShopBit(shop)) != 0; }
}
