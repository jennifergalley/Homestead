#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/HomesteadSimulation.h"
#include "HomesteadSmokeTest.generated.h"

class AHomesteadController;

UCLASS()
class SURVIVALGAME_API AHomesteadSmokeTest : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadSmokeTest();
    virtual void Tick(float DeltaSeconds) override;

private:
    struct FStep
    {
        FString Name;
        TFunction<void()> Action;
        TFunction<bool()> Check;
        TFunction<bool()> Skip;
        TFunction<void()> Repeat;
        float Wait = 0.35f;
        int32 NavigateToId = -1;
    };
    UPROPERTY() TObjectPtr<AHomesteadController> Controller;
    TArray<FStep> Steps;
    TArray<FString> Results;
    int32 StepIndex = 0;
    int32 PresentationTraceStep = -1;
    uint64 LitGuardSamples = 0;
    bool bStarted = false;
    bool bActed = false;
    bool bFinished = false;
    bool bFullLoopPrepared = false;
    bool bWeedingPrepared = false;
    bool bBookStoragePrepared = false;
    bool bAudioCapture = false;
    bool bCompletionPending = false;
    bool bPendingSuccess = false;
    FString PendingReason;
    float CompletionStarted = 0;
    double LastFrameWallTime = 0;
    double IgnoreProfileUntil = 0;
    TArray<double> FrameMilliseconds;
    float Elapsed = 0;
    float StepElapsed = 0;
    float LastNavigationAt = -1;
    FVector MovementStart = FVector::ZeroVector;
    float CameraStart = 0;
    double PausedHour = 0;
    double HungerBeforeFood = 0;
    int32 BerriesBeforeFood = 0;
    int32 SavedBranches = 0;
    int32 StructuresBeforeBuild = 0;
    int32 GardenPlotId = -1;

    void Prepare();
    void PrepareGeneratedWorldChecks();
    void PrepareCreekChecks();
    void PrepareDirectionalNavigationChecks();
    void PrepareNativeMenuChecks();
    void PrepareCraftingChecks();
    void PrepareNativeWardrobeChecks();
    void PrepareNativeInventoryTransactionChecks();
    void PrepareNativeResetChecks();
    void PrepareNativePresentationCoverageChecks();
    void PrepareNativeResumeChecks(const FString& ProducerOutput);
    bool VerifyNativeMenuPresentation() const;
    void PreparePresentation();
    void PrepareGatheringChecks();
    void PrepareWateringChecks();
    void PrepareWeedingChecks();
    void PrepareClearingChecks();
    void PreparePromptChecks();
    void PrepareCameraPreferenceChecks();
    void PrepareVideoSyncChecks();
    void PrepareFeedbackChecks();
    void PrepareHotkeyChecks();
    void PrepareHotbarChecks();
    void PrepareBookClarityChecks();
    void PrepareBookStorageChecks();
    void QueueBookCapture(const FString& Name);
    bool VerifyPresentationMaterials() const;
    void PrepareFullLoop();
    void QueueSelectRow(int32 Id);
    void QueueGatherTo(Homestead::Item Item, int32 TargetCount);
    void QueueCraft(Homestead::Recipe Recipe);
    void QueuePlace(Homestead::Piece Kind, int32 CellX, int32 CellY, int32 Rotation = 0);
    void QueueClearCell(int32 CellX, int32 CellY);
    void QueueEat(Homestead::Item Item);
    void Add(const FString& Name, TFunction<void()> Action, TFunction<bool()> Check, float Wait = 0.35f);
    void Tap(FKey Key);
    void Axis(FKey Key, float Value);
    void Teleport(Homestead::Point Position);
    void QueueHarvest(int32 ResourceId, Homestead::Item ExpectedItem);
    void Screenshot(const FString& Name);
    void TraceState(const FString& Label);
    void Finish(bool Success, const FString& Reason);
};
