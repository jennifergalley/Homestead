#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadWaterPool.generated.h"

class USplineComponent;
class UProceduralMeshComponent;
class UMaterialInterface;

/**
 * Still water on the Estate map (the estate lake): a closed spline tracing the shoreline at the water
 * level, drawn as a Single Layer Water surface. Tagged HomesteadWater, so the controller's water probe
 * (pail refill, dry-ground checks) finds it; for a closed spline the probe measures to the shoreline
 * itself and counts points inside it as in the water. The surface runs BankOverlap past the shoreline
 * under the graded banks, so the ground draws the waterline. An invisible wall a wading depth in from the
 * shore (pawns only; the camera and traces pass through) keeps her from walking under the water.
 */
UCLASS()
class SURVIVALGAME_API AHomesteadWaterPool : public AActor
{
    GENERATED_BODY()

public:
    AHomesteadWaterPool();
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
    TObjectPtr<USplineComponent> Shore;

    UPROPERTY(VisibleAnywhere, Category = "Water")
    TObjectPtr<UProceduralMeshComponent> Surface;

    /** The invisible wading limit (blocks pawns only). */
    UPROPERTY(VisibleAnywhere, Category = "Water")
    TObjectPtr<UProceduralMeshComponent> WadeLimit;

    /** How far in from the shore she can wade (cm): about knee deep (50-70 cm) over lake_basin.py's shelving bed. At 2.2 m
     *  she stopped in clear shallows that read as wet bank (Jenny, 2026-09-30: "I am not able to walk into the lake"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0"))
    float WadeInset = 380.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
    TObjectPtr<UMaterialInterface> Material;

    /** How far the surface runs past the shoreline under the banks (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0"))
    float BankOverlap = 60.0f;

    /** Shoreline samples around the loop. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "12", ClampMax = "512"))
    int32 ShoreSamples = 160;

    /** Concentric rings from the shore to the middle (the depth tint and foam fade across them). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "2", ClampMax = "32"))
    int32 Rings = 10;

    /** Water depth gained per centimetre in from the shore (the graded banks' slope). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0.01"))
    float ShelfSlope = 0.25f;

    /** Rebuilds the surface from the shoreline (also runs on construction). */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Water")
    void RebuildSurface();

    /** Replaces the shoreline with a closed loop of world points (cm) at water level WaterZ (cm). */
    UFUNCTION(BlueprintCallable, Category = "Water")
    void SetShore(const TArray<FVector2D>& WorldPoints, float WaterZ);
};
