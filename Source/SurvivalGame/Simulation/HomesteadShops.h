#pragma once

#include "HomesteadCalendar.h"
#include "HomesteadItems.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Money, shops and their stock. The transactions themselves are Simulation members
// (Simulation::Sell / Simulation::Buy, in HomesteadShops.cpp) so every coin moves with the goods.
namespace Homestead
{
class Simulation;
// Money is a whole number of coins: the smallest stored unit is one coin (the raw values and save
// bytes are unchanged from when it read as cents, so buying power is too).
using Coins = std::int64_t;
constexpr Coins StartingMoney = 1000; // 1,000 coins, a placeholder until Jenny tunes it.
constexpr Coins MaxMoney = INT64_C(100000000000); // 100,000,000,000 coins
// How near the counter she must stand to trade, in cm.
constexpr double CounterReach = 400.0;
// Shop goods cost their base price times this; her own goods sell back at what she was paid.
constexpr int ShopMarkupPercent = 125;
// Each morning at 6 townsfolk buy this share (rounded up, at least one) of each of her goods.
constexpr int SellDownPercent = 35;
constexpr double DayRolloverHour = 6.0;
constexpr int MaxShopStock = 9999;

// Whole coins, grouped: "1,234 coins", "1 coin", "0 coins"; negative amounts read "-100 coins".
// No currency sign or decimals anywhere.
std::string FormatMoney(Coins amount);
// Always signed: "+2 coins", "-40 coins", "+1 coin", "+0 coins".
std::string FormatMoneyDelta(Coins amount);

struct Shop
{
    int id = 0;
    ShopKind kind = ShopKind::GeneralStore;
    std::string name;
    double openHour = 8.0;
    double closeHour = 18.0;
    double counterX = 0.0; // Where the shopkeeper stands.
    double counterY = 0.0;
    double counterYaw = 0.0; // Unreal yaw the shopkeeper faces, toward the customer and the door.
    // Her sold goods, which townsfolk buy down each morning and she can buy back.
    std::array<int, ItemCount> heroineStock{};
    int greetings = 0; // Times she has been greeted, a stub for later friendship.
};

// The shop's own goods, restocked without limit in round 1.
const std::vector<Item>& ShopGoods(ShopKind kind);
// Carried goods accepted by this shop, in catalogue order; shared by the Sell pane and native tests.
std::vector<Item> ShopSellableItems(const Simulation& simulation, ShopKind kind);
const char* ShopDisplayName(ShopKind kind);
// Every shop keeps the Sabbath (Jenny, 2026-09-30): closed all day Sunday (the calendar day, 06:00 to 06:00).
constexpr Weekday ShopClosedDay = Weekday::Sunday;
// Whether `hour` (the running game hour) falls on a trading day.
bool IsShopDay(double hour);
// Open hours on a trading day.
bool IsShopOpen(const Shop& shop, double hour);
// The running game hour the shop next opens, past any closed day; `hour` itself while it's open.
double NextShopOpening(const Shop& shop, double hour);
// Hours from `hour` until the shop next opens; 0 while it's open.
double HoursUntilOpen(const Shop& shop, double hour);
// Ordinary overnight waiting, and a Sunday wait until Monday morning opening.
bool CanWaitForShop(const Shop& shop, double hour);
// How near the shop (its counter) she must be to wait for it to open, in cm: the door and the
// street outside it.
constexpr double ShopWaitReach = 1500.0;
// "Closed <en dash> opens at 8 AM"; "Closed <en dash> opens Monday at 8 AM" past a closed day; on one,
// "Closed today (Sunday) <en dash> opens Monday at 8 AM".
std::string ClosedMessage(const Shop& shop, double hour);
// The board hung on the shut door: "CLOSED\nopens at 8 AM", "CLOSED\nopens Mon 8 AM", "CLOSED\non Sundays".
std::string ClosedSignText(const Shop& shop, double hour);
// What she is paid per unit.
Coins SellPrice(Item item);
// What a shop's own goods cost per unit.
Coins BuyPrice(Item item);
// What she pays per unit to buy one of her own goods back.
Coins BuyBackPrice(Item item);
// The morning share townsfolk buy from a stack of `quantity`.
int SellDownAmount(int quantity);
std::string FormatHour(double hour);
}
