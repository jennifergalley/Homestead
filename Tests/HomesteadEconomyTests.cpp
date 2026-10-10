// Portable tests for the item catalogue, money and shops.
#include "HomesteadBackpack.h"
#include "HomesteadCrops.h"
#include "HomesteadEstate.h"
#include "HomesteadEstatePublicRoad.h"
#include "HomesteadGarmentShop.h"
#include "HomesteadHoldings.h"
#include "HomesteadItems.h"
#include "HomesteadOvergrowth.h"
#include "HomesteadPail.h"
#include "HomesteadToolUpgrades.h"
#include "HomesteadSimulation.h"
#include "HomesteadTravel.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <limits>
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
        CHECK(info.basePriceCoins >= 0);
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
    CHECK(GetItemInfo(Item::HerbedRoots).hunger == 38.0 && GetItemInfo(Item::HerbedRoots).energy == 40.0);
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
    for (const Coins raw : {Coins(0), Coins(1), Coins(80), Coins(1000), MaxMoney, Coins(INT64_MIN)})
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

Store OpenStore(bool emptyEstate = false)
{
    Store store;
    EstatePlacements empty;
    empty.bakeVersion = ProvisionalEstatePlacements().bakeVersion;
    OK(store.sim.NewEstateGame(ProvisionalEstateLayout(), emptyEstate ? empty : ProvisionalEstatePlacements()));
    const Shop* shop = store.sim.FindShop(ShopKind::GeneralStore);
    CHECK(shop != nullptr);
    store.shop = shop->id;
    store.counter = ProvisionalEstateLayout().PointOr(Anchor::GeneralStoreCounter, {});
    CHECK(shop->counterX == store.counter.x && shop->counterY == store.counter.y);
    CHECK(shop->counterYaw == ProvisionalEstateLayout().FindLandmark(Anchor::GeneralStoreCounter)->yaw);
    store.customer = {store.counter.x, store.counter.y - 150.0};
    OK(store.sim.DiscoverTravel(TravelDestination::Town, store.counter));
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
    const Coins before = sim.GetState().money;
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
    CHECK(!closed.ok && closed.message == "Closed \xE2\x80\x93 opens at 8 AM" && closed.code == ResultCode::Unavailable);
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
    CHECK(bought.message == "Bought 1 Meat pasty for 100 coins.");
    CHECK(sim.GetState().money == StartingMoney - 100 && sim.Count(Item::Pasty) == 1);
    const double energy = sim.GetState().energy;
    OK(sim.Eat(Item::Pasty));
    // A Meal: +40 Energy and Well fed; the estate has no hunger.
    CHECK(std::abs(sim.GetState().energy - (energy + 40.0)) < 1e-9 && sim.IsWellFed() && sim.GetState().hunger == 100.0);
    CHECK(sim.Count(Item::Pasty) == 0);
    // Three loaves at 50 coins.
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
    const Coins afterSale = sim.GetState().money;
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
    // With the backpack a ground stack may hold 240 (two drops merge up to it), and she picks it up whole.
    const auto branchGroup = [&sim]()
    {
        for (const auto& entry : sim.GetState().inventoryLayout)
            if (entry.wearableId == 0 && entry.item == Item::Branch) return entry;
        return LayoutEntry{};
    };
    const int branches = sim.Count(Item::Branch);
    CHECK(branches > InventoryCapacity + 60);
    OK(sim.DropGroup(branchGroup().groupId, branches - 60, store.customer, store.customer, sim.GetRevision()));
    OK(sim.DropGroup(branchGroup().groupId, 60, store.customer, store.customer, sim.GetRevision()));
    CHECK(sim.GetState().worldDrops.size() == 1 && sim.GetState().worldDrops.front().quantity == branches);
    OK(sim.PickUpDrop(sim.GetState().worldDrops.front().id, store.customer));
    CHECK(sim.UsedCapacity() == 240 && sim.GetState().worldDrops.empty());
    // Hiding it is a look only: capacity stays.
    const auto carried = sim.GetState().inventory;
    const auto groups = sim.GetState().inventoryLayout;
    OK(sim.SetBackpackShown(false));
    CHECK(!sim.GetState().backpackShown && sim.PackCapacity() == 240);
    CHECK(sim.GetState().inventory == carried && sim.GetState().inventoryLayout.size() == groups.size());
    OK(sim.SetBackpackShown(true));
    CHECK(sim.GetState().backpackShown && sim.PackCapacity() == 240 && sim.UsedCapacity() == 240);
    CHECK(sim.GetState().inventory == carried);
    OK(sim.SetBackpackShown(false));
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

void ClothingAtTheStore()
{
    // Jenny, 2026-10-04: clothes are bought at the general store from day one (none in the manor
    // chest), one of each, and wearing one swaps it for what she has on without unequipping first.
    Store store = OpenStore();
    auto& sim = store.sim;
    CHECK(std::size(GarmentShop::Offers) == 15);
    Coins total = 0;
    for (const auto& offer : GarmentShop::Offers)
    {
        total += offer.price;
        CHECK(GarmentShop::Offered(ShopKind::GeneralStore, offer.definition));
        CHECK(!GarmentShop::Owned(sim.GetState(), offer.definition));
    }
    CHECK(total == 3300 && GarmentShop::Price(WearableDefinition::LinenShirt) == 120);
    CHECK(GarmentShop::Price(WearableDefinition::BikerJacket) == 450 && GarmentShop::Price(WearableDefinition::ScoopTank) == 100);
    // Every garment the store sells has a name, a description and one equipment slot.
    for (const auto& offer : GarmentShop::Offers)
    {
        const auto* info = GetWearableDefinition(offer.definition);
        CHECK(info && std::strlen(info->name) > 0 && std::string(WearableDescription(offer.definition)) != "Unknown garment");
        CHECK(info->slots != 0 && (info->slots & (info->slots - 1)) == 0);
    }
    CHECK(!GarmentShop::Offered(ShopKind::GeneralStore, WearableDefinition::LinenTunic)
        && !GarmentShop::Offered(ShopKind::GeneralStore, WearableDefinition::LinenApron));
    // Refusals change nothing: not for sale, or too far from the counter.
    const std::uint64_t revision = sim.GetRevision();
    const std::string before = sim.Serialize();
    CHECK(!sim.BuyGarment(store.shop, WearableDefinition::LinenTunic, store.customer).ok);
    CHECK(!sim.BuyGarment(store.shop, WearableDefinition::LinenShirt, {store.counter.x + 5000.0, store.counter.y}).ok);
    CHECK(sim.GetRevision() == revision && sim.Serialize() == before);
    // 1,000 coins: the shirt, the trousers and the fur boots, then the coat is too dear.
    const auto bought = sim.BuyGarment(store.shop, WearableDefinition::LinenShirt, store.customer);
    OK(bought);
    CHECK(bought.message == "Bought the linen shirt for 120 coins.");
    const int shirt = sim.GetState().wearables.back().id;
    CHECK(sim.GetWearable(shirt)->definition == WearableDefinition::LinenShirt
        && sim.GetWearable(shirt)->owner == WearableOwner::Carried);
    CHECK(GarmentShop::Owned(sim.GetState(), WearableDefinition::LinenShirt) && sim.GetState().money == 880);
    const auto again = sim.BuyGarment(store.shop, WearableDefinition::LinenShirt, store.customer);
    CHECK(!again.ok && again.message == "You already own the linen shirt." && sim.GetState().money == 880);
    OK(sim.BuyGarment(store.shop, WearableDefinition::Trousers, store.customer));
    const int trousers = sim.GetState().wearables.back().id;
    OK(sim.BuyGarment(store.shop, WearableDefinition::FurBoots, store.customer));
    const auto dear = sim.BuyGarment(store.shop, WearableDefinition::FurCoat, store.customer);
    CHECK(!dear.ok && dear.message == "That costs 600 coins; you have 380 coins.");
    // A full pack has no room for one more.
    Store full = OpenStore();
    OK(full.sim.GrantItems(Item::Branch, full.sim.PackCapacity() - full.sim.UsedCapacity()));
    const auto crowded = full.sim.BuyGarment(full.shop, WearableDefinition::WovenSandals, full.customer);
    CHECK(!crowded.ok && crowded.code == ResultCode::Capacity && full.sim.GetState().money == 1000);

    // Swap from her pack: the shirt goes straight over the starter tunic, which drops to her pack.
    const int tunic = sim.GetState().equipment[static_cast<int>(EquipmentSlot::Torso)];
    CHECK(sim.GetWearable(tunic)->definition == WearableDefinition::LinenTunic);
    OK(sim.EquipWearable(shirt, sim.GetRevision()));
    CHECK(sim.GetWearable(shirt)->owner == WearableOwner::Equipped && sim.GetWearable(tunic)->owner == WearableOwner::Carried);
    // Swap from a chest she stands at: the displaced garment goes into that chest.
    const Structure* chest = nullptr;
    for (const auto& piece : sim.GetState().structures) if (piece.kind == Piece::Chest) chest = &piece;
    CHECK(chest != nullptr);
    const int chestId = chest->id;
    const Point side = sim.StructureCenter(*chest);
    OK(sim.MoveWearable(trousers, chestId, side, sim.GetRevision()));
    OK(sim.MoveWearable(tunic, chestId, side, sim.GetRevision()));
    const std::string stored = sim.Serialize();
    CHECK(!sim.EquipWearable(trousers, sim.GetRevision()).ok);
    CHECK(!sim.EquipWearable(trousers, {side.x + 5000.0, side.y}, sim.GetRevision()).ok && sim.Serialize() == stored);
    OK(sim.EquipWearable(trousers, side, sim.GetRevision()));
    CHECK(sim.GetWearable(trousers)->owner == WearableOwner::Equipped && sim.GetWearable(trousers)->chestId == 0);
    // The tunic covers torso and legs, so wearing it from the chest sends shirt and trousers there.
    OK(sim.EquipWearable(tunic, side, sim.GetRevision()));
    for (int id : {shirt, trousers})
        CHECK(sim.GetWearable(id)->owner == WearableOwner::Chest && sim.GetWearable(id)->chestId == chestId);
    CHECK(sim.GetWearable(tunic)->owner == WearableOwner::Equipped);
    CHECK(sim.GetState().equipment[static_cast<int>(EquipmentSlot::Torso)] == tunic
        && sim.GetState().equipment[static_cast<int>(EquipmentSlot::Legs)] == tunic);
    // It all survives save and reload.
    Simulation reloaded;
    reloaded.SetPlacements(ProvisionalEstatePlacements());
    OK(reloaded.Deserialize(sim.Serialize()));
    CHECK(reloaded.Serialize() == sim.Serialize());
}

void IronToolUpgrades()
{
    Store store = OpenStore();
    auto& sim = store.sim;
    CHECK(ToolUpgrade::IronPrice == 2000);
    CHECK(ToolUpgrade::Name(ToolKind::Axe) == "Iron axe" && ToolUpgrade::Name(ToolKind::Pail) == "Iron-bound pail");
    for (int tool = 0; tool < ToolKindCount; ++tool)
        CHECK(ToolUpgrade::Offered(sim.GetState(), ShopKind::GeneralStore, static_cast<ToolKind>(tool)));
    CHECK(!ToolUpgrade::Offered(sim.GetState(), ShopKind::Count, ToolKind::Axe));
    // The tool has to be crafted first.
    const std::uint64_t revision = sim.GetRevision();
    const auto unowned = sim.BuyToolUpgrade(store.shop, ToolKind::Axe, store.customer);
    CHECK(!unowned.ok && unowned.message == "Craft an axe first." && sim.GetRevision() == revision);
    for (int tool = 0; tool < ToolKindCount; ++tool)
        OK(sim.GrantItems(ToolItem(static_cast<ToolKind>(tool)), 1));
    // 1,000 coins isn't enough; nothing changes.
    const std::uint64_t poorRevision = sim.GetRevision();
    const auto poor = sim.BuyToolUpgrade(store.shop, ToolKind::Axe, store.customer);
    CHECK(!poor.ok && poor.message == "That costs 2,000 coins; you have 1,000 coins." && sim.GetRevision() == poorRevision);
    CHECK(sim.GetToolTier(ToolKind::Axe) == ToolTier::Worn && sim.GetState().money == 1000);
    // Too far from the counter, or a bad tool, is refused too.
    OK(sim.GrantMoney(5000));
    CHECK(!sim.BuyToolUpgrade(store.shop, ToolKind::Axe, {store.counter.x + 5000.0, store.counter.y}).ok);
    CHECK(!sim.BuyToolUpgrade(store.shop, ToolKind::Count, store.customer).ok);
    CHECK(sim.GetState().money == 6000);
    // Each purchase upgrades one tool by one tier and costs 2,000.
    OK(sim.BuyToolUpgrade(store.shop, ToolKind::Axe, store.customer));
    CHECK(sim.GetToolTier(ToolKind::Axe) == ToolTier::Iron && sim.GetToolTier(ToolKind::Hoe) == ToolTier::Worn);
    CHECK(sim.GetState().money == 4000);
    CHECK(!ToolUpgrade::Offered(sim.GetState(), ShopKind::GeneralStore, ToolKind::Axe));
    const auto again = sim.BuyToolUpgrade(store.shop, ToolKind::Axe, store.customer);
    CHECK(!again.ok && sim.GetState().money == 4000);
    OK(sim.BuyToolUpgrade(store.shop, ToolKind::Hoe, store.customer));
    CHECK(sim.GetToolTier(ToolKind::Hoe) == ToolTier::Iron && sim.GetState().money == 2000);
    // Saved and loaded: the tiers are part of the save already.
    const std::string saved = sim.Serialize();
    Simulation loaded;
    loaded.SetPlacements(ProvisionalEstatePlacements());
    OK(loaded.Deserialize(saved));
    CHECK(loaded.GetToolTier(ToolKind::Axe) == ToolTier::Iron && loaded.GetToolTier(ToolKind::Pail) ==     ToolTier::Worn);
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

void WholeCoinPurseGuards()
{
    // Whole-number currency: the stored unit is one coin, so raw values and saves are unchanged. A
    // playtest grant can't push the purse past its cap or below zero from any int64 amount, including
    // the extremes that used to overflow the check, and a refused grant changes nothing.
    Simulation sim;
    OK(sim.PlaceShop(ShopKind::GeneralStore, {100.0, 200.0}));
    OK(sim.GrantMoney(0));
    OK(sim.GrantMoney(1));
    CHECK(sim.GetState().money == 1);
    for (Coins wild : {std::numeric_limits<Coins>::max(), std::numeric_limits<Coins>::min(), MaxMoney, Coins{-2}})
    {
        const std::string before = sim.Serialize();
        const auto revision = sim.GetRevision();
        CHECK(!sim.GrantMoney(wild).ok);
        CHECK(sim.Serialize() == before && sim.GetRevision() == revision && sim.GetState().money == 1);
    }
    OK(sim.GrantMoney(MaxMoney - 1));
    CHECK(sim.GetState().money == MaxMoney);
    CHECK(!sim.GrantMoney(1).ok && sim.GetState().money == MaxMoney);
    OK(sim.GrantMoney(-MaxMoney));
    CHECK(sim.GetState().money == 0);
    // Save bytes: the purse is still one raw integer on the economy line, 2234 stays 2234.
    OK(sim.GrantMoney(2234));
    const std::string saved = sim.Serialize();
    CHECK(saved.find("\neconomy 2234 ") != std::string::npos);
    Simulation loaded;
    OK(loaded.Deserialize(saved));
    CHECK(loaded.GetState().money == 2234 && loaded.Serialize() == saved);
    // The reference prices keep their raw values: a pasty 80 base / 100 in the shop, bread 40 / 50.
    CHECK(SellPrice(Item::Pasty) == 80 && BuyPrice(Item::Pasty) == 100);
    CHECK(SellPrice(Item::Bread) == 40 && BuyPrice(Item::Bread) == 50);
    CHECK(StartingMoney == 1000);
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
    // Hunger never fails her on the estate, so an empty belly doesn't stop the wait (tried on a copy).
    Edit(sim, 5.0, 100.0);
    Simulation hungry = sim;
    OK(hungry.WaitForShop(store.shop, door));
    // Exhaustion does not strand her outside the estate shop or force a doze.
    Edit(sim, 100.0, 3.0);
    const int dozes = sim.DozeCount();
    const double tiredHour = sim.GetState().hour;
    OK(sim.WaitForShop(store.shop, door));
    CHECK(!sim.GetState().failed && sim.GetState().energy == 0.0
        && sim.DozeCount() == dozes && std::abs(sim.GetState().hour - tiredHour - 13.0) < 0.01);
    // Fed: she waits the night through, across midnight and the 6 AM rollover, and it's open.
    sim.SkipToHourOfDay(19.0);
    Edit(sim, 100.0, 100.0);
    const double before = sim.GetState().hour;
    OK(sim.WaitForShop(store.shop, door));
    const double after = sim.GetState().hour;
    CHECK(std::abs((after - before) - 13.0) < 0.01);
    CHECK(IsShopOpen(*sim.FindShop(store.shop), after));
    CHECK(sim.GetState().hunger == 100.0); // No hunger on the estate.
    OK(sim.CheckShopAccess(store.shop, store.customer));
}

// Every shop is closed all day Sunday (Jenny, 2026-09-30): at every hour, with the reopening day in each refusal,
// the door sign and the walk-to-town warning. Sunday waiting reaches Monday opening.
void ShopsCloseOnSundays()
{
    Store store = OpenStore();
    Simulation& sim = store.sim;
    const Shop& shop = *sim.FindShop(store.shop);
    const Point door{store.counter.x, store.counter.y - 700.0};
    // Day 0 (Monday, Spring 1, 1851) at 9 AM; the calendar day runs 06:00 to 06:00.
    const double monday = 0.0, saturday = 5 * 24.0, sunday = 6 * 24.0, nextMonday = 7 * 24.0;
    CHECK(Calendar::DateAt(sunday + 6.0).weekday == Weekday::Sunday && Calendar::DateAt(saturday + 6.0).weekday == Weekday::Saturday);
    CHECK(Calendar::DateAt(nextMonday + 6.0).weekday == Weekday::Monday);
    for (double hour = 6.0; hour < 30.0; hour += 0.25)
        CHECK(!IsShopOpen(shop, sunday + hour) && !IsShopDay(sunday + hour));
    // Saturday keeps its hours to the minute; Monday opens on the stroke of 8.
    CHECK(IsShopOpen(shop, saturday + 8.0) && IsShopOpen(shop, saturday + 17.0 + 59.0 / 60.0) && !IsShopOpen(shop, saturday + 18.0));
    CHECK(!IsShopOpen(shop, nextMonday + 7.99) && IsShopOpen(shop, nextMonday + 8.0) && IsShopOpen(shop, monday + 8.0));
    for (const double weekday : {0.0, 1.0, 2.0, 3.0, 4.0, 5.0})
        CHECK(IsShopOpen(shop, weekday * 24.0 + 12.0));
    // When it next opens, past Sunday.
    CHECK(NextShopOpening(shop, sunday + 12.0) == nextMonday + 8.0 && NextShopOpening(shop, saturday + 19.0) == nextMonday + 8.0);
    CHECK(NextShopOpening(shop, nextMonday + 3.0) == nextMonday + 8.0 && NextShopOpening(shop, monday + 19.0) == 24.0 + 8.0);
    CHECK(std::abs(HoursUntilOpen(shop, saturday + 19.0) - 37.0) < 1e-9);
    // The words: the closed day, a Saturday evening, an ordinary night.
    CHECK(ClosedMessage(shop, sunday + 12.0) == "Closed today (Sunday) \xE2\x80\x93 opens Monday at 8 AM");
    CHECK(ClosedMessage(shop, nextMonday + 3.0) == "Closed today (Sunday) \xE2\x80\x93 opens Monday at 8 AM");  // still Sunday's day
    CHECK(ClosedMessage(shop, saturday + 19.0) == "Closed \xE2\x80\x93 opens Monday at 8 AM");
    CHECK(ClosedMessage(shop, monday + 19.0) == "Closed \xE2\x80\x93 opens at 8 AM" && ClosedMessage(shop, saturday + 7.0) == "Closed \xE2\x80\x93 opens at 8 AM");
    CHECK(ClosedSignText(shop, sunday + 12.0) == "CLOSED\non Sundays");
    CHECK(ClosedSignText(shop, saturday + 19.0) == "CLOSED\nopens Mon 8 AM");
    CHECK(ClosedSignText(shop, monday + 19.0) == "CLOSED\nopens at 8 AM");
    // Sunday waiting is allowed; Saturday evening is still outside the selected wait policy.
    CHECK(CanWaitForShop(shop, monday + 19.0) && CanWaitForShop(shop, saturday + 7.0) && CanWaitForShop(shop, nextMonday + 3.0));
    CHECK(CanWaitForShop(shop, 4 * 24.0 + 18.0));  // Friday at closing: the full 14 h night
    CHECK(CanWaitForShop(shop, sunday + 19.0));    // Sunday evening: just the night to Monday's opening
    CHECK(CanWaitForShop(shop, sunday + 7.0) && CanWaitForShop(shop, sunday + 12.0) && !CanWaitForShop(shop, saturday + 19.0));
    CHECK(!CanWaitForShop(shop, saturday + 12.0));  // open: nothing to wait for

    // Sunday at noon: no trade yet, but waiting opens the shop on Monday.
    for (int day = 0; day < 6; ++day) sim.SkipToHourOfDay(9.0);
    sim.SkipToHourOfDay(12.0);
    CHECK(sim.Today().weekday == Weekday::Sunday);
    Edit(sim, 100.0, 100.0);
    const std::string sundayNoon = sim.Serialize();
    const auto access = sim.CheckShopAccess(store.shop, store.customer);
    CHECK(!access.ok && access.code == ResultCode::Unavailable && access.message == "Closed today (Sunday) \xE2\x80\x93 opens Monday at 8 AM");
    CHECK(!sim.Sell(store.shop, Item::Stone, 1, store.customer).ok && !sim.Buy(store.shop, Item::Pasty, 1, false, store.customer).ok);
    Simulation waiter = sim;
    const auto wait = waiter.WaitForShop(store.shop, door);
    CHECK(wait.ok && waiter.Today().weekday == Weekday::Monday
        && std::abs(waiter.GetState().hour - (nextMonday + 8.0)) < 1e-5);
    OK(waiter.CheckShopAccess(store.shop, store.customer));
    CHECK(sim.Serialize() == sundayNoon);
    // Old saves (no weekday anywhere in them) load and keep the rule: it comes from the clock alone.
    Simulation loaded;
    loaded.SetPlacements(ProvisionalEstatePlacements());
    OK(loaded.Deserialize(sundayNoon));
    CHECK(loaded.Serialize() == sundayNoon && !loaded.CheckShopAccess(store.shop, store.customer).ok);
    // Monday 8 AM: open again.
    sim.SkipToHourOfDay(8.0);
    CHECK(sim.Today().weekday == Weekday::Monday);
    OK(sim.CheckShopAccess(store.shop, store.customer));

    // The walk to town warns when she'd arrive on a Sunday (about 7 game hours on the road).
    const PublicRoadStop* manor = EstatePublicRoad().FindStop("Manor");
    CHECK(manor != nullptr);
    // Forward to `hourOfDay` on the next calendar day that's `day` (02:00 belongs to the day before).
    const auto Seek = [](Simulation& at, Weekday day, double hourOfDay)
    {
        for (int step = 0; step < 9; ++step)
        {
            at.SkipToHourOfDay(hourOfDay);
            if (Calendar::DateAt(at.GetState().hour).weekday == day) return;
        }
        CHECK(false);
    };
    Simulation walker = sim;
    const double walkHours = PlanTravel(walker.GetState(), manor->position, TravelDestination::Town).gameHours;  // the short walk to the village, about 1.3 game hours
    Seek(walker, Weekday::Sunday, 9.0);  // Sunday 9 AM: she'd arrive about 10:20 AM, in store hours on its closed day
    const TravelPlan late = PlanTravel(walker.GetState(), manor->position, TravelDestination::Town);
    CHECK(late.ok && late.storeClosedOnArrival && late.storeClosedAllDay);
    CHECK(late.summary.find("You'd arrive on a Sunday, when the general store is closed all day (it opens Monday at 8 AM).")
        != std::string::npos);
    walker.SkipToHourOfDay(21.0);  // Sunday 9 PM: she'd arrive about 10:20 PM, after its hours: shut for the night
    const TravelPlan sundayEvening = PlanTravel(walker.GetState(), manor->position, TravelDestination::Town);
    CHECK(sundayEvening.storeClosedOnArrival && !sundayEvening.storeClosedAllDay && !sundayEvening.nextDay
        && sundayEvening.summary.find("(it opens at 8 AM)") != std::string::npos);
    walker.SkipToHourOfDay(24.0 - 0.5 * walkHours);  // Sunday just before midnight: she'd arrive just after it, the next day by the clock; no "on a Sunday"
    const TravelPlan overnight = PlanTravel(walker.GetState(), manor->position, TravelDestination::Town);
    CHECK(overnight.nextDay && overnight.storeClosedOnArrival && !overnight.storeClosedAllDay);
    CHECK(overnight.summary.find("Sunday") == std::string::npos
        && overnight.summary.find(" the next day.") != std::string::npos
        && overnight.summary.find("(it opens at 8 AM)") != std::string::npos);
    walker.SkipToHourOfDay(7.5);   // Monday morning: open when she gets there
    const TravelPlan monday9 = PlanTravel(walker.GetState(), manor->position, TravelDestination::Town);
    CHECK(monday9.ok && !monday9.storeClosedOnArrival && !monday9.storeClosedAllDay);
    Simulation saturdayNoon = sim;
    Seek(saturdayNoon, Weekday::Saturday, 22.0);  // Saturday 10 PM: she'd arrive after closing, and it opens Monday
    // Early on Sunday, before what would be opening time (review): it opens Monday, not "at 8 AM".
    {
        Simulation early = saturdayNoon;
        const double walk = PlanTravel(early.GetState(), manor->position, TravelDestination::Town).gameHours;
        early.SkipToHourOfDay(7.5 - walk);  // she'd arrive about 07:30 on Sunday
        const TravelPlan dawn = PlanTravel(early.GetState(), manor->position, TravelDestination::Town);
        CHECK(Calendar::DateAt(dawn.arrivalHour).weekday == Weekday::Sunday
            && std::abs(std::fmod(dawn.arrivalHour, 24.0) - 7.5) < 1e-6);
        CHECK(dawn.storeClosedOnArrival && !dawn.storeClosedAllDay
            && dawn.summary.find("(it opens Monday at 8 AM)") != std::string::npos);
    }
    const TravelPlan evening = PlanTravel(saturdayNoon.GetState(), manor->position, TravelDestination::Town);
    CHECK(evening.storeClosedOnArrival && !evening.storeClosedAllDay
        && evening.summary.find("(it opens Monday at 8 AM)") != std::string::npos);
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

void RoadSignsOfferTheWalk()
{
    const PublicRoad& road = EstatePublicRoad();
    CHECK(road.signs.size() == 3);
    CHECK(RoadSignDestinations("ManorRoadSign") == std::vector<TravelDestination>{TravelDestination::Town});
    CHECK(RoadSignDestinations("TownRoadSign") == std::vector<TravelDestination>{TravelDestination::Manor});
    CHECK(RoadSignDestinations("GatewayRoadSign").size() == 2 && RoadSignDestinations("Milestone").empty());
    CHECK(RoadSignLabel("ManorRoadSign") == "To town" && RoadSignLabel("TownRoadSign") == "To the manor"
        && RoadSignLabel("GatewayRoadSign") == "Town / Manor");
    Store store = OpenStore();
    Simulation& sim = store.sim;
    sim.SkipToHourOfDay(9.0);
    for (const PublicRoadSign& sign : road.signs)
    {
        // She reads the sign she stands beside, and only within reach.
        CHECK(RoadSignNear(sign.position) == &sign);
        CHECK(RoadSignNear({sign.position.x + RoadSignReachCm + 50.0, sign.position.y}) == nullptr);
        // Every walk it offers plans from the verge where it stands.
        for (const TravelDestination destination : RoadSignDestinations(sign.name))
        {
            const TravelPlan plan = PlanTravel(sim.GetState(), sign.position, destination);
            CHECK(plan.ok && plan.gameHours > 0.0 && plan.connectorMetres < 30.0);
        }
    }
    // The relocated sign does not move the walk-home arrival, which still lands beside the manor front door.
    const PublicRoadSign* manorSign = road.FindSign("ManorRoadSign");
    const EstateLayout& layout = ProvisionalEstateLayout();
    const Point frontDoor = EstateManorFrontDoor(layout);
    CHECK(manorSign && std::hypot(manorSign->position.x - frontDoor.x, manorSign->position.y - frontDoor.y) > 5000.0);
    const PublicRoadStop* manorStop = road.FindStop("Manor");
    CHECK(manorStop && manorStop->hasArrival
        && std::hypot(manorStop->arrival.x - frontDoor.x, manorStop->arrival.y - frontDoor.y) < 1000.0);
    CHECK(manorStop->arrival.x == -26340.0 && manorStop->arrival.y == -64920.0);
    CHECK(PlanTravel(sim.GetState(), manorSign->position, TravelDestination::Manor).ok);
    for (const EstatePlacement& placement : ProvisionalEstatePlacements().placements)
    {
        CHECK(std::hypot(placement.position.x - manorStop->arrival.x, placement.position.y - manorStop->arrival.y) > 200.0);
        CHECK(std::hypot(placement.position.x - manorSign->position.x, placement.position.y - manorSign->position.y) > 200.0);
    }
    // The walk from the manor's sign is the Map tab's walk: one transaction, time passes, she's in town.
    const double before = sim.GetState().hour;
    const TravelPlan plan = PlanTravel(sim.GetState(), manorSign->position, TravelDestination::Town);
    OK(sim.WalkRoad(TravelDestination::Town, manorSign->position));
    CHECK(std::abs(sim.GetState().hour - (before + plan.gameHours)) < 1e-6);
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
    // Noon at the manor, default 60-minute day: the short road to the village at the conservative pace, about an hour and a quarter of game time (about 3 real minutes).
    sim.SkipToHourOfDay(12.0);
    const TravelPlan noon = PlanTravel(sim.GetState(), manor->position, TravelDestination::Town);
    CHECK(noon.ok && noon.connectorMetres < 1.0);
    CHECK(std::abs(noon.roadMetres - (town->chainage - manor->chainage)) < 1.0);
    const double expected = noon.totalMetres * 100.0 / RoadWalkPaceCmPerSecond / 3600.0 * 24.0 * 60.0 / sim.GetState().dayMinutes;
    CHECK(std::abs(noon.gameHours - expected) < 1e-9 && noon.gameHours > 0.8 && noon.gameHours < 1.6);
    CHECK(town->hasArrival && noon.arrival.x == town->arrival.x && noon.arrival.y == town->arrival.y && noon.arrivalZ == town->arrivalZ);
    CHECK(!noon.storeClosedOnArrival && noon.summary.find("closed") == std::string::npos && !noon.nextDay);
    CHECK(noon.summary.find(" m") != std::string::npos);
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
    sim.SkipToHourOfDay(7.5);
    const TravelPlan morning = PlanTravel(sim.GetState(), manor->position, TravelDestination::Town);
    CHECK(morning.ok && !morning.storeClosedOnArrival && morning.summary.find("closed") == std::string::npos);
    // Late: past midnight on the way.
    sim.SkipToHourOfDay(23.7);
    CHECK(PlanTravel(sim.GetState(), manor->position, TravelDestination::Town).nextDay);
    // Partway along (150 m on), only the rest of the road counts; home to the manor never warns about the store.
    const Point gateway = road.At(manor->chainage + 150.0);
    const TravelPlan half = PlanTravel(sim.GetState(), gateway, TravelDestination::Town);
    CHECK(half.ok && half.gameHours < noon.gameHours * 0.7);
    const TravelPlan home = PlanTravel(sim.GetState(), town->position, TravelDestination::Manor);
    // Home to the manor lands her by the ruin's front door (Jenny, 2026-10-01), the way there walked too.
    CHECK(home.ok && !home.storeClosedOnArrival && manor->hasArrival && home.arrival.x == manor->arrival.x
        && home.arrival.y == manor->arrival.y && home.arrivalZ == manor->arrivalZ);
    CHECK(home.totalMetres > road.WalkMetres(town->chainage, manor->chainage) + 30.0);
    // Off the road: the straight walk back to it counts too, and far off she has to find it herself.
    const TravelPlan field = PlanTravel(sim.GetState(), {gateway.x + 25000.0, gateway.y}, TravelDestination::Town);
    CHECK(field.ok && field.connectorMetres > 150.0 && field.totalMetres > half.totalMetres);
    CHECK(!PlanTravel(sim.GetState(), {gateway.x + 90000.0, gateway.y}, TravelDestination::Town).ok);
    CHECK(!PlanTravel(sim.GetState(), town->position, TravelDestination::Town).ok);
    Simulation woodland;
    CHECK(!PlanTravel(woodland.GetState(), manor->position, TravelDestination::Town).ok);
    // Hunger and exhaustion never strand her on the estate road.
    sim.SkipToHourOfDay(12.0);
    Edit(sim, 5.0, 100.0);
    Simulation hungry = sim;
    OK(hungry.WalkRoad(TravelDestination::Town, manor->position));
    Edit(sim, 100.0, 3.0);
    const double tiredHour = sim.GetState().hour;
    const int dozes = sim.DozeCount();
    const TravelPlan tiredWalk = PlanTravel(sim.GetState(), manor->position, TravelDestination::Town);
    OK(sim.WalkRoad(TravelDestination::Town, manor->position));
    CHECK(!sim.GetState().failed && sim.DozeCount() == dozes
        && std::abs(sim.GetState().hour - tiredHour - tiredWalk.gameHours) < 1e-6);
    const std::string there = sim.Serialize();
    CHECK(!sim.WalkRoad(TravelDestination::Town, town->position).ok && sim.Serialize() == there);
    // Rested: the clock runs for the whole walk, and she's a little more tired (the estate has no hunger).
    Edit(sim, 100.0, 100.0);
    const double before = sim.GetState().hour;
    const std::uint64_t revision = sim.GetRevision();
    const TravelPlan plan = PlanTravel(sim.GetState(), manor->position, TravelDestination::Town);
    OK(sim.WalkRoad(TravelDestination::Town, manor->position));
    CHECK(std::abs(sim.GetState().hour - before - plan.gameHours) < 1e-6);
    CHECK(sim.GetRevision() > revision && sim.GetState().hunger == 100.0 && sim.GetState().energy < 100.0);
    CHECK(sim.DozeCount() == 0);
}

// Shops and the pack show the canonical nominal Energy one food restores, from its catalogue row.
void FoodEnergyLabels()
{
    CHECK(FoodEnergyLabel(Item::Pasty) == "+40 Energy");
    CHECK(FoodEnergyLabel(Item::Bread) == "+12 Energy");
    CHECK(FoodEnergyLabel(Item::Cheese) == "+15 Energy");
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

// Filling the pail standing in the water (Jenny, 2026-09-30): the pail goes in ahead of her when that's water,
// otherwise toward deeper water, never back onto the bank, from anywhere in the water. A round pond 10 m across.
void PailDipsWhereSheStands()
{
    const auto edge = [](Point p) { return std::hypot(p.x, p.y) - 1000.0; };   // negative inside
    constexpr double forward = 59.0, right = 12.0, inside = 25.0;
    // The reach itself: 59 cm ahead, 12 cm to her right, turned by her heading (as UE's FRotator turns it).
    const Point east = InWaterDipPoint(Point{0.0, 0.0}, 0.0, forward, right, inside, edge);
    CHECK(std::abs(east.x - 59.0) < 1e-9 && std::abs(east.y - 12.0) < 1e-9);
    const Point north = InWaterDipPoint(Point{0.0, 0.0}, 90.0, forward, right, inside, edge);
    CHECK(std::abs(north.x + 12.0) < 1e-9 && std::abs(north.y - 59.0) < 1e-9);
    // From 1 cm to 2 m in, facing every way: the pail always lands in the water, as deep as where she stands or
    // deeper, and straight ahead whenever ahead is well in (review: 0-30 cm in used to turn her back to the bank).
    for (const double in : {1.0, 10.0, 24.0, 30.0, 60.0, 200.0})
        for (int facing = 0; facing < 16; ++facing)
        {
            const Point her{1000.0 - in, 0.0};
            const double yaw = facing * 22.5;
            const Point dip = InWaterDipPoint(her, yaw, forward, right, inside, edge);
            CHECK(edge(dip) < 0.0);
            CHECK(edge(dip) <= edge(her) + 1e-9 || edge(dip) <= -inside);
            const double r = yaw * 3.14159265358979323846 / 180.0;
            const Point ahead{her.x + forward * std::cos(r) - right * std::sin(r), her.y + forward * std::sin(r) + right * std::cos(r)};
            if (edge(ahead) <= -inside) CHECK(std::abs(dip.x - ahead.x) < 1e-9 && std::abs(dip.y - ahead.y) < 1e-9);
        }
    // Facing the bank from 10 cm in: the pail goes in behind her, toward open water.
    const Point turned = InWaterDipPoint(Point{990.0, 0.0}, 0.0, forward, right, inside, edge);
    CHECK(turned.x < 990.0 && edge(turned) < -40.0);
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
    for (int water : {0, 1, 6, PailCapacity})
    {
        const auto pail = With(1, water);
        CHECK(pail.gauge && pail.charge == water && pail.hidePackWater);
    }
    // More than one pail holds (an older save), or 1200: the gauge is full and the tile shows every portion.
    for (int water : {PailCapacity + 1, 1200})
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
    OK(sim.GrantItems(Item::Water, PailCapacity + 3));
    Simulation reloaded;
    reloaded.SetPlacements(ProvisionalEstatePlacements());
    OK(reloaded.Deserialize(sim.Serialize()));
    CHECK(reloaded.Count(Item::Water) == sim.Count(Item::Water) && reloaded.Count(Item::Water) >= PailCapacity + 3);
    const auto loaded = PresentPail(reloaded.GetState());
    CHECK(loaded.gauge && loaded.charge == PailCapacity && !loaded.hidePackWater);
    // The pack's footer names the charge (a controller has no hover for the tooltip).
    CHECK(PailChargeLabel(With(1, 5)) == "Water 5 / 15" && PailChargeLabel(With(1, 0)) == "Water 0 / 15");
    CHECK(PailChargeLabel(With(1, 1200)) == "Water 15 / 15" && PailChargeLabel(With(0, 4)).empty());
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

// Ripen a planted fixture through the ordinary save reader, without spending several seasons growing it.
void RipenSaleCrop(Simulation& sim)
{
    CHECK(sim.GetState().plots.size() == 1);
    const Plot& plot = sim.GetState().plots.front();
    const auto line = [&](double growth)
    {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::setprecision(std::numeric_limits<double>::max_digits10);
        out << '\n' << plot.id << ' ' << plot.cellX << ' ' << plot.cellY << ' ' << plot.planted << ' '
            << growth << ' ' << plot.moisture << ' ' << plot.weeds << ' ' << static_cast<int>(plot.kind) << '\n';
        return out.str();
    };
    const std::string saved = sim.Serialize(), old = line(plot.growth);
    std::string body = saved.substr(saved.find('\n') + 1);
    const auto at = body.find(old);
    CHECK(at != std::string::npos);
    body.replace(at, old.size(), line(1.0));
    OK(sim.Deserialize(Reseal(saved, body)));
    CHECK(IsRipe(sim.GetState().plots.front()));
}

void EveryHarvestAppearsInTheSellList()
{
    const Coins prices[] = {4, 6, 20, 16, 14, 90, 8, 13};
    static_assert(sizeof(prices) / sizeof(prices[0]) == static_cast<int>(CropKind::Count));
    for (int index = 0; index < static_cast<int>(CropKind::Count); ++index)
    {
        Store store = OpenStore(true);
        Simulation& sim = store.sim;
        const CropKind kind = static_cast<CropKind>(index);
        const CropInfo& crop = GetCropInfo(kind);
        for (int day = 0; !GrowsIn(kind, sim.Today().season) && day < Calendar::DaysPerYear; ++day)
            sim.SkipToHourOfDay(9.0);
        CHECK(GrowsIn(kind, sim.Today().season));
        OK(sim.GrantItems(Item::DiggingStick, 1));
        OK(sim.GrantItems(crop.seed, 1));
        const Point garden = GardenCellCenter(-190, -675);
        OK(sim.Till(-190, -675, garden));
        const int plotId = sim.GetState().plots.front().id;
        OK(sim.Plant(plotId, garden, kind));
        RipenSaleCrop(sim);
        const int before = sim.Count(crop.produce);
        OK(sim.HarvestCrop(plotId, garden));
        CHECK(sim.Count(crop.produce) == before + crop.produceCount);
        CHECK(SellPrice(crop.produce) == prices[index]);
        const auto listed = ShopSellableItems(sim, ShopKind::GeneralStore);
        CHECK(std::count(listed.begin(), listed.end(), crop.produce) == 1);
        CHECK(std::find(listed.begin(), listed.end(), Item::DiggingStick) == listed.end());

        sim.SkipToHourOfDay(9.0);
        if (!IsShopDay(sim.GetState().hour)) sim.SkipToHourOfDay(9.0);
        const Coins purse = sim.GetState().money;
        const int quantity = sim.Count(crop.produce);
        const auto revision = sim.GetRevision();
        OK(sim.Sell(store.shop, crop.produce, 1, store.customer));
        CHECK(sim.GetRevision() > revision && sim.GetState().money == purse + prices[index]);
        CHECK(sim.Count(crop.produce) == quantity - 1 && PackStock(sim, crop.produce) == quantity - 1);
        CHECK(sim.FindShop(store.shop)->heroineStock[static_cast<int>(crop.produce)] == 1);
        const std::string partial = sim.Serialize();
        Simulation loaded;
        EstatePlacements empty;
        empty.bakeVersion = ProvisionalEstatePlacements().bakeVersion;
        loaded.SetPlacements(empty);
        OK(loaded.Deserialize(partial));
        CHECK(loaded.Serialize() == partial);
        CHECK(loaded.GetState().money == purse + prices[index] && loaded.Count(crop.produce) == quantity - 1);
        const auto refused = sim.Sell(store.shop, crop.produce, quantity, store.customer);
        CHECK(!refused.ok && sim.Serialize() == partial);
        if (quantity > 1) OK(sim.Sell(store.shop, crop.produce, quantity - 1, store.customer));
        CHECK(sim.GetState().money == purse + quantity * prices[index]);
        CHECK(sim.Count(crop.produce) == 0 && PackStock(sim, crop.produce) == 0);
        const auto depleted = ShopSellableItems(sim, ShopKind::GeneralStore);
        CHECK(std::find(depleted.begin(), depleted.end(), crop.produce) == depleted.end());
        const std::string sold = sim.Serialize();
        CHECK(!sim.Sell(store.shop, crop.produce, 1, store.customer).ok && sim.Serialize() == sold);
        OK(loaded.Deserialize(sold));
        CHECK(loaded.Serialize() == sold && loaded.FindShop(store.shop)->heroineStock[static_cast<int>(crop.produce)] == quantity);
        OK(loaded.Buy(store.shop, crop.produce, 1, true, store.customer));
        CHECK(loaded.Count(crop.produce) == 1 && loaded.GetState().money == purse + (quantity - 1) * prices[index]);
        CHECK(loaded.FindShop(store.shop)->heroineStock[static_cast<int>(crop.produce)] == quantity - 1);
        loaded.SkipToHourOfDay(19.0);
        const std::string closed = loaded.Serialize();
        CHECK(!loaded.Sell(store.shop, crop.produce, 1, store.customer).ok);
        CHECK(!loaded.Buy(store.shop, crop.produce, 1, true, store.customer).ok && loaded.Serialize() == closed);
    }
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
    Run("every harvested crop appears in the Sell list and trades safely", EveryHarvestAppearsInTheSellList);
    Run("rejected trades change nothing", RejectedTradesChangeNothing);
    Run("buy and eat a pasty", BuyAndEatAPasty);
    Run("buy back and pack capacity", BuyBackAndCapacity);
    Run("the leather backpack upgrade", LeatherBackpackUpgrade);
    Run("clothing at the store, worn by swapping", ClothingAtTheStore);
    Run("iron tool upgrades", IronToolUpgrades);
    Run("her goods sell down each morning", StockSellsDownEachMorning);
    Run("money and shops survive save and reload", EconomySurvivesSaveAndReload);
    Run("no walk to town from town", NoWalkToTownFromTown);
    Run("playtest shop placement", PlaytestShopPlacement);
    Run("whole-coin purse guards and unchanged save bytes", WholeCoinPurseGuards);
    Run("wait for the store to open", WaitForTheStoreToOpen);
    Run("shops close on Sundays", ShopsCloseOnSundays);
    Run("walk the road to town and back", WalkTheRoad);
    Run("the road signs offer the same walk", RoadSignsOfferTheWalk);
    Run("pickup lines count only new things", PickupGainsCountOnlyNewThings);
    Run("pickup lines follow her actual stacks", PickupGainsFollowActualStacks);
    Run("pail water shows on the pail", PailWaterPresentation);
    Run("the pail dips where she stands in the water", PailDipsWhereSheStands);
    Run("food shows its Energy", FoodEnergyLabels);
    std::cout << cases << " scenarios, " << checks << " explicit checks passed.\n";
    return 0;
}
