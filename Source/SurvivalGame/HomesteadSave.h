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
    // Same-second tie-breaker; old saves without this field read as zero.
    UPROPERTY() int64 SavedRevision = 0;
    // For the save list: "Eleanor Cavendish — Trevennor, Spring 1" (empty for unnamed woodland games).
    UPROPERTY() FString SaveLabel;
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
    // MetaHuman hairstyle; -1 in saves written before it existed (derived from HairStyle).
    UPROPERTY() int32 MetaHair = -1;
    UPROPERTY() TArray<int32> HotbarSlots;
    UPROPERTY() int32 SelectedHotbarSlot = 0;
    // 0: saved before food could be pinned; 1: pinned food and the machete migration applied;
    // 2: the estate's hafted tools (scythe, billhook, pickaxe) join the hotbar.
    // 3: the oil lamp joins older hotbars (add-oil-lamp).
    // 4: the hotbar is the first row of her pack, saved with the simulation ("packrow"); HotbarSlots
    //    is then only a reference copy. Older saves' pins move into the row once on load.
    static constexpr int32 CurrentHotbarLayout = 4;
    UPROPERTY() int32 HotbarLayout = 0;
};
