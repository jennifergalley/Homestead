#pragma once

#include "CoreMinimal.h"
#include "Simulation/HomesteadCrops.h"
#include "Simulation/HomesteadItems.h"
#include "Simulation/HomesteadOvergrowth.h"

#include <vector>

namespace HomesteadControllerHelpers
{
// The estate's tools. The retired knife and machete no longer ride on the hotbar.
inline bool IsHotbarTool(Homestead::Item Item)
{
    return Homestead::ToolForItem(Item) != Homestead::ToolKind::Count || Item == Homestead::Item::OilLamp
        || Item == Homestead::Item::FishingPole;
}

template <typename FPredicate>
const Homestead::Plot* FindPlotWhere(const std::vector<Homestead::Plot>& Plots, FPredicate Predicate)
{
    for (const auto& Plot : Plots) if (Predicate(Plot)) return &Plot;
    return nullptr;
}

// Chosen on the hotbar to plant bare tilled soil: each seed grows its crop (a berry's seeds grow a bush).
inline TOptional<Homestead::CropKind> PlantingCrop(Homestead::Item Item)
{
    if (const auto* Crop = Homestead::CropForSeed(Item)) return Crop->kind;
    return {};
}

inline bool IsFoodItem(Homestead::Item Item) { return Homestead::IsEdible(Item); }

inline FName HotbarIcon(Homestead::Item Item)
{
    switch (Item)
    {
    case Homestead::Item::Berries: return TEXT("berries");
    case Homestead::Item::Seeds: return TEXT("seeds");
    case Homestead::Item::RoastedRoots: return TEXT("roasted-roots");
    case Homestead::Item::HerbedRoots: return TEXT("herbed-roots");
    case Homestead::Item::Knife: return TEXT("knife");
    case Homestead::Item::Hatchet: return TEXT("hatchet");
    case Homestead::Item::DiggingStick: return TEXT("digging-stick");
    case Homestead::Item::WateringCan: return TEXT("watering-can");
    case Homestead::Item::Machete: return TEXT("machete");
    case Homestead::Item::Scythe: return TEXT("scythe");
    case Homestead::Item::Billhook: return TEXT("billhook");
    case Homestead::Item::Pickaxe: return TEXT("pickaxe");
    case Homestead::Item::OilLamp: return TEXT("oil-lamp");
    default:
        // Everything else (seed, produce, food, materials: the hotbar is her pack's first row and
        // holds any stack) uses its catalogue glyph, as the pack grid does.
        if (static_cast<int>(Item) < 0 || static_cast<int>(Item) >= Homestead::ItemCount) return NAME_None;
        return FName(UTF8_TO_TCHAR(Homestead::ItemIcon(Item)));
    }
}

inline const TCHAR* SwingVerb(Homestead::Item Tool)
{
    switch (Tool)
    {
    case Homestead::Item::Hatchet: return TEXT("Chop with Axe");
    case Homestead::Item::Billhook: return TEXT("Hack with Billhook");
    case Homestead::Item::Scythe: return TEXT("Mow with Scythe");
    case Homestead::Item::Pickaxe: return TEXT("Break with Pickaxe");
    default: return TEXT("Clear");
    }
}

// Where she keeps a tool: 2 in the pack, 1 only in a storage chest, 0 not owned at all.
inline int32 ToolWhereabouts(const Homestead::Simulation& Sim, Homestead::Item Tool)
{
    if (Sim.Count(Tool) > 0) return 2;
    for (const auto& Structure : Sim.GetState().structures)
        if (Structure.storage[static_cast<int>(Tool)] > 0) return 1;
    return 0;
}

// "Select the scythe" when it's in her pack but not in hand, "Take the scythe from storage" when it's
// only in a chest, and "Requires a scythe" when she has none yet.
inline FString ToolPrompt(const Homestead::Simulation& Sim, Homestead::Item Tool, const FString& Name, const TCHAR* Purpose = TEXT(""))
{
    switch (ToolWhereabouts(Sim, Tool))
    {
    case 2: return TEXT("Select the ") + Name + Purpose;
    case 1: return TEXT("Take the ") + Name + TEXT(" from storage");
    default:
        return (FString(TEXT("aeiouAEIOU")).Contains(Name.Left(1)) ? TEXT("Requires an ") : TEXT("Requires a ")) + Name;
    }
}
}
