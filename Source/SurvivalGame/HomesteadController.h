#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Simulation/HomesteadSimulation.h"
#include "Simulation/HomesteadHoldings.h"
#include "Simulation/HomesteadTravel.h"
#include "HomesteadAppearance.h"
#include "HomesteadSaveRouting.h"
#include "HomesteadPromptIntent.h"
#include "HomesteadMusicPlaylist.h"
#include "UI/HomesteadHudTiming.h"
#include "Styling/SlateBrush.h"
#include "Templates/UniquePtr.h"
#include "HomesteadController.generated.h"

class AHomesteadWorld;
class AActor;
class UHomesteadSave;
class UWorldPartitionStreamingSourceComponent;
class UAudioComponent;
class USoundBase;
namespace HomesteadMenus { class SHomesteadMenu; }
using SHomesteadMenu = HomesteadMenus::SHomesteadMenu;
namespace HomesteadMenus { class SHomesteadHotbar; }
using SHomesteadHotbar = HomesteadMenus::SHomesteadHotbar;
namespace HomesteadMenus { class SHomesteadShop; }
class AHomesteadGeneralStore;
namespace HomesteadMenus { class SHomesteadNames; class SHomesteadArrival; }
class IInputProcessor;
class AHomesteadMenuPortrait;
class UHomesteadMapComponent;
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
    // A short state shown after the selected item's name in the pack's footer ("Water 5 / 6").
    FString Status;
    // The hotbar cell (0-9) holding this pack row, the first row of her pack; INDEX_NONE below it.
    int32 HotbarCell = INDEX_NONE;
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
    // Food in the cell: left-click eats one. Count is the cell's stack.
    bool Food = false;
    int32 Count = 0;
    // A material (or anything else that isn't a tool, food or seed): shows its count, does nothing.
    bool Material = false;
    // A garment carried in the cell (Tool stays Item::Count).
    bool Garment = false;
    FName Icon;
    // A level shown as a thin bar along the slot's foot (the lamp's oil), 0-1; negative for none.
    float Fill = -1.0f;
    // Sowing seed: shows how many she has; Pouch when other seed types can be switched in.
    bool Seed = false;
    bool Pouch = false;
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
    bool IsDeconstructing() const { return bPlanning && bDeconstructing; }
    int32 DeconstructTarget() const { return DeconstructId; }
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
    // Walking the public road to the manor or town (HomesteadControllerTravel.cpp): the preview from
    // where she stands, and the walk itself (the clock runs for its length; she's stood at the end).
    Homestead::TravelPlan MenuPlanTravel(Homestead::TravelDestination Destination) const;
    bool MenuTravel(Homestead::TravelDestination Destination, uint64 ExpectedRevision);
    // The dye chooser's live preview: shows her wearing the garment in `Dye` through the real wardrobe
    // path without changing anything saved; MenuEndDyePreview puts her own clothes back.
    bool MenuPreviewDye(int32 WearableId, int32 Dye);
    void MenuEndDyePreview();
    // The Appearance page's camera (orbit in degrees, zoom in wheel steps); nothing outside it.
    void MenuOrbitAppearance(float Yaw, float Pitch);
    void MenuZoomAppearance(float Steps);
    bool MenuSplitHalf(const FHomesteadRow& Row);
    // Moves a whole stack or garment between the pack and the open chest (as much as fits).
    bool MenuMoveWhole(const FHomesteadRow& Row);
    // The menu row for one owned garment wherever it is (worn, carried or stored).
    bool MenuWearableRow(int32 WearableId, FHomesteadRow& Out) const;
    bool MenuSortPack();
    // The open chest (HomesteadControllerChests.cpp): its name ("Storage chest" until she names it),
    // auto-store onto its matching stacks, and naming it (empty puts the default back).
    FString ChestDisplayName(int32 ChestId) const;
    bool MenuStoreMatching();
    bool MenuRenameChest(const FString& Name);
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
    // The floating action hints retire once each action has been done this many times; the counts
    // live in the user settings file, so they outlast saves and new woodlands.
    static constexpr int32 HintRetireUses = 3;
    // Stable id for a focus-cue verb ("Gather", "Fell with Hatchet") on the current focus.
    FString HintId(const FString& Verb) const;
    bool IsHintRetired(const FString& Verb) const;
    int32 HintUseCount(const FString& Verb) const;
    void ResetActionHints();
    // The top-left controls strip shows for its first minute on screen (UI/HomesteadHudTiming.h).
    bool ShowsControlsHint() const { return ControlsHint.Showing(); }
    double ControlsHintSecondsLeft() const { return ControlsHint.Remaining(); }
    bool IsControlsHintOnScreen() const;
    bool IsResourceFocused(int32 Id) const { return Focus == EFocus::Resource && FocusId == Id; }
    FString Toast() const { return ToastRemaining > 0 ? ToastText : FString(); }
    // Seconds the current toast has left, and a count of Notify calls (tells a repeated message apart).
    float ToastSecondsLeft() const { return ToastRemaining; }
    uint32 NoticeCount() const { return NoticeSerial; }
    FString PlacementLabel() const;
    // Whether the preview snaps, stands free, or why it can't be built there.
    FString PlacementStatus() const;
    // The bed's choices (Homestead::SleepOptions) for her Energy now, and the one the prompt shows:
    // the default first, or what she picked with the D-pad (Up/Down) while at this bed.
    std::vector<Homestead::SleepOption> BedSleepOptions() const;
    int32 BedSleepIndex() const;
    double BedSleepHours() const;
    // "Sleep until morning (wake 06:45)", "Sleep until rested (wake ~14:30)", "Nap 1 h (wake 23:15)".
    static FString SleepOptionLabel(const Homestead::SleepOption& Option);
    FString PreviewLabel() const;
    bool ToastIsError() const { return bToastError; }
    Homestead::Point PlayerPoint() const;
    float GroundHeight(float X, float Y) const;
    bool PrepareWorldAt(Homestead::Point Position);
    // True on the fixed Estate map, where the Landscape and baked placements replace the generated woodland.
    bool IsEstateMap() const { return bEstateMap; }
    // Distance (cm) from a point to the nearest drawable water's edge; <= 0 is in the water.
    double WaterEdgeDistance(Homestead::Point Position, bool bIncludeSea = true) const;
    bool IsWorldReady() const { return bWorldReady; }
    uint32 WorldRecoveryCount() const { return WorldRecoveries; }
    // Where the construction preview currently resolves (snapped or free-standing).
    const Homestead::PlacementTarget& CurrentPlacement() const { return BuildTarget; }
    bool IsPlacementValid() const { return bBuildValid; }
    bool HasNativeMenu() const { return NativeMenu.IsValid(); }
    // The held Craft recipe's progress through its cycle (0-1), or 0 when nothing is being crafted.
    float CraftProgress() const;
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
    bool MenuAcceptsPhysicalInput() const { return !bPendingGroundSnap && (!bAutomatedInputOnly || bSimulatedMenuEvent); }
    FString MenuSaveStatus() const;
    FString MenuLastError() const { return ToastText; }
    bool MenuNeedsTestReset() const { return bTestResetRequired; }
    FString MenuLoadProblem() const { return LoadProblem; }
    void MenuSetGameSpeed(double DayMinutes);
    void MenuAdjustSetting(int32 Id, int32 Direction);
    // Appearance rows: the next (+1) or previous (-1) choice, applied to her at once.
    void MenuStepAppearance(int32 Id, int32 Direction);
    void MenuSetAppearance(int32 Id, int32 Value);
    void MenuFocusAppearance(int32 Id);
    static int32 AppearanceChoiceCount(int32 Id);
    int32 AppearanceChoice(int32 Id) const;
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
    // The hotbar is the first row of her pack, as in Coral Island (Simulation/HomesteadPackRow.h):
    // ten cells holding her real carried stacks, no pins. The item in a cell (Item::Count when empty
    // or holding a garment), the carried entry itself, and the first cell holding an item.
    Homestead::Item HotbarItem(int32 Cell) const;
    const Homestead::LayoutEntry* HotbarEntry(int32 Cell) const;
    int32 HotbarCellOf(Homestead::Item Item) const;
    int32 FirstEmptyHotbarCell() const;
    // The field book's hotbar (HomesteadControllerHotbarEditor.cpp): put a pack or chest row in a
    // cell (onto an empty cell it moves, onto the same item it merges, else the two swap), move one
    // cell onto another, or move a cell's stack below the row (onto a stack there, or to the end
    // with no target). Stock is only ever moved, and the selected cell index stays where it is.
    // False (with an explanatory notice) when refused.
    bool MenuPlaceInHotbar(const FHomesteadRow& Row, int32 Cell);
    bool MenuMoveHotbarSlot(int32 From, int32 To);
    bool MenuMoveHotbarToPack(int32 Cell, const FHomesteadRow* Target);
    // The menu row for what `Cell` holds; false when it is empty.
    bool MenuHotbarRow(int32 Cell, FHomesteadRow& Out) const;
    // The menu row for one layout entry of `Container` (0 = her pack); false for the pail's hidden water.
    bool MenuEntryRow(const Homestead::LayoutEntry& Entry, int32 Container, FHomesteadRow& Row) const;
    // Moves her first stack of `Item` into the first empty cell if it isn't in the row, and selects
    // its cell, as a player would (tests, and choosing seed to sow). False when she has none or the
    // row is full.
    bool ChooseOnHotbar(Homestead::Item Item);
    // Seed pouch: a hotbar slot holding sowing seed steps through every seed type in her pack
    // (D-pad up/down, or Q / Shift+Q), so one slot carries them all. Returns false when the selected
    // slot isn't seed, so the D-pad can do its other jobs.
    static bool IsSowingSeed(Homestead::Item Item);
    bool CycleSeedPouch(int32 Direction);
    void NextSeed();
    // Other seed types the selected seed slot can switch to (0 when it isn't a seed slot).
    int32 OtherPouchSeeds(int32 SlotIndex) const;
    // "   [D-pad] Other seeds" after a prompt when the selected slot's pouch has more.
    FString SeedPouchHint() const;
    void HoverHotbarSlot(int32 Index) { HoveredHotbarSlot = Index >= 0 && Index < 10 ? Index : INDEX_NONE; }
    bool KnifePreviewRequested() const;
    // The carried tool in the selected (or hovered) hotbar slot, or Item::Count.
    Homestead::Item PresentedTool() const;
    // The carried tool in the selected slot regardless of menus (she keeps holding it while the
    // field book is open, so its preview shows it), or Item::Count.
    Homestead::Item SelectedCarriedTool() const;
    bool ShouldShowHotbar() const;
    // The estate map's snapshot, minimap and boundary toast.
    UHomesteadMapComponent* MapPresenter() const { return Map; }
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    // Console playtest aid: skip the clock ahead to the next morning (default 8:00) so there's light to see by.
    UFUNCTION(Exec) void HomesteadMorning(float Hour = 8.0f);
    // Console playtest aid: let whole days pass so crops actually grow (HomesteadMorning only moves
    // the clock). Tend 1 waters and weeds every plot as the days pass; 0 leaves them to dry out.
    UFUNCTION(Exec) void HomesteadGrowCrops(float Days = 1.0f, int32 Tend = 1);
    // Console playtest aid for screenshots: set every planted plot to this growth (0-1).
    UFUNCTION(Exec) void HomesteadCropGrowth(float Growth = 1.0f);
    // Console playtest aid: add items to her pack by name (spaces optional, e.g. HomesteadGive Berries 6).
    UFUNCTION(Exec) void HomesteadGive(const FString& ItemName, int32 Amount = 5);
    // Console playtest aid (woodland games): raise the manor's standing room around her, its
    // doorway ahead, as a new estate game would, and replay the arrival title card.
    UFUNCTION(Exec) void HomesteadStandingRoom();
    // Console playtest aid: run the new-game setup (Appearance, then Names, then the arrival card)
    // on the current game without resetting it.
    UFUNCTION(Exec) void HomesteadNewGameSetup();
    // The new-game flow after a new estate game: Appearance, then the Names step, then the arrival.
    void BeginNewGameSetup();
    bool IsNamingSetup() const { return NamesWidget.IsValid(); }
    bool IsNewGameSetup() const { return bNewGameSetup; }
    TSharedPtr<HomesteadMenus::SHomesteadNames> NamesStep() const { return NamesWidget; }
    // The estate title card over the first view; it never blocks input.
    void ShowArrival();
    bool IsArrivalShowing() const { return ArrivalCard.IsValid(); }
    // "Eleanor Cavendish — Trevennor, Spring 1" for the current game (empty for unnamed woodland).
    FString CurrentSaveLabel() const;
    // Console playtest aid: make one garment from freshly granted materials and put it on
    // (key or name, e.g. HomesteadWear fur-coat).
    UFUNCTION(Exec) void HomesteadWear(const FString& Garment);
    // Console playtest aid: open the pack's right-click menu (Mode 0) or Ctrl+click popover (Mode 1)
    // on the Nth pack tile, as a pointer would. Mode 2 opens the nearest storage chest (logging where
    // it is) when she stands within reach of it.
    UFUNCTION(Exec) void HomesteadPackMenu(int32 Tile = 0, int32 Mode = 0);

    // General store (HomesteadShopFlow.cpp).
    bool IsShopScreenOpen() const { return ShopScreen.IsValid(); }
    TSharedPtr<HomesteadMenus::SHomesteadShop> GetShopScreen() const { return ShopScreen; }
    // Greets her (counted) and opens the shop screen, pausing the game.
    void OpenShopScreen(int32 ShopId, bool bGreet = true);
    void CloseShopScreen();
    // One Sell or Buy at the shop's counter; shows the wallet delta on success.
    Homestead::Result ShopTrade(int32 ShopId, Homestead::Item Item, int32 Quantity, bool bSell, bool bHeroineStock);
    // The one-time leather backpack (Simulation/HomesteadBackpack.h).
    Homestead::Result ShopBuyBackpack(int32 ShopId);
    void ShopClick();
    void NoteShopDevice(bool bPad);
    // The name she gave the estate ("the estate" in woodland games), for "From {Estate}" and toasts.
    FString EstateName() const;
    // The signed change of the last trade and how visible its readout still is (1 fresh, 0 gone).
    int64 WalletDelta() const { return LastWalletDelta; }
    float WalletDeltaAlpha() const { return FMath::Clamp(WalletDeltaRemaining / 1.0f, 0.0f, 1.0f); }
    // What the last meal from the hotbar actually added to food and energy (after caps), for the
    // vitals' "+N" popups; Serial counts meals so a repeat of the same gain still shows.
    struct FMealGain { double Food = 0, Energy = 0; uint32 Serial = 0; };
    const FMealGain& LastMealGain() const { return MealGain; }
    // Items she has just gained (gathered, harvested, crafted, bought; not moved out of a chest or
    // picked back up), for the "+3 Berries" popup beside her (HomesteadControllerPickups.cpp,
    // UI/SHomesteadPickups). Shown is how long each has been on screen, in real seconds.
    struct FPickup { Homestead::Item Item = Homestead::Item::Count; int32 Amount = 0; float Shown = 0.0f; };
    const TArray<FPickup>& RecentPickups() const { return Pickups; }
    bool PickupsVisible() const;
    // Where the popup hangs from: her upper body projected to the viewport, in pixels.
    bool PickupAnchor(FVector2D& Pixel, FVector2D& ViewportPixels) const;
    // Sprint was asked for (or ran out) with too little Energy: a gentle notice, not a failure.
    void SprintTooTired();
    // Keyboard sprint: a tap of Shift toggles it on release, unless Shift was a modifier (Shift+Q,
    // Shift+click) meanwhile. Movement keys don't count, so Shift+W still toggles.
    void TrackSprintShift(const FInputKeyEventArgs& Params);
    // The selected hotbar slot's food, even when she has none left (Item::Count if it isn't food).
    Homestead::Item SelectedHotbarFood() const;
    // Console playtest aid: open the general store on the ground ahead of her (moving it if it exists).
    UFUNCTION(Exec) void HomesteadOpenStore();
    // Console playtest aid: add (or with a negative amount remove) coins from her purse.
    UFUNCTION(Exec) void HomesteadMoney(int32 Cents = 1000);
    // Playtest aid: set her Energy (0-100), e.g. to try dozing off or the bed's "until rested".
    UFUNCTION(Exec) void HomesteadEnergy(float Energy = 100.0f);
    // Playtest aid: tip the water out of her pail, e.g. to try filling it at the river again.
    UFUNCTION(Exec) void HomesteadEmptyPail();
    // Playtest aids for the bed (stand beside one): sleep with choice N as listed in the prompt (-1 =
    // the one shown), or step the shown choice by Delta, as Up/Down (D-pad) do.
    UFUNCTION(Exec) void HomesteadSleep(int32 Option = -1);
    UFUNCTION(Exec) void HomesteadBedChoice(int32 Delta = 1);
    // Console playtest aid: move her to world X,Y and stand her on the ground there (waiting for the
    // ground's collision to stream in). Give Z to land on the first surface at or below Z instead
    // (an upper floor, say); omit it for the terrain.
    UFUNCTION(Exec) void HomesteadTeleport(float X, float Y, float Z = -1000000.0f);
    // Oil lamp (add-oil-lamp): fill the lamp from a flask (the flask's pack menu, or F / X in hand).
    void MenuRefillLamp();
    // Console playtest aid: set the lamp's oil in game hours (0-6), e.g. HomesteadLampOil 0.5.
    UFUNCTION(Exec) void HomesteadLampOil(float Hours = 6.0f);

    float Sensitivity = 1.0f;
    bool bInvertY = false;
    float MusicVolume = 0.65f;
    float AmbienceVolume = 0.7f;
    float EffectsVolume = 0.8f;
    // Scales every sound through the audio device's primary volume.
    float MasterVolume = 1.0f;
    void ApplyMasterVolume() const;

