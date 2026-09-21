#pragma once

#include "CoreMinimal.h"
#include "HomesteadAppearance.h"
#include "HomesteadWardrobePresentation.generated.h"

namespace Homestead { struct State; }
class USkeletalMesh;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;

USTRUCT()
struct FHomesteadEquipmentSurface
{
    GENERATED_BODY()

    UPROPERTY() TObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
    int32 WearableId = 0;
    int32 Definition = INDEX_NONE;
    int32 Slot = INDEX_NONE;
    int32 Dye = 0;
};

// Retained render references only. Ownership remains in Homestead::State.
USTRUCT()
struct FHomesteadEquipmentPresentation
{
    GENERATED_BODY()

    UPROPERTY() FHomesteadEquipmentSurface Base;
    UPROPERTY() TArray<FHomesteadEquipmentSurface> Garments;
    UPROPERTY() TArray<TObjectPtr<USkeletalMesh>> AdmittedMeshes;
    FHomesteadAppearance Appearance;
    bool Ready = false;
};

namespace HomesteadWardrobePresentation
{
    bool Prepare(UObject* Owner, const Homestead::State& State,
        const FHomesteadAppearance& Appearance, USkeletalMesh& Reference,
        FHomesteadEquipmentPresentation& Out, FString& Error);
    void ApplySurface(const FHomesteadEquipmentSurface& Surface, USkeletalMeshComponent& Component);
}
