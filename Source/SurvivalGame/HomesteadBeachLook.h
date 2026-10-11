#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Simulation/HomesteadSimulation.h"

// Meshes and look constants for the beach (add-beach-gathering). Authored by the Blender recipes
// Scripts/Blender/Recipes/beach_shells.py, driftwood.py, shore_rock.py and beach_campfire.py and imported
// under /Game/SurvivalGame/Environment/Props/<Folder>. A missing mesh loads as null; callers log it.
namespace HomesteadBeachLook
{
// The driftwood sticks are authored at full size, so the pile and her hands show them at 1:1.
constexpr float DriftwoodScale = 1.0f;

inline UStaticMesh* LoadProp(const TCHAR* Folder, const TCHAR* Name)
{
    return LoadObject<UStaticMesh>(nullptr,
        *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/%s.%s"), Folder, Name, Name), nullptr,
        LOAD_NoWarn | LOAD_Quiet);
}

// SM_Driftwood_a/_b/_c for Index 0-2: the pile's three sticks; she lifts 1 and 2.
inline UStaticMesh* DriftwoodStick(int32 Index)
{
    static const TCHAR* const Names[] = {TEXT("SM_Driftwood_a"), TEXT("SM_Driftwood_b"), TEXT("SM_Driftwood_c")};
    return Index >= 0 && Index < UE_ARRAY_COUNT(Names) ? LoadProp(TEXT("Driftwood"), Names[Index]) : nullptr;
}

// What she holds in her pinch as she pouches a beach find (pivot at the pinch); null for anything else.
inline UStaticMesh* HeldMesh(Homestead::ResourceKind Kind)
{
    switch (Kind)
    {
    case Homestead::ResourceKind::Seashells: return LoadProp(TEXT("BeachShells"), TEXT("SM_SeashellHeld"));
    case Homestead::ResourceKind::MusselBed: return LoadProp(TEXT("ShoreRock"), TEXT("SM_MusselHeld"));
    case Homestead::ResourceKind::LaverRocks: return LoadProp(TEXT("ShoreRock"), TEXT("SM_LaverHeld"));
    default: return nullptr;
    }
}
}
