#include "HomesteadShops.h"
#include "HomesteadBackpack.h"
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
Coins RoundedMarkup(Coins base) { return (base * ShopMarkupPercent + 50) / 100; }
bool NearCounter(const Shop& shop, Point player)
{
    if (!std::isfinite(player.x) || !std::isfinite(player.y)) return false;
    const double dx = player.x - shop.counterX, dy = player.y - shop.counterY;
    return dx * dx + dy * dy <= CounterReach * CounterReach;
}
std::string Plural(int quantity, Item item) { return CountedName(item, quantity); }
std::string ToLowerAscii(std::string text)
{
    for (char& c : text) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return text;
}
}

std::string FormatMoney(Coins amount)
{
    const bool negative = amount < 0;
    // Magnitude as unsigned so the most negative value still formats.
    const std::uint64_t magnitude = negative ? static_cast<std::uint64_t>(-(amount + 1)) + 1 : static_cast<std::uint64_t>(amount);
    std::string digits = std::to_string(magnitude);
    for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3) digits.insert(static_cast<std::size_t>(i), ",");
    return std::string(negative ? "-" : "") + digits + (magnitude == 1 ? " coin" : " coins");
}

std::string FormatMoneyDelta(Coins amount)
{
    return amount < 0 ? FormatMoney(amount) : "+" + FormatMoney(amount);
}

const std::vector<Item>& ShopGoods(ShopKind kind)
{
    static const std::vector<Item> generalStore = {Item::Pasty, Item::Bread, Item::Cheese, Item::Twine, Item::OilFlask,
        Item::TurnipSeed, Item::CarrotSeed, Item::SeedPotato, Item::CabbageSeed, Item::BroadBeanSeed, Item::StrawberryRunner,
        Item::FishingPole};
    static const std::vector<Item> none;
    return kind == ShopKind::GeneralStore ? generalStore : none;
}

const char* ShopDisplayName(ShopKind kind)
{
    return kind == ShopKind::GeneralStore ? "General store" : "Shop";
}

std::vector<Item> ShopSellableItems(const Simulation& simulation, ShopKind kind)
{
    std::vector<Item> items;
    for (int index = 0; index < ItemCount; ++index)
    {
        const Item item = static_cast<Item>(index);
        if (simulation.Count(item) > 0 && ShopBuys(kind, item)) items.push_back(item);
    }
    return items;
}

bool IsShopDay(double hour)
{
    return std::isfinite(hour) && Calendar::DateAt(hour).weekday != ShopClosedDay;
}

bool IsShopOpen(const Shop& shop, double hour)
{
    if (!std::isfinite(hour)) return false;
    const double time = std::fmod(std::fmod(hour, 24.0) + 24.0, 24.0);
    return time >= shop.openHour && time < shop.closeHour && IsShopDay(hour);
}

double NextShopOpening(const Shop& shop, double hour)
{
    if (!std::isfinite(hour) || IsShopOpen(shop, hour)) return hour;
    const double midnight = std::floor(hour / 24.0) * 24.0;
    // Today's opening or a later one, past any closed day (a week covers every weekday).
    for (int day = 0; day <= Calendar::DaysPerWeek + 1; ++day)
    {
        const double opening = midnight + day * 24.0 + shop.openHour;
        if (opening > hour && IsShopOpen(shop, opening)) return opening;
    }
    return hour;
}

double HoursUntilOpen(const Shop& shop, double hour)
{
    return std::isfinite(hour) ? NextShopOpening(shop, hour) - hour : 0.0;
}

bool CanWaitForShop(const Shop& shop, double hour)
{
    if (!std::isfinite(hour) || IsShopOpen(shop, hour)) return false;
    // Only the ordinary night's closure (closing to opening, 14 h): never through a closed day's hours.
    return HoursUntilOpen(shop, hour) <= 24.0 - (shop.closeHour - shop.openHour) + 1e-6;
}