private:
    struct FHintUse { FString Id; uint32 Serial = 0; bool bHackPending = false; };
    // Before an action button is handled: which hint (if any) that button's cue shows now.
    FHintUse BeginHintUse(const FString& Button) const;
    // After: count it when the action succeeded (a non-error notice, or a machete swing began).
    void EndHintUse(const FHintUse& Use);
    void LoadActionHints();
    TMap<FString, int32> HintUses;
    uint32 NoticeSerial = 0;
    bool ResolveDropPoint(Homestead::Point& Result) const;
    bool CollectPreparedBaselines(Homestead::Generation::ChunkCoord Chunk,
        std::array<const Homestead::Generation::ChunkBaseline*, 9>& Prepared) const;
    friend class AHomesteadVisualPlaytest;
    friend class AHomesteadSmokeTest;
    friend class AHomesteadGardenProbe;
    friend class UHomesteadMapComponent;
    enum class EFocus { None, Resource, Drop, Plot, Fire, Bed, Chest, Water, Underbrush, Shopkeeper, StoreDoor, Hearth };
    // General store (HomesteadShopFlow.cpp).
    TSharedPtr<HomesteadMenus::SHomesteadShop> ShopScreen;
    UPROPERTY() TArray<TObjectPtr<AHomesteadGeneralStore>> Stores;
    int64 LastWalletDelta = 0;
    float WalletDeltaRemaining = 0.0f;
    FMealGain MealGain;
    TArray<FPickup> Pickups;
    // The pack and everything she owns (pack, chests, dropped) at PickupRevision: a gain raises both,
    // a move between them raises only the pack.
    Homestead::Holdings PickupHoldings;
    uint64 PickupRevision = 0;
    bool bPickupsPrimed = false;
    void UpdatePickups(float DeltaSeconds);
    TSharedPtr<SWidget> PickupsRoot;
    bool bSprintShiftDown = false;
    bool bSprintShiftModifier = false;
    // A/X with food selected and nothing to interact with: eat one (or say none is left).
    bool EatSelectedFoodInstead();
    void SyncStores();
    void TickStores(float DeltaSeconds);
    void ConsiderStoreFocus(TFunctionRef<void(EFocus, int32, Homestead::Point)> Consider) const;
    FString StoreFocusTitle() const;
    FString StoreFocusActions() const;
    void InteractWithStore();
    // Waiting at a closed shop's door: A asks, a second A (or B to cancel) answers.
    int32 WaitShopId = INDEX_NONE;
    double WaitAskedAt = 0.0;
    bool IsShopWaitArmed() const;
    bool CancelShopWait();
    FString GreetingFor(const Homestead::Shop& Shop) const;
    Homestead::Simulation Sim;
    FHomesteadAppearance Appearance;
    UPROPERTY() TObjectPtr<AHomesteadWorld> Landscape;
    UPROPERTY() TObjectPtr<UHomesteadMapComponent> Map;
    UPROPERTY() TObjectPtr<UAudioComponent> Music;
    UPROPERTY() TObjectPtr<UAudioComponent> Ambience;
    // The creek's burble: a looping, attenuated source kept at the point of the stream nearest
    // the listener, so it swells as she walks up to the water and fades into the woods.
    UPROPERTY() TObjectPtr<UAudioComponent> Creek;
    static constexpr float CreekGain = 0.35f;
    void UpdateCreekAudio();
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
    // Hatchet biting a standing trunk, one per stroke in turn.
    UPROPERTY() TArray<TObjectPtr<USoundBase>> ChopStrokes;
    UPROPERTY() TObjectPtr<USoundBase> TreeFallThud;
    // The scythe's one swish per sweep that cuts something (Scripts/generate_scythe_sound.py); never a footstep in its place.
    UPROPERTY() TObjectPtr<USoundBase> ScytheSwish;
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
    bool bDeconstructing = false;
    int32 DeconstructId = INDEX_NONE;
    bool bGamepad = true;
    uint32 PromptDeviceChanges = 0;
    FHomesteadPromptIntent PromptIntent;
    bool bPendingSpawn = true;
    bool bFreshTerrainSpawn = true;
    bool bEstateMap = false;
    float EstateSpawnWait = 0;
    // A teleport or fall recovery waiting to put her feet on the ground at X,Y (Z: search down from).
    bool bPendingGroundSnap = false;
    FVector GroundSnapTarget = FVector::ZeroVector;
    float GroundSnapWait = 0;
    FVector GroundSnapSafePosition = FVector::ZeroVector;
    FRotator GroundSnapSafeRotation = FRotator::ZeroRotator;
    FRotator GroundSnapSafeActorRotation = FRotator::ZeroRotator;
    double GroundSnapStartedAt = 0;
    double GroundSnapLastReportAt = 0;
    double GroundSnapLastInputNoticeAt = -1000.0;
    TUniquePtr<Homestead::Simulation> GroundSnapTravelBefore;
    UPROPERTY(Transient)
    TObjectPtr<AActor> GroundSnapStreamingActor;
    UPROPERTY(Transient)
    TObjectPtr<UWorldPartitionStreamingSourceComponent> GroundSnapStreamingSource;
    void BeginGroundSnap(FVector Target);
    void EndGroundSnap();
    void AbortGroundSnap();
    bool RejectPendingGroundSnapAction();
    // Oil lamp: the kneel to set it down at LampSpot, or take up the set-down lamp LampDropId,
    // commits when her hand reaches the ground (AHomesteadCharacter::ConsumeLampContact).
    enum class ELampHandoff : uint8 { None, SetDown, PickUp };
    ELampHandoff LampHandoff = ELampHandoff::None;
    Homestead::Point LampSpot{};
    int32 LampDropId = 0;
    double LastLampOil = -1.0;
    bool bLampWasInHand = false;
    void UpdateLamp();
    void StartLampSetDown();
    bool StartLampPickUp(int32 DropId);
    // Finds the surface she'd stand on below Target.Z at Target.X/Y and sets Target.Z to her capsule
    // centre on it. Until the ground there has collision (World Partition still streaming) it holds her
    // in the air with movement off, so she never drops from a height, and returns false.
    bool SettleOnGround(FVector& Target, float& Waited, float DeltaSeconds, float HoldLimitSeconds, const TCHAR* Why);
    mutable TArray<TWeakObjectPtr<class USplineComponent>> EstateWaterSplines;
    mutable double EstateWaterScanTime = -1000;
    void PrepareEstateSimulation(Homestead::Simulation& Target) const;
    void SetEstateSpawn();
    bool bWorldReady = false;
    double LastRegionSimulationMilliseconds = 0;
    double LastRegionWorldMilliseconds = 0;
    uint32 WorldRecoveries = 0;
    FVector LastSafeWorldPosition = FVector(-1000, 0, 180);
    bool bConfirmRestart = false;
    bool bMusicFading = false;
    bool bWasFailed = false;
    int32 Page = 0;
    int32 Selection = 0;
    int32 AutoSaveIndex = 0;
    // Quarter turns for pieces snapped onto a building; free-standing pieces turn by BuildYawOffset.
    int32 BuildRotation = 0;
    double BuildYawOffset = 0.0;
    Homestead::PlacementTarget BuildTarget;
    bool bBuildValid = false;
    FString BuildBlocker;
    FString BuildCheckKey;
    double LastBuildCheckTime = -1.0;
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
    // Overgrowth work (add-overgrown-estate-clearing). Blows in progress live here, not in the save:
    // the node clears on the swing that lands its last blow, and walking off resets the count.
    int32 SwingNode = INDEX_NONE;
    int32 SwingsLanded = 0;
    Homestead::Item SwingTool = Homestead::Item::Count;
    FVector2D SwingFrom = FVector2D::ZeroVector;
    bool bSwingPending = false;
    // The swing plays a felling-timed clip (axe chop or ground strike, pick strike, scythe mow).
    bool bSwingFellTimed = false;
    double SwingSince = 0;
    uint32 SwingFellStartsBefore = 0;
    // The hack clip's start counter at the press: only a hack started after it lands this swing.
    uint32 SwingHackStartsBefore = 0;
    // Blows this press plays in one go (the pickaxe strikes every blow a rock still needs), and how many
    // have landed so far; each counts at its own contact, and the rock clears on the last.
    int32 SwingStrokes = 1;
    // A tier-1 boulder needs five: every blow of it in one press would be too long a clip to cancel into.
    static constexpr int32 MaxPickStrokesPerPress = 3;
    int32 SwingStrokesLanded = 0;
    // The scythe's sweep: every grass and weed tuft in the forward arc when it began.
    TArray<int32> ScytheTargets;
    void SwingAtOvergrowth(Homestead::Item Tool);
    void UpdatePendingSwing();
    // bMoreComing: another blow of the same press follows, so no 'N more swings' notice in between.
    void LandOvergrowthSwing(bool bMoreComing = false);
    void ResetOvergrowthSwing();
    // HomesteadControllerWeedPull.cpp: weeds pulled by hand on both knees (a weed node, or a garden
    // square's weeds), committed once at the second root (AHomesteadCharacter::PullWeedsCommit). A
    // cancel before then changes nothing. False when the clip can't play, so the caller uses the pouch kneel.
    bool StartWeedPull(int32 NodeId, int32 PlotId, Homestead::Point Target);
    void UpdatePendingWeedPull();
    int32 PendingWeedNode = INDEX_NONE;
    int32 PendingWeedPlot = INDEX_NONE;
    double PendingWeedSince = 0;
    bool bPendingWeedStarted = false;
    uint32 PendingWeedStartsBefore = 0;
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
    HomesteadHud::ControlsHintWindow ControlsHint;
    float AutosaveRemaining = 240;
    bool bAutosaveEnabled = true;
    int32 AutosaveMinutes = 5;
    // Gathered Branches pile kept visible until the kneeling pickup lifts the last stick.
    int32 HeldStickPile = INDEX_NONE;
    int32 HeldPlot = INDEX_NONE;
    double HeldPlotSince = 0;
    bool bHeldPlotTilling = false;
    // A just-harvested plot whose ripe plant stays up until her hands lift the crop.
    int32 HeldHarvestPlot = INDEX_NONE;
    double HeldHarvestSince = 0;
    double HeldStickPileSince = 0;
    // Ground parts of the held produce to hide at the first pickup (the rest go with the second).
    int32 HeldPartsFirst = 0, HeldPartsCount = 0;
    float MusicGapRemaining = 18;
    float MusicElapsed = 0;
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
    // The last ReadSave failed because a newer build wrote the save; it's left untouched, never retired.
    mutable bool bReadNewer = false;
    bool bTestResetRequired = false;
    bool bHasPlayableSession = false;
    FString LoadProblem;
    TSharedPtr<SHomesteadMenu> NativeMenu;
    TSharedPtr<SHomesteadHotbar> HotbarWidget;
    TSharedPtr<SWidget> HotbarRoot;
    // Food, energy and the purse (UI/SHomesteadVitals), shown and removed with the hotbar.
    TSharedPtr<SWidget> VitalsRoot;
    TSharedPtr<SWidget> ClockRoot;
    // add-ruined-manor-and-arrival: the Names step and the arrival title card.
    bool bNewGameSetup = false;
    TSharedPtr<HomesteadMenus::SHomesteadNames> NamesWidget;
    TSharedPtr<SWidget> NamesRoot;
    TSharedPtr<HomesteadMenus::SHomesteadArrival> ArrivalCard;
    FString LatestSaveLabel;
    void ShowNames();
    void HideNames();
    void FinishNames(const FString& Heroine, const FString& Family, const FString& Estate);
    void UpdateArrival();
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
    // Successful sound-level writes to GameUserSettings (for the menu automation).
    int32 AudioPersistWrites = 0;
    bool PersistAutosaveEnabled(bool Requested);
    bool PersistAutosaveInterval(int32 Requested);
    bool PersistResolutionScale(float Requested);
    void ShowNativeMenu();
    void HideNativeMenu();
    void ShowHotbar();
    void HideHotbar();
    // A new game's row: the lamp in cell 8 (key 8) if she carries it, the rest empty for the tools
    // she hafts; selects cell 1.
    void ResetHotbar();
    // Applies a save's hotbar. Saves from before the row (HotbarLayout 3 or older) kept a pinned
    // list: her first carried stack of each pinned item moves into that cell once; pins she has
    // none of become ordinary empty cells.
    void SanitizeHotbar(const TArray<int32>& Slots, int32 Selected, int32 Layout);
    void EatFromHotbar(Homestead::Item Food);
    // The garden square the hoe lands on, just ahead of her.
    void TillSquareAhead(int32& X, int32& Y) const;
    // Till the square ahead with the hoe, or hoe out its weeds if it is already tilled.
    void HoeSquareAhead();
    // The garden outline for the selected hoe or pail (HomesteadControllerGarden.cpp), every tick.
    void UpdateGardenOutline();
    // Why the outlined square is red (the check's refusal), for the focus line; empty when it's green.
    FString GardenOutlineReason;
    // Plant the focused bare plot with Crop; she kneels to press in the seed.
    void PlantFocusedPlot(Homestead::CropKind Crop);
    // After HarvestCrop succeeds: she pulls or picks the crop, which stays in the ground until lifted.
    void PresentHarvest(int32 PlotId, Homestead::CropKind Crop, Homestead::Point Center);
    // Jenny's playtest kit (tools, bed, two chests; seeds on new games). Skipped in automation.
    void GrantPlaytestKit(bool bNewGame);
    void UseSelectedTool();
    // Selects a newly hafted tool's hotbar cell (it arrives in the first empty one).
    void SlotHaftedTool(Homestead::Recipe Recipe);
    void NotifyResourceAction(const Homestead::Result& Result, USoundBase* SuccessCue);
    int32 SelectedHotbarSlot = 0;
    TArray<FHomesteadHotbarSlot> BuildHotbarSnapshot() const;
    mutable TArray<FHomesteadHotbarSlot> HotbarSnapshotCache;
    mutable uint64 HotbarSnapshotFrame = MAX_uint64;
    mutable uint64 HotbarSnapshotRevision = 0;
    mutable int32 HotbarSnapshotSelected = INDEX_NONE;
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
    void OpenMap();
    void PreviousPage();
    void NextPage();
    void PreviousRow();
    void NextRow();
    void RotatePlacement();
    void RotatePlacementBy(int32 Direction);
    void ToggleDeconstruct();
    void UpdateDeconstruct(bool bForce);
    // Re-aims the preview ahead of the camera; revalidates it when it moved (at most ten times a
    // second unless forced).
    void UpdatePlacement(bool bForce);
    Homestead::Result SleepInBed(Homestead::Point Position);
    // Sleeps with the chosen option and makes the usual autosave and recovery checkpoint.
    void SleepAtBed(Homestead::Point Position);
    // At the bed, Up/Down (D-pad) steps through the sleep choices. False when not at a bed.
    bool CycleBedChoice(int32 Delta);
    Homestead::SleepChoice BedChoice = Homestead::SleepChoice::UntilMorning;
    int32 BedChoiceBed = INDEX_NONE;
    int32 SeenDozes = 0;
    void CycleZoom();
    void QuickSave();
    void QuickLoad();
    void ActivateRow();
    void ToggleVerticalSync();
    void OpenBook(int32 TargetPage);
    void CloseBook();
    void UpdateFocus();
    // HomesteadControllerToolFocus.cpp: the held tool's aimed overgrowth, and the 280-300 cm band.
    void FocusHeldToolTarget(Homestead::Point Position);
    void BeginPlacement(Homestead::Piece Kind);
    // Fill the watering pail at the nearest fresh water edge, with her kneeling fill when it succeeds.
    void FillPailAtStream(Homestead::Point Position);
    Homestead::Point FreshWaterDipPoint(Homestead::Point Position) const;
    // The pail goes in this far inside the waterline, so it visibly dips into the water.
    static constexpr double PailDipInsideCm = 25.0;
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
