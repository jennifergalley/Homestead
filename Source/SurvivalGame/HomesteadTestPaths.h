#pragma once

#include "CoreMinimal.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

inline bool HomesteadAutomatedActorsEnabled()
{
#if UE_BUILD_SHIPPING
    return FParse::Param(FCommandLine::Get(), TEXT("HomesteadShippingQA"))
        && (FParse::Param(FCommandLine::Get(), TEXT("HomesteadSmokeTest"))
            != FParse::Param(FCommandLine::Get(), TEXT("HomesteadVisualPlaytest")));
#else
    return true;
#endif
}

inline FString HomesteadTestOutputDirectory()
{
    FString Override;
    if (FParse::Value(FCommandLine::Get(), TEXT("HomesteadTestOutput="), Override) && !Override.IsEmpty())
        return FPaths::ConvertRelativePathToFull(Override);
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"));
}