std::string FormatHour(double hour)
{
    // Round to the minute first, so 7:59:50 reads 8 AM rather than 7 AM.
    const long total = std::lround(std::fmod(std::fmod(hour, 24.0) + 24.0, 24.0) * 60.0) % (24 * 60);
    const int whole = static_cast<int>(total / 60);
    const int minutes = static_cast<int>(total % 60);
    const int twelve = whole % 12 == 0 ? 12 : whole % 12;
    std::string text = std::to_string(twelve);
    if (minutes) text += (minutes < 10 ? ":0" : ":") + std::to_string(minutes);
    return text + (whole < 12 ? " AM" : " PM");
}

std::string ClosedMessage(const Shop& shop, double hour)
{
    const double next = NextShopOpening(shop, hour);
    const Calendar::Date today = Calendar::DateAt(hour), opens = Calendar::DateAt(next);
    const std::string at = FormatHour(next);
    if (!IsShopDay(hour))
        return std::string("Closed today (") + Calendar::WeekdayName(today.weekday) + ") - opens "
            + Calendar::WeekdayName(opens.weekday) + " at " + at;
    if (opens.dayIndex > today.dayIndex + 1) return std::string("Closed - opens ") + Calendar::WeekdayName(opens.weekday) + " at " + at;
    return "Closed - opens at " + at;
}

std::string ClosedSignText(const Shop& shop, double hour)
{
    const double next = NextShopOpening(shop, hour);
    if (!IsShopDay(hour)) return std::string("CLOSED\non ") + Calendar::WeekdayName(ShopClosedDay) + "s";
    if (Calendar::DateAt(next).dayIndex > Calendar::DateAt(hour).dayIndex + 1)
        return std::string("CLOSED\nopens ") + Calendar::WeekdayShort(Calendar::DateAt(next).weekday) + " " + FormatHour(next);
    return "CLOSED\nopens at " + FormatHour(next);
}

Coins SellPrice(Item item) { return BasePrice(item); }
Coins BuyPrice(Item item) { return item == Item::FishingPole ? Fishing::PolePriceCoins : RoundedMarkup(BasePrice(item)); }
Coins BuyBackPrice(Item item) { return SellPrice(item); }

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
    if (!IsShopOpen(*shop, state_.hour)) return ShopBad(ClosedMessage(*shop, state_.hour), revision_, ResultCode::Unavailable);
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
    const Coins earned = SellPrice(item) * quantity;
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
    if (!ValidShopItem(item) || quantity <= 0 || quantity > MaxPackCapacity)
        return ShopBad("Choose something to buy and how many.", revision_);
    const auto& goods = ShopGoods(shop->kind);
    if (fromHeroineStock)
    {
        if (shop->heroineStock[static_cast<int>(item)] < quantity)
            return ShopBad("The shop only has " + Plural(shop->heroineStock[static_cast<int>(item)], item) + " left.", revision_);
    }
    else if (std::find(goods.begin(), goods.end(), item) == goods.end())
        return ShopBad(std::string(ShopDisplayName(shop->kind)) + " doesn't sell " + ItemName(item) + ".", revision_);
    const Coins cost = (fromHeroineStock ? BuyBackPrice(item) : BuyPrice(item)) * quantity;
    if (cost > state_.money)
        return ShopBad("That costs " + FormatMoney(cost) + "; you have " + FormatMoney(state_.money) + ".", revision_);
    if (UsedCapacity() + quantity > PackCapacity() || Count(item) + quantity > PackCapacity())
        return ShopBad("Not enough pack space for " + Plural(quantity, item) + ".", revision_, ResultCode::Capacity);
    State candidate = state_;
    for (auto& value : candidate.shops)
        if (value.id == shopId && fromHeroineStock) value.heroineStock[static_cast<int>(item)] -= quantity;
    candidate.inventory[static_cast<int>(item)] += quantity;
    candidate.money -= cost;
    const std::string message = "Bought " + Plural(quantity, item) + " for " + FormatMoney(cost) + ".";
    return CommitInventory(std::move(candidate), message.c_str());
}

