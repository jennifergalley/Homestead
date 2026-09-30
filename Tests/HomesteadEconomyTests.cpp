// Portable tests for the item catalogue, money and shops.
#include "HomesteadBackpack.h"
#include "HomesteadEstate.h"
#include "HomesteadEstatePublicRoad.h"
#include "HomesteadHoldings.h"
#include "HomesteadItems.h"
#include "HomesteadPail.h"
#include "HomesteadSimulation.h"
#include "HomesteadTravel.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <locale>
#include <set>
#include <sstream>
#include <string>
#include <utility>

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
    CHECK(std::string(ItemName(Item::DiggingStick)) == "Hoe");
    CHECK(std::string(ItemIcon(Item::DiggingStick)) == "digging-stick");
    CHECK(std::string(ItemIcon(Item::Fur)) == "fur");
    CHECK(IsEdible(Item::Berries) && IsEdible(Item::RoastedRoots) && IsEdible(Item::HerbedRoots));
    CHECK(!IsEdible(Item::Roots) && !IsEdible(Item::Seeds));
    CHECK(GetItemInfo(Item::HerbedRoots).hunger == 38.0 && GetItemInfo(Item::HerbedRoots).energy == 18.0);
    for (Item tool : {Item::Knife, Item::Hatchet, Item::DiggingStick, Item::WateringCan, Item::Machete})
        CHECK(IsTool(tool) && ItemSortRank(tool) == 0);
    CHECK(ItemSortRank(Item::Fur) == 1 && ItemSortRank(Item::Flowers) == 2 && ItemSortRank(Item::Water) == 3);
    CHECK(std::string(ItemSource(Item::Timber)) == "Mature trees, large stumps and fallen logs, with the axe");
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

// Rewrites her needs in a save round trip so long waits don't fail her.
void Edit(Simulation& sim, double hunger, double energy)
{
    const std::string saved = sim.Serialize();
    const auto newline = saved.find('\n');
    std::string payload = saved.substr(newline + 1);
    std::istringstream first(payload.substr(0, payload.find('\n')));
    std::string hour, minutes, oldHunger, oldEnergy, rest;
    first >> hour >> minutes >> oldHunger >> oldEnergy;
    std::getline(first, rest);
    std::ostringstream line;
    line.imbue(std::locale::classic());
    line << hour << ' ' << minutes << ' ' << hunger << ' ' << energy << rest;
    payload = line.str() + payload.substr(payload.find('\n'));
    OK(sim.Deserialize(Reseal(saved, payload)));
}

