#pragma once

#include "CoreMinimal.h"

bool RunFernSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven = false);
bool RunHairWaveSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven);
bool RunWardrobeSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven);
bool RunTreeSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven);
bool RunGrassSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven);
bool RunWoodlandSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven);
bool RunTreePaletteSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven);
bool RunMatureFirSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven);
bool RunWoodlandMaterialUsage(const FString& Output);
