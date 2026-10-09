#pragma once

#include "HomesteadShops.h"
#include "HomesteadSimulation.h"

// Clothing at the General Store (Jenny, 2026-10-04: alternate clothes are bought, not found in the
// manor chest). Each garment sells once: she can own one of each, and it's cosmetic only.
namespace Homestead
{
namespace GarmentShop
{
struct Offer
{
    WearableDefinition definition;
    Coins price; // Balance Agent, balance.md section 5 (d2211552c): the full set is 1,610 coins.
};
constexpr Offer Offers[] = {
    {WearableDefinition::LinenShirt, 120},
    {WearableDefinition::LinenLongShirt, 180},
    {WearableDefinition::Trousers, 200},
    {WearableDefinition::FurCoat, 600},
    {WearableDefinition::FurBoots, 300},
    {WearableDefinition::WovenSandals, 60},
    {WearableDefinition::TurnShoes, 150},
};

// The garment's price, or 0 when the store doesn't sell it.
Coins Price(WearableDefinition definition);
// Whether she owns one already, wherever it is (worn, carried, stored or dropped).
bool Owned(const State& state, WearableDefinition definition);
// Whether a shop sells it: the General Store, for the garments above.
bool Offered(ShopKind kind, WearableDefinition definition);
}
}
