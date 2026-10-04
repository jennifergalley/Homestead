#pragma once

#include "Simulation/HomesteadItems.h"
#include <cstring>

namespace HomesteadOriginalItemArt
{
struct Entry
{
    Homestead::Item item;
    const char* icon;
    const char* folder;
    const char* serving;
    const char* portion;
    bool fish;
};

inline constexpr Entry Entries[] = {
    {Homestead::Item::BakedPotatoes, "baked-potatoes", "PreparedFood", "SM_BakedPotatoes", "SM_BakedPotatoesPortion", false},
    {Homestead::Item::RoastedTurnips, "roasted-turnips", "PreparedFood", "SM_RoastedTurnips", "SM_RoastedTurnipsPortion", false},
    {Homestead::Item::StewedCarrots, "stewed-carrots", "PreparedFood", "SM_StewedCarrots", "SM_StewedCarrotsPortion", false},
    {Homestead::Item::HerbedBroadBeans, "herbed-broad-beans", "PreparedFood", "SM_HerbedBroadBeans", "SM_HerbedBroadBeansPortion", false},
    {Homestead::Item::CabbagePotatoStew, "cabbage-potato-stew", "PreparedFood", "SM_CabbagePotatoStew", "SM_CabbagePotatoStewPortion", false},
    {Homestead::Item::BerryCompote, "berry-compote", "PreparedFood", "SM_BerryCompote", "SM_BerryCompotePortion", false},
    {Homestead::Item::StrawberryCompote, "strawberry-compote", "PreparedFood", "SM_StrawberryCompote", "SM_StrawberryCompotePortion", false},
    {Homestead::Item::RootVegetableHotpot, "root-vegetable-hotpot", "PreparedFood", "SM_RootVegetableHotpot", "SM_RootVegetableHotpotPortion", false},
    {Homestead::Item::RawFishSlices, "raw-fish-slices", "PreparedFood", "SM_RawFishSlices", "SM_RawFishSlicesPortion", false},
    {Homestead::Item::GrilledTrout, "grilled-trout", "PreparedFood", "SM_GrilledTrout", "SM_GrilledTroutPortion", false},
    {Homestead::Item::GrilledPerch, "grilled-perch", "PreparedFood", "SM_GrilledPerch", "SM_GrilledPerchPortion", false},
    {Homestead::Item::RiverTrout, "river-trout", "CaughtFish", "SM_RiverTrout", nullptr, true},
    {Homestead::Item::RiverSalmon, "river-salmon", "CaughtFish", "SM_RiverSalmon", nullptr, true},
    {Homestead::Item::LakePerch, "lake-perch", "CaughtFish", "SM_LakePerch", nullptr, true},
    {Homestead::Item::LakeCarp, "lake-carp", "CaughtFish", "SM_LakeCarp", nullptr, true},
    {Homestead::Item::SeaMackerel, "sea-mackerel", "CaughtFish", "SM_SeaMackerel", nullptr, true},
    {Homestead::Item::SeaBass, "sea-bass", "CaughtFish", "SM_SeaBass", nullptr, true},
    {Homestead::Item::FishingPole, "fishing-pole", "FishingPole", "SM_FishingPole", nullptr, false},
};

inline const Entry* Find(Homestead::Item item)
{
    for (const auto& entry : Entries) if (entry.item == item) return &entry;
    return nullptr;
}

inline const Entry* FindIcon(const char* icon)
{
    for (const auto& entry : Entries) if (std::strcmp(entry.icon, icon) == 0) return &entry;
    return nullptr;
}
}
