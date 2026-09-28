#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/HomesteadMapGeometry.h"
#include "HomesteadEstateMap.generated.h"

class UTexture2D;

// The baked top-down cartographic map (T_EstateMap) and the world rectangle it covers, shared by
// the HUD minimap and the field-book Map tab. Re-baked in the editor with Homestead.BakeEstateMap.
UCLASS(BlueprintType)
class SURVIVALGAME_API UHomesteadEstateMap : public UDataAsset
{
    GENERATED_BODY()
public:
    static constexpr const TCHAR* AssetPath = TEXT("/Game/SurvivalGame/UI/Map/DA_EstateMap.DA_EstateMap");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
    TObjectPtr<UTexture2D> Texture;
    // South-west corner (X south edge, Y west edge) and extent of the mapped world, in Unreal cm.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
    FVector2D WorldMin = FVector2D(-201600.0, -201600.0);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
    FVector2D WorldSize = FVector2D(403200.0, 403200.0);
    // What the bake came from ("provisional layout" or "Estate capture") and when.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
    FString Source;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
    FString BakedAt;

    HomesteadMap::MapTransform Transform() const
    {
        return {WorldMin.X, WorldMin.Y, FMath::Max(1.0, WorldSize.X), FMath::Max(1.0, WorldSize.Y)};
    }
};
