#pragma once

#include "HomesteadItems.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Money, shops and their stock. The transactions themselves are Simulation members
// (Simulation::Sell / Simulation::Buy, in HomesteadShops.cpp) so every cent moves with the goods.
namespace Homestead
{
// Money is always whole cents.
using Cents = std::int64_t;
constexpr Cents StartingMoney = 1000; // $10.00, a placeholder until Jenny tunes it.
constexpr Cents MaxMoney = INT64_C(100000000000); // $1,000,000,000.00
// How near the counter she must stand to trade, in cm.
constexpr double CounterReach = 400.0;
// Shop goods cost their base price times this; her own goods sell back at what she was paid.
constexpr int ShopMarkupPercent = 125;
// Each morning at 6 townsfolk buy this share (rounded up, at least one) of each of her goods.
constexpr int SellDownPercent = 35;
constexpr double DayRolloverHour = 6.0;
constexpr int MaxShopStock = 9999;

// "$1,234.05"; negative amounts read "-$1.00".
std::string FormatMoney(Cents cents);
// Always signed: "+$2.00", "-$0.40", "+$0.00".
std::string FormatMoneyDelta(Cents cents);

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
const char* ShopDisplayName(ShopKind kind);
bool IsShopOpen(const Shop& shop, double hour);
// "Closed - opens at 8 AM".
std::string ClosedMessage(const Shop& shop);
// What she is paid per unit.
Cents SellPrice(Item item);
// What a shop's own goods cost per unit.
Cents BuyPrice(Item item);
// What she pays per unit to buy one of her own goods back.
Cents BuyBackPrice(Item item);
// The morning share townsfolk buy from a stack of `quantity`.
int SellDownAmount(int quantity);
std::string FormatHour(double hour);
}
