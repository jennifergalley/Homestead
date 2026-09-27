#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadWaterRibbon.generated.h"

class USplineComponent;
class UProceduralMeshComponent;
class UMaterialInterface;

/**
 * An authored stretch of flowing water on the Estate map: a spline whose points sit on the water
 * surface, drawn as a Single Layer Water ribbon. Tagged HomesteadWater so the controller's water
 * probe (pail refill, dry-ground checks, the creek burble) finds it. Spline point scale Y sets
 * the half width in metres at that point.
 */
UCLASS()
class SURVIVALGAME_API AHomesteadWaterRibbon : public AActor
{
    GENERATED_BODY()

public:
    AHomesteadWaterRibbon();
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
    TObjectPtr<USplineComponent> Spline;

    UPROPERTY(VisibleAnywhere, Category = "Water")
    TObjectPtr<UProceduralMeshComponent> Surface;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
    TObjectPtr<UMaterialInterface> Material;

    /** Distance between cross-sections along the spline (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "25"))
    float SegmentLength = 100.0f;

    /** Cross-section columns across the ribbon. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "2", ClampMax = "32"))
    int32 Columns = 6;

    /** Rebuilds the surface from the spline (also runs on construction). */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Water")
    void RebuildSurface();

    /** Replaces the spline with world points (cm, Z = water surface) and half widths (cm). */
    UFUNCTION(BlueprintCallable, Category = "Water")
    void SetCourse(const TArray<FVector>& WorldPoints, const TArray<float>& HalfWidthsCm);
};
