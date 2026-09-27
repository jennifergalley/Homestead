#include "HomesteadShops.h"
#include "HomesteadEstate.h"
#include "HomesteadSimulation.h"

#include <algorithm>
#include <cmath>
#include <istream>
#include <ostream>
#include <set>

namespace Homestead
{
namespace
{
Result ShopGood(const std::string& text, std::uint64_t revision) { return {true, text, ResultCode::None, revision}; }
Result ShopBad(const std::string& text, std::uint64_t revision, ResultCode code = ResultCode::Invalid)
{
    return {false, text, code, revision};
}
bool ValidShopItem(Item item) { return static_cast<int>(item) >= 0 && static_cast<int>(item) < ItemCount; }
Cents RoundedMarkup(Cents base) { return (base * ShopMarkupPercent + 50) / 100; }
bool NearCounter(const Shop& shop, Point player)
{
    if (!std::isfinite(player.x) || !std::isfinite(player.y)) return false;
    const double dx = player.x - shop.counterX, dy = player.y - shop.counterY;
    return dx * dx + dy * dy <= CounterReach * CounterReach;
}
std::string Plural(int quantity, Item item)
{
    return std::to_string(quantity) + " " + ItemName(item);
}
}

std::string FormatMoney(Cents cents)
{
    const bool negative = cents < 0;
    // Magnitude as unsigned so the most negative value still formats.
    const std::uint64_t magnitude = negative ? static_cast<std::uint64_t>(-(cents + 1)) + 1 : static_cast<std::uint64_t>(cents);
    std::string dollars = std::to_string(magnitude / 100);
    for (int i = static_cast<int>(dollars.size()) - 3; i > 0; i -= 3) dollars.insert(static_cast<std::size_t>(i), ",");
    const unsigned rest = static_cast<unsigned>(magnitude % 100);
    std::string text = std::string(negative ? "-$" : "$") + dollars + "." + static_cast<char>('0' + rest / 10)
        + static_cast<char>('0' + rest % 10);
    return text;
}

std::string FormatMoneyDelta(Cents cents)
{
    return cents < 0 ? FormatMoney(cents) : "+" + FormatMoney(cents);
}

const std::vector<Item>& ShopGoods(ShopKind kind)
{
    static const std::vector<Item> generalStore = {Item::Pasty, Item::Bread, Item::Cheese, Item::Twine};
    static const std::vector<Item> none;
    return kind == ShopKind::GeneralStore ? generalStore : none;
}

const char* ShopDisplayName(ShopKind kind)
{
    return kind == ShopKind::GeneralStore ? "General store" : "Shop";
}

bool IsShopOpen(const Shop& shop, double hour)
{
    if (!std::isfinite(hour)) return false;
    const double time = std::fmod(std::fmod(hour, 24.0) + 24.0, 24.0);
    return time >= shop.openHour && time < shop.closeHour;
}

std::string FormatHour(double hour)
{
    const int whole = static_cast<int>(std::floor(std::fmod(std::fmod(hour, 24.0) + 24.0, 24.0)));
    const int minutes = static_cast<int>(std::lround((hour - std::floor(hour)) * 60.0)) % 60;
    const int twelve = whole % 12 == 0 ? 12 : whole % 12;
    std::string text = std::to_string(twelve);
    if (minutes) text += (minutes < 10 ? ":0" : ":") + std::to_string(minutes);
    return text + (whole < 12 ? " AM" : " PM");
}

std::string ClosedMessage(const Shop& shop)
{
    return "Closed - opens at " + FormatHour(shop.openHour);
}

Cents SellPrice(Item item) { return BasePrice(item); }
Cents BuyPrice(Item item) { return RoundedMarkup(BasePrice(item)); }
Cents BuyBackPrice(Item item) { return SellPrice(item); }

int SellDownAmount(int quantity)
{
    if (quantity <= 0) return 0;
    const int share = (quantity * SellDownPercent + 99) / 100;
    return std::min(quantity, std::max(1, share));
}

const Shop* Simulation::FindShop(int shopId) const
{
    for (const auto& shop : state_.shops) if (shop.id == shopId) return &shop;
    return nullptr;
}

const Shop* Simulation::FindShop(ShopKind kind) const
{
    for (const auto& shop : state_.shops) if (shop.kind == kind) return &shop;
    return nullptr;
}

Result Simulation::CheckShopAccess(int shopId, Point player) const
{
    if (state_.failed) return ShopBad("You need to recover first.", revision_, ResultCode::Unavailable);
    const Shop* shop = FindShop(shopId);
    if (!shop) return ShopBad("There is no such shop.", revision_);
    if (!IsShopOpen(*shop, state_.hour)) return ShopBad(ClosedMessage(*shop), revision_, ResultCode::Unavailable);
    if (!NearCounter(*shop, player)) return ShopBad("Step up to the counter to trade.", revision_);
    return ShopGood("", revision_);
}

Result Simulation::Sell(int shopId, Item item, int quantity, Point player)
{
    const auto access = CheckShopAccess(shopId, player);
    if (!access) return access;
    const Shop* shop = FindShop(shopId);
    if (!ValidShopItem(item) || quantity <= 0 || quantity > MaxShopStock)
        return ShopBad("Choose something to sell and how many.", revision_);
    if (!ShopBuys(shop->kind, item))
        return ShopBad(std::string(ShopDisplayName(shop->kind)) + " doesn't buy " + ItemName(item) + ".", revision_);
    if (Count(item) < quantity) return ShopBad("You're only carrying " + Plural(Count(item), item) + ".", revision_);
    const Cents earned = SellPrice(item) * quantity;
    if (state_.money + earned > MaxMoney) return ShopBad("Your purse can't hold any more.", revision_, ResultCode::Capacity);
    State candidate = state_;
    Shop* target = nullptr;
    for (auto& value : candidate.shops) if (value.id == shopId) target = &value;
    int& stocked = target->heroineStock[static_cast<int>(item)];
    if (stocked + quantity > MaxShopStock) return ShopBad("The shop has no room for more " + std::string(ItemName(item)) + ".",
        revision_, ResultCode::Capacity);
    candidate.inventory[static_cast<int>(item)] -= quantity;
    candidate.money += earned;
    stocked += quantity;
    const std::string message = "Sold " + Plural(quantity, item) + " for " + FormatMoney(earned) + ".";
    return CommitInventory(std::move(candidate), message.c_str());
}

Result Simulation::Buy(int shopId, Item item, int quantity, bool fromHeroineStock, Point player)
{
    const auto access = CheckShopAccess(shopId, player);
    if (!access) return access;
    const Shop* shop = FindShop(shopId);
    if (!ValidShopItem(item) || quantity <= 0 || quantity > InventoryCapacity)
        return ShopBad("Choose something to buy and how many.", revision_);
    const auto& goods = ShopGoods(shop->kind);
    if (fromHeroineStock)
    {
        if (shop->heroineStock[static_cast<int>(item)] < quantity)
            return ShopBad("The shop only has " + Plural(shop->heroineStock[static_cast<int>(item)], item) + " left.", revision_);
    }
    else if (std::find(goods.begin(), goods.end(), item) == goods.end())
        return ShopBad(std::string(ShopDisplayName(shop->kind)) + " doesn't sell " + ItemName(item) + ".", revision_);
    const Cents cost = (fromHeroineStock ? BuyBackPrice(item) : BuyPrice(item)) * quantity;
    if (cost > state_.money)
        return ShopBad("That costs " + FormatMoney(cost) + "; you have " + FormatMoney(state_.money) + ".", revision_);
    if (UsedCapacity() + quantity > InventoryCapacity || Count(item) + quantity > InventoryCapacity)
        return ShopBad("Not enough pack space for " + Plural(quantity, item) + ".", revision_, ResultCode::Capacity);
    State candidate = state_;
    for (auto& value : candidate.shops)
        if (value.id == shopId && fromHeroineStock) value.heroineStock[static_cast<int>(item)] -= quantity;
    candidate.inventory[static_cast<int>(item)] += quantity;
    candidate.money -= cost;
    const std::string message = "Bought " + Plural(quantity, item) + " for " + FormatMoney(cost) + ".";
    return CommitInventory(std::move(candidate), message.c_str());
}

Result Simulation::GreetShopkeeper(int shopId)
{
    for (auto& shop : state_.shops)
        if (shop.id == shopId)
        {
            ++shop.greetings;
            return ShopGood("", ++revision_);
        }
    return ShopBad("There is no such shop.", revision_);
}

Result Simulation::GrantMoney(Cents cents)
{
    if (cents < -state_.money || state_.money + cents > MaxMoney) return ShopBad("That would leave the purse out of range.", revision_);
    state_.money += cents;
    return ShopGood(FormatMoneyDelta(cents), ++revision_);
}

Result Simulation::PlaceShop(ShopKind kind, Point counter)
{
    if (!std::isfinite(counter.x) || !std::isfinite(counter.y) || std::abs(counter.x) > MaxWorldCoordinate
        || std::abs(counter.y) > MaxWorldCoordinate)
        return ShopBad("The counter is outside the world.", revision_);
    for (auto& shop : state_.shops)
        if (shop.kind == kind)
        {
            shop.counterX = counter.x;
            shop.counterY = counter.y;
            return ShopGood("Shop moved.", ++revision_);
        }
    Shop shop;
    shop.id = state_.nextId++;
    shop.kind = kind;
    shop.name = ShopDisplayName(kind);
    shop.counterX = counter.x;
    shop.counterY = counter.y;
    state_.shops.push_back(shop);
    return ShopGood("Shop opened.", ++revision_);
}

void Simulation::SeedEstateShops(State& candidate, const EstateLayout& layout)
{
    candidate.money = StartingMoney;
    const Point counter = layout.PointOr(Anchor::GeneralStoreCounter, {});
    Shop store;
    store.id = candidate.nextId++;
    store.kind = ShopKind::GeneralStore;
    store.name = ShopDisplayName(ShopKind::GeneralStore);
    store.counterX = counter.x;
    store.counterY = counter.y;
    candidate.shops.push_back(store);
}

void Simulation::RefreshShopCounters(State& candidate, const EstateLayout& layout)
{
    if (!candidate.fixedEstate) return;
    for (auto& shop : candidate.shops)
        if (shop.kind == ShopKind::GeneralStore)
        {
            const Landmark* counter = layout.FindLandmark(Anchor::GeneralStoreCounter);
            if (counter) { shop.counterX = counter->position.x; shop.counterY = counter->position.y; }
        }
}

void Simulation::SellDownShops()
{
    for (auto& shop : state_.shops)
        for (int& quantity : shop.heroineStock) quantity -= SellDownAmount(quantity);
}

void Simulation::WriteEconomy(std::ostream& body) const
{
    body << "economy " << state_.money << ' ' << state_.shops.size() << '\n';
    for (const auto& shop : state_.shops)
    {
        body << shop.id << ' ' << static_cast<int>(shop.kind) << ' ' << shop.counterX << ' ' << shop.counterY << ' '
             << shop.greetings;
        int stocked = 0;
        for (int quantity : shop.heroineStock) if (quantity > 0) ++stocked;
        body << ' ' << stocked;
        for (int i = 0; i < ItemCount; ++i)
            if (shop.heroineStock[i] > 0) body << ' ' << ItemKey(static_cast<Item>(i)) << ' ' << shop.heroineStock[i];
        body << '\n';
    }
}

bool Simulation::ReadEconomy(std::istream& input, State& candidate, std::set<int>& ids)
{
    // Optional: saves from before money load with an empty purse and no shops.
    input >> std::ws;
    if (input.eof()) return true;
    const auto start = input.tellg();
    std::string tag;
    if (!(input >> tag)) return false;
    if (tag != "economy")
    {
        input.clear();
        input.seekg(start);
        return true;
    }
    std::size_t count = 0;
    if (!(input >> candidate.money >> count) || candidate.money < 0 || candidate.money > MaxMoney
        || count > static_cast<std::size_t>(ShopKind::Count)) return false;
    for (std::size_t i = 0; i < count; ++i)
    {
        Shop shop;
        int kind = -1, stocked = 0;
        if (!(input >> shop.id >> kind >> shop.counterX >> shop.counterY >> shop.greetings >> stocked)
            || kind < 0 || kind >= static_cast<int>(ShopKind::Count) || shop.id <= 0 || shop.id >= candidate.nextId
            || !ids.insert(shop.id).second || shop.greetings < 0 || stocked < 0 || stocked > ItemCount
            || !std::isfinite(shop.counterX) || !std::isfinite(shop.counterY)
            || std::abs(shop.counterX) > MaxWorldCoordinate || std::abs(shop.counterY) > MaxWorldCoordinate)
            return false;
        shop.kind = static_cast<ShopKind>(kind);
        shop.name = ShopDisplayName(shop.kind);
        for (int s = 0; s < stocked; ++s)
        {
            std::string key;
            int quantity = 0;
            if (!(input >> key >> quantity) || quantity <= 0 || quantity > MaxShopStock) return false;
            const Item item = ItemFromKey(key.c_str());
            // Goods whose item no longer exists are quietly dropped.
            if (item == Item::Count) continue;
            if (shop.heroineStock[static_cast<int>(item)] != 0) return false;
            shop.heroineStock[static_cast<int>(item)] = quantity;
        }
        for (const auto& other : candidate.shops) if (other.kind == shop.kind) return false;
        candidate.shops.push_back(shop);
    }
    return true;
}
}