Result Simulation::BuyBackpack(int shopId, Point player)
{
    const auto access = CheckShopAccess(shopId, player);
    if (!access) return access;
    const Shop* shop = FindShop(shopId);
    if (state_.leatherBackpack) return ShopBad("You already have the leather backpack.", revision_);
    if (!Backpack::Offered(state_, shop->kind))
        return ShopBad(std::string(ShopDisplayName(shop->kind)) + " doesn't sell backpacks.", revision_);
    if (Backpack::Price > state_.money)
        return ShopBad("That costs " + FormatMoney(Backpack::Price) + "; you have " + FormatMoney(state_.money) + ".", revision_);
    State candidate = state_;
    candidate.leatherBackpack = true;
    candidate.backpackShown = true;
    candidate.money -= Backpack::Price;
    const std::string message = "Bought the leather backpack for " + FormatMoney(Backpack::Price)
        + ". You can carry " + std::to_string(MaxPackCapacity) + " now.";
    return CommitInventory(std::move(candidate), message.c_str());
}

Result Simulation::SetBackpackShown(bool shown)
{
    if (!state_.leatherBackpack) return ShopBad("You don't have a backpack yet.", revision_);
    if (state_.backpackShown == shown) return ShopBad(shown ? "Your backpack is already showing." : "Your backpack is already hidden.", revision_);
    state_.backpackShown = shown;
    return ShopGood(shown ? "Your backpack shows on your back." : "Your backpack is hidden. You can still carry as much.", ++revision_);
}

