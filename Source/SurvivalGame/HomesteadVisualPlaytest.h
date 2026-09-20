#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadVisualPlaytest.generated.h"

class AHomesteadController;

UCLASS()
class SURVIVALGAME_API AHomesteadVisualPlaytest : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadVisualPlaytest();
    virtual void Tick(float DeltaSeconds) override;

private:
    struct FPass
    {
        FString Label;
        float Duration = 1;
        FVector2D Move = FVector2D::ZeroVector;
        FVector2D Look = FVector2D::ZeroVector;
        FKey Press;
        bool WalkToForage = false;
    };
    UPROPERTY() TObjectPtr<AHomesteadController> PC;
    TArray<FPass> Passes;
    TArray<FString> Telemetry;
    TArray<FString> Observations;
    bool bReady = false;
    bool bEntered = false;
    bool bFinished = false;
    bool bReachedForage = false;
    bool bObservedGather = false;
    bool bGatherRecovered = false;
    float Elapsed = 0;
    float PassElapsed = 0;
    float CaptureElapsed = 0;
    int32 PassIndex = 0;
    int32 CaptureIndex = 0;
    int32 ForageId = -1;
    FVector2D ForageTarget = FVector2D::ZeroVector;
    int32 FoodBefore = 0;
    int32 HerbBefore = 0;
    double LastWallTime = 0;
    FString OutputDirectory;

    void Prepare();
    void Tap(FKey Key);
    void ApplyAxes(FVector2D Move, FVector2D Look);
    void Capture(const FString& Label);
    void Finish();
};
