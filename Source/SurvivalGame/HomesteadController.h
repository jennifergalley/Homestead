#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Simulation/HomesteadSimulation.h"
#include "HomesteadAppearance.h"
#include "HomesteadSaveRouting.h"
#include "HomesteadPromptIntent.h"
#include "Styling/SlateBrush.h"
#include "HomesteadController.generated.h"

class AHomesteadWorld;
class UHomesteadSave;
class UAudioComponent;
class USoundBase;
namespace HomesteadMenus { class SHomesteadMenu; }
using SHomesteadMenu = HomesteadMenus::SHomesteadMenu;
class IInputProcessor;
class AHomesteadMenuPortrait;

enum class EHomesteadMenuSubject : uint8 { Legacy, ItemGroup, Wearable, GarmentRecipe };
enum class EHomesteadItemAction : uint8 { Primary, Transfer, Split, Merge, MoveEarlier, MoveLater, Equip, Unequip, Dye };

struct FHomesteadRow
{
    int32 Id = 0;
    FString Label;
    FString Detail;
    FString Action;
    bool CanStore = false;
    bool CanTake = false;
    EHomesteadMenuSubject Subject = EHomesteadMenuSubject::Legacy;
    int32 SubjectId = 0;
    int32 ContainerId = 0;
    int32 DestinationId = 0;
    int32 Quantity = 0;
    FString Name;
    FString Location;
    FName Icon;
    FLinearColor IconTint = FLinearColor(0.92f, 0.74f, 0.43f);
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
    uint32 PromptDeviceChangeCount() const { return PromptDeviceChanges; }
    uint32 IgnoredExternalInputCount() const { return IgnoredExternalInputs; }
    const FHomesteadAppearance& GetAppearance() const { return Appearance; }
    bool HasHeroine() const;
    const Homestead::State& State() const { return Sim.GetState(); }
    const Homestead::Simulation& Simulation() const { return Sim; }
    int32 BookPage() const { return Page; }
    int32 SelectedRow() const { return Selection; }
    TArray<FHomesteadRow> Rows() const;
    TArray<FHomesteadRow> MenuRows() const;
    void MenuInventoryView(int32 View);
    int32 InventoryView() const { return MenuInventoryViewIndex; }
    bool MenuItemAction(const FHomesteadRow& Row, EHomesteadItemAction Action, int32 Amount, uint64 ExpectedRevision);
    FString MenuInventorySummary() const;
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
    float GroundHeight(float X, float Y) const;
    bool PrepareWorldAt(Homestead::Point Position);
    bool IsWorldReady() const { return bWorldReady; }
    uint32 WorldRecoveryCount() const { return WorldRecoveries; }
    void NudgePlacement(FVector2D Axis);
    bool HasNativeMenu() const { return NativeMenu.IsValid(); }
    void MenuPage(int32 TargetPage);
    void MenuSelect(int32 Row);
    void MenuActivate();
    void MenuStore();
    void MenuTake();
    void MenuBack();
    void MenuRequestExit();
    void MenuSaveAndQuit();
    void MenuQuitWithoutSaving();
    void MenuRestart();
    void MenuRetry();
    bool MenuPhysicalInput(FKey Key, EInputEvent Event, float Amount = 1);
    bool MenuPointerIntent(float X, float Y);
    bool MenuAcceptsPhysicalInput() const { return !bAutomatedInputOnly || bSimulatedMenuEvent; }
    FString MenuSaveStatus() const;
    FString MenuLastError() const { return ToastText; }
    bool MenuNeedsTestReset() const { return bTestResetRequired; }
    FString MenuLoadProblem() const { return LoadProblem; }
    const FSlateBrush* MenuPortraitBrush() const { return MenuPortrait ? &PortraitBrush : nullptr; }
    void RefreshMenuPortrait();
    void OrbitMenuPortrait(float Degrees);
    void ZoomMenuPortrait();
    FString MenuPortraitStatus() const;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
    bool bSimulatedMenuEvent = false;
    bool bLoggedExternalInput = false;
    uint32 IgnoredExternalInputs = 0;
    uint32 TestQuickSaves = 0, TestQuickLoads = 0;
    bool bAlternateStep = false;
    FVector LastStepPosition = FVector::ZeroVector;
    float StepDistance = 0;
    bool bBookOpen = false;
    bool bPlanning = false;
    bool bGamepad = true;
    uint32 PromptDeviceChanges = 0;
    FHomesteadPromptIntent PromptIntent;
    bool bPendingSpawn = true;
    bool bFreshTerrainSpawn = true;
    bool bWorldReady = false;
    uint32 WorldRecoveries = 0;
    FVector LastSafeWorldPosition = FVector(-1000, 0, 180);
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
    FHomesteadAppearance SessionAppearance;
    FString SessionWorld;
    FVector SessionLocation = FVector(-1000, 0, 180);
    FRotator SessionRotation = FRotator(-15, 15, 0);
    void CaptureSessionCheckpoint(FVector Location, FRotator Rotation);
    FString WorldId;
    int32 MenuInventoryViewIndex = 0;
    mutable bool bReadIncompatible = false;
    bool bTestResetRequired = false;
    bool bHasPlayableSession = false;
    FString LoadProblem;
    TSharedPtr<SHomesteadMenu> NativeMenu;
    TSharedPtr<IInputProcessor> MenuPointerInput;
    UPROPERTY() TObjectPtr<AHomesteadMenuPortrait> MenuPortrait;
    FSlateBrush PortraitBrush;
    bool bMenuSaveInProgress = false;
    FDateTime LastSuccessfulSave;
    TOptional<float> PendingResolutionScale;
    FString GraphicsSaveError;
    bool PersistResolutionScale(float Requested);
    void ShowNativeMenu();
    void HideNativeMenu();
    FHomesteadSaveRoute SaveRoute;
    bool bSaveRoutingReady = false;
    bool bSaveRoutingTestPending = false;
    FString StartupProbeDirectory, StartupProbeExpectedState, StartupProbeLoadedState, StartupProbeWorld;
    double StartupProbeNext = 0, StartupProbeDeadline = 0;
    int32 StartupProbeStep = 0;
    uint64 StartupProbeLitTicks = 0;
    bool StartupProbeNativeMenuObserved = false;
    bool PrepareStartupProbe();
    void TickStartupProbe();
    void FinishStartupProbe(const FString& Error);
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
    bool ApplySave(const UHomesteadSave& Save);
    void InitializeAudio();
    void PlayEffect(USoundBase* Cue, float Gain = 0.12f);
    UFUNCTION() void MusicFinished();
};
