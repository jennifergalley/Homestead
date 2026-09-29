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
 * the waterline half width in metres at that point; the surface runs BankOverlap further on each
 * side, under the graded banks, so the ground itself draws the waterline. Both ends round off
 * (a cap as long as the width at the source, EndCap at the mouth), and the water froths where it
 * wells up at the source and wherever the bed is steep.
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
    int32 Columns = 8;

    /** How far the surface runs past the waterline under each bank (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0"))
    float BankOverlap = 45.0f;

    /** Length of the rounded end at the source (cm); 0 rounds it off over its own half width. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0"))
    float StartCap = 0.0f;

    /** Length over which the mouth narrows to nothing (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0"))
    float EndCap = 600.0f;

    /** Froth where the spring wells up, fading out over this distance from the source (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0"))
    float SpringFroth = 400.0f;

    /** Rebuilds the surface from the spline (also runs on construction). */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Water")
    void RebuildSurface();

    /** Replaces the spline with world points (cm, Z = water surface) and waterline half widths (cm). */
    UFUNCTION(BlueprintCallable, Category = "Water")
    void SetCourse(const TArray<FVector>& WorldPoints, const TArray<float>& HalfWidthsCm);
};
