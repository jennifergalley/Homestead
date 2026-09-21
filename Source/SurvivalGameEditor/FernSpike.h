#pragma once

#include "CoreMinimal.h"

bool RunFernSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven = false);
bool RunHairWaveSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven);
bool RunWardrobeSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven);
