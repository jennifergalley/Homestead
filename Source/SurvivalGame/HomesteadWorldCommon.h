#pragma once

#include "CoreMinimal.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace HomesteadWorldCommon
{
inline bool ProfileChunkPublishing()
{
    return FParse::Param(FCommandLine::Get(), TEXT("HomesteadGeneratedWoodland"));
}

// The MVP woodland's interactables (add-mvp-woodland-biome; Scripts/Terrain/mvp_woodland.py).
inline bool IsMvpWoodlandId(int32 Id) { return Id >= 560000 && Id < 570000; }
}
