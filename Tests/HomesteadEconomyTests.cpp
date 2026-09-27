// Portable tests for the item catalogue, money and shops.
#include "HomesteadItems.h"
#include "HomesteadSimulation.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>
#include <string>

using namespace Homestead;

namespace
{
int checks = 0;
int cases = 0;
void Check(bool condition, const char* expression, int line)
{
    ++checks;
    if (!condition)
    {
        std::cerr << "FAIL line " << line << ": " << expression << '\n';
        std::exit(1);
    }
}
#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)

void CatalogueCoversEveryItem()
{
    std::set<std::string> keys;
    for (int i = 0; i < ItemCount; ++i)
    {
        const auto item = static_cast<Item>(i);
        const ItemInfo& info = GetItemInfo(item);
        CHECK(info.item == item);
        CHECK(std::strlen(info.name) > 0 && std::strlen(info.description) > 0 && std::strlen(info.icon) > 0);
        CHECK(keys.insert(info.key).second);
        CHECK(info.basePriceCents >= 0);
        CHECK(IsEdible(item) == (info.hunger > 0.0));
    }
    CHECK(std::string(ItemName(Item::Count)) == "Unknown item");
    CHECK(std::string(ItemName(static_cast<Item>(-1))) == "Unknown item");
    CHECK(!IsEdible(Item::Count) && !IsTool(Item::Count) && !ShopBuys(ShopKind::GeneralStore, Item::Count));
}

void CatalogueMatchesLegacyMetadata()
{
    CHECK(std::string(ItemName(Item::Flowers)) == "Meadow herb");
    CHECK(std::string(ItemName(Item::DiggingStick)) == "Stone hoe");
    CHECK(std::string(ItemIcon(Item::DiggingStick)) == "digging-stick");
    CHECK(std::string(ItemIcon(Item::Fur)) == "fur");
    CHECK(IsEdible(Item::Berries) && IsEdible(Item::RoastedRoots) && IsEdible(Item::HerbedRoots));
    CHECK(!IsEdible(Item::Roots) && !IsEdible(Item::Seeds));
    CHECK(GetItemInfo(Item::HerbedRoots).hunger == 38.0 && GetItemInfo(Item::HerbedRoots).energy == 18.0);
    for (Item tool : {Item::Knife, Item::Hatchet, Item::DiggingStick, Item::WateringCan, Item::Machete})
        CHECK(IsTool(tool) && ItemSortRank(tool) == 0);
    CHECK(ItemSortRank(Item::Fur) == 1 && ItemSortRank(Item::Flowers) == 2 && ItemSortRank(Item::Water) == 3);
    CHECK(std::string(ItemSource(Item::Timber)) == "Mature trees with Hatchet");
    CHECK(std::string(ItemSource(Item::Hatchet)).empty());
    CHECK(ShopBuys(ShopKind::GeneralStore, Item::Stone) && !ShopBuys(ShopKind::GeneralStore, Item::Knife));
}

const char* filter = nullptr;
void Run(const char* name, void (*test)())
{
    if (filter && !std::strstr(name, filter)) return;
    test();
    ++cases;
    std::cout << "PASS " << name << '\n';
}
}

int main(int argc, char** argv)
{
    if (argc > 1) filter = argv[1];
    Run("catalogue covers every item", CatalogueCoversEveryItem);
    Run("catalogue matches legacy metadata", CatalogueMatchesLegacyMetadata);
    std::cout << cases << " scenarios, " << checks << " explicit checks passed.\n";
    return 0;
}
