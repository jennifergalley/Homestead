#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Simulation/HomesteadSimulation.h"
#include "HomesteadAppearance.h"
#include "HomesteadSaveRouting.h"
#include "HomesteadPromptIntent.h"
#include "HomesteadMusicPlaylist.h"
#include "Styling/SlateBrush.h"
#include "HomesteadController.generated.h"

class AHomesteadWorld;
class UHomesteadSave;
class UAudioComponent;
class USoundBase;
namespace HomesteadMenus { class SHomesteadMenu; }
using SHomesteadMenu = HomesteadMenus::SHomesteadMenu;
namespace HomesteadMenus { class SHomesteadHotbar; }
using SHomesteadHotbar = HomesteadMenus::SHomesteadHotbar;
class IInputProcessor;
class AHomesteadMenuPortrait;
class SWidget;

enum class EHomesteadMenuSubject : uint8 { Legacy, ItemGroup, Wearable, GarmentRecipe, Recipe };
enum class EHomesteadItemAction : uint8 { Primary, Transfer, Split, Merge, MoveEarlier, MoveLater, Equip, Unequip, Dye, Drop, Pin };

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
    Homestead::RecipeAssessment RecipeState;
    bool HasRecipeState = false;
};

struct FHomesteadHotbarSlot
{
    int32 Index = 0;
    Homestead::Item Tool = Homestead::Item::Count;
    bool Assigned = false;
    bool Available = false;
    bool Selected = false;
    // Food pinned to the hotbar: left-click eats one. Count is how many are in the pack.
    bool Food = false;
    int32 Count = 0;
    FName Icon;
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
    // Bare-foot step on soil, called by UHomesteadFootstepNotify at each touchdown.
    void PlayFootstep(bool bLeftFoot, bool bRun);
    uint32 FootstepCount() const { return Footsteps; }

    bool IsBookOpen() const { return bBookOpen; }
    bool IsFailed() const { return Sim.GetState().failed; }
    bool IsPlanning() const { return bPlanning; }
    bool UsesGamepad() const { return bGamepad; }
    uint32 PromptDeviceChangeCount() const { return PromptDeviceChanges; }
    uint32 IgnoredExternalInputCount() const { return IgnoredExternalInputs; }
    const FHomesteadAppearance& GetAppearance() const { return Appearance; }
    bool HasHeroine() const;
    const Homestead::State& State() const { return Sim.GetState(); }
    Homestead::Result SpendSprintEnergy(double RealSeconds);
    const Homestead::Simulation& Simulation() const { return Sim; }
    int32 BookPage() const { return Page; }
    int32 SelectedRow() const { return Selection; }
    TArray<FHomesteadRow> Rows() const;
    TArray<FHomesteadRow> MenuRows() const;
    void MenuInventoryView(int32 View);
    int32 InventoryView() const { return MenuInventoryViewIndex; }
    bool MenuItemAction(const FHomesteadRow& Row, EHomesteadItemAction Action, int32 Amount, uint64 ExpectedRevision);
    bool MenuSplitHalf(const FHomesteadRow& Row);
    bool MenuSortPack();
    bool MenuDrop(const FHomesteadRow& Source, const FHomesteadRow& Target, uint64 ExpectedRevision);
    bool OpenChestStorage(int32 ChestId);
    TOptional<int32> ActiveStorageChest() const { return ActiveChestId; }
    bool MenuCraftRecipe(Homestead::Recipe Recipe);
    void MenuCraftBeat(int32 Beat);
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
    void MenuSave();
    void MenuSaveAndQuit();
    void MenuQuitWithoutSaving();
    void MenuRestart();
    void MenuRetry();
    bool MenuPhysicalInput(FKey Key, EInputEvent Event, float Amount = 1);
    bool MenuPointerButtonIntent(FKey Key);
    bool MenuPointerIntent(float X, float Y);
    bool MenuAcceptsPhysicalInput() const { return !bAutomatedInputOnly || bSimulatedMenuEvent; }
    FString MenuSaveStatus() const;
    FString MenuLastError() const { return ToastText; }
    bool MenuNeedsTestReset() const { return bTestResetRequired; }
    FString MenuLoadProblem() const { return LoadProblem; }
    void MenuSetGameSpeed(double DayMinutes);
    void MenuAdjustSetting(int32 Id, int32 Direction);
    float MenuAudioVolume(int32 Id) const;
    void MenuPreviewAudioVolume(int32 Id, float Value);
    bool MenuCommitAudioVolume(int32 Id, float Value, float Previous);
    bool IsAutosaveEnabled() const { return bAutosaveEnabled; }
    int32 AutosaveIntervalMinutes() const { return AutosaveMinutes; }
    void MenuSetAutosaveEnabled(bool Enabled);
    void MenuSetAutosaveInterval(int32 Minutes);
    const FSlateBrush* MenuPortraitBrush() const { return MenuPortrait ? &PortraitBrush : nullptr; }
    void RefreshMenuPortrait();
    void OrbitMenuPortrait(float Degrees);
    void ZoomMenuPortrait();
    FString MenuPortraitStatus() const;
    TArray<FHomesteadHotbarSlot> HotbarSnapshot() const;
    int32 SelectedHotbarIndex() const { return SelectedHotbarSlot; }
    void SelectHotbarSlot(int32 Index);
    void CycleHotbar(int32 Direction);
    // Tools and food can be pinned to the hotbar from the pack.
    static bool CanPinToHotbar(Homestead::Item Item);
    bool IsPinnedToHotbar(Homestead::Item Item) const;
    bool TogglePinnedToHotbar(Homestead::Item Item);
    void HoverHotbarSlot(int32 Index) { HoveredHotbarSlot = Index >= 0 && Index < 10 ? Index : INDEX_NONE; }
    bool KnifePreviewRequested() const;
    // The carried tool in the selected (or hovered) hotbar slot, or Item::Count.
    Homestead::Item PresentedTool() const;
    bool ShouldShowHotbar() const;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    // Console playtest aid: skip the clock ahead to the next morning (default 8:00) so there's light to see by.
    UFUNCTION(Exec) void HomesteadMorning(float Hour = 8.0f);
    // Console playtest aid: add items to her pack by name (spaces optional, e.g. HomesteadGive Berries 6).
    UFUNCTION(Exec) void HomesteadGive(const FString& ItemName, int32 Amount = 5);

