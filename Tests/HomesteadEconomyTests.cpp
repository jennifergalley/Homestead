// Portable tests for the item catalogue, money and shops.
#include "HomesteadEstate.h"
#include "HomesteadItems.h"
#include "HomesteadSimulation.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <locale>
#include <set>
#include <sstream>
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
    CHECK(ItemFromKey("pasty") == Item::Pasty && ItemFromKey("nope") == Item::Count && ItemFromKey(nullptr) == Item::Count);
}

void Okay(const Result& result, int line)
{
    ++checks;
    if (!result.ok)
    {
        std::cerr << "FAIL line " << line << ": " << result.message << '\n';
        std::exit(1);
    }
}
#define OK(expression) Okay(expression, __LINE__)

// A save's header with `payload` in place of its own.
std::string Reseal(const std::string& saved, const std::string& payload)
{
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned char c : payload) { hash ^= c; hash *= UINT64_C(1099511628211); }
    std::istringstream header(saved.substr(0, saved.find('\n')));
    std::string magic, version;
    header >> magic >> version;
    return magic + " " + version + " " + std::to_string(payload.size()) + " " + std::to_string(hash) + "\n" + payload;
}

// Rewrites her needs in a save round trip (warmth full) so long waits don't fail her.
void Edit(Simulation& sim, double hunger, double energy)
{
    const std::string saved = sim.Serialize();
    const auto newline = saved.find('\n');
    std::string payload = saved.substr(newline + 1);
    std::istringstream first(payload.substr(0, payload.find('\n')));
    std::string hour, minutes, oldHunger, oldEnergy, oldWarmth, rest;
    first >> hour >> minutes >> oldHunger >> oldEnergy >> oldWarmth;
    std::getline(first, rest);
    std::ostringstream line;
    line.imbue(std::locale::classic());
    line << hour << ' ' << minutes << ' ' << hunger << ' ' << energy << ' ' << 100 << rest;
    payload = line.str() + payload.substr(payload.find('\n'));
    OK(sim.Deserialize(Reseal(saved, payload)));
}

void MoneyFormatting()
{
    CHECK(FormatMoney(0) == "$0.00");
    CHECK(FormatMoney(5) == "$0.05");
    CHECK(FormatMoney(1240) == "$12.40");
    CHECK(FormatMoney(123456789) == "$1,234,567.89");
    CHECK(FormatMoney(100000) == "$1,000.00");
    CHECK(FormatMoney(-100) == "-$1.00");
    CHECK(FormatMoney(INT64_MIN) == "-$92,233,720,368,547,758.08");
    CHECK(FormatMoneyDelta(200) == "+$2.00" && FormatMoneyDelta(-40) == "-$0.40" && FormatMoneyDelta(0) == "+$0.00");
    CHECK(BuyPrice(Item::Pasty) == 100 && SellPrice(Item::Stone) == 5 && BuyBackPrice(Item::Stone) == 5);
    CHECK(SellDownAmount(0) == 0 && SellDownAmount(1) == 1 && SellDownAmount(10) == 4 && SellDownAmount(3) == 2);
    CHECK(FormatHour(8) == "8 AM" && FormatHour(18) == "6 PM" && FormatHour(0) == "12 AM" && FormatHour(12.5) == "12:30 PM");
}

struct Store
{
    Simulation sim;
    int shop = 0;
    Point counter{};
    Point customer{};
};

