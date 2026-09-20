#pragma once

#include "CoreMinimal.h"

struct FHomesteadSaveRoute
{
    FString Directory;
    FString Profile;
    FString Mode;
};

bool IsHomesteadPreviewProfile(const FString& Profile);
bool ResolveHomesteadSaveRoute(const TCHAR* CommandLine, const FString& ProjectSavedRoot,
    const FString& UserSettingsRoot, const FString& TestOutput, FHomesteadSaveRoute& Route, FString& Error);