void MoneyFormatting()
{
    // Whole coins: the raw stored value is the number of coins (no x100 migration), grouped, with
    // singular/plural and no currency sign or decimals.
    CHECK(FormatMoney(0) == "0 coins");
    CHECK(FormatMoney(1) == "1 coin");
    CHECK(FormatMoney(5) == "5 coins");
    CHECK(FormatMoney(40) == "40 coins");
    CHECK(FormatMoney(999) == "999 coins");
    CHECK(FormatMoney(1000) == "1,000 coins");
    CHECK(FormatMoney(1240) == "1,240 coins");
    CHECK(FormatMoney(2234) == "2,234 coins");
    CHECK(FormatMoney(123456789) == "123,456,789 coins");
    CHECK(FormatMoney(MaxMoney) == "100,000,000,000 coins");
    CHECK(FormatMoney(-1) == "-1 coin" && FormatMoney(-100) == "-100 coins");
    CHECK(FormatMoney(INT64_MIN) == "-9,223,372,036,854,775,808 coins");
    CHECK(FormatMoney(INT64_MAX) == "9,223,372,036,854,775,807 coins");
    CHECK(FormatMoneyDelta(200) == "+200 coins" && FormatMoneyDelta(-40) == "-40 coins" && FormatMoneyDelta(0) == "+0 coins"
        && FormatMoneyDelta(1) == "+1 coin" && FormatMoneyDelta(-1) == "-1 coin");
    for (const Cents raw : {Cents(0), Cents(1), Cents(80), Cents(1000), MaxMoney, Cents(INT64_MIN)})
        CHECK(FormatMoney(raw).find('$') == std::string::npos && FormatMoney(raw).find('.') == std::string::npos);
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
    CHECK(sold.message == "Sold 12 Stone for 60 coins.");
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
    unchanged(sim.Buy(store.shop, Item::Pasty, 11, false, store.customer)); // 1,100 coins of her 1,000.
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
    CHECK(bought.message == "Bought 1 Cornish pasty for 100 coins.");
    CHECK(sim.GetState().money == StartingMoney - 100 && sim.Count(Item::Pasty) == 1);
    const double hunger = sim.GetState().hunger, energy = sim.GetState().energy;
    OK(sim.Eat(Item::Pasty));
    CHECK(sim.GetState().hunger > hunger && sim.GetState().energy > energy);
    CHECK(sim.Count(Item::Pasty) == 0);
    // Three loaves at 50 cents.
    const auto loaves = sim.Buy(store.shop, Item::Bread, 3, false, store.customer);
    OK(loaves);
    CHECK(loaves.message == "Bought 3 loaves of bread for 150 coins.");
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

void LeatherBackpackUpgrade()
{
    Store store = OpenStore();
    auto& sim = store.sim;
    CHECK(sim.PackCapacity() == InventoryCapacity && !sim.GetState().leatherBackpack);
    CHECK(Backpack::Offered(sim.GetState(), ShopKind::GeneralStore) && Backpack::Price == 1500);
    const std::string before = sim.Serialize();
    // No section until she owns one, so a new game saves exactly as before.
    CHECK(before.find(std::string("\n") + Backpack::SaveTag + " ") == std::string::npos);
    // 1,000 coins isn't enough; nothing changes.
    const std::uint64_t revision = sim.GetRevision();
    const auto poor = sim.BuyBackpack(store.shop, store.customer);
    CHECK(!poor.ok && poor.message == "That costs 1,500 coins; you have 1,000 coins." && sim.GetRevision() == revision);
    CHECK(!sim.SetBackpackShown(false).ok && sim.Serialize() == before);
    // Too far from the counter is refused too.
    OK(sim.GrantMoney(1000));
    CHECK(!sim.BuyBackpack(store.shop, {store.counter.x + 5000.0, store.counter.y}).ok);
    // Bought once: 2,000 - 1,500 leaves 500, and her pack doubles.
    const auto bought = sim.BuyBackpack(store.shop, store.customer);
    OK(bought);
    CHECK(bought.message == "Bought the leather backpack for 1,500 coins. You can carry 240 now.");
    CHECK(sim.GetState().money == 500 && sim.GetState().leatherBackpack && sim.GetState().backpackShown);
    CHECK(sim.PackCapacity() == 240 && !Backpack::Offered(sim.GetState(), ShopKind::GeneralStore));
    const auto again = sim.BuyBackpack(store.shop, store.customer);
    CHECK(!again.ok && sim.GetState().money == 500);
    // She can carry 240, but not 241.
    OK(sim.GrantItems(Item::Branch, 240 - sim.UsedCapacity()));
    CHECK(sim.UsedCapacity() == 240);
    CHECK(!sim.GrantItems(Item::Stone, 1).ok);
    const auto full = sim.Buy(store.shop, Item::Bread, 1, false, store.customer);
    CHECK(!full.ok && full.code == ResultCode::Capacity);
    // Hiding it is a look only: capacity stays.
    OK(sim.SetBackpackShown(false));
    CHECK(!sim.GetState().backpackShown && sim.PackCapacity() == 240);
    // Saved and loaded with a full 240 pack and the backpack hidden.
    const std::string saved = sim.Serialize();
    CHECK(saved.find(std::string("\n") + Backpack::SaveTag + " 1 0\n") != std::string::npos);
    Simulation loaded;
    loaded.SetPlacements(ProvisionalEstatePlacements());
    OK(loaded.Deserialize(saved));
    CHECK(loaded.GetState().leatherBackpack && !loaded.GetState().backpackShown && loaded.PackCapacity() == 240);
    CHECK(loaded.UsedCapacity() == 240 && loaded.Serialize() == saved);
    // An older save (no section) loads with the plain pack.
    Simulation older;
    older.SetPlacements(ProvisionalEstatePlacements());
    OK(older.Deserialize(before));
    CHECK(!older.GetState().leatherBackpack && older.GetState().backpackShown && older.PackCapacity() == InventoryCapacity);
    // A 240-item pack without the backpack is refused as corrupt, not truncated.
    std::string stripped = saved;
    const std::string payload = saved.substr(saved.find('\n') + 1);
    const std::string line = std::string(Backpack::SaveTag) + " 1 0\n";
    const auto at = payload.find(line);
    CHECK(at != std::string::npos);
    {
        const std::string trimmed = payload.substr(0, at) + payload.substr(at + line.size());
        std::uint64_t hash = UINT64_C(14695981039346656037);
        for (unsigned char c : trimmed) { hash ^= c; hash *= UINT64_C(1099511628211); }
        std::istringstream header(saved.substr(0, saved.find('\n')));
        std::string magic, version;
        header >> magic >> version;
        stripped = magic + " " + version + " " + std::to_string(trimmed.size()) + " " + std::to_string(hash) + "\n" + trimmed;
    }
    Simulation truncated;
    truncated.SetPlacements(ProvisionalEstatePlacements());
    CHECK(!truncated.Deserialize(stripped).ok);
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

void WaitForTheStoreToOpen()
{
    Store store = OpenStore();
    Simulation& sim = store.sim;
    const Point door{store.counter.x, store.counter.y - 700.0};
    CHECK(HoursUntilOpen(*sim.FindShop(store.shop), 9.0) == 0.0);
    CHECK(std::abs(HoursUntilOpen(*sim.FindShop(store.shop), 19.0) - 13.0) < 1e-9);
    CHECK(std::abs(HoursUntilOpen(*sim.FindShop(store.shop), 7.5) - 0.5) < 1e-9);
    // Open now: nothing to wait for, and no time passes.
    const std::string morning = sim.Serialize();
    CHECK(!sim.WaitForShop(store.shop, door).ok && sim.Serialize() == morning);
    // 7 PM: too far from the shop to wait for it.
    sim.SkipToHourOfDay(19.0);
    Edit(sim, 100.0, 100.0);
    const std::string evening = sim.Serialize();
    CHECK(!sim.WaitForShop(store.shop, {door.x, door.y - 5000.0}).ok && sim.Serialize() == evening);
    CHECK(!sim.WaitForShop(store.shop + 999, door).ok && sim.Serialize() == evening);
    // Too hungry to last the night: refused before any time passes.
    Edit(sim, 5.0, 100.0);
    const std::string hungry = sim.Serialize();
    const auto refused = sim.WaitForShop(store.shop, door);
    CHECK(!refused.ok && refused.message.find("too hungry") != std::string::npos);
    CHECK(sim.Serialize() == hungry);
    // Too tired to last the night (she'd doze off in the street): refused before any time passes.
    Edit(sim, 100.0, 3.0);
    const std::string tired = sim.Serialize();
    const auto sleepy = sim.WaitForShop(store.shop, door);
    CHECK(!sleepy.ok && sleepy.message.find("too tired") != std::string::npos);
    CHECK(sim.Serialize() == tired);
    // Fed: she waits the night through, across midnight and the 6 AM rollover, and it's open.
    Edit(sim, 100.0, 100.0);
    const double before = sim.GetState().hour;
    OK(sim.WaitForShop(store.shop, door));
    const double after = sim.GetState().hour;
    CHECK(std::abs((after - before) - 13.0) < 0.01);
    CHECK(IsShopOpen(*sim.FindShop(store.shop), after));
    CHECK(sim.GetState().hunger < 100.0);
    OK(sim.CheckShopAccess(store.shop, store.customer));
}

void NoWalkToTownFromTown()
{
    Store store = OpenStore();
    Simulation& sim = store.sim;
    const EstateLayout& layout = ProvisionalEstateLayout();
    const Point counter = store.counter;
    const Point door = layout.PointOr(Anchor::GeneralStoreDoor, {});
    const Point square = layout.PointOr(Anchor::TownSquare, {});
    const auto Refused = [&sim, &layout](Point from, const char* text)
    {
        const TravelPlan plan = PlanTravel(sim.GetState(), from, TravelDestination::Town, layout);
        return !plan.ok && plan.error == text;
    };
    // Inside the store and on its step: already there, open or closed.
    for (double hour : {9.0, 20.0})
    {
        sim.SkipToHourOfDay(hour);
        CHECK(Refused(counter, "You're already at the general store."));
        CHECK(Refused(store.customer, "You're already at the general store."));
        CHECK(Refused(door, "You're already at the general store."));
    }
    // About the square: already in town.
    CHECK(Refused(square, "You're already in town."));
    // A little way off down the street she can still walk to the road's end in town, briefly.
    const TravelPlan street = PlanTravel(sim.GetState(), {square.x, square.y - 6000.0}, TravelDestination::Town, layout);
    CHECK(street.ok && street.gameHours < 1.0);
    // From the gateway it's a real walk, and home to the manor from inside the store still is too.
    const Point gateway = EstatePublicRoad().At(EstatePublicRoad().FindStop("Gateway")->chainage);
    CHECK(PlanTravel(sim.GetState(), gateway, TravelDestination::Town, layout).ok);
    CHECK(PlanTravel(sim.GetState(), counter, TravelDestination::Manor, layout).ok);
    // Refused at the store: no time passes and nothing changes.
    const std::string before = sim.Serialize();
    const auto refused = sim.WalkRoad(TravelDestination::Town, counter);
    CHECK(!refused.ok && refused.message == "You're already at the general store." && sim.Serialize() == before);
}

void WalkTheRoad()
{
    CHECK(FormatHour(7.9999) == "8 AM" && FormatHour(23.999) == "12 AM" && FormatHour(19.2) == "7:12 PM");
    CHECK(FormatWalkDuration(7.2) == "7 h 12 min" && FormatWalkDuration(0.5) == "30 min" && FormatWalkDuration(2.0) == "2 h");
    const PublicRoad& road = EstatePublicRoad();
    const PublicRoadStop* manor = road.FindStop("Manor");
    const PublicRoadStop* town = road.FindStop("Town");
    CHECK(manor && town);
    Store store = OpenStore();
    Simulation& sim = store.sim;
    // Noon at the manor, default 60-minute day: the whole road at the conservative pace, ~7 game hours.
    sim.SkipToHourOfDay(12.0);
    const TravelPlan noon = PlanTravel(sim.GetState(), manor->position, TravelDestination::Town);
    CHECK(noon.ok && noon.connectorMetres < 1.0);
    CHECK(std::abs(noon.roadMetres - (town->chainage - manor->chainage)) < 1.0);
    const double expected = noon.totalMetres * 100.0 / RoadWalkPaceCmPerSecond / 3600.0 * 24.0 * 60.0 / sim.GetState().dayMinutes;
    CHECK(std::abs(noon.gameHours - expected) < 1e-9 && noon.gameHours > 6.8 && noon.gameHours < 7.4);
    CHECK(noon.arrival.x == town->position.x && noon.arrival.y == town->position.y && noon.arrivalZ == town->z);
    CHECK(noon.storeClosedOnArrival && noon.summary.find("closed") != std::string::npos && !noon.nextDay);
    CHECK(noon.summary.find("1.9 km") != std::string::npos);
    // The saved day length scales it: a 30-minute day doubles the game hours, 120 halves them.
    OK(sim.SetDayMinutes(30.0));
    CHECK(std::abs(PlanTravel(sim.GetState(), manor->position, TravelDestination::Town).gameHours - noon.gameHours * 2.0) < 1e-9);
    OK(sim.SetDayMinutes(120.0));
    CHECK(std::abs(PlanTravel(sim.GetState(), manor->position, TravelDestination::Town).gameHours - noon.gameHours / 2.0) < 1e-9);
    // ...and survives a save and reload.
    Simulation reloaded;
    reloaded.SetPlacements(ProvisionalEstatePlacements());
    OK(reloaded.Deserialize(sim.Serialize()));
    CHECK(std::abs(PlanTravel(reloaded.GetState(), manor->position, TravelDestination::Town).gameHours - noon.gameHours / 2.0) < 1e-9);
    OK(sim.SetDayMinutes(60.0));
    // Early enough, the store is open when she gets there.
    sim.SkipToHourOfDay(6.5);
    const TravelPlan morning = PlanTravel(sim.GetState(), manor->position, TravelDestination::Town);
    CHECK(morning.ok && !morning.storeClosedOnArrival && morning.summary.find("closed") == std::string::npos);
    // Late: past midnight on the way.
    sim.SkipToHourOfDay(22.0);
    CHECK(PlanTravel(sim.GetState(), manor->position, TravelDestination::Town).nextDay);
    // Partway along (the gateway), only the rest of the road counts; home to the manor never warns about the store.
    const Point gateway = road.At(road.FindStop("Gateway")->chainage);
    const TravelPlan half = PlanTravel(sim.GetState(), gateway, TravelDestination::Town);
    CHECK(half.ok && half.gameHours < noon.gameHours * 0.7);
    const TravelPlan home = PlanTravel(sim.GetState(), town->position, TravelDestination::Manor);
    CHECK(home.ok && !home.storeClosedOnArrival && home.arrival.x == manor->position.x);
    // Off the road: the straight walk back to it counts too, and far off she has to find it herself.
    const TravelPlan field = PlanTravel(sim.GetState(), {gateway.x + 20000.0, gateway.y}, TravelDestination::Town);
    CHECK(field.ok && field.connectorMetres > 150.0 && field.totalMetres > half.totalMetres);
    CHECK(!PlanTravel(sim.GetState(), {gateway.x + 90000.0, gateway.y}, TravelDestination::Town).ok);
    CHECK(!PlanTravel(sim.GetState(), town->position, TravelDestination::Town).ok);
    Simulation woodland;
    CHECK(!PlanTravel(woodland.GetState(), manor->position, TravelDestination::Town).ok);
    // Refusals pass no time at all.
    sim.SkipToHourOfDay(12.0);
    Edit(sim, 5.0, 100.0);
    const std::string hungry = sim.Serialize();
    const auto starving = sim.WalkRoad(TravelDestination::Town, manor->position);
    CHECK(!starving.ok && starving.message.find("too hungry") != std::string::npos && sim.Serialize() == hungry);
    Edit(sim, 100.0, 3.0);
    const std::string tired = sim.Serialize();
    const auto sleepy = sim.WalkRoad(TravelDestination::Town, manor->position);
    CHECK(!sleepy.ok && sleepy.message.find("too tired") != std::string::npos && sim.Serialize() == tired);
    const std::string there = sim.Serialize();
    CHECK(!sim.WalkRoad(TravelDestination::Town, town->position).ok && sim.Serialize() == there);
    // Fed and rested: the clock runs for the whole walk, and she's hungrier and a little more tired.
    Edit(sim, 100.0, 100.0);
    const double before = sim.GetState().hour;
    const std::uint64_t revision = sim.GetRevision();
    const TravelPlan plan = PlanTravel(sim.GetState(), manor->position, TravelDestination::Town);
    OK(sim.WalkRoad(TravelDestination::Town, manor->position));
    CHECK(std::abs(sim.GetState().hour - before - plan.gameHours) < 1e-6);
    CHECK(sim.GetRevision() > revision && sim.GetState().hunger < 100.0 && sim.GetState().energy < 100.0);
    CHECK(sim.DozeCount() == 0);
}

// Shops and the pack show the canonical nominal Energy one food restores, from its catalogue row.
void FoodEnergyLabels()
{
    CHECK(FoodEnergyLabel(Item::Pasty) == "+25 Energy");
    CHECK(FoodEnergyLabel(Item::Bread) == "+8 Energy");
    CHECK(FoodEnergyLabel(Item::Cheese) == "+12 Energy");
    for (int i = 0; i < ItemCount; ++i)
    {
        const auto item = static_cast<Item>(i);
        const std::string label = FoodEnergyLabel(item);
        const long energy = std::lround(GetItemInfo(item).energy);
        if (IsEdible(item) && energy > 0) CHECK(label == "+" + std::to_string(energy) + " Energy");
        else CHECK(label.empty());
    }
    // Not food: tools, materials and raw produce that needs cooking show nothing.
    CHECK(FoodEnergyLabel(Item::Hatchet).empty() && FoodEnergyLabel(Item::Stone).empty() && FoodEnergyLabel(Item::Potato).empty());
    CHECK(FoodEnergyLabel(Item::Count).empty() && FoodEnergyLabel(static_cast<Item>(-1)).empty());
}

void PailWaterPresentation()
{
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    const auto With = [&sim](int pails, int water)
    {
        State state = sim.GetState();
        state.inventory[static_cast<int>(Item::WateringCan)] = pails;
        state.inventory[static_cast<int>(Item::Water)] = water;
        return PresentPail(state);
    };
    // One carried pail: its water is a gauge and the pack's Water tile folds into it.
    for (int water : {0, 1, 6})
    {
        const auto pail = With(1, water);
        CHECK(pail.gauge && pail.charge == water && pail.hidePackWater);
    }
    // More than one pail holds (an older save), or 1200: the gauge is full and the tile shows every portion.
    for (int water : {7, 1200})
    {
        const auto pail = With(1, water);
        CHECK(pail.gauge && pail.charge == PailCapacity && !pail.hidePackWater);
    }
    // No pail carried (the starter pail is in a chest): no gauge, and any pack water shows as a tile.
    CHECK(!With(0, 0).gauge && !With(0, 0).hidePackWater);
    CHECK(!With(0, 4).gauge && !With(0, 4).hidePackWater);
    // Two pails: the gauge shows, and the water stays an ordinary tile rather than be split between them.
    CHECK(With(2, 4).gauge && With(2, 4).charge == 4 && !With(2, 4).hidePackWater);
    // Presentation only: the stock itself round-trips through a save untouched.
    OK(sim.GrantItems(Item::WateringCan, 1));
    OK(sim.GrantItems(Item::Water, 9));
    Simulation reloaded;
    reloaded.SetPlacements(ProvisionalEstatePlacements());
    OK(reloaded.Deserialize(sim.Serialize()));
    CHECK(reloaded.Count(Item::Water) == sim.Count(Item::Water) && reloaded.Count(Item::Water) >= 9);
    const auto loaded = PresentPail(reloaded.GetState());
    CHECK(loaded.gauge && loaded.charge == PailCapacity && !loaded.hidePackWater);
    // The pack's footer names the charge (a controller has no hover for the tooltip).
    CHECK(PailChargeLabel(With(1, 5)) == "Water 5 / 6" && PailChargeLabel(With(1, 0)) == "Water 0 / 6");
    CHECK(PailChargeLabel(With(1, 1200)) == "Water 6 / 6" && PailChargeLabel(With(0, 4)).empty());
}

// The "+3 Berries" line counts only real gains: gathering, buying, crafting and grants, never a chest
// move, a drop picked back up, spent ingredients or pail water.
void PickupGainsCountOnlyNewThings()
{
    Store store = OpenStore();
    auto& sim = store.sim;
    const auto Gain = [&sim](const Holdings& before, Item item) { return PickupGain(before, CountHoldings(sim.GetState()), item); };
    // Gathering berries at a bush on the estate.
    const ResourceNode* bush = nullptr;
    for (const auto& node : sim.GetState().resources)
        if (node.kind == ResourceKind::BerryBush && !node.cleared && node.readyAtHour <= sim.GetState().hour) { bush = &node; break; }
    CHECK(bush != nullptr);
    const Point bushAt = bush->position;
    Holdings before = CountHoldings(sim.GetState());
    const int berries = sim.Count(Item::Berries);
    OK(sim.Harvest(bush->id, bushAt));
    CHECK(sim.Count(Item::Berries) > berries && Gain(before, Item::Berries) == sim.Count(Item::Berries) - berries);
    // Buying at the store.
    before = CountHoldings(sim.GetState());
    OK(sim.Buy(store.shop, Item::Pasty, 2, false, store.customer));
    CHECK(Gain(before, Item::Pasty) == 2);
    // Crafting: the axe shows, the branch and stone it used don't (and never as a negative).
    OK(sim.GrantItems(Item::RustedAxeHead, 1));
    OK(sim.GrantItems(Item::Branch, 10));
    OK(sim.GrantItems(Item::Stone, 10));
    before = CountHoldings(sim.GetState());
    CHECK(Gain(before, Item::Branch) == 0);
    OK(sim.Craft(Recipe::HaftAxe, bushAt));
    CHECK(Gain(before, Item::Hatchet) == 1);
    for (int index = 0; index < ItemCount; ++index)
        if (static_cast<Item>(index) != Item::Hatchet) CHECK(Gain(before, static_cast<Item>(index)) == 0);
    // Setting branches down and picking them back up is a move.
    int group = 0;
    for (const auto& entry : *sim.GetLayout(0)) if (entry.item == Item::Branch) { group = entry.groupId; break; }
    CHECK(group != 0);
    OK(sim.DropGroup(group, 2, bushAt, bushAt, sim.GetRevision()));
    before = CountHoldings(sim.GetState());
    OK(sim.PickUpDrop(sim.GetState().worldDrops.back().id, bushAt));
    CHECK(Gain(before, Item::Branch) == 0);
    // Pail water fills the pail's gauge, not a pickup line.
    before = CountHoldings(sim.GetState());
    OK(sim.GrantItems(Item::Water, 3));
    CHECK(Gain(before, Item::Water) == 0);
    // Taking the pail from the manor chest and putting it back are moves, too.
    Simulation manor;
    manor.SetPlacements(ProvisionalEstatePlacements());
    OK(manor.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    const Structure* chest = nullptr;
    for (const auto& piece : manor.GetState().structures) if (piece.kind == Piece::Chest) { chest = &piece; break; }
    CHECK(chest != nullptr && chest->storage[static_cast<int>(Item::WateringCan)] >= 1);
    const int chestId = chest->id;
    const Point chestSide = manor.StructureCenter(*chest);
    before = CountHoldings(manor.GetState());
    OK(manor.Transfer(chestId, Item::WateringCan, -1, chestSide));
    const Holdings taken = CountHoldings(manor.GetState());
    CHECK(manor.Count(Item::WateringCan) == 1 && PickupGain(before, taken, Item::WateringCan) == 0);
    OK(manor.Transfer(chestId, Item::WateringCan, 1, chestSide));
    CHECK(PickupGain(taken, CountHoldings(manor.GetState()), Item::WateringCan) == 0);
}

int PackStock(const Simulation& sim, Item item)
{
    int total = 0;
    for (const auto& entry : *sim.GetLayout(0))
        if (entry.wearableId == 0 && entry.item == item) total += entry.quantity;
    return total;
}

// Stock truth on the actual-stack hotbar: a "+N" is exactly what her stacks gained, and arranging
// those stacks (hotbar row, split, sort, back below the row), eating, garments and the lamp never
// add a line or a second one for the same thing.
void PickupGainsFollowActualStacks()
{
    Store store = OpenStore();
    auto& sim = store.sim;
    const Point here = store.customer;
    const auto Gains = [&sim](const Holdings& before)
    {
        int lines = 0, total = 0;
        const Holdings now = CountHoldings(sim.GetState());
        for (int index = 0; index < ItemCount; ++index)
            if (const int gain = PickupGain(before, now, static_cast<Item>(index)); gain > 0) { ++lines; total += gain; }
        return std::make_pair(lines, total);
    };
    // Buying five pasties is one line of exactly what her stacks gained.
    const int stacked = PackStock(sim, Item::Pasty);
    Holdings before = CountHoldings(sim.GetState());
    OK(sim.Buy(store.shop, Item::Pasty, 5, false, here));
    CHECK(PackStock(sim, Item::Pasty) - stacked == 5 && sim.Count(Item::Pasty) == PackStock(sim, Item::Pasty));
    CHECK(PickupGain(before, CountHoldings(sim.GetState()), Item::Pasty) == 5);
    CHECK(Gains(before) == std::make_pair(1, 5));
    int group = 0;
    for (const auto& entry : *sim.GetLayout(0)) if (entry.item == Item::Pasty) group = entry.groupId;
    CHECK(group != 0);
    // Arranging stacks: into the hotbar row, split, sorted and back out again.
    before = CountHoldings(sim.GetState());
    OK(sim.MoveToPackRow(group, 0, 9, sim.GetRevision()));
    OK(sim.MoveFromPackRow(9, 0, 0, sim.GetRevision()));
    OK(sim.SplitHalf(0, group, here, sim.GetRevision()));
    OK(sim.SortPack(sim.GetRevision()));
    CHECK(Gains(before) == std::make_pair(0, 0) && sim.Count(Item::Pasty) == PackStock(sim, Item::Pasty));
    // Eating one is a loss, never a line.
    Edit(sim, 40.0, 100.0);
    for (const auto& entry : *sim.GetLayout(0)) if (entry.item == Item::Pasty) { group = entry.groupId; break; }
    before = CountHoldings(sim.GetState());
    OK(sim.EatGroup(group, sim.GetRevision()));
    CHECK(Gains(before) == std::make_pair(0, 0));
    // A garment set down and picked back up (its drop has no item) is a move.
    int garment = 0;
    for (const auto& wearable : sim.GetState().wearables)
        if (wearable.owner == WearableOwner::Carried) { garment = wearable.id; break; }
    if (garment == 0)
    {
        OK(sim.GrantItems(Item::Fiber, 20));
        if (sim.Count(Item::Knife) == 0) OK(sim.GrantItems(Item::Knife, 1));
        before = CountHoldings(sim.GetState());
        OK(sim.CraftGarment(WearableDefinition::LinenTunic, here, sim.GetRevision()));
        CHECK(Gains(before) == std::make_pair(0, 0));
        garment = sim.GetState().wearables.back().id;
    }
    before = CountHoldings(sim.GetState());
    OK(sim.DropWearable(garment, here, here, sim.GetRevision()));
    CHECK(sim.GetState().worldDrops.back().item == Item::Count);
    OK(sim.PickUpDrop(sim.GetState().worldDrops.back().id, here));
    CHECK(Gains(before) == std::make_pair(0, 0));
    // So is the lamp set down and taken up again.
    if (sim.Count(Item::OilLamp) == 0) OK(sim.GrantItems(Item::OilLamp, 1));
    before = CountHoldings(sim.GetState());
    OK(sim.SetDownLamp(here, here));
    OK(sim.PickUpDrop(sim.SetDownLampDrop()->id, here));
    CHECK(Gains(before) == std::make_pair(0, 0) && sim.Count(Item::OilLamp) == 1);
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
    Run("the leather backpack upgrade", LeatherBackpackUpgrade);
    Run("her goods sell down each morning", StockSellsDownEachMorning);
    Run("money and shops survive save and reload", EconomySurvivesSaveAndReload);
    Run("no walk to town from town", NoWalkToTownFromTown);
    Run("playtest shop placement", PlaytestShopPlacement);
    Run("wait for the store to open", WaitForTheStoreToOpen);
    Run("walk the road to town and back", WalkTheRoad);
    Run("pickup lines count only new things", PickupGainsCountOnlyNewThings);
    Run("pickup lines follow her actual stacks", PickupGainsFollowActualStacks);
    Run("pail water shows on the pail", PailWaterPresentation);
    Run("food shows its Energy", FoodEnergyLabels);
    std::cout << cases << " scenarios, " << checks << " explicit checks passed.\n";
    return 0;
}
