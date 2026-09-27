#pragma once

#include "Simulation/HomesteadSimulation.h"
#include <algorithm>
#include <set>

namespace Homestead
{
struct WardrobeSelection
{
    std::string body;
    std::string base;
    std::vector<WearableDefinition> ownedDefinitions;
    std::vector<WearableInstance> equipped;
};

inline const char* GarmentAssetSuffix(WearableDefinition definition)
{
    switch (definition)
    {
    case WearableDefinition::LinenTunic: return "Tunic";
    case WearableDefinition::LinenApron: return "Apron";
    case WearableDefinition::LeatherShoes: return "Shoes";
    case WearableDefinition::WovenFootwraps: return "Footwraps";
    default: return nullptr;
    }
}

inline bool SelectWardrobe(const State& state, int body, int hair,
    WardrobeSelection& out, std::string& error)
{
    out = {};
    error.clear();
    if (body < 0 || body >= 3 || hair < 0 || hair >= 3)
    {
        error = "Unsupported character body or hairstyle fit.";
        return false;
    }
    const char* bodies[] = {"Preferred", "Willow", "Hazel"};
    const char* styles[] = {"LongWave", "Bob", "Ponytail"};
    WardrobeSelection candidate;
    candidate.body = bodies[body];
    candidate.base = "SK_Modular_" + candidate.body + "_Base_" + styles[hair];
    std::set<int> ids;
    std::array<int, EquipmentSlotCount> slots{};
    for (const auto& item : state.wearables)
    {
        const auto* definition = GetWearableDefinition(item.definition);
        if (!definition || item.id <= 0
            || !ids.insert(item.id).second || item.dye < 0
            || item.dye >= (definition->dyeable ? 4 : 1))
        {
            error = "Invalid wearable identity, definition or dye in presentation candidate.";
            return false;
        }
        if (item.owner != WearableOwner::Carried && item.owner != WearableOwner::Chest
            && item.owner != WearableOwner::Equipped)
        {
            error = "Unknown wearable owner in presentation candidate.";
            return false;
        }
        // Garments made for the MetaHuman heroine have no modular fit; the legacy body skips them.
        const bool presented = GarmentAssetSuffix(item.definition) != nullptr;
        if (presented && std::find(candidate.ownedDefinitions.begin(), candidate.ownedDefinitions.end(),
            item.definition) == candidate.ownedDefinitions.end())
            candidate.ownedDefinitions.push_back(item.definition);
        if (item.owner != WearableOwner::Equipped) continue;
        for (int slot = 0; slot < EquipmentSlotCount; ++slot)
        {
            if ((definition->slots & (1u << slot)) == 0) continue;
            if (slots[slot] != 0)
            {
                error = "Two equipped garments occupy the same slot.";
                return false;
            }
            slots[slot] = item.id;
        }
        if (presented) candidate.equipped.push_back(item);
    }
    if (slots != state.equipment
        || (slots[static_cast<int>(EquipmentSlot::Apron)] != 0
            && slots[static_cast<int>(EquipmentSlot::Torso)] == 0))
    {
        error = "Equipment references or dependent apron are inconsistent.";
        return false;
    }
    out = std::move(candidate);
    return true;
}
}
