#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/HomesteadSimulation.h"
#include "HomesteadEnduranceState.h"
#include "HomesteadRenewalState.h"
#include "HomesteadVisualPlaytest.generated.h"

class AHomesteadController;
class UStaticMeshComponent;

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
    bool bPresentationDiagnostics = false;
    TArray<FString> PresentationTimings;
    TArray<FString> PresentationSettings;
    void RecordPresentationSettings(const TCHAR* Phase);
    void RecordGroveInventory();
    void RecordGrassGroundInventory();
    TWeakObjectPtr<UStaticMeshComponent> ObservedTree;
    FVector2D TreeCenter = FVector2D::ZeroVector;
    FVector2D TreeStaging = FVector2D::ZeroVector;
    FVector2D TreeRetreatStart = FVector2D::ZeroVector;
    TArray<FString> TreeContactSamples;
    double PreviousTreeDistance = 0;
    bool bHaveTreeDistance = false;
    double TreeRetreatDistance = 0;
    bool bTreeRoute = false;
    bool bTreeReady = false;
    bool bReachedTree = false;
    bool bTreeFramed = false;
    bool bTreeBlocked = false;
    bool bTreeRetreated = false;
    float TreeBlockedSeconds = 0;
    void PrepareTreeEncounter();
    void TickTreeEncounter(const FPass& Pass, float Delta, FVector2D& Move, FVector2D& Look);
    bool bWaterRoute = false;
    bool bClearRoute = false;
    bool bCleared = false;
    bool bObservedClear = false;
    bool bObservedHatchet = false;
    bool bClearRecovered = false;
    Homestead::Simulation ClearingExpected;
    double ClearingHour = 0;
    bool bWeedRoute = false;
    bool bWeeded = false;
    bool bWatered = false;
    bool bObservedWater = false;
    bool bObservedTool = false;
    bool bWaterRecovered = false;
    int32 WaterStage = 0;
    int32 PreviousWaterStage = -1;
    float WaterStageElapsed = 0;
    float WaterSettle = 0;
    bool bWaterTargetReady = false;
    FVector2D WaterTarget = FVector2D::ZeroVector;
    FVector2D GardenCenter = FVector2D::ZeroVector;
    int32 WaterPlotId = -1;
    int32 WaterBefore = 0;
    bool bWaterInputPending = false;
    int32 WaterSupplyItem = 0;
    int32 WaterSupplyBefore = 0;

    void Prepare();
    bool bForageRenewal = false;
    FHomesteadRenewalState Renewal;
    void PrepareRenewal();
    void TickRenewal(float Delta);
    bool RenewalCheck(bool Condition, const FString& Message);
    bool RenewalVisuals(int32 Id, bool Ready, bool Focused, const FString& Label, bool Screenshot);
    void RenewalEvent(const FString& Message);
    bool WriteRenewal(const FString& Status, const FString& Reason);
    void FinishRenewal(const FString& Status, const FString& Reason);
    bool bEndurance = false;
    FHomesteadEnduranceState Endurance;
    void PrepareEndurance();
    void TickEndurance(float EngineDelta);
    void EnduranceEvent(const FString& Message);
    bool WriteEnduranceProgress(const FString& Status, const FString& Reason);
    bool InspectEnduranceSaves();
    bool TapEnduranceLoad();
    void FinishEndurance(const FString& Status, const FString& Reason);
    void Tap(FKey Key);
    void ApplyAxes(FVector2D Move, FVector2D Look);
    void Capture(const FString& Label);
    void Finish();
    void TickWatering(float WallDelta);
    void TickWeeding(float WallDelta);
    bool WalkWaterTarget(FVector2D Target, float Tolerance, float Delta, FVector2D& Move, FVector2D& Look);
};
