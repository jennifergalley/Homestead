#include "../Source/SurvivalGame/HomesteadWardrobeSelection.h"
#include <cstdlib>
#include <iostream>

namespace
{
void Check(bool condition, const char* message)
{
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
}

int main()
{
    using namespace Homestead;
    Simulation simulation;
    const State original = simulation.GetState();
    WardrobeSelection result;
    std::string error;
    for (int body = 0; body < 3; ++body)
        for (int hair = 0; hair < 3; ++hair)
        {
            Check(SelectWardrobe(original, body, hair, result, error), "Valid fit rejected");
            Check(result.equipped.size() == 2, "Starter equipment does not have two instances");
            Check(result.ownedDefinitions.size() == 2, "Owned definitions incorrectly deduplicated");
            Check(result.base.find("SK_Modular_") == 0, "Wrong asset namespace");
        }
    State empty = original;
    empty.wearables.clear();
    empty.equipment.fill(0);
    Check(SelectWardrobe(empty, 0, 0, result, error) && result.equipped.empty(), "Covered base-only rejected");
    State crafted = original;
    crafted.wearables.push_back({3, WearableDefinition::WovenFootwraps, 0, WearableOwner::Carried, 0});
    Check(SelectWardrobe(crafted, 0, 0, result, error), "Carried craft result rejected");
    Check(result.ownedDefinitions.size() == 3 && result.equipped.size() == 2,
        "Craft admission must include unequipped definition without rendering it");
    State duplicate = original;
    duplicate.wearables.push_back(duplicate.wearables.front());
    Check(!SelectWardrobe(duplicate, 0, 0, result, error) && !error.empty(), "Duplicate ID admitted");
    Check(result.equipped.empty(), "Failed selection leaked prepared equipment");
    State bad = original;
    bad.wearables[1].dye = 1;
    Check(!SelectWardrobe(bad, 0, 0, result, error), "Footwear dye admitted");
    bad = original;
    bad.equipment[static_cast<int>(EquipmentSlot::Legs)] = 0;
    Check(!SelectWardrobe(bad, 0, 0, result, error), "Partial two-slot tunic admitted");
    bad = empty;
    bad.wearables.push_back({3, WearableDefinition::LinenApron, 0, WearableOwner::Equipped, 0});
    bad.equipment[static_cast<int>(EquipmentSlot::Apron)] = 3;
    Check(!SelectWardrobe(bad, 0, 0, result, error), "Apron without tunic admitted");
    bad = original;
    bad.wearables.push_back({3, WearableDefinition::WovenFootwraps, 0, WearableOwner::Equipped, 0});
    Check(!SelectWardrobe(bad, 0, 0, result, error), "Overlapping footwear admitted");
    bad = original;
    bad.wearables[0].definition = static_cast<WearableDefinition>(999);
    Check(!SelectWardrobe(bad, 0, 0, result, error), "Unknown garment admitted");
    Check(!SelectWardrobe(original, 3, 0, result, error), "Unknown body admitted");
    Check(!SelectWardrobe(original, 0, -1, result, error), "Unknown hair admitted");
    Check(original.wearables.size() == simulation.GetState().wearables.size()
        && original.equipment == simulation.GetState().equipment, "Selection mutated authority");
    std::cout << "Wardrobe selection checks passed\n";
}