    float Sensitivity = 1.0f;
    bool bInvertY = false;
    float MusicVolume = 0.65f;
    float AmbienceVolume = 0.7f;
    float EffectsVolume = 0.8f;

private:
    bool ResolveDropPoint(Homestead::Point& Result) const;
    bool CollectPreparedBaselines(Homestead::Generation::ChunkCoord Chunk,
        std::array<const Homestead::Generation::ChunkBaseline*, 9>& Prepared) const;
    friend class AHomesteadVisualPlaytest;
    friend class AHomesteadSmokeTest;
    enum class EFocus { None, Resource, Drop, Plot, Fire, Bed, Chest, Water, Underbrush };
    Homestead::Simulation Sim;
    FHomesteadAppearance Appearance;
    UPROPERTY() TObjectPtr<AHomesteadWorld> Landscape;
    UPROPERTY() TObjectPtr<UAudioComponent> Music;
    UPROPERTY() TObjectPtr<UAudioComponent> Ambience;
    UPROPERTY() TObjectPtr<USoundBase> GrassStepA;
    UPROPERTY() TObjectPtr<USoundBase> GrassStepB;
    UPROPERTY() TArray<TObjectPtr<USoundBase>> BareWalkSteps;
    UPROPERTY() TArray<TObjectPtr<USoundBase>> BareRunSteps;
    UPROPERTY() TArray<TObjectPtr<USoundBase>> MusicTracks;
    TArray<float> MusicTrackGains;
    TArray<FString> MusicTrackNames;
    Homestead::MusicShuffleBag MusicBag;
    int32 MusicTrack = INDEX_NONE;
    // Music component level: the Settings music volume times the current track's level match.
    float MusicLevel() const;
    void StartNextMusicTrack();
    UPROPERTY() TObjectPtr<USoundBase> WoodTapA;
    UPROPERTY() TObjectPtr<USoundBase> WoodTapB;
    UPROPERTY() TObjectPtr<USoundBase> CraftStrikeA;
    UPROPERTY() TObjectPtr<USoundBase> CraftStrikeB;
    UPROPERTY() TObjectPtr<USoundBase> CraftStrikeC;
    UPROPERTY() TObjectPtr<USoundBase> UIClick;
    bool bAudioEnabled = true;
    bool bAutomatedInputOnly = false;
    bool bSimulatedMenuEvent = false;
    bool bControlDown = false;
    bool bLoggedExternalInput = false;
    uint32 IgnoredExternalInputs = 0;
    uint32 TestQuickSaves = 0, TestQuickLoads = 0;
    uint32 TestCraftBeatRequests = 0, TestAudibleCraftBeats = 0;
    bool bAlternateStep = false;
    FVector LastStepPosition = FVector::ZeroVector;
    float StepDistance = 0;
    double LastFootstepTime = -1;
    int32 LastBareStep = INDEX_NONE;
    uint32 Footsteps = 0;
    bool bBookOpen = false;
    bool bPlanning = false;
    bool bGamepad = true;
    uint32 PromptDeviceChanges = 0;
    FHomesteadPromptIntent PromptIntent;
    bool bPendingSpawn = true;
    bool bFreshTerrainSpawn = true;
    bool bWorldReady = false;
    double LastRegionSimulationMilliseconds = 0;
    double LastRegionWorldMilliseconds = 0;
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
    // The underbrush plant in focus while the machete is selected (EFocus::Underbrush).
    FIntPoint FocusBrushChunk = FIntPoint::ZeroValue;
    int32 FocusBrushIndex = INDEX_NONE;
    uint8 FocusBrushSpecies = 0;
    FVector2D FocusBrushPosition = FVector2D::ZeroVector;
    bool bFocusBrushWoody = false;
    // A machete hack in progress: the plant is cleared when her second stroke lands, so it
    // stands until then and an interrupted hack changes nothing.
    bool bHackPending = false;
    FIntPoint HackChunk = FIntPoint::ZeroValue;
    int32 HackIndex = INDEX_NONE;
    FVector2D HackPosition = FVector2D::ZeroVector;
    bool bHackWoody = false;
    double HackSince = 0;
    void UpdatePendingHack();
    void StartMacheteHack();
    // Felling in progress: the tree is already cleared; its standing copy topples after the last
    // stroke (or at once if she stops), with a chop sound per stroke.
    int32 FellResource = INDEX_NONE;
    int32 FellStrokes = 0, FellStrokesHeard = 0;
    uint32 FellStartsBefore = 0;
    bool bFellSeen = false;
    double FellSince = 0;
    // Present a committed tree or sapling clear: the felling clip when she has it, else PlayClear.
    void PresentFelling(int32 ResourceId, Homestead::Point Target, bool bTree);
    void UpdatePendingFell();
    float RefreshRemaining = 0;
    float ToastRemaining = 0;
    float AutosaveRemaining = 240;
    bool bAutosaveEnabled = true;
    int32 AutosaveMinutes = 5;
    // Gathered Branches pile kept visible until the kneeling pickup lifts the last stick.
    int32 HeldStickPile = INDEX_NONE;
    double HeldStickPileSince = 0;
    // Ground parts of the held produce to hide at the first pickup (the rest go with the second).
    int32 HeldPartsFirst = 0, HeldPartsCount = 0;
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
    TOptional<int32> ActiveChestId;
    mutable bool bReadIncompatible = false;
    bool bTestResetRequired = false;
    bool bHasPlayableSession = false;
    FString LoadProblem;
    TSharedPtr<SHomesteadMenu> NativeMenu;
    TSharedPtr<SHomesteadHotbar> HotbarWidget;
    TSharedPtr<SWidget> HotbarRoot;
    TSharedPtr<IInputProcessor> MenuPointerInput;
    UPROPERTY() TObjectPtr<AHomesteadMenuPortrait> MenuPortrait;
    FSlateBrush PortraitBrush;
    bool bMenuSaveInProgress = false;
    FDateTime LastSuccessfulSave;
    TOptional<float> PendingResolutionScale;
    FString GraphicsSaveError;
    void LoadCameraPreferences();
    void LoadUserPreferences();
    bool PersistCameraSensitivity(float Requested);
    bool PersistCameraInversion(bool Requested);
    bool PersistAudioVolume(int32 Id, float Requested, float Previous);
    bool PersistAutosaveEnabled(bool Requested);
    bool PersistAutosaveInterval(int32 Requested);
    bool PersistResolutionScale(float Requested);
    void ShowNativeMenu();
    void HideNativeMenu();
    void ShowHotbar();
    void HideHotbar();
    void ResetHotbar();
    // Layout is the save's HotbarLayout: older hotbars gain the machete and berries once.
    void SanitizeHotbar(const TArray<int32>& Slots, int32 Selected, int32 Layout);
    void EatFromHotbar(Homestead::Item Food);
    // Jenny's playtest kit (tools, bed, two chests; seeds on new games). Skipped in automation.
    void GrantPlaytestKit(bool bNewGame);
    void UseSelectedTool();
    void NotifyResourceAction(const Homestead::Result& Result, USoundBase* SuccessCue);
    TArray<int32> HotbarSlots;
    int32 SelectedHotbarSlot = 0;
    int32 HoveredHotbarSlot = INDEX_NONE;
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
    void OpenFocusedChestWithMouse();
    void Secondary();
    void Withdraw();
    void Back();
    void ToggleBook();
    void OpenSettings();
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