Result Simulation::WaitForShop(int shopId, Point player)
{
    if (state_.failed) return ShopBad("You need to recover first.", revision_, ResultCode::Unavailable);
    const Shop* shop = FindShop(shopId);
    if (!shop) return ShopBad("There is no such shop.", revision_);
    const std::string name = ShopDisplayName(shop->kind);
    if (IsShopOpen(*shop, state_.hour)) return ShopBad("The " + ToLowerAscii(name) + " is open now.", revision_);
    // No waiting out a whole closed day in the street: only the ordinary night's closure.
    if (!CanWaitForShop(*shop, state_.hour))
    {
        const double next = NextShopOpening(*shop, state_.hour);
        return ShopBad("The " + ToLowerAscii(name) + " is closed on " + Calendar::WeekdayName(ShopClosedDay) + "s. It opens "
            + Calendar::WeekdayName(Calendar::DateAt(next).weekday) + " at " + FormatHour(next) + ".", revision_,
            ResultCode::Unavailable);
    }
    if (!std::isfinite(player.x) || !std::isfinite(player.y)
        || std::hypot(player.x - shop->counterX, player.y - shop->counterY) > ShopWaitReach)
        return ShopBad("Wait by the shop's door.", revision_);
    const double openHour = shop->openHour;
    // A hair past the hour, so the step boundaries can't leave the clock a rounding error short.
    const double hours = HoursUntilOpen(*shop, state_.hour) + 1e-6;
    // Try it on a copy first: if she'd collapse before it opens, she waits no time at all.
    Simulation trial = *this;
    trial.AdvanceGameHours(hours, player);
    if (trial.state_.failed)
        return ShopBad("You're too hungry to wait until " + FormatHour(openHour) + ". Eat something first.", revision_,
            ResultCode::Unavailable);
    // Worn out, she'd doze off in the street before it opens: that isn't the wait she agreed to.
    if (trial.DozeCount() != DozeCount())
        return ShopBad("You're too tired to wait until " + FormatHour(openHour) + ". Rest or eat first.", revision_,
            ResultCode::Unavailable);
    // AdvanceGameHours passes no time at all past the calendar's supported limit.
    if (trial.state_.hour < state_.hour + hours - 1e-3)
        return ShopBad("The calendar has reached its supported limit.", revision_);
    // The ordinary passage of time: crops, weather, fires, vitals and the morning sell-down all run.
    AdvanceGameHours(hours, player);
    ++revision_;
    const Shop* opened = FindShop(shopId);
    if (!opened || !IsShopOpen(*opened, state_.hour))
        return ShopBad("You waited, but the " + ToLowerAscii(name) + " is still closed.", revision_, ResultCode::Unavailable);
    return ShopGood("You wait by the door until " + FormatHour(openHour) + ". The " + ToLowerAscii(name) + " is open.", revision_);
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

Result Simulation::GrantMoney(Coins coins)
{
    // The purse always holds 0..MaxMoney, so neither bound can overflow: a huge grant is compared with
    // the room left rather than added first (INT64_MAX used to wrap past the cap).
    const bool outOfRange = coins < 0 ? coins < -state_.money : coins > MaxMoney - state_.money;
    if (outOfRange) return ShopBad("That would leave the purse out of range.", revision_);
    state_.money += coins;
    return ShopGood(FormatMoneyDelta(coins), ++revision_);
}

Result Simulation::PlaceShop(ShopKind kind, Point counter, double yaw)
{
    if (!std::isfinite(counter.x) || !std::isfinite(counter.y) || std::abs(counter.x) > MaxWorldCoordinate
        || std::abs(counter.y) > MaxWorldCoordinate || !std::isfinite(yaw))
        return ShopBad("The counter is outside the world.", revision_);
    for (auto& shop : state_.shops)
        if (shop.kind == kind)
        {
            shop.counterX = counter.x;
            shop.counterY = counter.y;
            shop.counterYaw = yaw;
            return ShopGood("Shop moved.", ++revision_);
        }
    Shop shop;
    shop.id = state_.nextId++;
    shop.kind = kind;
    shop.name = ShopDisplayName(kind);
    shop.counterX = counter.x;
    shop.counterY = counter.y;
    shop.counterYaw = yaw;
    state_.shops.push_back(shop);
    return ShopGood("Shop opened.", ++revision_);
}

void Simulation::SeedEstateShops(State& candidate, const EstateLayout& layout)
{
    candidate.money = StartingMoney;
    const Landmark* landmark = layout.FindLandmark(Anchor::GeneralStoreCounter);
    const Point counter = landmark ? landmark->position : Point{};
    Shop store;
    store.id = candidate.nextId++;
    store.kind = ShopKind::GeneralStore;
    store.name = ShopDisplayName(ShopKind::GeneralStore);
    store.counterX = counter.x;
    store.counterY = counter.y;
    store.counterYaw = landmark ? landmark->yaw : 0.0;
    candidate.shops.push_back(store);
}

void Simulation::RefreshShopCounters(State& candidate, const EstateLayout& layout)
{
    if (!candidate.fixedEstate) return;
    for (auto& shop : candidate.shops)
        if (shop.kind == ShopKind::GeneralStore)
        {
            const Landmark* counter = layout.FindLandmark(Anchor::GeneralStoreCounter);
            if (counter) { shop.counterX = counter->position.x; shop.counterY = counter->position.y; shop.counterYaw = counter->yaw; }
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
        body << shop.id << ' ' << static_cast<int>(shop.kind) << ' ' << shop.counterX << ' ' << shop.counterY << ' ' << shop.counterYaw
             << ' ' << shop.greetings;
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
        if (!(input >> shop.id >> kind >> shop.counterX >> shop.counterY >> shop.counterYaw >> shop.greetings >> stocked)
            || kind < 0 || kind >= static_cast<int>(ShopKind::Count) || shop.id <= 0 || shop.id >= candidate.nextId
            || !ids.insert(shop.id).second || shop.greetings < 0 || stocked < 0 || stocked > ItemCount
            || !std::isfinite(shop.counterX) || !std::isfinite(shop.counterY) || !std::isfinite(shop.counterYaw)
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
