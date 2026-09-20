#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Simulation/HomesteadSimulation.h"
#include "HomesteadAppearance.h"
#include "HomesteadSaveRouting.h"
#include "HomesteadPromptIntent.h"
#include "HomesteadController.generated.h"

class AHomesteadWorld;
class UHomesteadSave;
class UAudioComponent;
class USoundBase;

struct FHomesteadRow
{
    int32 Id = 0;
    FString Label;
    FString Detail;
    FString Action;
    bool CanStore = false;
    bool CanTake = false;
};

UCLASS()
class SURVIVALGAME_API AHomesteadController : public APlayerController
{
    GENERATED_BODY()
public:
    AHomesteadController();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupInputComponent() override;
    virtual bool InputKey(const FInputKeyEventArgs& Params) override;

    bool IsBookOpen() const { return bBookOpen; }
    bool IsFailed() const { return Sim.GetState().failed; }
    bool IsPlanning() const { return bPlanning; }
    bool UsesGamepad() const { return bGamepad; }
    uint32 IgnoredExternalInputCount() const { return IgnoredExternalInputs; }
    const FHomesteadAppearance& GetAppearance() const { return Appearance; }
    bool HasHeroine() const;
    const Homestead::State& State() const { return Sim.GetState(); }
    const Homestead::Simulation& Simulation() const { return Sim; }
    int32 BookPage() const { return Page; }
    int32 SelectedRow() const { return Selection; }
    TArray<FHomesteadRow> Rows() const;
    FString BookTitle() const;
    FString BookSummary() const;
    FString BookFooter() const;
    FString FocusTitle() const;
    FString FocusActions() const;
    bool IsResourceFocused(int32 Id) const { return Focus == EFocus::Resource && FocusId == Id; }
    FString Toast() const { return ToastRemaining > 0 ? ToastText : FString(); }
    FString PlacementLabel() const;
    FString PreviewLabel() const;
    bool ToastIsError() const { return bToastError; }
    Homestead::Point PlayerPoint() const;
    void NudgePlacement(FVector2D Axis);

    float Sensitivity = 1.0f;
    bool bInvertY = false;
    float MusicVolume = 0.65f;
    float AmbienceVolume = 0.7f;
    float EffectsVolume = 0.8f;

private:
    friend class AHomesteadVisualPlaytest;
    friend class AHomesteadSmokeTest;
    enum class EFocus { None, Resource, Plot, Fire, Bed, Chest, Water };
    Homestead::Simulation Sim;
    FHomesteadAppearance Appearance;
    UPROPERTY() TObjectPtr<AHomesteadWorld> Landscape;
    UPROPERTY() TObjectPtr<UAudioComponent> Music;
    UPROPERTY() TObjectPtr<UAudioComponent> Ambience;
    UPROPERTY() TObjectPtr<USoundBase> GrassStepA;
    UPROPERTY() TObjectPtr<USoundBase> GrassStepB;
    UPROPERTY() TObjectPtr<USoundBase> WoodTapA;
    UPROPERTY() TObjectPtr<USoundBase> WoodTapB;
    UPROPERTY() TObjectPtr<USoundBase> UIClick;
    bool bAudioEnabled = true;
    bool bAutomatedInputOnly = false;
    bool bLoggedExternalInput = false;
    uint32 IgnoredExternalInputs = 0;
    uint32 TestQuickSaves = 0, TestQuickLoads = 0;
    bool bAlternateStep = false;
    FVector LastStepPosition = FVector::ZeroVector;
    float StepDistance = 0;
    bool bBookOpen = false;
    bool bPlanning = false;
    bool bGamepad = true;
    FHomesteadPromptIntent PromptIntent;
    bool bPendingSpawn = true;
    bool bConfirmRestart = false;
    bool bMusicFading = false;
    bool bWasFailed = false;
    int32 Page = 3;
    int32 Selection = 0;
    int32 AutoSaveIndex = 0;
    int32 BuildCellX = 0;
    int32 BuildCellY = 0;
    int32 BuildRotation = 0;
    Homestead::Piece BuildKind = Homestead::Piece::Foundation;
    EFocus Focus = EFocus::None;
    int32 FocusId = -1;
    float RefreshRemaining = 0;
    float ToastRemaining = 0;
    float AutosaveRemaining = 240;
    float MusicGapRemaining = 18;
    float MusicElapsed = 0;
    double LastNudgeTime = -1;
    FString ToastText;
    bool bToastError = false;
    FString SessionCheckpoint;
    FString WorldId;
    FHomesteadSaveRoute SaveRoute;
    bool bSaveRoutingReady = false;
    bool bSaveRoutingTestPending = false;
    FVector PendingLocation = FVector(-1000, 0, 180);
    FRotator PendingRotation = FRotator(-15, 15, 0);

    void Interact();
    void Secondary();
    void Withdraw();
    void Back();
    void ToggleBook();
    void OpenCraft();
    void OpenBuild();
    void OpenJournal();
    void PreviousPage();
    void NextPage();
    void PreviousRow();
    void NextRow();
    void RotatePlacement();
    void CycleZoom();
    void QuickSave();
    void QuickLoad();
    void ActivateRow();
    void ToggleVerticalSync();
    void OpenBook(int32 TargetPage);
    void CloseBook();
    void UpdateFocus();
    void BeginPlacement(Homestead::Piece Kind);
    void EndPlacement();
    void Notify(const Homestead::Result& Result, USoundBase* SuccessCue = nullptr);
    void Notify(const FString& Text, bool Error = false);
    void RetryCheckpoint();
    void NewGame();
    bool SaveSlot(const FString& Slot, bool Quiet = false);
    FString SavePath(const FString& Slot) const;
    void RunSaveRoutingChecks();
    bool LoadLatest(bool RecoveryOnly = false);
    UHomesteadSave* ReadSave(const FString& Filename) const;
    void ApplySave(const UHomesteadSave& Save);
    void InitializeAudio();
    void PlayEffect(USoundBase* Cue, float Gain = 0.12f);
    UFUNCTION() void MusicFinished();
};
