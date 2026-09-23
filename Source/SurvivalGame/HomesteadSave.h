#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "HomesteadSave.generated.h"

UCLASS()
class SURVIVALGAME_API UHomesteadSave : public USaveGame
{
    GENERATED_BODY()
public:
    static constexpr int32 CurrentVersion = 7;
    UPROPERTY() int32 Version = CurrentVersion;
    bool IsCurrentVersion() const { return Version == CurrentVersion; }
    UPROPERTY() FString WorldId;
    UPROPERTY() FString SimulationData;
    UPROPERTY() FVector PlayerLocation = FVector(-1000, 0, 150);
    UPROPERTY() FRotator ViewRotation = FRotator(-15, 0, 0);
    UPROPERTY() int64 SavedAtUtc = 0;
    UPROPERTY() float CameraSensitivity = 1.0f;
    UPROPERTY() bool InvertCameraY = false;
    UPROPERTY() float MusicVolume = 0.65f;
    UPROPERTY() float AmbienceVolume = 0.7f;
    UPROPERTY() float EffectsVolume = 0.8f;
    UPROPERTY() int32 HairStyle = 0;
    UPROPERTY() int32 HairColor = 0;
    UPROPERTY() int32 SkinTone = 0;
    UPROPERTY() int32 EyeColor = 0;
    UPROPERTY() int32 TunicColor = 0;
    UPROPERTY() int32 Outfit = 0;
    UPROPERTY() int32 BodyPreset = 0;
    UPROPERTY() TArray<int32> HotbarSlots;
    UPROPERTY() int32 SelectedHotbarSlot = 0;
};