Store OpenStore()
{
    Store store;
    OK(store.sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    const Shop* shop = store.sim.FindShop(ShopKind::GeneralStore);
    CHECK(shop != nullptr);
    store.shop = shop->id;
    store.counter = ProvisionalEstateLayout().PointOr(Anchor::GeneralStoreCounter, {});
    CHECK(shop->counterX == store.counter.x && shop->counterY == store.counter.y);
    CHECK(shop->counterYaw == ProvisionalEstateLayout().FindLandmark(Anchor::GeneralStoreCounter)->yaw);
    store.customer = {store.counter.x, store.counter.y - 150.0};
    store.sim.SkipToHourOfDay(9.0);
    return store;
}

void NewEstateStartsWithMoneyAndAStore()
{
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    CHECK(sim.GetState().money == StartingMoney && StartingMoney == 1000);
    CHECK(sim.GetState().shops.size() == 1);
    const Shop& shop = sim.GetState().shops[0];
    CHECK(shop.openHour == 8.0 && shop.closeHour == 18.0);
    for (int quantity : shop.heroineStock) CHECK(quantity == 0);
    Simulation woodland;
    CHECK(woodland.GetState().money == 0 && woodland.GetState().shops.empty());
}

void SellAStack()
{
    Store store = OpenStore();
    auto& sim = store.sim;
    OK(sim.GrantItems(Item::Stone, 20));
    const Cents before = sim.GetState().money;
    const auto revision = sim.GetRevision();
    const auto sold = sim.Sell(store.shop, Item::Stone, 12, store.customer);
    OK(sold);
    CHECK(sold.message == "Sold 12 Stone for $0.60.");
    CHECK(sold.revision > revision);
    CHECK(sim.Count(Item::Stone) == 8);
    CHECK(sim.GetState().money == before + 12 * SellPrice(Item::Stone));
    CHECK(sim.FindShop(store.shop)->heroineStock[static_cast<int>(Item::Stone)] == 12);
    // The pack layout follows the counts.
    int laidOut = 0;
    for (const auto& entry : *sim.GetLayout(0)) if (entry.item == Item::Stone) laidOut += entry.quantity;
    CHECK(laidOut == 8);
    OK(sim.Sell(store.shop, Item::Stone, 8, store.customer));
    CHECK(sim.Count(Item::Stone) == 0 && sim.FindShop(store.shop)->heroineStock[static_cast<int>(Item::Stone)] == 20);
}

void RejectedTradesChangeNothing()
{
    Store store = OpenStore();
    auto& sim = store.sim;
    OK(sim.GrantItems(Item::Stone, 5));
    OK(sim.GrantItems(Item::Hatchet, 1));
    const std::string saved = sim.Serialize();
    const auto unchanged = [&](const Result& result) {
        CHECK(!result.ok);
        CHECK(sim.Serialize() == saved);
    };
    unchanged(sim.Sell(store.shop, Item::Stone, 6, store.customer));
    unchanged(sim.Sell(store.shop, Item::Stone, 0, store.customer));
    unchanged(sim.Sell(store.shop, Item::Stone, -3, store.customer));
    unchanged(sim.Sell(store.shop, Item::Hatchet, 1, store.customer));
    unchanged(sim.Sell(store.shop, Item::Count, 1, store.customer));
    unchanged(sim.Sell(store.shop + 999, Item::Stone, 1, store.customer));
    unchanged(sim.Sell(store.shop, Item::Stone, 1, {store.counter.x, store.counter.y - 1000.0}));
    unchanged(sim.Buy(store.shop, Item::Pasty, 11, false, store.customer)); // $11.00 of her $10.00.
    unchanged(sim.Buy(store.shop, Item::Stone, 1, false, store.customer));  // Not a shop good.
    unchanged(sim.Buy(store.shop, Item::Stone, 1, true, store.customer));   // She hasn't sold any.
    unchanged(sim.Buy(store.shop, Item::Pasty, 0, false, store.customer));
    // Closed at 7 PM, with the opening time in the message.
    sim.SkipToHourOfDay(19.0);
    const std::string evening = sim.Serialize();
    const auto closed = sim.Sell(store.shop, Item::Stone, 1, store.customer);
    CHECK(!closed.ok && closed.message == "Closed - opens at 8 AM" && closed.code == ResultCode::Unavailable);
    CHECK(sim.Serialize() == evening);
    CHECK(!sim.CheckShopAccess(store.shop, store.customer).ok);
    sim.SkipToHourOfDay(8.0);
    OK(sim.CheckShopAccess(store.shop, store.customer));
    sim.SkipToHourOfDay(18.0);
    CHECK(!sim.CheckShopAccess(store.shop, store.customer).ok);
}

void BuyAndEatAPasty()
{
    Store store = OpenStore();
    auto& sim = store.sim;
    Edit(sim, 40.0, 50.0);
    const auto bought = sim.Buy(store.shop, Item::Pasty, 1, false, store.customer);
    OK(bought);
    CHECK(bought.message == "Bought 1 Cornish pasty for $1.00.");
    CHECK(sim.GetState().money == StartingMoney - 100 && sim.Count(Item::Pasty) == 1);
    const double hunger = sim.GetState().hunger, energy = sim.GetState().energy;
    OK(sim.Eat(Item::Pasty));
    CHECK(sim.GetState().hunger > hunger && sim.GetState().energy > energy);
    CHECK(sim.Count(Item::Pasty) == 0);
    // Three loaves at 50 cents.
    OK(sim.Buy(store.shop, Item::Bread, 3, false, store.customer));
    CHECK(sim.GetState().money == StartingMoney - 100 - 150 && sim.Count(Item::Bread) == 3);
    // Everything she has left, exactly.
    OK(sim.GrantMoney(-sim.GetState().money + 25));
    OK(sim.Buy(store.shop, Item::Twine, 1, false, store.customer));
    CHECK(sim.GetState().money == 0);
    CHECK(!sim.Buy(store.shop, Item::Twine, 1, false, store.customer).ok);
}

void BuyBackAndCapacity()
{
    Store store = OpenStore();
    auto& sim = store.sim;
    OK(sim.GrantItems(Item::Stone, 10));
    OK(sim.Sell(store.shop, Item::Stone, 10, store.customer));
    const Cents afterSale = sim.GetState().money;
    OK(sim.Buy(store.shop, Item::Stone, 4, true, store.customer));
    CHECK(sim.GetState().money == afterSale - 4 * SellPrice(Item::Stone));
    CHECK(sim.FindShop(store.shop)->heroineStock[static_cast<int>(Item::Stone)] == 6 && sim.Count(Item::Stone) == 4);
    // A full pack refuses more.
    OK(sim.GrantMoney(10000));
    OK(sim.GrantItems(Item::Branch, InventoryCapacity - sim.UsedCapacity()));
    CHECK(sim.UsedCapacity() == InventoryCapacity);
    const auto full = sim.Buy(store.shop, Item::Bread, 1, false, store.customer);
    CHECK(!full.ok && full.code == ResultCode::Capacity);
}

void StockSellsDownEachMorning()
{
    Store store = OpenStore();
    auto& sim = store.sim;
    OK(sim.GrantItems(Item::Flowers, 10));
    OK(sim.Sell(store.shop, Item::Flowers, 10, store.customer));
    const auto stock = [&] { return sim.FindShop(store.shop)->heroineStock[static_cast<int>(Item::Flowers)]; };
    // Nothing sells before the next 6 AM.
    Edit(sim, 100.0, 100.0);
    sim.AdvanceGameHours(20.0, store.customer); // 9 AM -> 5 AM.
    CHECK(stock() == 10);
    Edit(sim, 100.0, 100.0);
    sim.AdvanceGameHours(2.0, store.customer); // Past 6 AM.
    CHECK(stock() == 6);
    CHECK(stock() < 10 && stock() > 0);
    int previous = stock();
    for (int day = 0; day < 10; ++day)
    {
        Edit(sim, 100.0, 100.0);
        sim.AdvanceGameHours(24.0, store.customer);
        CHECK(stock() < previous || stock() == 0);
        previous = stock();
    }
    CHECK(stock() == 0);
}

void EconomySurvivesSaveAndReload()
{
    Store store = OpenStore();
    auto& sim = store.sim;
    OK(sim.GrantItems(Item::Stone, 7));
    OK(sim.Sell(store.shop, Item::Stone, 7, store.customer));
    OK(sim.Buy(store.shop, Item::Cheese, 2, false, store.customer));
    OK(sim.GreetShopkeeper(store.shop));
    const std::string saved = sim.Serialize();
    Simulation loaded;
    loaded.SetPlacements(ProvisionalEstatePlacements());
    OK(loaded.Deserialize(saved));
    CHECK(loaded.GetState().money == sim.GetState().money);
    CHECK(loaded.GetState().shops.size() == 1);
    const Shop& shop = loaded.GetState().shops[0];
    CHECK(shop.id == store.shop && shop.greetings == 1);
    CHECK(shop.heroineStock == sim.FindShop(store.shop)->heroineStock);
    CHECK(loaded.Count(Item::Cheese) == 2);
    CHECK(loaded.Serialize() == saved);
    // A save from before money has no economy section and loads with none.
    Simulation woodland;
    const std::string plain = woodland.Serialize();
    const auto section = plain.find("economy 0 0\n");
    CHECK(section != std::string::npos);
    const auto newline = plain.find('\n');
    const std::string payload = plain.substr(newline + 1, section - newline - 1) + plain.substr(section + 12);
    Simulation reloaded;
    OK(reloaded.Deserialize(Reseal(plain, payload)));
    CHECK(reloaded.GetState().money == 0 && reloaded.GetState().shops.empty());
}

void PlaytestShopPlacement()
{
    Simulation sim;
    OK(sim.PlaceShop(ShopKind::GeneralStore, {100.0, 200.0}));
    const Shop* shop = sim.FindShop(ShopKind::GeneralStore);
    CHECK(shop && shop->counterX == 100.0 && shop->counterY == 200.0);
    OK(sim.PlaceShop(ShopKind::GeneralStore, {300.0, 200.0}, 45.0));
    CHECK(sim.GetState().shops.size() == 1 && sim.FindShop(ShopKind::GeneralStore)->counterX == 300.0);
    CHECK(!sim.PlaceShop(ShopKind::GeneralStore, {1e12, 0.0}).ok);
    CHECK(!sim.GrantMoney(-1).ok);
    OK(sim.GrantMoney(250));
    Simulation loaded;
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(loaded.GetState().money == 250 && loaded.FindShop(ShopKind::GeneralStore)->counterX == 300.0);
    CHECK(loaded.FindShop(ShopKind::GeneralStore)->counterYaw == 45.0);
}

const char* filter = nullptr;void Run(const char* name, void (*test)())
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
    Run("money formatting and prices", MoneyFormatting);
    Run("a new estate starts with money and a store", NewEstateStartsWithMoneyAndAStore);
    Run("sell a stack at the counter", SellAStack);
    Run("rejected trades change nothing", RejectedTradesChangeNothing);
    Run("buy and eat a pasty", BuyAndEatAPasty);
    Run("buy back and pack capacity", BuyBackAndCapacity);
    Run("her goods sell down each morning", StockSellsDownEachMorning);
    Run("money and shops survive save and reload", EconomySurvivesSaveAndReload);
    Run("playtest shop placement", PlaytestShopPlacement);
    std::cout << cases << " scenarios, " << checks << " explicit checks passed.\n";
    return 0;
}
