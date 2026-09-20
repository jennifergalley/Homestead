#pragma once

#include "CoreMinimal.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

inline FString HomesteadTestOutputDirectory()
{
    FString Override;
    if (FParse::Value(FCommandLine::Get(), TEXT("HomesteadTestOutput="), Override) && !Override.IsEmpty())
        return FPaths::ConvertRelativePathToFull(Override);
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"));
}
