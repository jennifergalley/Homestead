#include "HomesteadController.h"
#include "Simulation/HomesteadOvergrowth.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWorld.h"
#include "HomesteadEstateTerrain.h"
#include "Simulation/HomesteadEstate.h"
#include "Components/SplineComponent.h"
#include "EngineUtils.h"
#include "HomesteadSave.h"
#include "Engine/GameViewportClient.h"
#include "Misc/SecureHash.h"
#include "HomesteadSmokeTest.h"
#include "HomesteadVisualPlaytest.h"
#include "HomesteadTestPaths.h"
#include "Components/AudioComponent.h"
#include "AudioDevice.h"
#include "Sound/SoundAttenuation.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Engine/Engine.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMisc.h"
#include "InputKeyEventArgs.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Crc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "UI/SHomesteadMenu.h"
#include "UI/SHomesteadShop.h"
#include "UI/SHomesteadHotbar.h"
#include "UI/SHomesteadVitals.h"
#include "HomesteadMapComponent.h"
#include "Simulation/HomesteadManor.h"
#include "UI/SHomesteadNames.h"
#include "UI/SHomesteadArrival.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"
#include "UI/HomesteadMenuPortrait.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Misc/ScopeExit.h"

// "log LogHomesteadFootsteps Verbose" lists every footstep with its game time.
DEFINE_LOG_CATEGORY_STATIC(LogHomesteadFootsteps, Log, All);

namespace
{
constexpr const TCHAR* CameraSettingsSection = TEXT("Homestead.Camera");
constexpr const TCHAR* CameraSensitivityKey = TEXT("Sensitivity");
constexpr const TCHAR* CameraInvertYKey = TEXT("InvertY");
constexpr const TCHAR* AudioSettingsSection = TEXT("Homestead.Audio");
constexpr const TCHAR* AudioKeys[] = {TEXT("Music"), TEXT("Ambience"), TEXT("Effects"), TEXT("Master")};
int32 AudioKeyIndex(int32 Id) { return Id == 16 ? 3 : Id >= 5 && Id <= 7 ? Id - 5 : -1; }
constexpr const TCHAR* LastMusicTrackKey = TEXT("LastMusicTrack");
constexpr const TCHAR* AutosaveSettingsSection = TEXT("Homestead.Autosave");
constexpr const TCHAR* AutosaveEnabledKey = TEXT("Enabled");
constexpr const TCHAR* AutosaveMinutesKey = TEXT("IntervalMinutes");
constexpr const TCHAR* ActionHintSection = TEXT("Homestead.ActionHints");
constexpr int32 FieldBookPages[] = {0, 1, 2, 7, 3, 6};

// The estate's tools. The retired knife and machete no longer ride on the hotbar.
bool IsHotbarTool(Homestead::Item Item)
{
    return Homestead::ToolForItem(Item) != Homestead::ToolKind::Count;
}

template <typename FPredicate>
const Homestead::Plot* FindPlotWhere(const std::vector<Homestead::Plot>& Plots, FPredicate Predicate)
{
    for (const auto& Plot : Plots) if (Predicate(Plot)) return &Plot;
    return nullptr;
}

// Chosen on the hotbar to plant bare tilled soil: seeds grow roots, a berry's seeds grow a bush.
TOptional<Homestead::CropKind> PlantingCrop(Homestead::Item Item)
{
    if (Item == Homestead::Item::Seeds) return Homestead::CropKind::Roots;
    if (Item == Homestead::Item::Berries) return Homestead::CropKind::Berries;
    return {};
}

bool IsFoodItem(Homestead::Item Item) { return Homestead::IsEdible(Item); }

FName HotbarIcon(Homestead::Item Item)
{
    switch (Item)
    {
    case Homestead::Item::Berries: return TEXT("berries");
    case Homestead::Item::Seeds: return TEXT("seeds");
    case Homestead::Item::RoastedRoots: return TEXT("roasted-roots");
    case Homestead::Item::HerbedRoots: return TEXT("herbed-roots");
    case Homestead::Item::Knife: return TEXT("knife");
    case Homestead::Item::Hatchet: return TEXT("hatchet");
    case Homestead::Item::DiggingStick: return TEXT("digging-stick");
    case Homestead::Item::WateringCan: return TEXT("watering-can");
    case Homestead::Item::Machete: return TEXT("machete");
    case Homestead::Item::Scythe: return TEXT("scythe");
    case Homestead::Item::Billhook: return TEXT("billhook");
    case Homestead::Item::Pickaxe: return TEXT("pickaxe");
    default: return NAME_None;
    }
}

const TCHAR* SwingVerb(Homestead::Item Tool)
{
    switch (Tool)
    {
    case Homestead::Item::Hatchet: return TEXT("Chop with Axe");
    case Homestead::Item::Billhook: return TEXT("Hack with Billhook");
    case Homestead::Item::Scythe: return TEXT("Mow with Scythe");
    case Homestead::Item::Pickaxe: return TEXT("Break with Pickaxe");
    default: return TEXT("Clear");
    }
}

struct FCameraConfigSnapshot
{
    bool Existed = false;
    TArray<uint8> Bytes;
};

bool CaptureCameraConfig(const FString& Path, FCameraConfigSnapshot& Snapshot)
{
    Snapshot.Existed = IFileManager::Get().FileExists(*Path);
    return !Snapshot.Existed || FFileHelper::LoadFileToArray(Snapshot.Bytes, *Path);
}

bool RestoreCameraConfig(const FString& Path, const FCameraConfigSnapshot& Snapshot)
{
    return Snapshot.Existed
        ? FFileHelper::SaveArrayToFile(Snapshot.Bytes, *Path)
        : !IFileManager::Get().FileExists(*Path) || IFileManager::Get().Delete(*Path, false, true, true);
}

int32 ShiftFieldBookPage(int32 Page, int32 Direction)
{
    int32 Index = 0;
    for (int32 I = 0; I < UE_ARRAY_COUNT(FieldBookPages); ++I)
        if (FieldBookPages[I] == Page) { Index = I; break; }
    return FieldBookPages[(Index + UE_ARRAY_COUNT(FieldBookPages) + Direction) % UE_ARRAY_COUNT(FieldBookPages)];
}

bool PersistFloatProperty(const FString& Path, const TCHAR* Section, const TCHAR* Key, float Value)
{
    FCameraConfigSnapshot Snapshot;
    if (!CaptureCameraConfig(Path, Snapshot)) return false;
    FConfigFile Property;
    Property.SetFloat(Section, Key, Value);
    const bool Saved = Property.UpdateSinglePropertyInSection(*Path, Key, Section);
    FConfigFile Disk;
    float Persisted = -1;
    const bool Verified = Saved && Disk.Combine(Path) && Disk.GetFloat(Section, Key, Persisted)
        && FMath::IsNearlyEqual(Persisted, Value, 0.001f);
    if (!Verified && Saved) RestoreCameraConfig(Path, Snapshot);
    return Verified;
}

bool PersistBoolProperty(const FString& Path, const TCHAR* Section, const TCHAR* Key, bool Value)
{
    FCameraConfigSnapshot Snapshot;
    if (!CaptureCameraConfig(Path, Snapshot)) return false;
    FConfigFile Property;
    Property.SetBool(Section, Key, Value);
    const bool Saved = Property.UpdateSinglePropertyInSection(*Path, Key, Section);
    FConfigFile Disk;
    bool Persisted = !Value;
    const bool Verified = Saved && Disk.Combine(Path) && Disk.GetBool(Section, Key, Persisted) && Persisted == Value;
    if (!Verified && Saved) RestoreCameraConfig(Path, Snapshot);
    return Verified;
}

bool PersistIntProperty(const FString& Path, const TCHAR* Section, const TCHAR* Key, int32 Value)
{
    FCameraConfigSnapshot Snapshot;
    if (!CaptureCameraConfig(Path, Snapshot)) return false;
    FConfigFile Property;
    Property.SetString(Section, Key, *FString::FromInt(Value));
    const bool Saved = Property.UpdateSinglePropertyInSection(*Path, Key, Section);
    FConfigFile Disk;
    int32 Persisted = -1;
    const bool Verified = Saved && Disk.Combine(Path) && Disk.GetInt(Section, Key, Persisted) && Persisted == Value;
    if (!Verified && Saved) RestoreCameraConfig(Path, Snapshot);
    return Verified;
}

FString Text(const char* Value) { return UTF8_TO_TCHAR(Value); }
bool Edible(Homestead::Item Item) { return Homestead::IsEdible(Item); }
const TCHAR* RecipeDescription(Homestead::Recipe Recipe)
{
    switch (Recipe)
    {
    case Homestead::Recipe::HaftAxe: return TEXT("Fit a salvaged axe head to a new haft. Fells trees and clears stumps and fallen timber.");
    case Homestead::Recipe::HaftHoe: return TEXT("Fit a salvaged hoe blade to a new handle, to break and tend garden soil.");
    case Homestead::Recipe::HaftScythe: return TEXT("Fit a salvaged scythe blade to a snath. Mows tall grass and weeds in a wide sweep.");
    case Homestead::Recipe::HaftBillhook: return TEXT("Fit a salvaged billhook head to a handle. Hacks through bramble and saplings.");
    case Homestead::Recipe::HaftPickaxe: return TEXT("Fit a salvaged pick head to a haft. Breaks rubble and rocks into stone and scrap.");
    case Homestead::Recipe::RoastedRoots: return TEXT("Wild roots softened and warmed over a fueled cookfire.");
    case Homestead::Recipe::HerbedRoots: return TEXT("Roasted roots brightened with meadow herbs.");
    case Homestead::Recipe::SplitFirewood: return TEXT("Prepared fuel split from timber with a carried axe.");
    default: return TEXT("");
    }
}

class FHomesteadMenuPointerInput final : public IInputProcessor
{
public:
    explicit FHomesteadMenuPointerInput(AHomesteadController* InController) : Controller(InController) {}
    virtual void Tick(float, FSlateApplication&, TSharedRef<ICursor>) override {}
    virtual bool HandleMouseMoveEvent(FSlateApplication&, const FPointerEvent& Event) override
    {
        if (!Controller.IsValid()) return false;
        Controller->MenuPointerIntent(Event.GetCursorDelta().X, Event.GetCursorDelta().Y);
        return !Controller->MenuAcceptsPhysicalInput();
    }
    virtual bool HandleMouseButtonDownEvent(FSlateApplication&, const FPointerEvent& Event) override
    {
        return Controller.IsValid() && !Controller->MenuPointerButtonIntent(Event.GetEffectingButton());
    }
    virtual bool HandleMouseButtonUpEvent(FSlateApplication&, const FPointerEvent& Event) override
    {
        return Controller.IsValid() && !Controller->MenuPointerButtonIntent(Event.GetEffectingButton());
    }
    virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication&, const FPointerEvent& Event, const FPointerEvent*) override
    {
        return Controller.IsValid() && !Controller->MenuPhysicalInput(EKeys::MouseWheelAxis, IE_Axis, Event.GetWheelDelta());
    }
private:
    TWeakObjectPtr<AHomesteadController> Controller;
};
}

AHomesteadController::AHomesteadController()
{
    PrimaryActorTick.bCanEverTick = true;
    Map = CreateDefaultSubobject<UHomesteadMapComponent>(TEXT("Map"));
    Music = CreateDefaultSubobject<UAudioComponent>(TEXT("Music"));
    Music->bAutoActivate = false;
    Music->bAllowSpatialization = false;
    Ambience = CreateDefaultSubobject<UAudioComponent>(TEXT("Ambience"));
    Ambience->bAutoActivate = false;
    Ambience->bAllowSpatialization = false;
    Creek = CreateDefaultSubobject<UAudioComponent>(TEXT("Creek"));
    Creek->bAutoActivate = false;
    Creek->bAllowSpatialization = true;
    Creek->SetUsingAbsoluteLocation(true);
    Creek->bOverrideAttenuation = true;
    FSoundAttenuationSettings& Falloff = Creek->AttenuationOverrides;
    Falloff.bAttenuate = true;
    Falloff.bSpatialize = true;
    Falloff.AttenuationShape = EAttenuationShape::Sphere;
    Falloff.AttenuationShapeExtents = FVector(250, 0, 0);
    Falloff.FalloffDistance = 2200;
    Falloff.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
    Falloff.dBAttenuationAtMax = -48;
    // Standing at the bank the water surrounds her; farther off it narrows to a point and the
    // highs roll off, the way a brook muffles through the trees.
    Falloff.NonSpatializedRadiusStart = 150;
    Falloff.NonSpatializedRadiusEnd = 400;
    Falloff.bAttenuateWithLPF = true;
    Falloff.LPFRadiusMin = 400;
    Falloff.LPFRadiusMax = 2400;
    Falloff.LPFFrequencyAtMin = 20000;
    Falloff.LPFFrequencyAtMax = 2500;
}

void AHomesteadController::BeginPlay()
{
    Super::BeginPlay();
    const bool SmokeTest = FParse::Param(FCommandLine::Get(), TEXT("HomesteadSmokeTest"));
    const bool VisualPlaytest = FParse::Param(FCommandLine::Get(), TEXT("HomesteadVisualPlaytest"));
#if UE_BUILD_SHIPPING
    const bool ShippingQA = FParse::Param(FCommandLine::Get(), TEXT("HomesteadShippingQA"));
    if (ShippingQA || SmokeTest || VisualPlaytest)
    {
        FString Output;
        const auto* Graphics = GConfig->FindBranch(TEXT("GameUserSettings"), {});
        const bool Admitted = HomesteadAutomatedActorsEnabled()
            && FParse::Value(FCommandLine::Get(), TEXT("HomesteadTestOutput="), Output)
            && !Output.IsEmpty() && !FPaths::IsRelative(Output)
            && IFileManager::Get().DirectoryExists(*Output)
            && FPaths::IsUnderDirectory(FPaths::ProjectSavedDir(), FPaths::Combine(Output, TEXT("EngineUser")))
            && Graphics && FPaths::IsSamePath(Graphics->IniPath, FPaths::Combine(Output, TEXT("Graphics/GameUserSettings.ini")))
            && !IFileManager::Get().DirectoryExists(*FPaths::Combine(Output, TEXT("SmokeSave")))
            && !IFileManager::Get().DirectoryExists(*FPaths::Combine(Output, TEXT("Frames")))
            && !IFileManager::Get().FileExists(*FPaths::Combine(Output, TEXT("qa-admission.txt")))
            && !IFileManager::Get().FileExists(*FPaths::Combine(Output, TEXT("smoke-result.txt")))
            && !IFileManager::Get().FileExists(*FPaths::Combine(Output, TEXT("telemetry.csv")));
        if (!Admitted)
        {
            FPlatformMisc::LowLevelOutputDebugString(TEXT("SHIPPING_QA_REJECTED: Explicit single route and fresh isolated output/graphics/user/save directories are required.\n"));
            FPlatformMisc::RequestExitWithStatus(true, 2);
            return;
        }
    }
#endif
    FString RoutingError;
    bSaveRoutingReady = ResolveHomesteadSaveRoute(FCommandLine::Get(), FPaths::ProjectSavedDir(),
        FPlatformProcess::UserSettingsDir(), HomesteadTestOutputDirectory(), SaveRoute, RoutingError);
    if (!bSaveRoutingReady)
    {
        UE_LOG(LogTemp, Error, TEXT("SAVE_ROUTING_REJECTED: %s"), *RoutingError);
        // UE's Windows graceful shutdown can discard the requested code. No save IO has begun.
        FPlatformMisc::RequestExitWithStatus(true, 2);
        return;
    }
#if UE_BUILD_SHIPPING
    if (ShippingQA)
    {
        const FString Output = HomesteadTestOutputDirectory();
        const FString Admission = FString::Printf(TEXT("version=1\nshipping=1\ntrace_compiled=%d\nroute=%s\nsave_directory=%s\nproject_saved_directory=%s\n"),
            UE_TRACE_ENABLED != 0, SmokeTest ? TEXT("smoke") : TEXT("visual"), *SaveRoute.Directory, *FPaths::ProjectSavedDir());
        if (SaveRoute.Mode != TEXT("test-sandbox")
            || !FPaths::IsSamePath(SaveRoute.Directory, FPaths::Combine(Output, TEXT("SmokeSave")))
            || !FFileHelper::SaveStringToFile(Admission, *FPaths::Combine(Output, TEXT("qa-admission.txt"))))
        {
            FPlatformMisc::LowLevelOutputDebugString(TEXT("SHIPPING_QA_REJECTED: Isolated save routing or admission evidence failed.\n"));
            FPlatformMisc::RequestExitWithStatus(true, 2);
            return;
        }
    }
#endif
    if (!PrepareStartupProbe()) return;
    LoadCameraPreferences();
    LoadUserPreferences();
#if !UE_BUILD_SHIPPING
    bSaveRoutingTestPending = FParse::Param(FCommandLine::Get(), TEXT("HomesteadSaveRoutingTest"));
    if (bSaveRoutingTestPending)
    {
        UE_LOG(LogTemp, Display, TEXT("SAVE_ROUTING_DEFAULT_READ_ONLY: %s"), *SaveRoute.Directory);
        SaveRoute.Directory = FPaths::Combine(HomesteadTestOutputDirectory(), TEXT("SaveRoutingFixtures"), TEXT("Default"), TEXT("SaveGames"));
        SaveRoute.Mode = TEXT("routing-fixture");
        SaveRoute.Profile.Empty();
    }
#endif
    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());
    ResetHotbar();
    if (!SmokeTest && !VisualPlaytest
        && FParse::Param(FCommandLine::Get(), TEXT("HomesteadHeroineTrialVitruvian01")))
    {
        Appearance.HairStyle = 1; Appearance.MetaHair = HomesteadLook::MetaHairForLegacy(1);
    }
    WorldId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    bEstateMap = UGameplayStatics::GetCurrentLevelName(this, true) == TEXT("Estate");
    if (bEstateMap)
    {
        if (!HomesteadEstateTerrain::Activate())
        {
            Notify(TEXT("The estate terrain data is missing from this build. Check the game log."), true);
            return;
        }
        PrepareEstateSimulation(Sim);
        // Estate saves are a separate line from the generated-woodland saves.
        SaveRoute.Directory = FPaths::Combine(SaveRoute.Directory, TEXT("Estate"));
    }
    else
    {
        HomesteadEstateTerrain::Deactivate();
    }
    if (!SmokeTest && !VisualPlaytest && !bSaveRoutingTestPending)
    {
        const FGuid Seed = FGuid::NewGuid();
        const auto Result = bEstateMap
            ? Sim.NewEstateGame(Homestead::ProvisionalEstateLayout(), Homestead::ProvisionalEstatePlacements())
            : Sim.NewGame((static_cast<uint64>(Seed.A) << 32) | Seed.B);
        if (!Result) { Notify(Result); return; }
        if (bEstateMap) SetEstateSpawn();
    }
    Landscape = GetWorld()->SpawnActor<AHomesteadWorld>();
    if (!Landscape)
    {
        UE_LOG(LogTemp, Error, TEXT("Unable to create the homestead world."));
        Notify(TEXT("The world could not be created. Check the game log."), true);
        return;
    }
    bWorldReady = Landscape->Initialize(Sim);
    if (!bWorldReady) { Notify(TEXT("The generated woodland could not be prepared. Movement is disabled; no save was changed."), true); return; }
    CaptureSessionCheckpoint(PendingLocation, PendingRotation);
    bAutomatedInputOnly = (HomesteadAutomatedActorsEnabled() && (SmokeTest || VisualPlaytest)) || bSaveRoutingTestPending;
    bAutomatedInputOnly |= !StartupProbeDirectory.IsEmpty();
    UE_LOG(LogTemp, Display, TEXT("SAVE_ROUTING version=1 mode=%s profile=%s directory=\"%s\" automation_input=%d smoke_actor=%d visual_actor=%d"),
        *SaveRoute.Mode, *SaveRoute.Profile, *SaveRoute.Directory, bAutomatedInputOnly, SmokeTest, VisualPlaytest);
    const bool Loaded = !SmokeTest && !VisualPlaytest && !bSaveRoutingTestPending && LoadLatest();
    if (!Loaded) GrantPlaytestKit(true);
    bHasPlayableSession = !bTestResetRequired;
    if (!Loaded && bEstateMap && !bTestResetRequired && !SmokeTest && !VisualPlaytest) BeginNewGameSetup();
    else if (!Loaded) OpenBook(bTestResetRequired ? 4 : 3);
    ShowHotbar();
    if (!StartupProbeDirectory.IsEmpty() && !Loaded) { FinishStartupProbe(TEXT("The isolated prepared save did not load.")); return; }
    InitializeAudio();
    if (HomesteadAutomatedActorsEnabled())
    {
        if (SmokeTest) GetWorld()->SpawnActor<AHomesteadSmokeTest>();
        else if (VisualPlaytest) GetWorld()->SpawnActor<AHomesteadVisualPlaytest>();
    }
}

bool AHomesteadController::InputKey(const FInputKeyEventArgs& Params)
{
    if (bAutomatedInputOnly && !Params.IsSimulatedInput())
    {
        ++IgnoredExternalInputs;
        if (!bLoggedExternalInput && (Params.Event == IE_Pressed || FMath::Abs(Params.AmountDepressed) > 0.15f))
        {
            UE_LOG(LogTemp, Display, TEXT("Automation ignored external input (test-mode isolation; no key contents recorded)."));
            bLoggedExternalInput = true;
        }
        return true;
    }
    FInputAxisProperties AxisProperties;
    const bool HasAxisProperties = Params.Key.IsGamepadKey() && Params.Key.IsAnalog() && PlayerInput
        && PlayerInput->GetAxisProperties(Params.Key, AxisProperties);
    const EHomesteadPromptDevice Intent = PromptIntent.Classify(Params, FPlatformTime::Seconds(), HasAxisProperties ? &AxisProperties : nullptr);
    if (Intent != EHomesteadPromptDevice::None)
    {
        const bool NextGamepad = Intent == EHomesteadPromptDevice::Gamepad;
        if (NextGamepad != bGamepad) ++PromptDeviceChanges;
        bGamepad = NextGamepad;
    }
    if (Params.Key == EKeys::LeftControl || Params.Key == EKeys::RightControl)
    {
        if (Params.Event == IE_Pressed) bControlDown = true;
        else if (Params.Event == IE_Released) bControlDown = false;
    }
    if (NamesWidget.IsValid())
    {
        // The Names step owns input; real keys reach it through Slate focus first.
        if (Params.Event == IE_Pressed) NamesWidget->HandleKey(Params.Key);
        return true;
    }
    if (NativeMenu.IsValid()) bShowMouseCursor = !bGamepad;
    if (NativeMenu.IsValid() && (bBookOpen || IsFailed()))
    {
        if (bMenuSaveInProgress) return true;
        if (Params.Event == IE_Pressed && Params.Key == EKeys::F5)
        { if (NativeMenu->PrepareQuickAction()) QuickSave(); return true; }
        if (Params.Event == IE_Pressed && Params.Key == EKeys::F9)
        { if (NativeMenu->PrepareQuickAction()) QuickLoad(); return true; }
        const auto Menu = NativeMenu;
        return Menu->HandleKey(Params.Key, Params.Event, Params.AmountDepressed);
    }
    if (ShopScreen.IsValid())
    {
        const auto Shop = ShopScreen;
        return Shop->HandleKey(Params.Key, Params.Event, Params.AmountDepressed);
    }
    if (bPlanning && !bBookOpen && !IsFailed() && Params.Key == EKeys::MouseWheelAxis && Params.Event == IE_Axis
        && FMath::Abs(Params.AmountDepressed) >= 1.0f
        && !(bControlDown || IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl)))
    {
        RotatePlacementBy(Params.AmountDepressed > 0 ? 1 : -1);
        return true;
    }
    if (!bBookOpen && !bPlanning && !IsFailed())
    {
        static const FKey NumberKeys[] = {
            EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
            EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine, EKeys::Zero
        };
        if (Params.Event == IE_Pressed)
            for (int32 Index = 0; Index < UE_ARRAY_COUNT(NumberKeys); ++Index)
                if (Params.Key == NumberKeys[Index])
                {
                    SelectHotbarSlot(Index);
                    return true;
                }
        if (Params.Key == EKeys::MouseWheelAxis && Params.Event == IE_Axis
            && FMath::Abs(Params.AmountDepressed) >= 1.0f)
        {
            const bool Control = bControlDown || IsInputKeyDown(EKeys::LeftControl)
                || IsInputKeyDown(EKeys::RightControl);
            if (Control)
            {
                if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
                    Avatar->Zoom(Params.AmountDepressed);
            }
            else CycleHotbar(Params.AmountDepressed > 0 ? -1 : 1);
            return true;
        }
    }
    return Super::InputKey(Params);
}

bool AHomesteadController::MenuPhysicalInput(FKey Key, EInputEvent Event, float Amount)
{
    if (bAutomatedInputOnly && bSimulatedMenuEvent)
        InputKey(FInputKeyEventArgs::CreateSimulated(Key, Event, Amount));
    else
        InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Key, Event, Amount, false, FPlatformTime::Cycles64()));
    return MenuAcceptsPhysicalInput();
}

bool AHomesteadController::MenuPointerButtonIntent(FKey Key)
{
    if (!MenuAcceptsPhysicalInput()) return false;
    if (Key.IsMouseButton())
    {
        if (bGamepad) ++PromptDeviceChanges;
        bGamepad = false;
        bShowMouseCursor = true;
    }
    return true;
}

bool AHomesteadController::MenuPointerIntent(float X, float Y)
{
    MenuPhysicalInput(EKeys::MouseX, IE_Axis, X);
    MenuPhysicalInput(EKeys::MouseY, IE_Axis, Y);
    if (NativeMenu.IsValid() && FSlateApplication::IsInitialized())
        NativeMenu->PointerItemDragMove(FSlateApplication::Get().GetCursorPos());
    return !bAutomatedInputOnly && !bGamepad;
}

void AHomesteadController::ShowNativeMenu()
{
    if (!GEngine || !GEngine->GameViewport) return;
    HoveredHotbarSlot = INDEX_NONE;
    RefreshMenuPortrait();
    if (!NativeMenu.IsValid())
    {
        FlushPressedKeys();
        NativeMenu = SNew(SHomesteadMenu).Controller(this);
        GEngine->GameViewport->AddViewportWidgetContent(NativeMenu.ToSharedRef(), 100);
        MenuPointerInput = MakeShared<FHomesteadMenuPointerInput>(this);
        FSlateApplication::Get().RegisterInputPreProcessor(MenuPointerInput);
    }
    bShowMouseCursor = !bGamepad;
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(NativeMenu);
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
    NativeMenu->Refresh();
}

void AHomesteadController::HideNativeMenu()
{
    if (MenuPointerInput.IsValid() && FSlateApplication::IsInitialized())
        FSlateApplication::Get().UnregisterInputPreProcessor(MenuPointerInput);
    MenuPointerInput.Reset();
    if (NativeMenu.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(NativeMenu.ToSharedRef());
    NativeMenu.Reset();
    if (MenuPortrait) MenuPortrait->Destroy();
    MenuPortrait = nullptr;
    PortraitBrush.SetResourceObject(nullptr);
    FlushPressedKeys();
    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());
}

void AHomesteadController::ShowHotbar()
{
    if (HotbarRoot.IsValid() || !GEngine || !GEngine->GameViewport) return;
    HotbarWidget = SNew(SHomesteadHotbar).Controller(this);
    HotbarRoot = SNew(SBox)
        .Visibility_Lambda([this]()
        {
            return ShouldShowHotbar()
                ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
        })
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Bottom)
        .Padding(0, 0, 0, 22)
        [
            SNew(SScaleBox).Stretch(EStretch::UserSpecified)
            .UserSpecifiedScale_Lambda([]()
            {
                const FViewport* Viewport = GEngine && GEngine->GameViewport
                    ? GEngine->GameViewport->Viewport : nullptr;
                return Viewport ? FMath::Min(1.0f,
                    1080.0f / FMath::Max(720, Viewport->GetSizeXY().Y)) : 1.0f;
            })
            [
                HotbarWidget.ToSharedRef()
            ]
        ];
    GEngine->GameViewport->AddViewportWidgetContent(HotbarRoot.ToSharedRef(), 50);
    // Unlike the hotbar, the vitals stay up while she places a plan.
    VitalsRoot = SNew(SBox)
        .Visibility_Lambda([this]()
        {
            return bWorldReady && !bBookOpen && !IsFailed() && !ShopScreen.IsValid() && !HasNativeMenu()
                && !IsNewGameSetup() && !IsNamingSetup() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
        })
        [
            SNew(HomesteadMenus::SHomesteadVitals).Controller(this)
        ];
    GEngine->GameViewport->AddViewportWidgetContent(VitalsRoot.ToSharedRef(), 50);
}

void AHomesteadController::HideHotbar()
{
    HoveredHotbarSlot = INDEX_NONE;
    if (HotbarRoot.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(HotbarRoot.ToSharedRef());
    if (VitalsRoot.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(VitalsRoot.ToSharedRef());
    VitalsRoot.Reset();
    HotbarWidget.Reset();
    HotbarRoot.Reset();
}

bool AHomesteadController::ShouldShowHotbar() const
{
    return bWorldReady && !bBookOpen && !bPlanning && !IsFailed() && !ShopScreen.IsValid();
}

void AHomesteadController::ResetHotbar()
{
    HotbarSlots.Init(-1, 10);
    // In the order she hafts them: the billhook first, for the bramble at the door.
    HotbarSlots[0] = static_cast<int32>(Homestead::Item::Billhook);
    HotbarSlots[1] = static_cast<int32>(Homestead::Item::Hatchet);
    HotbarSlots[2] = static_cast<int32>(Homestead::Item::Scythe);
    HotbarSlots[3] = static_cast<int32>(Homestead::Item::Pickaxe);
    HotbarSlots[4] = static_cast<int32>(Homestead::Item::DiggingStick);
    HotbarSlots[5] = static_cast<int32>(Homestead::Item::WateringCan);
    HotbarSlots[6] = static_cast<int32>(Homestead::Item::Berries);
    SelectedHotbarSlot = 0;
    HoveredHotbarSlot = INDEX_NONE;
}

void AHomesteadController::SanitizeHotbar(const TArray<int32>& Slots, int32 Selected, int32 Layout)
{
    HotbarSlots.Init(-1, 10);
    TSet<int32> Seen;
    for (int32 Index = 0; Index < FMath::Min(10, Slots.Num()); ++Index)
    {
        if (Slots[Index] >= 0 && Slots[Index] < static_cast<int32>(Homestead::Item::Count)
            && CanPinToHotbar(static_cast<Homestead::Item>(Slots[Index])) && !Seen.Contains(Slots[Index]))
        {
            HotbarSlots[Index] = Slots[Index];
            Seen.Add(Slots[Index]);
        }
    }
    // Older hotbars get the estate tools and pinned food once, in free slots.
    if (Layout < UHomesteadSave::CurrentHotbarLayout)
        for (const auto Item : {Homestead::Item::Billhook, Homestead::Item::Scythe, Homestead::Item::Pickaxe,
            Homestead::Item::Berries})
        {
            const int32 Value = static_cast<int32>(Item);
            const int32 Free = HotbarSlots.IndexOfByKey(-1);
            if (!Seen.Contains(Value) && Free != INDEX_NONE) HotbarSlots[Free] = Value;
        }
    SelectedHotbarSlot = FMath::Clamp(Selected, 0, 9);
}

bool AHomesteadController::CanPinToHotbar(Homestead::Item Item)
{
    return IsHotbarTool(Item) || IsFoodItem(Item) || PlantingCrop(Item).IsSet();
}

bool AHomesteadController::IsPinnedToHotbar(Homestead::Item Item) const
{
    return HotbarSlots.Contains(static_cast<int32>(Item));
}

bool AHomesteadController::TogglePinnedToHotbar(Homestead::Item Item)
{
    const FString Name = UTF8_TO_TCHAR(Homestead::ItemName(Item));
    if (!CanPinToHotbar(Item))
    {
        Notify(TEXT("Only tools, food and seeds can go on the hotbar."), true);
        return false;
    }
    const int32 Value = static_cast<int32>(Item);
    const int32 Pinned = HotbarSlots.IndexOfByKey(Value);
    if (Pinned != INDEX_NONE)
    {
        HotbarSlots[Pinned] = -1;
        Notify(Name + TEXT(" unpinned from the hotbar."));
        return true;
    }
    // Food goes to the right-hand slots first, leaving 1-5 for tools.
    int32 Free = INDEX_NONE;
    for (int32 Step = 0; Step < 10 && Free == INDEX_NONE; ++Step)
    {
        const int32 Index = (Step + 5) % 10;
        if (HotbarSlots.IsValidIndex(Index) && HotbarSlots[Index] < 0) Free = Index;
    }
    if (Free == INDEX_NONE)
    {
        Notify(TEXT("The hotbar is full. Unpin something first."), true);
        return false;
    }
    HotbarSlots[Free] = Value;
    Notify(FString::Printf(TEXT("%s pinned to hotbar slot %d."), *Name, Free == 9 ? 0 : Free + 1));
    return true;
}

void AHomesteadController::EatFromHotbar(Homestead::Item Food)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    // One mouthful at a time: clicks while she is still eating are ignored.
    if (Animation && Animation->IsEating()) return;
    const auto Result = Sim.Eat(Food);
    Notify(Result);
    if (Result.ok && Avatar) Avatar->PlayEat(Food == Homestead::Item::Berries);
}

TArray<FHomesteadHotbarSlot> AHomesteadController::HotbarSnapshot() const
{
    TArray<FHomesteadHotbarSlot> Result;
    Result.Reserve(10);
    for (int32 Index = 0; Index < 10; ++Index)
    {
        FHomesteadHotbarSlot Slot;
        Slot.Index = Index;
        Slot.Selected = Index == SelectedHotbarSlot;
        if (HotbarSlots.IsValidIndex(Index) && HotbarSlots[Index] >= 0)
        {
            Slot.Tool = static_cast<Homestead::Item>(HotbarSlots[Index]);
            Slot.Assigned = CanPinToHotbar(Slot.Tool);
            Slot.Food = IsFoodItem(Slot.Tool);
            Slot.Count = Slot.Assigned ? Sim.Count(Slot.Tool) : 0;
            Slot.Available = Slot.Assigned && Slot.Count > 0;
            Slot.Icon = HotbarIcon(Slot.Tool);
        }

        Result.Add(Slot);
    }
    return Result;
}

bool AHomesteadController::KnifePreviewRequested() const
{
    return PresentedTool() == Homestead::Item::Knife;
}

Homestead::Item AHomesteadController::PresentedTool() const
{
    if (!ShouldShowHotbar()) return Homestead::Item::Count;
    const int32 Slot = HoveredHotbarSlot != INDEX_NONE ? HoveredHotbarSlot : SelectedHotbarSlot;
    if (!HotbarSlots.IsValidIndex(Slot) || HotbarSlots[Slot] < 0) return Homestead::Item::Count;
    const auto Tool = static_cast<Homestead::Item>(HotbarSlots[Slot]);
    return IsHotbarTool(Tool) && Sim.Count(Tool) > 0 ? Tool : Homestead::Item::Count;
}

Homestead::Item AHomesteadController::SelectedCarriedTool() const
{
    if (!HotbarSlots.IsValidIndex(SelectedHotbarSlot) || HotbarSlots[SelectedHotbarSlot] < 0) return Homestead::Item::Count;
    const auto Tool = static_cast<Homestead::Item>(HotbarSlots[SelectedHotbarSlot]);
    return IsHotbarTool(Tool) && Sim.Count(Tool) > 0 ? Tool : Homestead::Item::Count;
}

void AHomesteadController::SelectHotbarSlot(int32 Index)
{
    if (!ShouldShowHotbar() || Index < 0 || Index >= 10) return;
    if (Index != SelectedHotbarSlot)
        if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
            Avatar->CancelAction(true);
    SelectedHotbarSlot = Index;
    const auto Snapshot = HotbarSnapshot();
    ToastText = Snapshot[Index].Assigned
        ? Text(Homestead::ItemName(Snapshot[Index].Tool)) : TEXT("Empty slot");
    bToastError = false;
    ToastRemaining = 1.0f;
    PlayEffect(UIClick, 0.05f);
}

void AHomesteadController::CycleHotbar(int32 Direction)
{
    if (!ShouldShowHotbar() || Direction == 0) return;
    SelectHotbarSlot((SelectedHotbarSlot + (Direction > 0 ? 1 : 9)) % 10);
}

void AHomesteadController::UseSelectedTool()
{
    // While planning, the left mouse button or right trigger places the piece, as E / A does.
    if (bPlanning && !bBookOpen) { Interact(); return; }
    if (!ShouldShowHotbar() || !HotbarSlots.IsValidIndex(SelectedHotbarSlot)) return;
    const int32 ToolValue = HotbarSlots[SelectedHotbarSlot];
    // Seeds or a berry on bare tilled soil: plant it there (a berry is eaten anywhere else).
    if (ToolValue >= 0)
        if (const auto Crop = PlantingCrop(static_cast<Homestead::Item>(ToolValue)))
        {
            if (bWorldReady && PrepareWorldAt(PlayerPoint()))
            {
                UpdateFocus();
                const auto* Bare = Focus == EFocus::Plot
                    ? FindPlotWhere(State().plots, [this](const Homestead::Plot& Plot) { return Plot.id == FocusId; })
                    : nullptr;
                if (Bare && !Bare->planted)
                {
                    PlantFocusedPlot(*Crop);
                    return;
                }
            }
            if (static_cast<Homestead::Item>(ToolValue) == Homestead::Item::Seeds)
            {
                Notify(TEXT("Aim at bare tilled soil to plant seeds."), true);
                return;
            }
        }
    if (ToolValue >= 0 && IsFoodItem(static_cast<Homestead::Item>(ToolValue)))
    {
        const auto Food = static_cast<Homestead::Item>(ToolValue);
        if (Sim.Count(Food) <= 0)
            Notify(FString::Printf(TEXT("No %s left in your pack."), UTF8_TO_TCHAR(Homestead::ItemName(Food))), true);
        else EatFromHotbar(Food);
        return;
    }
    if (ToolValue < 0 || !IsHotbarTool(static_cast<Homestead::Item>(ToolValue)))
    {
        Notify(TEXT("Choose a carried tool first."), true);
        return;
    }
    const auto Tool = static_cast<Homestead::Item>(ToolValue);
    if (Sim.Count(Tool) <= 0)
    {
        Notify(TEXT("That tool is not in your pack."), true);
        return;
    }
    const auto Position = PlayerPoint();
    if (!bWorldReady || !PrepareWorldAt(Position)) return;
    UpdateFocus();
    const FHintUse Hint = BeginHintUse(bGamepad ? TEXT("RT") : TEXT("LMB"));
    ON_SCOPE_EXIT { EndHintUse(Hint); };

    if (Tool == Homestead::Item::Hatchet && Focus == EFocus::Resource)
        for (const auto& Node : State().resources)
            if (Node.id == FocusId && Node.kind == Homestead::ResourceKind::ForestTree)
            {
                // Standing trees keep the axe's felling presentation.
                const Homestead::Point Target = Node.position;
                const int32 Cleared = FocusId;
                auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
                const bool bFell = Avatar && Avatar->CanFell();
                const auto Result = Sim.Clear(FocusId, Position);
                NotifyResourceAction(Result, bFell ? nullptr : WoodTapB.Get());
                if (Result.ok && Avatar) PresentFelling(Cleared, Target, true);
                return;
            }
    if (Tool == Homestead::Item::Hatchet || Tool == Homestead::Item::Billhook
        || Tool == Homestead::Item::Scythe || Tool == Homestead::Item::Pickaxe)
    {
        SwingAtOvergrowth(Tool);
        return;
    }

    if (Tool == Homestead::Item::WateringCan)
    {
        if (Focus == EFocus::Water)
        {
            Notify(Sim.FillWater(Position));
            return;
        }
        if (Focus != EFocus::Plot)
        {
            Notify(TEXT("Aim at a growing crop or stand by the stream."), true);
            return;
        }
        for (const auto& Plot : State().plots)
            if (Plot.id == FocusId)
            {
                const auto Result = Sim.Water(FocusId, Position);
                Notify(Result, GrassStepB);
                if (Result.ok)
                    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
                        Avatar->PlayWater(Homestead::PlotCenter(Plot));
                return;
            }
    }

    if (Tool == Homestead::Item::DiggingStick) HoeSquareAhead();
}

void AHomesteadController::RefreshMenuPortrait()
{
    // Appearance shows her in the world itself; only the pack page has a portrait.
    if (!bBookOpen || Page != 0)
    {
        if (MenuPortrait) MenuPortrait->Destroy();
        MenuPortrait = nullptr;
        PortraitBrush.SetResourceObject(nullptr);
        return;
    }
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar) return;
    if (!MenuPortrait)
    {
        FActorSpawnParameters Parameters;
        Parameters.ObjectFlags |= RF_Transient;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        MenuPortrait = GetWorld()->SpawnActor<AHomesteadMenuPortrait>(FVector(0, 0, -20000), FRotator::ZeroRotator, Parameters);
    }
    if (MenuPortrait && MenuPortrait->Refresh(*Avatar))
    {
        PortraitBrush.SetResourceObject(MenuPortrait->BrushResource());
        PortraitBrush.ImageSize = FVector2D(384, 768);
        PortraitBrush.DrawAs = ESlateBrushDrawType::Image;
    }
    else
    {
        if (MenuPortrait) MenuPortrait->Destroy();
        MenuPortrait = nullptr;
        PortraitBrush.SetResourceObject(nullptr);
        UE_LOG(LogTemp, Warning, TEXT("Menu character preview is unavailable."));
    }
}

void AHomesteadController::OrbitMenuPortrait(float Degrees)
{
    if (MenuPortrait) MenuPortrait->Orbit(Degrees);
}

void AHomesteadController::ZoomMenuPortrait()
{
    if (MenuPortrait) MenuPortrait->ToggleCloseup();
}

FString AHomesteadController::MenuPortraitStatus() const
{
    return TEXT("As you look now");
}

void AHomesteadController::HomesteadPackMenu(int32 Tile, int32 Mode)
{
    if (Mode == 2)
    {
        const auto Position = PlayerPoint();
        const Homestead::Structure* Nearest = nullptr;
        float Best = TNumericLimits<float>::Max();
        for (const auto& Structure : State().structures)
        {
            if (Structure.kind != Homestead::Piece::Chest) continue;
            const auto Center = Homestead::StructureCenter(State(), Structure);
            const float Distance = FMath::Square(Center.x - Position.x) + FMath::Square(Center.y - Position.y);
            if (Distance < Best) { Best = Distance; Nearest = &Structure; }
        }
        if (!Nearest) { Notify(TEXT("There is no storage chest nearby."), true); return; }
        const auto ChestCenter = Homestead::StructureCenter(State(), *Nearest);
        UE_LOG(LogTemp, Display, TEXT("HomesteadPackMenu: nearest chest %d at (%.0f, %.0f)"), Nearest->id, ChestCenter.x, ChestCenter.y);
        if (bBookOpen) CloseBook();
        OpenChestStorage(Nearest->id);
        return;
    }
    if (!NativeMenu.IsValid() || !bBookOpen || Page != 0) { Notify(TEXT("Open the pack first."), true); return; }
    const auto Rows = MenuRows();
    if (!Rows.IsValidIndex(Tile)) { Notify(TEXT("There is no item in that tile."), true); return; }
    if (Mode == 1) NativeMenu->OpenQuantityPrompt(Rows[Tile]);
    else NativeMenu->OpenItemContextMenu(Tile, false);
}

void AHomesteadController::HomesteadMorning(float Hour)
{
    Sim.SkipToHourOfDay(Hour);
    RefreshRemaining = 0;
}

void AHomesteadController::HomesteadStandingRoom()
{
    const APawn* Avatar = GetPawn();
    if (!Avatar) return;
    // She wakes facing the doorway on the room's west side, so the grid heading is her yaw + 90.
    const double RoomYaw = FMath::Fmod(Avatar->GetActorRotation().Yaw + 90.0 + 720.0, 360.0);
    const Homestead::Point Offset = Homestead::RotateYaw({-150.0, 0.0}, RoomYaw);
    const auto Position = PlayerPoint();
    const auto Result = Sim.SeedStandingRoomAt({Position.x - Offset.x, Position.y - Offset.y}, RoomYaw);
    Notify(UTF8_TO_TCHAR(Result.message.c_str()), !Result);
    RefreshRemaining = 0;
    if (Result) ShowArrival();
}

void AHomesteadController::HomesteadGive(const FString& ItemName, int32 Amount)
{
    const FString Wanted = ItemName.Replace(TEXT(" "), TEXT(""));
    for (int32 Index = 0; Index < static_cast<int32>(Homestead::Item::Count); ++Index)
    {
        const auto Item = static_cast<Homestead::Item>(Index);
        if (!FString(UTF8_TO_TCHAR(Homestead::ItemName(Item))).Replace(TEXT(" "), TEXT("")).Equals(Wanted, ESearchCase::IgnoreCase))
            continue;
        const auto Result = Sim.GrantItems(Item, Amount);
        Notify(UTF8_TO_TCHAR(Result.message.c_str()), !Result);
        return;
    }
    Notify(FString::Printf(TEXT("No item called %s."), *ItemName), true);
}

void AHomesteadController::HomesteadWear(const FString& Garment)
{
    const auto Key = [](FString Text) { return Text.Replace(TEXT(" "), TEXT("")).Replace(TEXT("-"), TEXT("")); };
    for (int32 Index = 0; Index < static_cast<int32>(Homestead::WearableDefinition::Count); ++Index)
    {
        const auto Definition = static_cast<Homestead::WearableDefinition>(Index);
        const auto* Info = Homestead::GetWearableDefinition(Definition);
        if (!Info || (!Key(UTF8_TO_TCHAR(Info->key)).Equals(Key(Garment), ESearchCase::IgnoreCase)
            && !Key(UTF8_TO_TCHAR(Info->name)).Equals(Key(Garment), ESearchCase::IgnoreCase)))
            continue;
        if (Info->fiberCost <= 0 && Info->furCost <= 0) { Notify(TEXT("That garment cannot be made."), true); return; }
        if (Sim.Count(Homestead::Item::Knife) == 0) Sim.GrantItems(Homestead::Item::Knife, 1);
        if (Info->fiberCost > 0) Sim.GrantItems(Homestead::Item::Fiber, Info->fiberCost);
        if (Info->furCost > 0) Sim.GrantItems(Homestead::Item::Fur, Info->furCost);
        FHomesteadRow Recipe;
        Recipe.Subject = EHomesteadMenuSubject::GarmentRecipe;
        Recipe.Id = Recipe.SubjectId = Index;
        if (!MenuItemAction(Recipe, EHomesteadItemAction::Equip, 1, Sim.GetRevision())) return;
        int32 Made = 0;
        for (const auto& Item : State().wearables)
            if (Item.definition == Definition && Item.owner == Homestead::WearableOwner::Carried) Made = FMath::Max(Made, Item.id);
        FHomesteadRow Worn;
        if (Made && MenuWearableRow(Made, Worn)) MenuItemAction(Worn, EHomesteadItemAction::Equip, 1, Sim.GetRevision());
        return;
    }
    Notify(FString::Printf(TEXT("No garment called %s."), *Garment), true);
}

void AHomesteadController::EndPlay(const EEndPlayReason::Type Reason)
{
    HideNames();
    if (ArrivalCard.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(StaticCastSharedPtr<SWidget>(ArrivalCard).ToSharedRef());
    ArrivalCard.Reset();
    HideHotbar();
    HideNativeMenu();
    if (ShopScreen.IsValid()) CloseShopScreen();
    if (const UWorld* World = GetWorld())
        if (FAudioDeviceHandle Device = World->GetAudioDevice())
            Device->SetTransientPrimaryVolume(1.0f);
    Super::EndPlay(Reason);
}

void AHomesteadController::MenuPage(int32 TargetPage)
{
    if (!bMenuSaveInProgress) OpenBook(TargetPage);
}
void AHomesteadController::MenuSelect(int32 Row)
{
    Selection = FMath::Clamp(Row, 0, FMath::Max(0, Rows().Num() - 1));
    if (bBookOpen && Page == 6)
    {
        const auto Items = Rows();
        MenuFocusAppearance(Items.IsValidIndex(Selection) ? Items[Selection].Id : -1);
    }
}
void AHomesteadController::MenuActivate()
{
    if (bMenuSaveInProgress || !bBookOpen) return;
    if (bTestResetRequired && (Page != 4 || !Rows().IsValidIndex(Selection)
        || (Rows()[Selection].Id != 1 && Rows()[Selection].Id != 8 && Rows()[Selection].Id != 9)))
    { Notify(LoadProblem, true); return; }
    if (IsFailed() && Page != 4 && Page != 3 && Page != 5)
    { Notify(TEXT("Retry a checkpoint before changing possessions or appearance."), true); return; }
    if (IsFailed() && Page == 4 && Rows().IsValidIndex(Selection) && Rows()[Selection].Id == 0)
    { Notify(TEXT("A failed state cannot replace your checkpoint. Retry or quit without saving."), true); return; }
    ActivateRow();
}
bool AHomesteadController::MenuCraftRecipe(Homestead::Recipe Recipe)
{
    if (bMenuSaveInProgress || !bBookOpen || Page != 1 || IsFailed() || bTestResetRequired)
        return false;
    const auto Assessment = Sim.AssessRecipe(Recipe, PlayerPoint());
    if (!Assessment.craftable)
    {
        Notify(Text(Assessment.blocker.c_str()), true);
        return false;
    }
    const auto Result = Sim.Craft(Recipe, PlayerPoint());
    Notify(Result);
    if (Result.ok)
    {
        Sim.AdvanceGameHours(0.05, PlayerPoint());
        SlotHaftedTool(Recipe);
    }
    return Result.ok;
}

void AHomesteadController::SlotHaftedTool(Homestead::Recipe Recipe)
{
    Homestead::Item Tool = Homestead::Item::Count;
    switch (Recipe)
    {
    case Homestead::Recipe::HaftAxe: Tool = Homestead::Item::Hatchet; break;
    case Homestead::Recipe::HaftHoe: Tool = Homestead::Item::DiggingStick; break;
    case Homestead::Recipe::HaftScythe: Tool = Homestead::Item::Scythe; break;
    case Homestead::Recipe::HaftBillhook: Tool = Homestead::Item::Billhook; break;
    case Homestead::Recipe::HaftPickaxe: Tool = Homestead::Item::Pickaxe; break;
    default: return;
    }
    // A newly hafted tool goes straight to hand: onto the hotbar if it isn't there, and selected.
    const int32 Value = static_cast<int32>(Tool);
    int32 Slot = HotbarSlots.IndexOfByKey(Value);
    if (Slot == INDEX_NONE)
    {
        Slot = HotbarSlots.IndexOfByKey(-1);
        if (Slot == INDEX_NONE) return;
        HotbarSlots[Slot] = Value;
    }
    SelectedHotbarSlot = Slot;
}
void AHomesteadController::MenuCraftBeat(int32 Beat)
{
    USoundBase* Strikes[] = {CraftStrikeA.Get(), CraftStrikeB.Get(), CraftStrikeC.Get()};
    USoundBase* Strike = Strikes[FMath::Abs(Beat) % UE_ARRAY_COUNT(Strikes)];
    ++TestCraftBeatRequests;
    if (Strike && bAudioEnabled && EffectsVolume > 0) ++TestAudibleCraftBeats;
    PlayEffect(Strike, 0.16f);
}
void AHomesteadController::MenuStore() { if (!bMenuSaveInProgress) Secondary(); }
void AHomesteadController::MenuTake() { if (!bMenuSaveInProgress) Withdraw(); }
void AHomesteadController::MenuBack()
{
    if (bTestResetRequired) { Notify(LoadProblem, true); return; }
    if (!bMenuSaveInProgress) CloseBook();
}
void AHomesteadController::MenuRetry()
{
    if (bMenuSaveInProgress) return;
    RetryCheckpoint();
    if (!IsFailed()) CloseBook();
}
void AHomesteadController::MenuRequestExit() { if (NativeMenu.IsValid()) NativeMenu->RequestExit(); }
void AHomesteadController::MenuSave() { if (!bMenuSaveInProgress) QuickSave(); }
FString AHomesteadController::MenuSaveStatus() const
{
    const FString When = LastSuccessfulSave.GetTicks() > 0
        ? LastSuccessfulSave.ToString(TEXT("%Y-%m-%d %H:%M:%S UTC")) : TEXT("not known in this session");
    const FString Label = CurrentSaveLabel();
    return FString::Printf(TEXT("%s\nLast successful save: %s"),
        !Label.IsEmpty() ? *Label : PreviewLabel().IsEmpty() ? TEXT("Current homestead") : *PreviewLabel(), *When);
}
void AHomesteadController::MenuSaveAndQuit()
{
    if (bMenuSaveInProgress) return;
    if (IsFailed())
    {
        if (NativeMenu.IsValid()) NativeMenu->ShowSaveFailure(TEXT("A failed state cannot replace your checkpoint. Retry or quit without saving."));
        return;
    }
    bMenuSaveInProgress = true;
    const bool Saved = SaveSlot(TEXT("Homestead_Manual"));
    bMenuSaveInProgress = false;
    if (Saved)
    {
        if (PendingResolutionScale.IsSet() && !PersistResolutionScale(PendingResolutionScale.GetValue()))
        {
            if (NativeMenu.IsValid()) NativeMenu->ShowGraphicsSaveFailure(GraphicsSaveError);
            return;
        }
        UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
    }
    else if (NativeMenu.IsValid()) NativeMenu->ShowSaveFailure(ToastText);
}
void AHomesteadController::MenuQuitWithoutSaving()
{
    if (!bMenuSaveInProgress) UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
void AHomesteadController::MenuRestart() { if (!bMenuSaveInProgress) NewGame(); }

void AHomesteadController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AHomesteadController::Interact);
    InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AHomesteadController::Interact);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, this, &AHomesteadController::Interact);
    InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AHomesteadController::Secondary);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &AHomesteadController::Secondary);
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AHomesteadController::UseSelectedTool);
    InputComponent->BindKey(EKeys::Gamepad_RightTrigger, IE_Pressed, this, &AHomesteadController::UseSelectedTool);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AHomesteadController::OpenFocusedChestWithMouse);
    InputComponent->BindKey(EKeys::G, IE_Pressed, this, &AHomesteadController::OpenJournal);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed, this, &AHomesteadController::Withdraw);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AHomesteadController::Back);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &AHomesteadController::Back);
    InputComponent->BindKey(EKeys::I, IE_Pressed, this, &AHomesteadController::ToggleBook);
    InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AHomesteadController::ToggleBook);
    InputComponent->BindKey(EKeys::Gamepad_Special_Right, IE_Pressed, this, &AHomesteadController::OpenSettings);
    InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AHomesteadController::OpenCraft);
    InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AHomesteadController::OpenBuild);
    InputComponent->BindKey(EKeys::H, IE_Pressed, this, &AHomesteadController::OpenJournal);
    InputComponent->BindKey(EKeys::M, IE_Pressed, this, &AHomesteadController::OpenMap);
    InputComponent->BindKey(EKeys::Gamepad_Special_Left, IE_Pressed, this, &AHomesteadController::OpenJournal);
    InputComponent->BindKey(EKeys::Left, IE_Pressed, this, &AHomesteadController::PreviousPage);
    InputComponent->BindKey(EKeys::Right, IE_Pressed, this, &AHomesteadController::NextPage);
    InputComponent->BindKey(EKeys::Gamepad_LeftShoulder, IE_Pressed, this, &AHomesteadController::PreviousPage);
    InputComponent->BindKey(EKeys::Gamepad_RightShoulder, IE_Pressed, this, &AHomesteadController::NextPage);
    InputComponent->BindKey(EKeys::Up, IE_Pressed, this, &AHomesteadController::PreviousRow);
    InputComponent->BindKey(EKeys::Down, IE_Pressed, this, &AHomesteadController::NextRow);
    InputComponent->BindKey(EKeys::Gamepad_DPad_Up, IE_Pressed, this, &AHomesteadController::PreviousRow);
    InputComponent->BindKey(EKeys::Gamepad_DPad_Down, IE_Pressed, this, &AHomesteadController::NextRow);
    InputComponent->BindKey(EKeys::Gamepad_DPad_Left, IE_Pressed, this, &AHomesteadController::PreviousPage);
    InputComponent->BindKey(EKeys::Gamepad_DPad_Right, IE_Pressed, this, &AHomesteadController::NextPage);
    InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AHomesteadController::RotatePlacement);
    InputComponent->BindKey(EKeys::Gamepad_RightThumbstick, IE_Pressed, this, &AHomesteadController::CycleZoom);
    InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &AHomesteadController::QuickSave);
    InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AHomesteadController::QuickLoad);
}

Homestead::Point AHomesteadController::PlayerPoint() const
{
    const FVector Position = GetPawn() ? GetPawn()->GetActorLocation() : PendingLocation;
    return { Position.X, Position.Y };
}

float AHomesteadController::GroundHeight(float X, float Y) const
{
    return AHomesteadWorld::GroundHeight(X, Y, State().world);
}

void AHomesteadController::PrepareEstateSimulation(Homestead::Simulation& Target) const
{
    Target.SetLayout(Homestead::ProvisionalEstateLayout());
    Target.SetPlacements(Homestead::ProvisionalEstatePlacements());
    const TWeakObjectPtr<const AHomesteadController> Self(this);
    Target.SetWaterProbe([Self](Homestead::Point Position)
    {
        // Only fresh water counts for the pail; the sea is salt.
        return Self.IsValid() && Self->WaterEdgeDistance(Position, false) <= 120.0;
    });
}

void AHomesteadController::SetEstateSpawn()
{
    const Homestead::EstateLayout& Layout = Sim.Layout();
    const Homestead::Landmark* Spawn = Layout.FindLandmark(Homestead::Anchor::StandingRoomSpawn);
    const Homestead::Point At = Spawn ? Spawn->position : Homestead::Point{};
    PendingLocation = FVector(At.x, At.y, GroundHeight(At.x, At.y) + 100.0f);
    PendingRotation = FRotator(-12.0f, Spawn ? Spawn->yaw : 0.0f, 0.0f);
    LastSafeWorldPosition = PendingLocation;
    bFreshTerrainSpawn = true;
    EstateSpawnWait = 0;
}

double AHomesteadController::WaterEdgeDistance(Homestead::Point Position, bool bIncludeSea) const
{
    if (!bEstateMap)
        return FMath::Abs(Position.x - Homestead::StreamX(Position.y)) - Homestead::Generation::StreamWaterHalfWidthCm;
    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (Now - EstateWaterScanTime > 5.0 || Now < EstateWaterScanTime)
    {
        // Rivers and ponds are splines tagged HomesteadWater in the Estate level.
        EstateWaterScanTime = Now;
        EstateWaterSplines.Reset();
        if (UWorld* World = GetWorld())
            for (TActorIterator<AActor> It(World); It; ++It)
                if (It->ActorHasTag(TEXT("HomesteadWater")))
                    for (USplineComponent* Spline : TInlineComponentArray<USplineComponent*>(*It))
                        EstateWaterSplines.Add(Spline);
    }
    double Best = TNumericLimits<double>::Max();
    const FVector Here(Position.x, Position.y, GroundHeight(Position.x, Position.y));
    for (const auto& Weak : EstateWaterSplines)
        if (const USplineComponent* Spline = Weak.Get())
        {
            const FVector Point = Spline->FindLocationClosestToWorldLocation(Here, ESplineCoordinateSpace::World);
            const float Key = Spline->FindInputKeyClosestToWorldLocation(Here);
            const double HalfWidth = 100.0 * Spline->GetScaleAtSplineInputKey(Key).Y;
            Best = FMath::Min(Best, FVector::Dist2D(Point, Here) - HalfWidth);
        }
    if (!bIncludeSea)
        return Best;
    // The sea and the estuary: ground below sea level within a couple of metres.
    if (HomesteadEstateTerrain::Height(Position.x, Position.y) < -15.0f)
        return 0.0;
    for (const double Radius : {60.0, 120.0, 180.0, 240.0})
        for (int32 Step = 0; Step < 8; ++Step)
        {
            const double Angle = Step * UE_PI / 4.0;
            if (HomesteadEstateTerrain::Height(Position.x + Radius * FMath::Cos(Angle), Position.y + Radius * FMath::Sin(Angle)) < -15.0f)
                return FMath::Min(Best, Radius);
        }
    return Best;
}
bool AHomesteadController::ResolveDropPoint(Homestead::Point& Result) const
{
    const Homestead::Point PlayerPosition = PlayerPoint();
    const float Yaw = GetPawn() ? GetPawn()->GetActorRotation().Yaw : GetControlRotation().Yaw;
    static constexpr float Angles[] = {0, -35, 35, -70, 70, 180};
    static constexpr float Distances[] = {150, 185, 210};
    for (const float Distance : Distances)
        for (const float Angle : Angles)
        {
            const FVector Direction = FRotator(0, Yaw + Angle, 0).Vector();
            const Homestead::Point Candidate{PlayerPosition.x + Direction.X * Distance,
                PlayerPosition.y + Direction.Y * Distance};
            if (bEstateMap ? WaterEdgeDistance(Candidate) <= 60.0 : Homestead::IsNearWater(Candidate)) continue;
            bool Clear = true;
            for (const auto& Structure : State().structures)
                if (FVector2D::Distance(FVector2D(Candidate.x, Candidate.y),
                    FVector2D(Homestead::StructureCenter(State(), Structure).x,
                    Homestead::StructureCenter(State(), Structure).y)) < 240)
                { Clear = false; break; }
            if (!Clear) continue;
            for (const auto& Plot : State().plots)
                if (FVector2D::Distance(FVector2D(Candidate.x, Candidate.y),
                    FVector2D(Homestead::PlotCenter(Plot).x, Homestead::PlotCenter(Plot).y)) < 90)
                { Clear = false; break; }
            if (!Clear) continue;
            const FVector Center(Candidate.x, Candidate.y,
                GroundHeight(Candidate.x, Candidate.y) + 45);
            FCollisionQueryParams Params(SCENE_QUERY_STAT(HomesteadDropPlacement), false, GetPawn());
            if (GetWorld()->OverlapAnyTestByChannel(Center, FQuat::Identity,
                ECC_WorldStatic, FCollisionShape::MakeSphere(22), Params))
                continue;
            Result = Candidate;
            return true;
        }
    return false;
}

bool AHomesteadController::CollectPreparedBaselines(Homestead::Generation::ChunkCoord Chunk,
    std::array<const Homestead::Generation::ChunkBaseline*, 9>& Prepared) const
{
    bool bComplete = true;
    for (int32 Y = -1; Y <= 1; ++Y)
        for (int32 X = -1; X <= 1; ++X)
        {
            const int32 Index = (Y + 1) * 3 + X + 1;
            Prepared[Index] = Landscape ? Landscape->CachedBaselineFor(
                State().world, {Chunk.x + X, Chunk.y + Y}) : nullptr;
            bComplete &= Prepared[Index] != nullptr;
        }
    return bComplete;
}

bool AHomesteadController::PrepareWorldAt(Homestead::Point Position)
{
    if (State().fixedEstate)
    {
        // The Estate level streams itself; only the first publication needs the world actor.
        if (bWorldReady && Landscape && Landscape->IsPreparedFor(State())) return true;
        if (!Landscape || !Landscape->Refresh(Sim))
        {
            bWorldReady = false;
            Notify(TEXT("The estate could not be prepared. Movement stopped; existing saves are untouched."), true);
            return false;
        }
        bWorldReady = true;
        return true;
    }
    Homestead::Generation::ChunkCoord Chunk;
    if (!FMath::IsFinite(Position.x) || !FMath::IsFinite(Position.y)
        || FMath::Abs(Position.x) > Homestead::MaxWorldCoordinate
        || FMath::Abs(Position.y) > Homestead::MaxWorldCoordinate
        || Homestead::Generation::ChunkAt(FMath::FloorToInt64(Position.x), FMath::FloorToInt64(Position.y), Chunk)
            != Homestead::Generation::Status::Ok)
    {
        Notify(TEXT("This position exceeds the supported 10 km coordinate range. Your world changes are retained."), true);
        return false;
    }
    if (bWorldReady && State().activeChunk == Chunk && Landscape && Landscape->IsPreparedFor(State())) return true;
    const double PreparationStarted = FPlatformTime::Seconds();
    Homestead::Simulation Candidate = Sim;
    Homestead::PreparedWorldRegion Prepared;
    Prepared.world = State().world;
    CollectPreparedBaselines(Chunk, Prepared.chunks);
    const auto Result = Candidate.SetActiveWorldRegion(Position, &Prepared);
    if (!Result) { Notify(Result); return false; }
    LastRegionSimulationMilliseconds = (FPlatformTime::Seconds() - PreparationStarted) * 1000;
    const double PublicationStarted = FPlatformTime::Seconds();
    if (!Landscape || !Landscape->Refresh(Candidate))
    {
        if (Landscape) Landscape->CancelStagedResources();
        bWorldReady = false;
        Notify(TEXT("The next woodland region could not be prepared. Movement stopped; existing saves are untouched."), true);
        return false;
    }
    LastRegionWorldMilliseconds = (FPlatformTime::Seconds() - PublicationStarted) * 1000;
    Sim = MoveTemp(Candidate);
    bWorldReady = true;
    Focus = EFocus::None;
    FocusId = -1;
    RefreshRemaining = 0;
    return true;
}

Homestead::Result AHomesteadController::SpendSprintEnergy(double RealSeconds)
{
    return Sim.SpendSprintEnergy(RealSeconds);
}

bool AHomesteadController::HasHeroine() const
{
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    return Avatar && Avatar->HasHeroine();
}

void AHomesteadController::UpdateCreekAudio()
{
    if (!Creek->Sound || !bAudioEnabled) return;
    FVector Listener;
    FRotator View;
    GetPlayerViewPoint(Listener, View);
    if (bEstateMap)
    {
        // The burble follows the nearest point of the estate's river splines.
        double Best = TNumericLimits<double>::Max();
        FVector Where = FVector::ZeroVector;
        WaterEdgeDistance({Listener.X, Listener.Y});
        for (const auto& Weak : EstateWaterSplines)
            if (const USplineComponent* Spline = Weak.Get())
            {
                const FVector Point = Spline->FindLocationClosestToWorldLocation(Listener, ESplineCoordinateSpace::World);
                const double Distance = FVector::Dist2D(Point, Listener);
                if (Distance < Best) { Best = Distance; Where = Point; }
            }
        if (Best > 6000.0) { if (Creek->IsPlaying()) Creek->FadeOut(2.0f, 0.0f); return; }
        Creek->SetWorldLocation(Where + FVector(0, 0, 20));
        if (!Creek->IsPlaying()) Creek->FadeIn(2.0f, 1.0f);
        return;
    }
    // Nearest point of the meandering centreline: a coarse sweep, then a fine one.
    auto Distance = [&](double Y) { return FMath::Square(Homestead::StreamX(Y) - Listener.X) + FMath::Square(Y - Listener.Y); };
    double BestY = Listener.Y;
    for (double Step : {50.0, 5.0})
    {
        const double From = BestY - (Step > 10 ? 1000.0 : 50.0);
        const double To = BestY + (Step > 10 ? 1000.0 : 50.0);
        for (double Y = From; Y <= To; Y += Step)
            if (Distance(Y) < Distance(BestY)) BestY = Y;
    }
    const APawn* Avatar = GetPawn();
    const double Z = Avatar ? Avatar->GetActorLocation().Z - 60.0 : Listener.Z - 200.0;
    Creek->SetWorldLocation(FVector(Homestead::StreamX(BestY), BestY, Z));
    if (!Creek->IsPlaying()) Creek->FadeIn(2.0f, 1.0f);
}

void AHomesteadController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateArrival();
    if (!Landscape) return;
    if (!StartupProbeDirectory.IsEmpty()) TickStartupProbe();
    UpdateCreekAudio();
#if !UE_BUILD_SHIPPING
    if (bSaveRoutingTestPending && GetPawn())
    {
        bSaveRoutingTestPending = false;
        RunSaveRoutingChecks();
        return;
    }
#endif
    if (bPendingSpawn && GetPawn())
    {
        if (!PrepareWorldAt({PendingLocation.X, PendingLocation.Y})) return;
        const float Ground = GroundHeight(PendingLocation.X, PendingLocation.Y);
        if (bEstateMap && EstateSpawnWait < 20.0f)
        {
            // Wait for World Partition to stream in the ground under the spawn before placing her.
            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(HomesteadEstateSpawn), false, GetPawn());
            if (!GetWorld()->LineTraceSingleByChannel(Hit, FVector(PendingLocation.X, PendingLocation.Y, Ground + 500),
                FVector(PendingLocation.X, PendingLocation.Y, Ground - 500), ECC_WorldStatic, Params))
            {
                EstateSpawnWait += DeltaSeconds;
                GetPawn()->SetActorLocation(FVector(PendingLocation.X, PendingLocation.Y, Ground + 100), false, nullptr, ETeleportType::TeleportPhysics);
                if (auto* Waiting = Cast<AHomesteadCharacter>(GetPawn())) Waiting->GetCharacterMovement()->StopMovementImmediately();
                return;
            }
        }
        EstateSpawnWait = 0;
        PendingLocation.Z = bFreshTerrainSpawn ? Ground + 100.0f : FMath::Max(PendingLocation.Z, Ground + 100.0f);
        GetPawn()->SetActorLocation(PendingLocation, false, nullptr, ETeleportType::TeleportPhysics);
        LastStepPosition = PendingLocation;
        LastSafeWorldPosition = PendingLocation;
        StepDistance = 0;
        // The estate spawn anchor faces the standing-room door on purpose; the open-terrain view search would turn her to a wall.
        if (bFreshTerrainSpawn && !bEstateMap)
            if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
                PendingRotation = Avatar->ChooseStartingView(*Landscape, PendingRotation);
        SetControlRotation(PendingRotation);
        if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
        {
            Avatar->SetRestingViewRotation(PendingRotation);
            if (bEstateMap && bFreshTerrainSpawn) Avatar->SetActorRotation(FRotator(0, PendingRotation.Yaw, 0));
            Avatar->GetCharacterMovement()->StopMovementImmediately();
            Avatar->GetCharacterMovement()->AddTickPrerequisiteActor(this);
            FString Error;
            if (Avatar->PrepareEquipment(State(), Appearance, Error))
            {
                if (!Avatar->ApplyPreparedEquipment(Error)) Notify(Error, true);
            }
            else
            {
                if (!Avatar->IsEquipmentPresentationReady()) Avatar->ApplyAppearance(Appearance);
                Notify(TEXT("Owned clothing could not be displayed; the character preview is provisional. ") + Error, true);
            }
            Avatar->SetAppearancePreview(false);
        }
        bPendingSpawn = false;
        if (bFreshTerrainSpawn) CaptureSessionCheckpoint(PendingLocation, PendingRotation);
        bFreshTerrainSpawn = false;
        RefreshMenuPortrait();
    }
    if (APawn* ControlledPawn = GetPawn())
    {
        const FVector Position = ControlledPawn->GetActorLocation();
        if (!PrepareWorldAt({Position.X, Position.Y}))
        {
            if (auto* Avatar = Cast<AHomesteadCharacter>(ControlledPawn))
                Avatar->GetCharacterMovement()->StopMovementImmediately();
            ControlledPawn->SetActorLocation(LastSafeWorldPosition, false, nullptr, ETeleportType::TeleportPhysics);
            return;
        }
        const float Surface = GroundHeight(Position.X, Position.Y);
        if (bEstateMap && Surface < -70.0f)
        {
            // No swimming in round 1: she wades to about knee depth and no further.
            if (auto* Avatar = Cast<AHomesteadCharacter>(ControlledPawn))
                Avatar->GetCharacterMovement()->StopMovementImmediately();
            ControlledPawn->SetActorLocation(LastSafeWorldPosition, false, nullptr, ETeleportType::TeleportPhysics);
            if (ToastRemaining <= 0) Notify(TEXT("The water's too deep to wade any further."));
            return;
        }
        if (Position.Z < Surface - 200)
        {
            ++WorldRecoveries;
            ControlledPawn->SetActorLocation(FVector(Position.X, Position.Y, Surface + 100),
                false, nullptr, ETeleportType::TeleportPhysics);
            Notify(TEXT("Recovered the character above the generated terrain; this traversal needs review."), true);
        }
        LastSafeWorldPosition = ControlledPawn->GetActorLocation();
    }

    Sim.Advance(DeltaSeconds, PlayerPoint(), bBookOpen || bPlanning || bTestResetRequired || ShopScreen.IsValid());
    TickStores(DeltaSeconds);
    if (bPlanning && !bBookOpen) UpdatePlacement(false);
    if (IsFailed() && !bWasFailed)
    {
        EndPlacement();
        CloseBook();
        if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->GetCharacterMovement()->StopMovementImmediately();
        Notify(TEXT("You could not continue. Retry your latest recovery checkpoint."), true);
    }
    bWasFailed = IsFailed();
    if (const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
    {
        const FVector Position = Avatar->GetActorLocation();
        const float Distance = FVector::Dist2D(Position, LastStepPosition);
        LastStepPosition = Position;
        // The MetaHuman's steps come from footstep notifies on her locomotion clips (PlayFootstep).
        // The legacy heroine's clips have none, so she keeps a step every 70 cm.
        if (!Avatar->IsMetaHumanActive() && !bBookOpen && !bPlanning && !IsFailed()
            && Avatar->GetCharacterMovement()->IsMovingOnGround()
            && Distance < 120 && Avatar->GetVelocity().Size2D() > 12)
        {
            StepDistance += Distance;
            if (StepDistance >= 70)
            {
                StepDistance = FMath::Fmod(StepDistance, 70.0f);
                PlayEffect(bAlternateStep ? GrassStepA.Get() : GrassStepB.Get(), 0.12f);
                bAlternateStep = !bAlternateStep;
            }
        }
        else StepDistance = 0;
    }
    ToastRemaining = FMath::Max(0.0f, ToastRemaining - DeltaSeconds);
    UpdatePendingHack();
    UpdatePendingSwing();
    UpdatePendingFell();
    if (HeldPlot != INDEX_NONE)
    {
        const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
        const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
        // Until the hoe bites (tilling) or she has covered the seed (planting), or it was interrupted.
        const bool bDone = bHeldPlotTilling
            ? !Animation || !Animation->IsTilling() || Animation->TillPhase() >= AHomesteadCharacter::HoeFirstChop
            : !Avatar || !Avatar->IsStickPileOnGround();
        if (bDone || GetWorld()->GetTimeSeconds() - HeldPlotSince > 7.0)
        {
            if (Landscape) Landscape->ReleasePlot();
            HeldPlot = INDEX_NONE;
            bHeldPlotTilling = false;
            RefreshRemaining = 0;
        }
    }
    if (HeldStickPile != INDEX_NONE)
    {
        const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
        if (Avatar && Landscape && Avatar->SticksLiftedFromPile() >= 1)
            for (int32 Part = HeldPartsFirst; Part < HeldPartsFirst + HeldPartsCount; ++Part) Landscape->HideHeldProducePart(Part);
        // Safety limit in paused-aware game time, in case the kneeling clip never starts.
        if (!Avatar || !Avatar->IsStickPileOnGround() || GetWorld()->GetTimeSeconds() - HeldStickPileSince > 6.0)
        {
            if (Landscape) Landscape->ReleaseProduce();
            HeldStickPile = INDEX_NONE;
            RefreshRemaining = 0;
        }
    }
    RefreshRemaining -= DeltaSeconds;
    if (RefreshRemaining <= 0)
    {
        bWorldReady = Landscape->Refresh(Sim);
        if (!bWorldReady) { Notify(TEXT("World refresh failed. Movement is disabled; existing saves are retained."), true); return; }
        UpdateFocus();
        RefreshRemaining = 0.25f;
    }
    if (bWorldReady && !bPendingSpawn && !IsFailed())
        if (APawn* ControlledPawn = GetPawn())
        {
            const FVector Position = ControlledPawn->GetActorLocation();
            const FVector Velocity = ControlledPawn->GetVelocity();
            const auto Chunk = State().activeChunk;
            const double LocalX = Position.X - static_cast<double>(Chunk.x)
                * Homestead::Generation::ChunkSizeCm;
            const double LocalY = Position.Y - static_cast<double>(Chunk.y)
                * Homestead::Generation::ChunkSizeCm;
            const double DistanceX = Velocity.X < -12 ? LocalX
                : Velocity.X > 12 ? Homestead::Generation::ChunkSizeCm - LocalX
                : Homestead::Generation::ChunkSizeCm;
            const double DistanceY = Velocity.Y < -12 ? LocalY
                : Velocity.Y > 12 ? Homestead::Generation::ChunkSizeCm - LocalY
                : Homestead::Generation::ChunkSizeCm;
            if (FMath::Min(DistanceX, DistanceY) <= 900.0)
            {
                const bool bAlongX = DistanceX <= DistanceY;
                const Homestead::Generation::ChunkCoord Next{
                    Chunk.x + (bAlongX ? (Velocity.X < 0 ? -1 : 1) : 0),
                    Chunk.y + (bAlongX ? 0 : (Velocity.Y < 0 ? -1 : 1))};
                const Homestead::Point Target{
                    (static_cast<double>(Next.x) + 0.5) * Homestead::Generation::ChunkSizeCm,
                    (static_cast<double>(Next.y) + 0.5) * Homestead::Generation::ChunkSizeCm};
                Homestead::PreparedWorldRegion Prepared;
                Prepared.world = State().world;
                if (FMath::Abs(Target.x) <= Homestead::MaxWorldCoordinate
                    && FMath::Abs(Target.y) <= Homestead::MaxWorldCoordinate
                    && CollectPreparedBaselines(Next, Prepared.chunks))
                {
                    Homestead::Simulation Destination = Sim;
                    const auto Result = Destination.SetActiveWorldRegion(Target, &Prepared);
                    if (!Result || !Landscape->StageAdjacentResources(Destination, Sim.GetRevision()))
                    {
                        bWorldReady = false;
                        Notify(TEXT("Adjacent woodland resources could not be prepared. Movement stopped; saves are untouched."), true);
                        return;
                    }
                }
            }
            else if (FMath::Min(LocalX, LocalY) > 1000.0
                && FMath::Min(Homestead::Generation::ChunkSizeCm - LocalX,
                    Homestead::Generation::ChunkSizeCm - LocalY) > 1000.0)
                Landscape->CancelStagedResources();
        }
    if (bAutosaveEnabled && !bBookOpen && !bPlanning && !IsFailed() && !bMenuSaveInProgress)
    {
        AutosaveRemaining -= DeltaSeconds;
        if (AutosaveRemaining <= 0)
        {
            if (SaveSlot(FString::Printf(TEXT("Homestead_Auto_%d"), AutoSaveIndex), true))
            {
                AutoSaveIndex = (AutoSaveIndex + 1) % 3;
                AutosaveRemaining = AutosaveMinutes * 60.0f;
            }
            else AutosaveRemaining = 60.0f;
        }
    }
    if (bAudioEnabled && !MusicTracks.IsEmpty())
    {
        if (Music->IsPlaying() && Music->Sound)
        {
            MusicElapsed += DeltaSeconds;
            const float Duration = Music->Sound->GetDuration();
            if (!bMusicFading && Duration > 8 && MusicElapsed >= Duration - 4)
            {
                bMusicFading = true;
                Music->FadeOut(4, 0);
            }
        }
        else if (!bMusicFading)
        {
            MusicGapRemaining -= DeltaSeconds;
            if (MusicGapRemaining <= 0 && MusicVolume > 0)
            {
                MusicElapsed = 0;
                StartNextMusicTrack();
                Music->FadeIn(4, 1);
                // Playback can report "not playing" for a frame after FadeIn; don't advance the bag
                // again. MusicFinished sets the real gap when the track ends.
                MusicGapRemaining = 10;
            }
        }
    }
}

void AHomesteadController::UpdateFocus()
{
    Focus = EFocus::None;
    FocusId = -1;
    const auto Position = PlayerPoint();
    double Best = 280.0;
    auto Consider = [&](EFocus Kind, int Id, Homestead::Point Target)
    {
        const double Distance = FMath::Sqrt(FMath::Square(Target.x - Position.x) + FMath::Square(Target.y - Position.y));
        if (Distance < Best) { Best = Distance; Focus = Kind; FocusId = Id; }
    };
    for (const auto& Node : State().resources)
        if (!Node.cleared) Consider(EFocus::Resource, Node.id, Node.position);
    for (const auto& Drop : State().worldDrops)
        Consider(EFocus::Drop, Drop.id, Drop.position);
    for (const auto& Plot : State().plots)
        Consider(EFocus::Plot, Plot.id, Homestead::PlotCenter(Plot));
    for (const auto& Structure : State().structures)
    {
        EFocus Kind = EFocus::None;
        if (Structure.kind == Homestead::Piece::Fire) Kind = EFocus::Fire;
        if (Structure.kind == Homestead::Piece::Hearth) Kind = EFocus::Hearth;
        if (Structure.kind == Homestead::Piece::Bed) Kind = EFocus::Bed;
        if (Structure.kind == Homestead::Piece::Chest) Kind = EFocus::Chest;
        if (Kind != EFocus::None) Consider(Kind, Structure.id, Homestead::StructureCenter(State(), Structure));
    }
    ConsiderStoreFocus(Consider);
    if (Sim.NearWater(Position))
    {
        // With the watering can out and not full, the stream wins over a crop on the bank when she
        // is at least as close to the water's edge, and always once the can is empty.
        const bool bCan = HotbarSlots.IsValidIndex(SelectedHotbarSlot)
            && HotbarSlots[SelectedHotbarSlot] == static_cast<int32>(Homestead::Item::WateringCan)
            && Sim.Count(Homestead::Item::WateringCan) > 0;
        const int32 Water = Sim.Count(Homestead::Item::Water);
        const double Edge = FMath::Max(0.0, WaterEdgeDistance(Position, false));
        if (Focus == EFocus::None || (bCan && Water < 6 && (Water == 0 || Edge <= Best)))
        {
            Focus = EFocus::Water;
            FocusId = -1;
        }
    }
    // With the machete out, the nearest bush or bramble within arm's reach takes the focus.
    const bool bMachete = HotbarSlots.IsValidIndex(SelectedHotbarSlot)
        && HotbarSlots[SelectedHotbarSlot] == static_cast<int32>(Homestead::Item::Machete)
        && Sim.Count(Homestead::Item::Machete) > 0;
    AHomesteadWorld::FUnderbrushTarget Brush;
    if (bMachete && Landscape && Landscape->FindUnderbrushNear(Sim, FVector2D(Position.x, Position.y), 110.0f, Brush))
    {
        Focus = EFocus::Underbrush;
        FocusId = Brush.Index;
        FocusBrushChunk = Brush.Chunk;
        FocusBrushIndex = Brush.Index;
        FocusBrushSpecies = Brush.Species;
        FocusBrushPosition = Brush.Position;
        bFocusBrushWoody = Brush.bWoody;
    }
}

FString AHomesteadController::FocusTitle() const
{
    switch (Focus)
    {
    case EFocus::Resource:
        for (const auto& Node : State().resources)
            if (Node.id == FocusId)
            {
                FString Status;
                if (Node.kind == Homestead::ResourceKind::ForestTree && Sim.Count(Homestead::Item::Hatchet) == 0)
                    Status = TEXT("  (axe needed)");
                else if (const auto* Overgrowth = Homestead::FindOvergrowth(Node.kind);
                    Overgrowth && Overgrowth->tool != Homestead::ToolKind::Count && !Overgrowth->byHand)
                {
                    const auto Needed = FMath::Max(Overgrowth->minTier, Node.minTier);
                    if (Sim.Count(Homestead::ToolItem(Overgrowth->tool)) == 0)
                        Status = FString::Printf(TEXT("  (%s needed)"), UTF8_TO_TCHAR(Homestead::ToolName(Overgrowth->tool)));
                    else if (Sim.GetToolTier(Overgrowth->tool) < Needed)
                        Status = TEXT("  (") + Text(Homestead::NeedsToolMessage(Overgrowth->tool, Needed).c_str()).ToLower() + TEXT(")");
                }
                if (Node.kind == Homestead::ResourceKind::DeerRemains) return TEXT("Deer bones");
                return Text(Homestead::ResourceName(Node.kind)) + Status;
            }
        break;
    case EFocus::Plot:
        for (const auto& Plot : State().plots)
        {
            if (Plot.id != FocusId) continue;
            if (!Plot.planted) return TEXT("A little patch of earth");
            return FString::Printf(TEXT("%s  |  %d%% grown  |  %d%% watered  |  %d%% weeds"),
                *Text(Homestead::CropName(Plot.kind)), FMath::RoundToInt(Plot.growth * 100),
                FMath::RoundToInt(Plot.moisture * 100), FMath::RoundToInt(Plot.weeds * 100));
        }
        break;
    case EFocus::Drop:
        for (const auto& Drop : State().worldDrops)
            if (Drop.id == FocusId)
            {
                if (Drop.wearableId)
                {
                    const auto* Wearable = Sim.GetWearable(Drop.wearableId);
                    return Wearable ? Text(Homestead::WearableName(Wearable->definition))
                        : TEXT("Dropped garment");
                }
                return FString::Printf(TEXT("%s x%d"),
                    *Text(Homestead::ItemName(Drop.item)), Drop.quantity);
            }
        break;
    case EFocus::Fire: return TEXT("Cookfire");
    case EFocus::Hearth: return TEXT("Hearth");
    case EFocus::Bed: return TEXT("Bedroll");
    case EFocus::Chest: return TEXT("Storage chest");
    case EFocus::Water: return TEXT("Fresh stream water");
    case EFocus::Underbrush: return AHomesteadWorld::UnderbrushName(FocusBrushSpecies);
    case EFocus::Shopkeeper:
    case EFocus::StoreDoor: return StoreFocusTitle();
    default: break;
    }
    return TEXT("Woodland");
}

FString AHomesteadController::FocusActions() const
{
    const FString A = bGamepad ? TEXT("[A]") : TEXT("[E]");
    const FString X = bGamepad ? TEXT("[X]") : TEXT("[F]");
    const FString Use = bGamepad ? TEXT("[RT]") : TEXT("[LMB]");
    Homestead::Item SelectedTool = Homestead::Item::Count;
    const bool ToolAvailable = HotbarSlots.IsValidIndex(SelectedHotbarSlot)
        && HotbarSlots[SelectedHotbarSlot] >= 0
        && IsHotbarTool(SelectedTool = static_cast<Homestead::Item>(
            HotbarSlots[SelectedHotbarSlot]))
        && Sim.Count(SelectedTool) > 0;
    switch (Focus)
    {
    case EFocus::Resource:
        for (const auto& Node : State().resources)
            if (Node.id == FocusId)
            {
                if (Node.readyAtHour > State().hour) return FString();
                if (Node.kind == Homestead::ResourceKind::ForestTree)
                    return ToolAvailable && SelectedTool == Homestead::Item::Hatchet
                        ? Use + TEXT(" Fell with Axe") : TEXT("Select the axe to fell");
                if (Node.kind == Homestead::ResourceKind::DeerRemains || Node.kind == Homestead::ResourceKind::Reeds)
                    return FString();
                if (const auto* Overgrowth = Homestead::FindOvergrowth(Node.kind))
                {
                    const bool Handles = ToolAvailable && Homestead::ToolForItem(SelectedTool) == Overgrowth->tool;
                    if (Node.kind == Homestead::ResourceKind::SalvagePile) return A + TEXT(" Search");
                    if (Handles) return Use + TEXT(" ") + SwingVerb(SelectedTool)
                        + (Overgrowth->byHand ? TEXT("   ") + A + TEXT(" Gather") : FString());
                    if (Overgrowth->byHand) return A + TEXT(" Gather");
                    return TEXT("Select the ") + FString(UTF8_TO_TCHAR(Homestead::ToolName(Overgrowth->tool)));
                }
                return A + TEXT(" Gather");
            }
        return A + TEXT(" Gather");
    case EFocus::Plot:
        for (const auto& Plot : State().plots)
            if (Plot.id == FocusId)
            {
                if (!Plot.planted) return A + TEXT(" Plant roots   ") + X + TEXT(" Plant berry seeds");
                if (Plot.growth >= 1) return A + TEXT(" Harvest");
                if (ToolAvailable && SelectedTool == Homestead::Item::WateringCan)
                    return Use + TEXT(" Water");
                if (ToolAvailable && SelectedTool == Homestead::Item::DiggingStick)
                    return Use + TEXT(" Weed");
                return TEXT("Select the pail or hoe");
            }
        break;
    case EFocus::Fire: return A + TEXT(" Cook   ") + X + TEXT(" Add firewood / branch");
    case EFocus::Hearth: return A + TEXT(" Cook");
    case EFocus::Drop: return A + TEXT(" Pick up");
    case EFocus::Bed: return A + TEXT(" Sleep 8 hours");
    case EFocus::Chest: return A + TEXT(" Open pack / storage");
    case EFocus::Water: return ToolAvailable && SelectedTool == Homestead::Item::WateringCan
        ? Use + TEXT(" Fill Pail") : A + TEXT(" Fill carried Pail");
    case EFocus::Underbrush: return Use + TEXT(" Clear with Machete");
    case EFocus::Shopkeeper:
    case EFocus::StoreDoor: return StoreFocusActions();
    default: return ToolAvailable && SelectedTool == Homestead::Item::DiggingStick
        ? Use + TEXT(" Till ground") : (bGamepad ? TEXT("[Menu] Field book") : TEXT("[I] Field book"));
    }
    return FString();
}

FString AHomesteadController::HintId(const FString& Verb) const
{
    FString Noun;
    if (Focus == EFocus::Resource)
        for (const auto& Node : State().resources)
            if (Node.id == FocusId)
            {
                Noun = Node.kind == Homestead::ResourceKind::ForestTree || Node.kind == Homestead::ResourceKind::Sapling
                    ? FString(TEXT("Tree")) : Text(Homestead::ResourceName(Node.kind));
                break;
            }
    FString Id;
    for (const TCHAR Letter : Verb + TEXT("_") + Noun)
        if (FChar::IsAlnum(Letter) || Letter == TEXT('_')) Id.AppendChar(Letter);
    return Id;
}

int32 AHomesteadController::HintUseCount(const FString& Verb) const
{
    const int32* Uses = HintUses.Find(HintId(Verb));
    return Uses ? *Uses : 0;
}

bool AHomesteadController::IsHintRetired(const FString& Verb) const
{
    return HintUseCount(Verb) >= HintRetireUses;
}

AHomesteadController::FHintUse AHomesteadController::BeginHintUse(const FString& Button) const
{
    FHintUse Use;
    Use.Serial = NoticeSerial;
    Use.bHackPending = bHackPending;
    TArray<FString> Parts;
    FocusActions().ParseIntoArray(Parts, TEXT("   "));
    const FString Prefix = TEXT("[") + Button + TEXT("] ");
    for (const FString& Part : Parts)
        if (Part.TrimStartAndEnd().StartsWith(Prefix))
        {
            Use.Id = HintId(Part.TrimStartAndEnd().Mid(Prefix.Len()));
            break;
        }
    return Use;
}

void AHomesteadController::EndHintUse(const FHintUse& Use)
{
    if (Use.Id.IsEmpty()) return;
    const bool Succeeded = (NoticeSerial != Use.Serial && !bToastError) || (!Use.bHackPending && bHackPending);
    if (!Succeeded) return;
    int32& Uses = HintUses.FindOrAdd(Use.Id);
    if (Uses >= HintRetireUses) return;
    ++Uses;
    if (const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr)
        if (PersistIntProperty(Branch->IniPath, ActionHintSection, *Use.Id, Uses))
            GConfig->SetInt(ActionHintSection, *Use.Id, Uses, GGameUserSettingsIni);
}

void AHomesteadController::LoadActionHints()
{
    HintUses.Reset();
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    FConfigFile Disk;
    if (!Branch || !Disk.Combine(Branch->IniPath)) return;
    if (const FConfigSection* Section = Disk.FindSection(ActionHintSection))
        for (const auto& Pair : *Section)
        {
            const int32 Uses = FCString::Atoi(*Pair.Value.GetValue());
            if (Uses > 0) HintUses.Add(Pair.Key.ToString(), FMath::Min(Uses, HintRetireUses));
        }
}

void AHomesteadController::ResetActionHints()
{
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    for (const auto& Pair : HintUses)
        if (Branch && PersistIntProperty(Branch->IniPath, ActionHintSection, *Pair.Key, 0))
            GConfig->SetInt(ActionHintSection, *Pair.Key, 0, GGameUserSettingsIni);
    HintUses.Reset();
    Notify(TEXT("Action hints will show again for your next few tries."));
}

void AHomesteadController::Notify(const Homestead::Result& Result, USoundBase* SuccessCue)
{
    Notify(UTF8_TO_TCHAR(Result.message.c_str()), !Result.ok);
    if (Result.ok && SuccessCue) PlayEffect(SuccessCue);
    RefreshRemaining = 0;
}

void AHomesteadController::NotifyResourceAction(const Homestead::Result& Result, USoundBase* SuccessCue)
{
    if (!Result.ok)
    {
        Notify(Result);
        return;
    }
    ToastText.Reset();
    ToastRemaining = 0;
    bToastError = false;
    ++NoticeSerial;
    if (SuccessCue) PlayEffect(SuccessCue);
    RefreshRemaining = 0;
}

void AHomesteadController::StartMacheteHack()
{
    if (bHackPending || Focus != EFocus::Underbrush) return;
    if (Sim.Count(Homestead::Item::Machete) == 0)
    {
        Notify(TEXT("Take your machete from storage to hack through undergrowth."), true);
        return;
    }
    // Refuse before the swing rather than after it when she's too tired to clear this plant.
    const auto Rested = Sim.CheckExertion(bFocusBrushWoody
        ? Homestead::Exertion::WoodyUnderbrushEnergy : Homestead::Exertion::SoftUnderbrushEnergy);
    if (!Rested)
    {
        Notify(Rested);
        return;
    }
    const Homestead::Point Target{FocusBrushPosition.X, FocusBrushPosition.Y};
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    HackChunk = FocusBrushChunk;
    HackIndex = FocusBrushIndex;
    HackPosition = FocusBrushPosition;
    bHackWoody = bFocusBrushWoody;
    if (Avatar && Avatar->PlayMacheteHack(Target))
    {
        bHackPending = true;
        HackSince = GetWorld()->GetTimeSeconds();
        return;
    }
    // No hacking clip (legacy heroine): clear at once with the generic swing.
    const auto Result = Sim.ClearUnderbrush({HackChunk.X, HackChunk.Y}, HackIndex, bHackWoody, Target, PlayerPoint());
    NotifyResourceAction(Result, WoodTapB);
    if (Result.ok && Avatar) Avatar->PlayClear(Target);
}

void AHomesteadController::PresentFelling(int32 ResourceId, Homestead::Point Target, bool bTree)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar) return;
    // Called just after the clear committed, before the woodland rebuilds, so the tree is still
    // in the world's batches. A mature tree takes three strokes; a sapling one.
    FVector2D Trunk(Target.x, Target.y);
    float Radius = 5.0f;
    const bool bTreeTrunk = (Landscape && Landscape->TreeChopTarget(ResourceId, Trunk, Radius)) || bTree;
    const auto* Animation = Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance());
    const int32 Strokes = bTreeTrunk ? 3 : 1;
    if (Animation && Avatar->PlayFell({Trunk.X, Trunk.Y}, Strokes, FMath::Max(Radius, 5.0f)))
    {
        if (Landscape) Landscape->BeginFelling(ResourceId);
        FellResource = ResourceId;
        FellStrokes = Strokes;
        FellStrokesHeard = 0;
        FellStartsBefore = Animation->FellStarts();
        bFellSeen = false;
        FellSince = GetWorld()->GetTimeSeconds();
        return;
    }
    Avatar->PlayClear(Target);
}

void AHomesteadController::UpdatePendingFell()
{
    FVector Landing;
    if (Landscape && Landscape->TakeFelledTreeLanding(Landing))
    {
        if (TreeFallThud) PlayEffect(TreeFallThud, 0.7f);
        else PlayEffect(WoodTapA, 0.6f);
    }
    if (FellResource == INDEX_NONE) return;
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    const bool Felling = Animation && Animation->IsFelling() && Animation->FellStarts() != FellStartsBefore;
    bFellSeen |= Felling;
    // Waiting for the clip to start (the request lands on the next animation update).
    if (!bFellSeen && Animation && (Avatar->IsApproachingFell() || GetWorld()->GetTimeSeconds() - FellSince < 0.5))
    {
        if (Avatar->IsApproachingFell()) FellSince = GetWorld()->GetTimeSeconds();
        return;
    }
    const float Phase = Felling ? Animation->FellPhase() : 1e6f;
    while (Felling && FellStrokesHeard < FellStrokes
        && Phase >= AHomesteadCharacter::FellStrikeSeconds(FellStrokesHeard))
    {
        // About 7 dB under the old wood taps; the hammer-cut variant is a touch hotter.
        if (!ChopStrokes.IsEmpty())
        {
            const int32 Pick = FellStrokesHeard % ChopStrokes.Num();
            PlayEffect(ChopStrokes[Pick].Get(), Pick == 2 ? 0.65f : 0.8f);
        }
        else PlayEffect(FellStrokesHeard % 2 ? WoodTapA.Get() : WoodTapB.Get(), 0.55f);
        ++FellStrokesHeard;
    }
    // The last stroke through the notch, or she stopped: the tree goes over.
    if (!Felling || Phase >= AHomesteadCharacter::FellStrikeSeconds(FellStrokes - 1) + 0.2f)
    {
        if (Landscape)
        {
            const FVector From = Avatar ? Avatar->GetActorLocation() : FVector::ZeroVector;
            Landscape->DropFelledTree(FVector2D(From.X, From.Y));
        }
        FellResource = INDEX_NONE;
    }
}

void AHomesteadController::UpdatePendingHack()
{
    if (!bHackPending) return;
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    if (Animation && Animation->IsHacking() && Animation->MachetePhase() >= AHomesteadCharacter::MacheteClearSeconds)
    {
        bHackPending = false;
        const auto Result = Sim.ClearUnderbrush({HackChunk.X, HackChunk.Y}, HackIndex, bHackWoody,
            {HackPosition.X, HackPosition.Y}, PlayerPoint());
        NotifyResourceAction(Result, WoodTapB);
        return;
    }
    // Interrupted (she moved, opened the book) before the cut landed: nothing is cleared.
    if (!Animation || (!Animation->IsHacking() && GetWorld()->GetTimeSeconds() - HackSince > 0.4))
        bHackPending = false;
}

void AHomesteadController::ResetOvergrowthSwing()
{
    SwingNode = INDEX_NONE;
    SwingsLanded = 0;
    bSwingPending = false;
    ScytheTargets.Reset();
}

void AHomesteadController::SwingAtOvergrowth(Homestead::Item Tool)
{
    if (bSwingPending) return;
    const auto Position = PlayerPoint();
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const FVector Forward = GetPawn() ? GetPawn()->GetActorForwardVector() : FVector::ForwardVector;
    const Homestead::Point Facing{Forward.X, Forward.Y};
    // Walking off resets a half-cleared target: its earlier blows never persist.
    if (SwingNode != INDEX_NONE && FVector2D::Distance(SwingFrom, FVector2D(Position.x, Position.y)) > 150.0)
        ResetOvergrowthSwing();

    int32 Target = INDEX_NONE;
    if (Tool == Homestead::Item::Scythe)
    {
        const auto Arc = Sim.ScytheArcTargets(Position, Facing);
        ScytheTargets.Reset();
        for (const int32 Id : Arc) if (Sim.CheckOvergrowth(Id, Tool, Position)) ScytheTargets.Add(Id);
        if (!ScytheTargets.IsEmpty()) Target = ScytheTargets[0];
    }
    else
    {
        // What she's aimed at wins; otherwise the nearest thing this tool handles just ahead of her.
        const Homestead::Point Ahead{Position.x + Forward.X * 80.0, Position.y + Forward.Y * 80.0};
        Target = Sim.FindNearestOvergrowth(Ahead, 200.0, Tool);
        if (Focus == EFocus::Resource)
            for (const auto& Node : State().resources)
                if (Node.id == FocusId && Homestead::IsOvergrowth(Node.kind)
                    && Homestead::FindOvergrowth(Node.kind)->tool == Homestead::ToolForItem(Tool))
                    Target = FocusId;
    }
    if (Target == INDEX_NONE)
    {
        // Aimed at overgrowth another tool clears: say which.
        if (Focus == EFocus::Resource)
            for (const auto& Node : State().resources)
                if (Node.id == FocusId && Homestead::IsOvergrowth(Node.kind))
                {
                    const auto Check = Sim.CheckOvergrowth(FocusId, Tool, Position);
                    // This tool handles it, but it's past the swing's reach (the scythe's arc is short).
                    if (Check) Notify(Tool == Homestead::Item::Scythe ? TEXT("Step closer to mow.") : TEXT("Step closer."), true);
                    else Notify(Check);
                    return;
                }
        Notify(Tool == Homestead::Item::Scythe ? TEXT("Face tall grass or weeds to mow.")
            : Tool == Homestead::Item::Billhook ? TEXT("Aim at bramble or a sapling.")
            : Tool == Homestead::Item::Pickaxe ? TEXT("Aim at rubble or a rock.")
            : TEXT("Aim at a tree, stump or fallen timber."), true);
        return;
    }
    const auto Ready = Sim.CheckOvergrowth(Target, Tool, Position);
    if (!Ready)
    {
        Notify(Ready);
        // Out of tier: the blade glances off with a dull knock, and nothing changes.
        if (Ready.code == Homestead::ResultCode::ToolTier)
        {
            PlayEffect(WoodTapA, 0.45f);
            if (Avatar) Avatar->PlayClear();
        }
        return;
    }
    if (Target != SwingNode)
    {
        SwingNode = Target;
        SwingsLanded = 0;
    }
    SwingFrom = FVector2D(Position.x, Position.y);
    SwingTool = Tool;
    Homestead::Point Aim = Position;
    auto Kind = Homestead::ResourceKind::Count;
    for (const auto& Node : State().resources) if (Node.id == Target) { Aim = Node.position; Kind = Node.kind; }
    bool bAnimated = false;
    bSwingFellTimed = false;
    if (Avatar)
    {
        // Rough footprint radius (cm) of what she strikes, so the point or bit lands on its near side.
        const float Radius = Kind == Homestead::ResourceKind::StumpSmall ? 16.0f
            : Kind == Homestead::ResourceKind::StumpLarge ? 28.0f
            : Kind == Homestead::ResourceKind::StumpAncient ? 45.0f
            : Kind == Homestead::ResourceKind::FallenLog ? 20.0f
            : Kind == Homestead::ResourceKind::GiantLog ? 38.0f
            : Kind == Homestead::ResourceKind::Rubble ? 35.0f
            : Kind == Homestead::ResourceKind::Boulder ? 50.0f
            : Kind == Homestead::ResourceKind::SmallRock ? 18.0f : 8.0f;
        if (Tool == Homestead::Item::Scythe)
        {
            // Mowing turns about her: she keeps facing the swath rather than the first tuft.
            const Homestead::Point Ahead{Position.x + Forward.X * 100.0, Position.y + Forward.Y * 100.0};
            bSwingFellTimed = Avatar->PlayStrike(Ahead, Tool, 1, -1.0f);
        }
        else if (Tool == Homestead::Item::Hatchet || Tool == Homestead::Item::Pickaxe)
        {
            bSwingFellTimed = Avatar->PlayStrike(Aim, Tool, 1, Radius);
            // Without the strike clip the axe falls back to its felling chop.
            if (!bSwingFellTimed && Tool == Homestead::Item::Hatchet) bSwingFellTimed = Avatar->PlayFell(Aim, 1, 12.0f);
        }
        bAnimated = bSwingFellTimed;
        // The billhook reuses the machete hack; a scythe without its mowing clip borrows it too.
        if (!bAnimated && Tool != Homestead::Item::Pickaxe && Tool != Homestead::Item::Hatchet)
            bAnimated = Avatar->PlayMacheteHack(Aim, Tool);
    }
    if (!bAnimated)
    {
        if (Avatar) Avatar->PlayClear(Aim);
        LandOvergrowthSwing();
        return;
    }
    bSwingPending = true;
    SwingSince = GetWorld()->GetTimeSeconds();
    if (const auto* Animation = Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()))
        SwingFellStartsBefore = Animation->FellStarts();
}

void AHomesteadController::UpdatePendingSwing()
{
    if (!bSwingPending) return;
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    const double Age = GetWorld()->GetTimeSeconds() - SwingSince;
    if (bSwingFellTimed)
    {
        const bool Felling = Animation && Animation->IsFelling() && Animation->FellStarts() != SwingFellStartsBefore;
        if (Felling && Animation->FellPhase() >= AHomesteadCharacter::FellStrikeSeconds(0))
        {
            bSwingPending = false;
            LandOvergrowthSwing();
            return;
        }
        // Still stepping into the stance, or the clip hasn't started yet.
        if (!Felling && Animation && (Avatar->IsApproachingFell() || Age < 0.5))
        {
            if (Avatar->IsApproachingFell()) SwingSince = GetWorld()->GetTimeSeconds();
            return;
        }
        if (!Felling) bSwingPending = false;
        return;
    }
    if (Animation && Animation->IsHacking() && Animation->MachetePhase() >= AHomesteadCharacter::MacheteClearSeconds)
    {
        bSwingPending = false;
        LandOvergrowthSwing();
        return;
    }
    // Interrupted before the blow landed: this swing doesn't count.
    if (!Animation || (!Animation->IsHacking() && Age > 0.4)) bSwingPending = false;
}

void AHomesteadController::LandOvergrowthSwing()
{
    const auto Position = PlayerPoint();
    if (SwingTool == Homestead::Item::Scythe)
    {
        // One sweep mows everything in the arc, each tuft its own transaction, with one summary.
        const int32 HayBefore = Sim.Count(Homestead::Item::Hay), WeedsBefore = Sim.Count(Homestead::Item::Weeds);
        int32 Mown = 0;
        FString Problem;
        for (const int32 Id : ScytheTargets)
        {
            const auto Result = Sim.ClearOvergrowth(Id, Homestead::Item::Scythe, Position);
            if (Result.ok) ++Mown;
            else if (Problem.IsEmpty()) Problem = UTF8_TO_TCHAR(Result.message.c_str());
        }
        ResetOvergrowthSwing();
        if (Mown == 0)
        {
            Notify(Problem.IsEmpty() ? TEXT("Nothing left in reach to mow.") : Problem, true);
            return;
        }
        FString Summary = FString::Printf(TEXT("Mowed %d %s"), Mown, Mown == 1 ? TEXT("tuft") : TEXT("tufts"));
        const int32 Hay = Sim.Count(Homestead::Item::Hay) - HayBefore, Weeds = Sim.Count(Homestead::Item::Weeds) - WeedsBefore;
        if (Hay > 0 || Weeds > 0) Summary += TEXT(":");
        if (Hay > 0) Summary += FString::Printf(TEXT(" +%d Hay"), Hay);
        if (Weeds > 0) Summary += FString::Printf(TEXT("%s +%d Weeds"), Hay > 0 ? TEXT(",") : TEXT(""), Weeds);
        Notify(Summary + TEXT("."));
        PlayEffect(GrassStepA, 0.8f);
        return;
    }
    if (SwingNode == INDEX_NONE) return;
    ++SwingsLanded;
    const int32 Needed = Sim.OvergrowthSwings(SwingNode);
    if (SwingsLanded < Needed)
    {
        // A hit that doesn't break it yet: a chop or a crack, and how much is left.
        if (!ChopStrokes.IsEmpty()) PlayEffect(ChopStrokes[SwingsLanded % ChopStrokes.Num()].Get(), 0.75f);
        else PlayEffect(WoodTapA, 0.6f);
        const int32 Left = Needed - SwingsLanded;
        Notify(FString::Printf(TEXT("%d more %s."), Left, Left == 1 ? TEXT("swing") : TEXT("swings")));
        return;
    }
    const auto Result = Sim.ClearOvergrowth(SwingNode, SwingTool, Position);
    ResetOvergrowthSwing();
    Notify(Result, SwingTool == Homestead::Item::Pickaxe ? CraftStrikeA.Get() : WoodTapB.Get());
}

void AHomesteadController::Notify(const FString& Message, bool Error)
{
    ++NoticeSerial;
    ToastText = Message;
    bToastError = Error;
    ToastRemaining = Error ? 8 : 5;
    if (Error) UE_LOG(LogTemp, Warning, TEXT("Homestead: %s"), *Message);
}

void AHomesteadController::Interact()
{
    if (IsFailed()) { RetryCheckpoint(); return; }
    if (bBookOpen) { ActivateRow(); return; }
    const auto Position = PlayerPoint();
    if (!bWorldReady || !PrepareWorldAt(Position)) return;
    if (bPlanning)
    {
        UpdatePlacement(true);
        const auto Result = Sim.Place(BuildTarget, Position);
        Notify(Result, WoodTapA);
        if (Result.ok) Sim.AdvanceGameHours(0.1, Position);
        UpdatePlacement(true);
        return;
    }
    UpdateFocus();
    const FHintUse Hint = BeginHintUse(bGamepad ? TEXT("A") : TEXT("E"));
    ON_SCOPE_EXIT { EndHintUse(Hint); };
    switch (Focus)
    {
    case EFocus::Resource:
    {
        bool Forage = false;
        bool Tree = false;
        bool Reeds = false;
        bool Sticks = false;
        auto Kind = Homestead::ResourceKind::Count;
        Homestead::Point ActionTarget = Position;
        for (const auto& Node : State().resources)
            if (Node.id == FocusId)
            {
                Tree = Node.kind == Homestead::ResourceKind::ForestTree;
                Reeds = Node.kind == Homestead::ResourceKind::Reeds;
                Sticks = Node.kind == Homestead::ResourceKind::Branches;
                Kind = Node.kind;
                Forage = !Tree && Node.kind != Homestead::ResourceKind::Sapling;
                ActionTarget = Node.position;
                break;
            }
        const int32 Harvested = FocusId;
        const auto* Feller = Cast<AHomesteadCharacter>(GetPawn());
        const bool bFell = Tree && Feller && Feller->CanFell();
        const auto Result = Sim.Harvest(FocusId, Position);
        // Salvage and fallen boughs say what she found; ordinary forage shows it in her hands instead.
        if (Homestead::IsOvergrowth(Kind)) Notify(Result, WoodTapA);
        else NotifyResourceAction(Result, bFell ? nullptr : Tree ? WoodTapB.Get() : GrassStepA.Get());
        if (Result.ok && Forage)
            if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
                if (Reeds)
                {
                    // Kneel, gather the stems in her left fist and saw them free with the knife.
                    if (Avatar->PlayKneelGather(EHomesteadKneelGather::Reeds, FVector2D(ActionTarget.x, ActionTarget.y)) && Landscape)
                    {
                        Landscape->HoldProduce(FocusId);
                        HeldStickPile = FocusId;
                        HeldStickPileSince = GetWorld()->GetTimeSeconds();
                        HeldPartsFirst = 0;
                        HeldPartsCount = 1;
                    }
                    else if (!Avatar->IsCuttingReeds()) Avatar->PlayKnifeCut(ActionTarget);
                }
                else if (Kind == Homestead::ResourceKind::DeerRemains)
                    Avatar->PlayKnifeCut(ActionTarget); // Work the dried hide free with the knife.
                else if (Sticks || Kind == Homestead::ResourceKind::Stones || Kind == Homestead::ResourceKind::Roots
                    || Kind == Homestead::ResourceKind::BerryBush)
                {
                    const bool Berries = Kind == Homestead::ResourceKind::BerryBush;
                    const auto Gather = Sticks ? EHomesteadKneelGather::Sticks
                        : Kind == Homestead::ResourceKind::Stones ? EHomesteadKneelGather::Stones : EHomesteadKneelGather::Pouch;
                    FVector2D Target(ActionTarget.x, ActionTarget.y);
                    // Berries are picked from the near side of the bush, not its centre.
                    if (Berries)
                    {
                        const FVector2D Toward = FVector2D(Position.x, Position.y) - Target;
                        if (Toward.Size() > 1.0f) Target += Toward.GetSafeNormal() * 22.0f;
                    }
                    if (Avatar->PlayKneelGather(Gather, Target, Berries) && Landscape)
                    {
                        Landscape->HoldProduce(FocusId);
                        HeldStickPile = FocusId;
                        HeldStickPileSince = GetWorld()->GetTimeSeconds();
                        // Sticks and stones: component 1 is the first one lifted. Berries: the
                        // first half of the bush's clusters. Roots: the one root crown.
                        HeldPartsFirst = Berries ? 0 : Gather == EHomesteadKneelGather::Pouch ? 0 : 1;
                        HeldPartsCount = Berries ? 4 : 1;
                    }
                }
                else Avatar->PlayGather();
        if (Result.ok && Tree) PresentFelling(Harvested, ActionTarget, true);
        break;
    }
    case EFocus::Drop:
        Notify(Sim.PickUpDrop(FocusId, Position));
        break;
    case EFocus::Plot:
        for (const auto& Plot : State().plots)
        {
            if (Plot.id != FocusId) continue;
            const bool Planted = Plot.planted;
            const bool Mature = Plot.growth >= 1;
            if (!Planted)
            {
                PlantFocusedPlot(Homestead::CropKind::Roots);
                break;
            }
            const auto Result = Mature ? Sim.HarvestCrop(FocusId, Position) : Sim.Water(FocusId, Position);
            Notify(Result, GrassStepB);
            if (Result.ok && !Mature)
                if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
                    Avatar->PlayWater(Homestead::PlotCenter(Plot));
            break;
        }
        break;
    case EFocus::Fire:
    case EFocus::Hearth:
        OpenBook(1);
        Selection = static_cast<int32>(Homestead::Recipe::RoastedRoots);
        if (NativeMenu && !NativeMenu->FocusSubject(EHomesteadMenuSubject::Recipe, Selection, 0))
            Notify(TEXT("The cookfire recipe could not be selected."), true);
        break;
    case EFocus::Bed:
    {
        Notify(Sim.Sleep(8, Position));
        if (!IsFailed())
        {
            if (bAutosaveEnabled && SaveSlot(FString::Printf(TEXT("Homestead_Auto_%d"), AutoSaveIndex), true))
                AutoSaveIndex = (AutoSaveIndex + 1) % 3;
            if (Sim.IsSheltered(Position) && State().hunger >= 35)
                SaveSlot(TEXT("Homestead_Recovery"), true);
        }
        break;
    }
    case EFocus::Chest: OpenChestStorage(FocusId); break;
    case EFocus::Water: Notify(Sim.FillWater(Position)); break;
    case EFocus::Underbrush: StartMacheteHack(); break;
    case EFocus::Shopkeeper:
    case EFocus::StoreDoor: InteractWithStore(); break;
    default: Notify(TEXT("Walk closer to a plant, resource, or work area.")); break;
    }

}

void AHomesteadController::OpenFocusedChestWithMouse()
{
    if (bBookOpen || bPlanning || IsFailed() || !bWorldReady) return;
    UpdateFocus();
    if (Focus == EFocus::Chest) OpenChestStorage(FocusId);
}

bool AHomesteadController::OpenChestStorage(int32 ChestId)
{
    if (bPlanning || IsFailed() || ChestId <= 0) return false;
    const auto Position = PlayerPoint();
    const Homestead::Structure* Target = nullptr;
    for (const auto& Structure : State().structures)
        if (Structure.id == ChestId && Structure.kind == Homestead::Piece::Chest)
        { Target = &Structure; break; }
    if (!Target)
    { Notify(TEXT("That storage chest is no longer available."), true); return false; }
    const auto Center = Homestead::StructureCenter(State(), *Target);
    if (FMath::Square(Center.x - Position.x) + FMath::Square(Center.y - Position.y)
        > FMath::Square(Homestead::ChestReach))
    { Notify(TEXT("Move within 280 cm of this chest."), true); return false; }
    ActiveChestId = ChestId;
    MenuInventoryViewIndex = 1;
    OpenBook(0);
    return true;
}

void AHomesteadController::Secondary()
{
    if (IsFailed()) return;
    if (bBookOpen)
    {
        if (Page != 0) return;
        const auto Items = Rows();
        if (!Items.IsValidIndex(Selection)) return;
        const int ChestId = Sim.FindNearestStructure(PlayerPoint(), Homestead::Piece::Chest, 280);
        if (ChestId < 0) { Notify(TEXT("Stand near a storage chest to put items away."), true); return; }
        const auto Item = static_cast<Homestead::Item>(Items[Selection].Id);
        Notify(Sim.Transfer(ChestId, Item, 1, PlayerPoint()));
        Selection = FMath::Clamp(Selection, 0, FMath::Max(0, Rows().Num() - 1));
        return;
    }
    if (bPlanning) { RotatePlacement(); return; }
    if (!bWorldReady || !PrepareWorldAt(PlayerPoint())) return;
    UpdateFocus();
    const FHintUse Hint = BeginHintUse(bGamepad ? TEXT("X") : TEXT("F"));
    ON_SCOPE_EXIT { EndHintUse(Hint); };
    if (Focus == EFocus::Resource)
    {
        bool Sapling = false;
        Homestead::Point ActionTarget = PlayerPoint();
        for (const auto& Node : State().resources)
            if (Node.id == FocusId)
            {
                Sapling = Node.kind == Homestead::ResourceKind::ForestTree;
                ActionTarget = Node.position;
                break;
            }
        const int32 Cleared = FocusId;
        auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
        const bool bFell = Sapling && Avatar && Avatar->CanFell();
        const auto Result = Sim.Clear(FocusId, PlayerPoint());
        NotifyResourceAction(Result, bFell ? nullptr : WoodTapB.Get());
        if (Result.ok && Avatar)
        {
            if (Sapling) PresentFelling(Cleared, ActionTarget, true);
            else Avatar->PlayClear(ActionTarget);
        }
    }
    else if (Focus == EFocus::Plot)
    {
        for (const auto& Plot : State().plots)
        {
            if (Plot.id != FocusId) continue;
            if (!Plot.planted)
            {
                PlantFocusedPlot(Homestead::CropKind::Berries);
                break;
            }
            const auto Result = Sim.Weed(FocusId, PlayerPoint());
            Notify(Result, GrassStepA);
            if (Result.ok)
                if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->PlayGather();
            break;
        }
    }
    else if (Focus == EFocus::Fire) Notify(Sim.AddFuel(FocusId, PlayerPoint()), WoodTapA);
    else HoeSquareAhead();
}

void AHomesteadController::TillSquareAhead(int32& X, int32& Y) const
{
    // The hoe's blade bites about 85 cm out; probing there keeps the bite inside the chosen square.
    const auto Position = PlayerPoint();
    const FVector Forward = GetPawn() ? GetPawn()->GetActorForwardVector() : FVector::ForwardVector;
    X = Homestead::GardenCell(Position.x + Forward.X * 85);
    Y = Homestead::GardenCell(Position.y + Forward.Y * 85);
}

void AHomesteadController::HoeSquareAhead()
{
    int32 X = 0, Y = 0;
    TillSquareAhead(X, Y);
    const auto Position = PlayerPoint();
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    // Already tilled: hoe out its weeds instead.
    if (const auto* Tilled = FindPlotWhere(State().plots, [X, Y](const Homestead::Plot& Plot)
        { return Plot.cellX == X && Plot.cellY == Y; }))
    {
        const auto Result = Sim.Weed(Tilled->id, Position);
        Notify(Result, GrassStepA);
        if (Result.ok && Avatar) Avatar->PlayTill(Homestead::PlotCenter(*Tilled));
        return;
    }
    const auto Result = Sim.Till(X, Y, Position);
    Notify(Result, GrassStepB);
    if (!Result.ok || !Avatar) return;
    Avatar->PlayTill(Homestead::GardenCellCenter(X, Y));
    // The turned soil appears when the hoe first bites.
    if (Avatar->UsesHoeTill() && Landscape && !State().plots.empty())
    {
        Landscape->HoldPlot(State().plots.back().id, true);
        HeldPlot = State().plots.back().id;
        HeldPlotSince = GetWorld()->GetTimeSeconds();
        bHeldPlotTilling = true;
    }
}

void AHomesteadController::PlantFocusedPlot(Homestead::CropKind Crop)
{
    const auto* Plot = FindPlotWhere(State().plots, [this](const Homestead::Plot& Candidate) { return Candidate.id == FocusId; });
    if (!Plot) return;
    const auto Target = Homestead::PlotCenter(*Plot);
    const auto Result = Sim.Plant(FocusId, PlayerPoint(), Crop);
    Notify(Result, GrassStepB);
    if (!Result.ok) return;
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (Avatar && Avatar->PlayPlant(Target) && Landscape)
    {
        Landscape->HoldPlot(FocusId);
        HeldPlot = FocusId;
        bHeldPlotTilling = false;
        HeldPlotSince = GetWorld()->GetTimeSeconds();
    }
}

void AHomesteadController::OpenBook(int32 TargetPage)
{
    EndPlacement();
    HoveredHotbarSlot = INDEX_NONE;
    bBookOpen = true;
    Page = FMath::Clamp(TargetPage, 0, 7);
    Selection = 0;
    bConfirmRestart = false;
    PlayEffect(UIClick, 0.08f);
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
    {
        Avatar->CancelAction(true);
        Avatar->CancelSprint();
        Avatar->GetCharacterMovement()->StopMovementImmediately();
        // Appearance turns the camera to face her where she stands, beside its column of choices.
        Avatar->SetAppearancePreview(Page == 6);
    }
    ShowNativeMenu();
}

void AHomesteadController::Withdraw()
{
    if (IsFailed() || bPlanning) return;
    if (!bBookOpen) { OpenBook(0); return; }
    if (Page != 0) return;
    const auto Items = Rows();
    if (!Items.IsValidIndex(Selection)) return;
    const int ChestId = Sim.FindNearestStructure(PlayerPoint(), Homestead::Piece::Chest, 280);
    if (ChestId < 0) { Notify(TEXT("Stand near a storage chest to take an item."), true); return; }
    Notify(Sim.Transfer(ChestId, static_cast<Homestead::Item>(Items[Selection].Id), -1, PlayerPoint()));
    Selection = FMath::Clamp(Selection, 0, FMath::Max(0, Rows().Num() - 1));
}

void AHomesteadController::CloseBook()
{
    if (bBookOpen) PlayEffect(UIClick, 0.08f);
    bBookOpen = false;
    ActiveChestId.Reset();
    MenuInventoryViewIndex = 0;
    bConfirmRestart = false;
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->SetAppearancePreview(false);
    if (IsFailed()) ShowNativeMenu();
    else HideNativeMenu();
    // New-game setup: leaving Appearance moves on to the Names step.
    if (bNewGameSetup && !IsFailed() && !NamesWidget.IsValid()) ShowNames();
}
void AHomesteadController::ToggleBook() { if (IsFailed()) return; if (bBookOpen) CloseBook(); else OpenBook(0); }
void AHomesteadController::OpenSettings() { if (bBookOpen && Page == 4) CloseBook(); else OpenBook(4); }
void AHomesteadController::OpenCraft() { if (!IsFailed()) OpenBook(1); }
void AHomesteadController::OpenBuild() { if (!IsFailed()) OpenBook(2); }
void AHomesteadController::OpenJournal() { if (!IsFailed()) OpenBook(3); }
void AHomesteadController::OpenMap() { if (!IsFailed()) OpenBook(7); }
void AHomesteadController::Back()
{
    if (IsFailed()) { RetryCheckpoint(); return; }
    if (bBookOpen) CloseBook();
    else if (bPlanning) EndPlacement();
    else OpenBook(4);
}
void AHomesteadController::PreviousPage()
{
    if (bPlanning) { RotatePlacementBy(-1); return; }
    if (!bBookOpen) { CycleHotbar(-1); return; }
    Page = ShiftFieldBookPage(Page, -1); Selection = 0; bConfirmRestart = false;
    PlayEffect(UIClick, 0.08f);
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->SetAppearancePreview(Page == 6);
}
void AHomesteadController::NextPage()
{
    if (bPlanning) { RotatePlacement(); return; }
    if (!bBookOpen) { CycleHotbar(1); return; }
    Page = ShiftFieldBookPage(Page, 1); Selection = 0; bConfirmRestart = false;
    PlayEffect(UIClick, 0.08f);
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->SetAppearancePreview(Page == 6);
}
void AHomesteadController::PreviousRow()
{
    if (!bBookOpen) return;
    const int Count = Rows().Num();
    if (Count) Selection = (Selection + Count - 1) % Count;
    PlayEffect(UIClick, 0.06f);
    bConfirmRestart = false;
}
void AHomesteadController::NextRow()
{
    if (!bBookOpen) return;
    const int Count = Rows().Num();
    if (Count) Selection = (Selection + 1) % Count;
    PlayEffect(UIClick, 0.06f);
    bConfirmRestart = false;
}

TArray<FHomesteadRow> AHomesteadController::Rows() const
{
    TArray<FHomesteadRow> Result;
    if (Page == 0)
    {
        const int ChestId = Sim.FindNearestStructure(PlayerPoint(), Homestead::Piece::Chest, 280);
        const Homestead::Structure* Chest = nullptr;
        for (const auto& Structure : State().structures) if (Structure.id == ChestId) Chest = &Structure;
        for (int Index = 0; Index < Homestead::ItemCount; ++Index)
        {
            const auto Item = static_cast<Homestead::Item>(Index);
            const int InPack = Sim.Count(Item);
            const int InChest = Chest ? Chest->storage[Index] : 0;
            if (!InPack && !InChest) continue;
            const FString Action = InPack && Edible(Item) ? TEXT("eat 1")
                : InChest ? TEXT("take 1") : TEXT("");
            const FString Detail = !InPack ? TEXT("Stored nearby, not carried. Take one into your pack.")
                : Edible(Item) ? TEXT("Food - eat one from your pack.")
                : TEXT("Used in the world or in recipes.");
            const FString Label = Chest
                ? FString::Printf(TEXT("%s  |  Carried: %d  |  Chest: %d"), *Text(Homestead::ItemName(Item)), InPack, InChest)
                : FString::Printf(TEXT("%s  |  Carried: %d"), *Text(Homestead::ItemName(Item)), InPack);
            Result.Add({ Index, Label, Detail, Action, Chest && InPack > 0, InChest > 0 });
        }
    }
    else if (Page == 1)
    {
        for (int Index = 0; Index < static_cast<int>(Homestead::Recipe::Count); ++Index)
        {
            const auto Recipe = static_cast<Homestead::Recipe>(Index);
            const auto Assessment = Sim.AssessRecipe(Recipe, PlayerPoint());
            FHomesteadRow Row;
            Row.Id = Index;
            Row.SubjectId = Index;
            Row.Subject = EHomesteadMenuSubject::Recipe;
            Row.Name = Row.Label = Text(Homestead::RecipeName(Recipe));
            Row.Location = FString::Printf(TEXT("Makes %d %s"), Assessment.outputCount,
                *Text(Homestead::ItemName(Assessment.output)));
            Row.Detail = RecipeDescription(Recipe);
            Row.IconTint = Assessment.craftable
                ? FLinearColor(0.92f, 0.74f, 0.43f)
                : FLinearColor(0.34f, 0.36f, 0.34f);
            Row.RecipeState = Assessment;
            Row.HasRecipeState = true;
            Result.Add(MoveTemp(Row));
        }
    }
    else if (Page == 2)
    {
        for (int Index = 0; Index < static_cast<int>(Homestead::Piece::Count); ++Index)
        {
            const auto Piece = static_cast<Homestead::Piece>(Index);
            if (!Homestead::IsBuildable(Piece)) continue;
            Result.Add({ Index, Text(Homestead::PieceName(Piece)),
                FString::Printf(TEXT("Needs: %s"), *Text(Homestead::PieceRequirements(Piece))), TEXT("plan") });
        }
    }
    else if (Page == 3)
    {
        // Journal entries head the guidebook once there are any (the arrival note on the estate).
        for (int32 Entry = 0; Entry < static_cast<int32>(State().journal.size()); ++Entry)
        {
            const std::string& Key = State().journal[Entry];
            Result.Add({100 + Entry, FString(TEXT("Journal: ")) + UTF8_TO_TCHAR(Homestead::Manor::JournalTitle(Key).c_str()),
                UTF8_TO_TCHAR(Homestead::Manor::JournalText(Key, State()).c_str())});
        }
        Result.Add({0, TEXT("Choose your own home"), TEXT("Explore the seeded woodland. There is no prepared house clearing; find a place you like and make room.")});
        Result.Add({1, TEXT("1. Find a little breakfast"), TEXT("Gather berries, then eat them from the Pack page.")});
        Result.Add({2, TEXT("2. Make your first tools"), TEXT("Search the salvage piles around the manor for rusted heads, then haft each on two branches from the Craft page.")});
        Result.Add({3, TEXT("3. Make a home"), TEXT("Fell the trees at your chosen site with the axe. Place a floor, walls, doorway and roof. Felled trees stay gone when you return.")});
        Result.Add({4, TEXT("4. Tend a little garden"), TEXT("Haft a hoe. Each swing tills one small square; plant each square with A/E (root seeds) or X/F (berry seeds), or pick seeds or a berry on the hotbar and click.")});
        Result.Add({5, TEXT("5. Water and weed"), TEXT("Fill your pail at the stream. F/X removes weeds from a plot.")});
        Result.Add({6, TEXT("6. Cook and rest"), TEXT("Split timber with a carried axe. Cookfires use prepared firewood first, then branches. Roast roots; sleep in a sheltered bedroll.")});
        Result.Add({7, TEXT("Make this place your own"), TEXT("Inventory manages carried, stored and worn items. Appearance changes your hair, colors and body preset; clothing is cosmetic.")});
        Result.Add({8, TEXT("Move naturally through the menu"), TEXT("Use the D-pad, left stick, or arrow keys within lists and across their edges to nearby sections. A/Enter activates; B/Esc backs out. LB/RB change tabs. Triggers or Tab are optional section shortcuts. Choose Amount and activate it before editing a quantity.")});
    }
    else if (Page == 4)
    {
        Result.Add({0, TEXT("Save"), TEXT("Write a manual save and remain in Settings.")});
        Result.Add({1, TEXT("Load latest save"), LatestSaveLabel.IsEmpty()
            ? FString(TEXT("Resume the newest valid manual or automatic save."))
            : FString::Printf(TEXT("Resume the newest valid manual or automatic save: %s."), *LatestSaveLabel)});
        const FString Speed = State().dayMinutes >= 119 ? TEXT("Leisurely") : State().dayMinutes <= 31 ? TEXT("Fast") : TEXT("Balanced");
        Result.Add({2, TEXT("Game speed: ") + Speed, TEXT("Leisurely, Balanced, or Fast.")});
        Result.Add({3, FString::Printf(TEXT("Camera sensitivity: %.1f"), Sensitivity), TEXT("Cycle a comfortable turn speed.")});
        Result.Add({4, FString::Printf(TEXT("Invert camera Y: %s"), bInvertY ? TEXT("On") : TEXT("Off")), TEXT("Change vertical look direction.")});
        Result.Add({16, FString::Printf(TEXT("Overall volume: %d%%"), FMath::RoundToInt(MasterVolume * 100)), TEXT("Scales every sound in the game.")});
        Result.Add({5, FString::Printf(TEXT("Music volume: %d%%"), FMath::RoundToInt(MusicVolume * 100)), TEXT("Music playback level.")});
        Result.Add({6, FString::Printf(TEXT("Ambience volume: %d%%"), FMath::RoundToInt(AmbienceVolume * 100)), TEXT("Wind, woodland and creek ambience.")});
        Result.Add({7, FString::Printf(TEXT("Effects volume: %d%%"), FMath::RoundToInt(EffectsVolume * 100)), TEXT("Footsteps, gathering, crafting, and interface sounds.")});
        Result.Add({8, TEXT("Start a new woodland"), TEXT("Create a new seed after confirmation. Cancel keeps your current woodland. This build uses a new test-save version.")});
        Result.Add({9, TEXT("Quit game"), TEXT("Choose Save & Quit or Quit without Saving.")});
        if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
        {
            float Normalized = 0, Scale = 100, Minimum = 0, Maximum = 100;
            Settings->GetResolutionScaleInformationEx(Normalized, Scale, Minimum, Maximum);
            Result.Add({10, FString::Printf(TEXT("3D resolution scale: %.0f%%"), Scale),
                TEXT("Cycle 100 / 85 / 70 percent. UI stays sharp; TSR upscales the scene.")});
            const auto* VSync = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
            const bool Requested = Settings->IsVSyncEnabled();
            FString Label = FString::Printf(TEXT("Vertical sync: %s"), Requested ? TEXT("On") : TEXT("Off"));
            if (!VSync) Label += TEXT(" (unavailable)");
            else if ((VSync->GetInt() != 0) != Requested)
                Label += FString::Printf(TEXT(" | active %s (override)"), VSync->GetInt() ? TEXT("On") : TEXT("Off"));
            else if ((VSync->GetFlags() & ECVF_SetByMask) > ECVF_SetByGameSetting)
                Label += TEXT(" (engine override)");
            Result.Add({11, Label, TEXT("May reduce tearing, but can add input delay. Does not fix every flicker.")});
        }
        Result.Add({12, FString::Printf(TEXT("Autosave: %s"), bAutosaveEnabled ? TEXT("On") : TEXT("Off")),
            TEXT("Periodic rotating saves. Recovery checkpoints remain separate.")});
        Result.Add({13, FString::Printf(TEXT("Autosave interval: %d minutes"), AutosaveMinutes),
            bAutosaveEnabled ? TEXT("Counts only unpaused gameplay time.") : TEXT("Stored interval; Autosave is Off.")});
        if (Map)
            Result.Add({17, FString::Printf(TEXT("Minimap: %s"), Map->RotatesWithCamera() ? TEXT("turns with your view") : TEXT("north up")),
                TEXT("North up keeps the map still; turning with your view keeps ahead at the top, and the N marker shows north.")});
        Result.Add({15, TEXT("Show action hints again"),
            FString::Printf(TEXT("Each floating action hint retires after you've done that action %d times. This brings them all back."), HintRetireUses)});
        if (!PreviewLabel().IsEmpty())
            Result.Add({14, PreviewLabel(), TEXT("This preview uses isolated saves.")});
    }
    else if (Page == 7)
    {
        Result.Add({0, TEXT("Map"), TEXT("The estate and the country around it.")});
    }
    else if (Page == 6)
    {
        Result.Add({0, FString::Printf(TEXT("Hair: %s"), HomesteadLook::MetaHairName(Appearance.MetaHair)), TEXT("MetaHuman hairstyles: long, bobbed, tied back, braided or cropped.")});
        Result.Add({1, FString::Printf(TEXT("Hair color: %s"), HomesteadLook::HairColorName(Appearance.HairColor)), TEXT("Chestnut, dark brown, black, copper, or blonde. Hair color is independent of hairstyle.")});
        Result.Add({2, FString::Printf(TEXT("Skin: %s"), HomesteadLook::SkinToneName(Appearance.SkinTone)),         TEXT("Natural, warm, deep or light. Her face and body change together.")});
                Result.Add({3, FString::Printf(TEXT("Eyes: %s"), HomesteadLook::EyeColorName(Appearance.EyeColor)), TEXT("Blue, green, hazel or grey. The view moves close to her face while you choose.")});
        Result.Add({4, FString::Printf(TEXT("Tunic dye: %s"), HomesteadLook::TunicColorName(Appearance.TunicColor)), TEXT("A color choice for the current original outfit.")});
        Result.Add({5, FString::Printf(TEXT("Outfit: %s"), HomesteadLook::OutfitName(Appearance.Outfit)), TEXT("Cosmetic linen choices.")});
    }
    else
    {
        Result.Add({0, TEXT("Music by Kevin MacLeod (incompetech.com)"), TEXT("Evening Fall (Harp), Ascending the Vale, Teller of the Tales, Meditation Impromptu 02, At Rest")});
        Result.Add({1, TEXT("Creative Commons Attribution 4.0"), TEXT("https://creativecommons.org/licenses/by/4.0/")});
        Result.Add({2, TEXT("Music playback"), TEXT("Converted for game playback; playback fades and level matching applied.")});
        Result.Add({3, TEXT("Forest and creek ambience"), TEXT("TinyWorlds - OpenGameArt - CC0; creek: SamsterBirdies - Freesound - CC0")});
        Result.Add({4, TEXT("Brown Mud Leaves 01"), TEXT("Rob Tuytel - Poly Haven - CC0")});
        Result.Add({5, TEXT("Rock Moss Set 02"), TEXT("Kless Gyzen - Poly Haven - CC0")});
        Result.Add({6, TEXT("Complete credits"), TEXT("See docs/asset-credits.md in the project or packaged build.")});
        Result.Add({7, TEXT("Interaction and footstep sounds"), TEXT("Kenney - Impact Sounds and Interface Sounds - CC0")});
        Result.Add({8, TEXT("Character foundation"), TEXT("MakeHuman Community / MPFB graphical assets - CC0; original outfit and motion.")});
        Result.Add({9, TEXT("Estate terrain"), TEXT("Reshaped from Environment Agency LIDAR. Contains Environment Agency information \u00A9 Environment Agency and/or database right 2022, licensed under the Open Government Licence v3.0.")});
    }
    return Result;
}

FString AHomesteadController::BookTitle() const
{
    switch (Page)
    {
    case 0: return ActiveChestId.IsSet() ? TEXT("Storage") : TEXT("Your pack");
    case 1: return TEXT("Crafting recipes");
    case 2: return TEXT("Building plans");
    default: return TEXT("Field book");
    }
}

FString AHomesteadController::BookSummary() const
{
    switch (Page)
    {
    case 0: return ActiveChestId.IsSet()
        ? TEXT("Move whole stacks between this chest and your pack.")
        : TEXT("Carried items and equipped clothing.");
    case 1: return FString();
    case 2: return TEXT("Choose a plan to start placing it. Materials are spent when you place it.");
    case 6: return bGamepad ? TEXT("D-pad Left / Right: change the highlighted choice. She changes as you choose.")
        : TEXT("Click a swatch or style to wear it. She changes as you choose.");
    case 3: return FString::Printf(TEXT("Woodland seed %llu | generation %u | trees you fell stay cleared."),
        static_cast<unsigned long long>(State().world.seed), State().world.generationVersion);
    default: return {};
    }
}

FString AHomesteadController::BookFooter() const
{
    if (Page == 3 || Page == 5)
        return bGamepad ? TEXT("D-pad: scroll   LB / RB: pages   B: close")
            : TEXT("Up / Down: scroll   Left / Right: pages   Esc: close");
    if (Page == 4 && Rows().IsValidIndex(Selection) && Rows()[Selection].Id == 11)
        return bGamepad ? TEXT("D-pad: select   LB / RB: pages   A: toggle   B: close")
            : TEXT("Up / Down: select   Left / Right: pages   Enter: toggle   Esc: close");
    if (Page > 2)
        return bGamepad ? TEXT("D-pad: select   LB / RB: pages   A: use   B: close")
            : TEXT("Up / Down: select   Left / Right: pages   Enter: use   Esc: close");
    const auto Items = Rows();
    FString Footer = Items.IsEmpty()
        ? (bGamepad ? TEXT("LB/RB: pages") : TEXT("Left / Right: pages"))
        : (bGamepad ? TEXT("D-pad: select   LB/RB: pages") : TEXT("Arrows: select/pages"));
    if (Items.IsValidIndex(Selection))
    {
        const auto& Row = Items[Selection];
        if (!Row.Action.IsEmpty())
            Footer += FString::Printf(TEXT("   %s: %s"), bGamepad ? TEXT("A") : TEXT("Enter"), *Row.Action);
        if (Row.CanStore) Footer += bGamepad ? TEXT("   X: store 1") : TEXT("   F: store 1");
        if (Row.CanTake)
            Footer += bGamepad ? TEXT("   Y: take 1") : TEXT("   G: take 1");
    }
    Footer += bGamepad ? TEXT("   B: close") : TEXT("   Esc: close");
    return Footer;
}

void AHomesteadController::MenuStepAppearance(int32 Id, int32 Direction)
{
    if (!bBookOpen || Page != 6 || bMenuSaveInProgress) return;
    if (IsFailed()) { Notify(TEXT("Retry a checkpoint before changing possessions or appearance."), true); return; }
    const int32 Count = AppearanceChoiceCount(Id);
    if (Count <= 0) return;
    const int32 Current = AppearanceChoice(Id);
    MenuSetAppearance(Id, ((Current + (Direction < 0 ? -1 : 1)) % Count + Count) % Count);
}

int32 AHomesteadController::AppearanceChoiceCount(int32 Id)
{
    switch (Id)
    {
    case 0: return HomesteadLook::MetaHairCount;
    case 1: return HomesteadLook::HairColorCount;
    case 2: case 3: case 4: return 4;
    case 5: return 2;
    default: return 0;
    }
}

int32 AHomesteadController::AppearanceChoice(int32 Id) const
{
    switch (Id)
    {
    case 0: return Appearance.MetaHair;
    case 1: return Appearance.HairColor;
    case 2: return Appearance.SkinTone;
    case 3: return Appearance.EyeColor;
    case 4: return Appearance.TunicColor;
    case 5: return Appearance.Outfit;
    default: return 0;
    }
}

void AHomesteadController::MenuFocusAppearance(int32 Id)
{
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
        Avatar->SetAppearanceFaceFocus(bBookOpen && Page == 6 && Id == 3);
}

void AHomesteadController::MenuSetAppearance(int32 Id, int32 Value)
{
    if (!bBookOpen || Page != 6 || bMenuSaveInProgress) return;
    if (IsFailed()) { Notify(TEXT("Retry a checkpoint before changing possessions or appearance."), true); return; }
    const int32 Count = AppearanceChoiceCount(Id);
    if (Count <= 0 || Value < 0 || Value >= Count) return;
    MenuFocusAppearance(Id);
    if (AppearanceChoice(Id) == Value) return;
    FHomesteadAppearance Next = Appearance;
    switch (Id)
    {
    case 0: Next.MetaHair = Value; Next.HairStyle = HomesteadLook::LegacyHairStyle(Value); break;
    case 1: Next.HairColor = Value; break;
    case 2: Next.SkinTone = Value; break;
    case 3: Next.EyeColor = Value; break;
    case 4: Next.TunicColor = Value; break;
    case 5: Next.Outfit = Value; break;
    default: return;
    }
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    FString AppearanceError;
    const bool Applied = Avatar && (Avatar->IsEquipmentPresentationReady()
        ? Avatar->PrepareEquipment(State(), Next, AppearanceError) && Avatar->ApplyPreparedEquipment(AppearanceError)
        : Avatar->ApplyAppearance(Next));
    if (!Applied)
    {
        Notify(TEXT("That appearance could not be applied. Your saved selection has not changed. ") + AppearanceError, true);
        return;
    }
    Appearance = Next;
    PlayEffect(UIClick, 0.08f);
}
void AHomesteadController::ActivateRow()
{
    const auto Items = Rows();
    if (!Items.IsValidIndex(Selection)) return;
    const int Id = Items[Selection].Id;
    if (Page == 0)
    {
        const auto Item = static_cast<Homestead::Item>(Id);
        const int ChestId = Sim.FindNearestStructure(PlayerPoint(), Homestead::Piece::Chest, 280);
        if (Sim.Count(Item) == 0 && ChestId >= 0) Notify(Sim.Transfer(ChestId, Item, -1, PlayerPoint()));
        else if (Edible(Item)) Notify(Sim.Eat(Item));
        else if (ChestId >= 0)
        {
            bool Stored = false;
            for (const auto& Structure : State().structures)
                if (Structure.id == ChestId && Structure.storage[Id] > 0) Stored = true;
            if (Stored) Notify(Sim.Transfer(ChestId, Item, -1, PlayerPoint()));
            else Notify(TEXT("This tool or material is used in the world or in recipes."));
        }
        else Notify(TEXT("This tool or material is used in the world or in recipes."));
        Selection = FMath::Clamp(Selection, 0, FMath::Max(0, Rows().Num() - 1));
    }
    else if (Page == 1)
    {
        const auto Result = Sim.Craft(static_cast<Homestead::Recipe>(Id), PlayerPoint());
        Notify(Result, WoodTapB);
        if (Result.ok)
        {
            Sim.AdvanceGameHours(0.05, PlayerPoint());
            SlotHaftedTool(static_cast<Homestead::Recipe>(Id));
        }
    }
    else if (Page == 2) BeginPlacement(static_cast<Homestead::Piece>(Id));
    else if (Page == 6) MenuStepAppearance(Id, 1);
    else if (Page == 4)
    {
        switch (Id)
        {
        case 0: MenuSave(); break;
        case 1: QuickLoad(); break;
        case 2: Notify(Sim.SetDayMinutes(State().dayMinutes < 60 ? 60 : State().dayMinutes < 120 ? 120 : 30)); break;
        case 3: PersistCameraSensitivity(Sensitivity >= 1.8f ? 0.6f : Sensitivity + 0.2f); break;
        case 4: PersistCameraInversion(!bInvertY); break;
        case 5:
            PersistAudioVolume(5, MusicVolume >= 0.99f ? 0 : MusicVolume + 0.2f, MusicVolume);
            break;
        case 6:
            PersistAudioVolume(6, AmbienceVolume >= 0.99f ? 0 : AmbienceVolume + 0.2f, AmbienceVolume);
            break;
        case 7: PersistAudioVolume(7, EffectsVolume >= 0.99f ? 0 : EffectsVolume + 0.2f, EffectsVolume); break;
        case 16: PersistAudioVolume(16, MasterVolume >= 0.99f ? 0 : MasterVolume + 0.2f, MasterVolume); break;
        case 8: if (bConfirmRestart) NewGame(); else bConfirmRestart = true; break;
        case 9:
            MenuRequestExit();
            break;
        case 10:
            if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
            {
                float Normalized = 0, Scale = 100, Minimum = 0, Maximum = 100;
                Settings->GetResolutionScaleInformationEx(Normalized, Scale, Minimum, Maximum);
                PersistResolutionScale(Scale > 99 ? 85 : Scale > 84 ? 70 : 100);
            }
            else Notify(TEXT("Video settings are unavailable in this session."), true);
            break;
        case 11: ToggleVerticalSync(); break;
        case 12: MenuSetAutosaveEnabled(!bAutosaveEnabled); break;
        case 13: MenuSetAutosaveInterval(AutosaveMinutes == 5 ? 10 : AutosaveMinutes == 10 ? 20 : AutosaveMinutes == 20 ? 30 : 5); break;
        case 15: ResetActionHints(); break;
        case 17: if (Map) Map->SetRotatesWithCamera(!Map->RotatesWithCamera()); break;
        default: break;
        }
    }
}

void AHomesteadController::LoadCameraPreferences()
{
    Sensitivity = 1.0f;
    bInvertY = false;
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    FConfigFile Disk;
    if (!Branch || !Disk.Combine(Branch->IniPath))
    {
        UE_LOG(LogTemp, Display, TEXT("CAMERA_SETTINGS defaults active; no readable user-settings file."));
        return;
    }
    float StoredSensitivity = Sensitivity;
    if (Disk.GetFloat(CameraSettingsSection, CameraSensitivityKey, StoredSensitivity)
        && FMath::IsFinite(StoredSensitivity) && StoredSensitivity >= 0.2f && StoredSensitivity <= 3.0f)
    {
        Sensitivity = StoredSensitivity;
    }
    bool StoredInvertY = false;
    if (Disk.GetBool(CameraSettingsSection, CameraInvertYKey, StoredInvertY))
    {
        bInvertY = StoredInvertY;
    }
    UE_LOG(LogTemp, Display, TEXT("CAMERA_SETTINGS loaded sensitivity=%.3f invert_y=%d file=%s"),
        Sensitivity, bInvertY, *Branch->IniPath);
}

void AHomesteadController::LoadUserPreferences()
{
    MusicVolume = 0.65f;
    AmbienceVolume = 0.70f;
    EffectsVolume = 0.80f;
    MasterVolume = 1.0f;
    ApplyMasterVolume();
    bAutosaveEnabled = true;
    AutosaveMinutes = 5;
    LoadActionHints();
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    FConfigFile Disk;
    if (!Branch || !Disk.Combine(Branch->IniPath))
    {
        AutosaveRemaining = AutosaveMinutes * 60.0f;
        return;
    }
    for (const int32 AudioId : {5, 6, 7, 16})
    {
        float Stored = MenuAudioVolume(AudioId);
        if (Disk.GetFloat(AudioSettingsSection, AudioKeys[AudioKeyIndex(AudioId)], Stored) && FMath::IsFinite(Stored)
            && Stored >= 0 && Stored <= 1)
            MenuPreviewAudioVolume(AudioId, Stored);
    }
    FString EnabledText;
    if (Disk.GetString(AutosaveSettingsSection, AutosaveEnabledKey, EnabledText))
    {
        if (EnabledText.Equals(TEXT("True"), ESearchCase::IgnoreCase)) bAutosaveEnabled = true;
        else if (EnabledText.Equals(TEXT("False"), ESearchCase::IgnoreCase)) bAutosaveEnabled = false;
    }
    int32 Minutes = 5;
    if (Disk.GetInt(AutosaveSettingsSection, AutosaveMinutesKey, Minutes)
        && (Minutes == 5 || Minutes == 10 || Minutes == 20 || Minutes == 30))
        AutosaveMinutes = Minutes;
    AutosaveRemaining = AutosaveMinutes * 60.0f;
}

float AHomesteadController::MenuAudioVolume(int32 Id) const
{
    return Id == 16 ? MasterVolume : Id == 5 ? MusicVolume : Id == 6 ? AmbienceVolume : EffectsVolume;
}

void AHomesteadController::ApplyMasterVolume() const
{
    if (const UWorld* World = GetWorld())
        if (FAudioDeviceHandle Device = World->GetAudioDevice())
            Device->SetTransientPrimaryVolume(MasterVolume);
}

void AHomesteadController::MenuPreviewAudioVolume(int32 Id, float Value)
{
    Value = FMath::Clamp(Value, 0.0f, 1.0f);
    if (Id == 16)
    {
        MasterVolume = Value;
        ApplyMasterVolume();
    }
    else if (Id == 5)
    {
        MusicVolume = Value;
        Music->SetVolumeMultiplier(MusicLevel());
    }
    else if (Id == 6)
    {
        AmbienceVolume = Value;
        Ambience->SetVolumeMultiplier(Value);
        Creek->SetVolumeMultiplier(Value * CreekGain);
    }
    else EffectsVolume = Value;
}

bool AHomesteadController::PersistAudioVolume(int32 Id, float Requested, float Previous)
{
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    const int32 Index = AudioKeyIndex(Id);
    Requested = FMath::Clamp(Requested, 0.0f, 1.0f);
    if (!Branch || Index < 0
        || !PersistFloatProperty(Branch->IniPath, AudioSettingsSection, AudioKeys[Index], Requested))
    {
        MenuPreviewAudioVolume(Id, Previous);
        Notify(TEXT("Could not save that audio preference. The previous level was restored."), true);
        return false;
    }
    GConfig->SetFloat(AudioSettingsSection, AudioKeys[Index], Requested, GGameUserSettingsIni);
    MenuPreviewAudioVolume(Id, Requested);
    return true;
}

bool AHomesteadController::MenuCommitAudioVolume(int32 Id, float Value, float Previous)
{
    return PersistAudioVolume(Id, Value, Previous);
}

bool AHomesteadController::PersistAutosaveEnabled(bool Requested)
{
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    if (!Branch || !PersistBoolProperty(Branch->IniPath, AutosaveSettingsSection, AutosaveEnabledKey, Requested))
    {
        Notify(TEXT("Could not save the Autosave preference. The previous choice was restored."), true);
        return false;
    }
    GConfig->SetBool(AutosaveSettingsSection, AutosaveEnabledKey, Requested, GGameUserSettingsIni);
    return true;
}

bool AHomesteadController::PersistAutosaveInterval(int32 Requested)
{
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    if (!Branch || (Requested != 5 && Requested != 10 && Requested != 20 && Requested != 30)
        || !PersistIntProperty(Branch->IniPath, AutosaveSettingsSection, AutosaveMinutesKey, Requested))
    {
        Notify(TEXT("Could not save the Autosave interval. The previous interval was restored."), true);
        return false;
    }
    GConfig->SetInt(AutosaveSettingsSection, AutosaveMinutesKey, Requested, GGameUserSettingsIni);
    return true;
}

void AHomesteadController::MenuSetAutosaveEnabled(bool Enabled)
{
    if (!PersistAutosaveEnabled(Enabled)) return;
    bAutosaveEnabled = Enabled;
    AutosaveRemaining = AutosaveMinutes * 60.0f;
    Notify(Enabled ? TEXT("Autosave On.") : TEXT("Autosave Off. Existing autosaves are retained."));
}

void AHomesteadController::MenuSetAutosaveInterval(int32 Minutes)
{
    if (!PersistAutosaveInterval(Minutes)) return;
    AutosaveMinutes = Minutes;
    AutosaveRemaining = AutosaveMinutes * 60.0f;
    Notify(FString::Printf(TEXT("Autosave interval %d minutes."), AutosaveMinutes));
}

void AHomesteadController::MenuSetGameSpeed(double DayMinutes)
{
    Notify(Sim.SetDayMinutes(DayMinutes));
}

void AHomesteadController::MenuAdjustSetting(int32 Id, int32 Direction)
{
    if (!Direction) return;
    if (Id == 2)
    {
        const double Values[] = {120, 60, 30};
        int32 Index = State().dayMinutes >= 119 ? 0 : State().dayMinutes <= 31 ? 2 : 1;
        MenuSetGameSpeed(Values[FMath::Clamp(Index + Direction, 0, 2)]);
    }
    else if (Id == 3) PersistCameraSensitivity(FMath::Clamp(Sensitivity + Direction * 0.2f, 0.2f, 3.0f));
    else if (Id == 4) PersistCameraInversion(Direction > 0);
    else if ((Id >= 5 && Id <= 7) || Id == 16)
    {
        const float Previous = MenuAudioVolume(Id);
        PersistAudioVolume(Id, Previous + Direction * 0.05f, Previous);
    }
    else if (Id == 12) MenuSetAutosaveEnabled(Direction > 0);
    else if (Id == 17 && Map) Map->SetRotatesWithCamera(Direction > 0);
    else if (Id == 13 && bAutosaveEnabled)
    {
        const int32 Values[] = {5, 10, 20, 30};
        int32 Index = 0;
        for (int32 I = 0; I < UE_ARRAY_COUNT(Values); ++I) if (Values[I] == AutosaveMinutes) Index = I;
        MenuSetAutosaveInterval(Values[FMath::Clamp(Index + Direction, 0, UE_ARRAY_COUNT(Values) - 1)]);
    }
}

bool AHomesteadController::PersistCameraSensitivity(float Requested)
{
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    if (!Branch || !FMath::IsFinite(Requested) || Requested < 0.2f || Requested > 3.0f)
    {
        Notify(TEXT("Camera sensitivity settings are unavailable. Your previous preference is unchanged."), true);
        return false;
    }
    FCameraConfigSnapshot Snapshot;
    if (!CaptureCameraConfig(Branch->IniPath, Snapshot))
    {
        Notify(TEXT("Could not read camera settings before saving. Your previous preference is unchanged."), true);
        return false;
    }
    FConfigFile Property;
    Property.SetFloat(CameraSettingsSection, CameraSensitivityKey, Requested);
    const bool Saved = Property.UpdateSinglePropertyInSection(
        *Branch->IniPath, CameraSensitivityKey, CameraSettingsSection);
    FConfigFile Disk;
    float Persisted = -1.0f;
    const bool Verified = Saved && Disk.Combine(Branch->IniPath)
        && Disk.GetFloat(CameraSettingsSection, CameraSensitivityKey, Persisted)
        && FMath::IsNearlyEqual(Persisted, Requested, 0.001f);
    if (!Verified)
    {
        const bool Restored = !Saved || RestoreCameraConfig(Branch->IniPath, Snapshot);
        Notify(Restored
            ? TEXT("Could not save camera sensitivity. Your previous preference was restored.")
            : TEXT("Could not save or restore camera sensitivity. Check the settings file permissions."), true);
        UE_LOG(LogTemp, Error, TEXT("CAMERA_SETTINGS sensitivity persistence failed file=%s restored=%d"),
            *Branch->IniPath, Restored);
        return false;
    }
    GConfig->SetFloat(CameraSettingsSection, CameraSensitivityKey, Requested, GGameUserSettingsIni);
    Sensitivity = Requested;
    Notify(FString::Printf(TEXT("Camera sensitivity %.1f. Choice saved in game settings."), Sensitivity));
    return true;
}

bool AHomesteadController::PersistCameraInversion(bool Requested)
{
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    if (!Branch)
    {
        Notify(TEXT("Camera inversion settings are unavailable. Your previous preference is unchanged."), true);
        return false;
    }
    FCameraConfigSnapshot Snapshot;
    if (!CaptureCameraConfig(Branch->IniPath, Snapshot))
    {
        Notify(TEXT("Could not read camera settings before saving. Your previous preference is unchanged."), true);
        return false;
    }
    FConfigFile Property;
    Property.SetBool(CameraSettingsSection, CameraInvertYKey, Requested);
    const bool Saved = Property.UpdateSinglePropertyInSection(
        *Branch->IniPath, CameraInvertYKey, CameraSettingsSection);
    FConfigFile Disk;
    bool Persisted = false;
    const bool Verified = Saved && Disk.Combine(Branch->IniPath)
        && Disk.GetBool(CameraSettingsSection, CameraInvertYKey, Persisted)
        && Persisted == Requested;
    if (!Verified)
    {
        const bool Restored = !Saved || RestoreCameraConfig(Branch->IniPath, Snapshot);
        Notify(Restored
            ? TEXT("Could not save camera inversion. Your previous preference was restored.")
            : TEXT("Could not save or restore camera inversion. Check the settings file permissions."), true);
        UE_LOG(LogTemp, Error, TEXT("CAMERA_SETTINGS inversion persistence failed file=%s restored=%d"),
            *Branch->IniPath, Restored);
        return false;
    }
    GConfig->SetBool(CameraSettingsSection, CameraInvertYKey, Requested, GGameUserSettingsIni);
    bInvertY = Requested;
    Notify(Requested ? TEXT("Camera Y inversion On. Choice saved in game settings.")
        : TEXT("Camera Y inversion Off. Choice saved in game settings."));
    return true;
}

bool AHomesteadController::PersistResolutionScale(float Requested)
{
    auto* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
    if (!Settings)
    {
        PendingResolutionScale = Requested;
        GraphicsSaveError = TEXT("Video settings are unavailable; the requested 3D resolution scale was not saved.");
        Notify(GraphicsSaveError, true);
        return false;
    }
    float Normalized = 0, Previous = 100, Minimum = 0, Maximum = 100;
    Settings->GetResolutionScaleInformationEx(Normalized, Previous, Minimum, Maximum);
    Settings->SetResolutionScaleValueEx(Requested);
    Settings->ApplyNonResolutionSettings();
    if (!FParse::Param(FCommandLine::Get(), TEXT("HomesteadSmokeTest")))
    {
        Settings->SaveSettings();
        FConfigFile Disk;
        float Persisted = -1;
        if (!Disk.Combine(GGameUserSettingsIni)
            || !Disk.GetFloat(TEXT("ScalabilityGroups"), TEXT("sg.ResolutionQuality"), Persisted)
            || !FMath::IsNearlyEqual(Persisted, Requested, 0.1f))
        {
            Settings->SetResolutionScaleValueEx(Previous);
            Settings->ApplyNonResolutionSettings();
            PendingResolutionScale = Requested;
            GraphicsSaveError = TEXT("Could not verify the saved 3D resolution scale. The previous runtime scale was restored; the disk preference is unverified.");
            Notify(GraphicsSaveError, true);
            return false;
        }
    }
    PendingResolutionScale.Reset();
    GraphicsSaveError.Reset();
    return true;
}

void AHomesteadController::ToggleVerticalSync()
{
    auto* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
    auto* VSync = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    if (!Settings || !VSync || !Branch)
    {
        Notify(TEXT("Vertical sync settings are unavailable in this session."), true);
        UE_LOG(LogTemp, Error, TEXT("VSYNC_SETTING unavailable"));
        return;
    }
    const bool Previous = Settings->IsVSyncEnabled();
    const int32 PreviousRuntime = VSync->GetInt();
    const bool Requested = !Previous;
    VSync->Set(Requested ? 1 : 0, ECVF_SetByGameSetting);
    if ((VSync->GetInt() != 0) != Requested)
    {
        Notify(TEXT("An engine override controls vertical sync. Your preference was not changed."), true);
        UE_LOG(LogTemp, Warning, TEXT("VSYNC_SETTING blocked requested=%d applied=%d priority=%u"),
            Requested, VSync->GetInt(), VSync->GetFlags() & ECVF_SetByMask);
        return;
    }
    Settings->SetVSyncEnabled(Requested);
    // Avoid reapplying other video settings or flushing unrelated pending config changes.
    const FString Section = Settings->GetClass()->GetPathName();
    FConfigFile Property;
    Property.SetBool(*Section, TEXT("bUseVSync"), Settings->IsVSyncEnabled());
    const bool Saved = Property.UpdateSinglePropertyInSection(*Branch->IniPath, TEXT("bUseVSync"), *Section);
    FConfigFile Disk;
    bool Persisted = false;
    const bool Read = Saved && Disk.Combine(Branch->IniPath)
        && Disk.GetBool(*Section, TEXT("bUseVSync"), Persisted);
    if (!Read || Persisted != Requested)
    {
        Settings->SetVSyncEnabled(Previous);
        VSync->Set(PreviousRuntime, ECVF_SetByGameSetting);
        Notify(TEXT("Could not save vertical sync. Your previous preference was restored."), true);
        UE_LOG(LogTemp, Error, TEXT("VSYNC_SETTING persistence failed file=%s requested=%d applied=%d"),
            *Branch->IniPath, Requested, VSync->GetInt());
        return;
    }
    GConfig->SetBool(*Section, TEXT("bUseVSync"), Requested, GGameUserSettingsIni);
    UE_LOG(LogTemp, Display, TEXT("VSYNC_SETTING saved requested=%d applied=%d priority=%u file=%s"),
        Requested, VSync->GetInt(), VSync->GetFlags() & ECVF_SetByMask, *Branch->IniPath);
    Notify(Requested ? TEXT("Vertical sync On. Choice saved for this game's graphics settings.")
        : TEXT("Vertical sync Off. Choice saved for this game's graphics settings."));
}

void AHomesteadController::BeginPlacement(Homestead::Piece Kind)
{
    CloseBook();
    HoveredHotbarSlot = INDEX_NONE;
    bPlanning = true;
    BuildKind = Kind;
    BuildRotation = 0;
    BuildYawOffset = 0.0;
    BuildCheckKey.Reset();
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->SetPlanning(true);
    UpdatePlacement(true);
}

void AHomesteadController::EndPlacement()
{
    bPlanning = false;
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->SetPlanning(false);
    if (Landscape) Landscape->SetPlacementPreview(false, BuildTarget, false);
}

void AHomesteadController::UpdatePlacement(bool bForce)
{
    if (!bPlanning) return;
    const auto Position = PlayerPoint();
    // Aim 3.5 m ahead of her along the camera; whole centimetres keep the snap search stable.
    const FRotator View(0, GetControlRotation().Yaw, 0);
    const FVector Forward = View.Vector();
    const Homestead::Point Aim{FMath::RoundToDouble(Position.x + Forward.X * 350.0),
        FMath::RoundToDouble(Position.y + Forward.Y * 350.0)};
    // Free-standing pieces face the way the camera does, in 5 degree steps, plus her own turns.
    const double FreeYaw = FMath::RoundToDouble(View.Yaw / 5.0) * 5.0 + BuildYawOffset;
    BuildTarget = Sim.ResolvePlacement(BuildKind, Aim, FreeYaw, BuildRotation);
    const FString Key = FString::Printf(TEXT("%d:%d:%d:%d:%d:%.0f:%.0f:%.1f:%llu"), static_cast<int>(BuildTarget.kind),
        BuildTarget.buildingId, BuildTarget.cellX, BuildTarget.cellY, BuildTarget.rotation,
        BuildTarget.frame.origin.x, BuildTarget.frame.origin.y, BuildTarget.frame.yaw,
        static_cast<unsigned long long>(Sim.GetRevision()));
    const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
    if (bForce || (Key != BuildCheckKey && Now - LastBuildCheckTime >= 0.1))
    {
        auto Check = Sim.CheckPlacement(BuildTarget, Position, true);
        if (Check.ok) Check = Sim.CheckBuildCost(BuildKind);
        bBuildValid = Check.ok;
        BuildBlocker = Check.ok ? FString() : Text(Check.message.c_str());
        BuildCheckKey = Key;
        LastBuildCheckTime = Now;
    }
    if (Landscape) Landscape->SetPlacementPreview(true, BuildTarget, bBuildValid);
}

void AHomesteadController::RotatePlacement() { RotatePlacementBy(1); }

void AHomesteadController::RotatePlacementBy(int32 Direction)
{
    if (!bPlanning) return;
    const bool bQuarterTurns = BuildTarget.snapped || BuildKind == Homestead::Piece::Wall
        || BuildKind == Homestead::Piece::Doorway || BuildKind == Homestead::Piece::Roof;
    if (bQuarterTurns) BuildRotation = ((BuildRotation + Direction) % 4 + 4) % 4;
    else BuildYawOffset = FMath::Fmod(BuildYawOffset + 15.0 * Direction + 360.0, 360.0);
    UpdatePlacement(true);
}

FString AHomesteadController::PlacementLabel() const
{
    return FString::Printf(TEXT("%s  |  %s"), *Text(Homestead::PieceName(BuildKind)), *Text(Homestead::PieceRequirements(BuildKind)));
}

FString AHomesteadController::PlacementStatus() const
{
    if (!BuildBlocker.IsEmpty()) return BuildBlocker;
    return BuildTarget.snapped ? FString(TEXT("Snaps onto your building."))
        : FString(TEXT("Free-standing: it faces the way you look; rotate turns it."));
}

void AHomesteadController::CycleZoom()
{
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->CycleZoom();
}

UHomesteadSave* AHomesteadController::ReadSave(const FString& Filename) const
{
    bReadIncompatible = false;
    TArray<uint8> Data;
    if (IFileManager::Get().FileSize(*Filename) > 20 * 1024 * 1024) return nullptr;
    if (!FFileHelper::LoadFileToArray(Data, *Filename)) return nullptr;
    const char Magic[] = "HOMESAV1";
    if (Data.Num() < 16 || FMemory::Memcmp(Data.GetData(), Magic, 8) != 0) return nullptr;
    uint32 ExpectedCrc = 0;
    FMemory::Memcpy(&ExpectedCrc, Data.GetData() + 8, sizeof(ExpectedCrc));
    if (FCrc::MemCrc32(Data.GetData() + 12, Data.Num() - 12) != ExpectedCrc) return nullptr;
    Data.RemoveAt(0, 12, EAllowShrinking::No);
    UHomesteadSave* Save = Cast<UHomesteadSave>(UGameplayStatics::LoadGameFromMemory(Data));
    FGuid ParsedWorld;
    if (Save && !Save->IsCurrentVersion()) { bReadIncompatible = true; return nullptr; }
    if (!Save || Save->SavedAtUtc < 0 || Save->SavedAtUtc > 253402300799LL
        || Save->PlayerLocation.ContainsNaN() || Save->ViewRotation.ContainsNaN()
        || !FGuid::Parse(Save->WorldId, ParsedWorld) || !ParsedWorld.IsValid()
        || FMath::Abs(Save->PlayerLocation.X) > Homestead::MaxWorldCoordinate
        || FMath::Abs(Save->PlayerLocation.Y) > Homestead::MaxWorldCoordinate
        || FMath::Abs(Save->PlayerLocation.Z) > Homestead::MaxWorldCoordinate || !FMath::IsFinite(Save->CameraSensitivity)
        || Save->CameraSensitivity < 0.2 || Save->CameraSensitivity > 3
        || !FMath::IsFinite(Save->MusicVolume) || Save->MusicVolume < 0 || Save->MusicVolume > 1
        || !FMath::IsFinite(Save->AmbienceVolume) || Save->AmbienceVolume < 0 || Save->AmbienceVolume > 1
        || !FMath::IsFinite(Save->EffectsVolume) || Save->EffectsVolume < 0 || Save->EffectsVolume > 1)
        return nullptr;
    FHomesteadAppearance SavedLook;
    SavedLook.HairStyle = Save->HairStyle; SavedLook.MetaHair = Save->MetaHair >= 0 ? Save->MetaHair : HomesteadLook::MetaHairForLegacy(Save->HairStyle);
    SavedLook.HairColor = Save->HairColor;
    SavedLook.SkinTone = Save->SkinTone;
    SavedLook.EyeColor = Save->EyeColor;
    SavedLook.TunicColor = Save->TunicColor;
    SavedLook.Outfit = Save->Outfit;
    SavedLook.BodyPreset = Save->BodyPreset;
    if (!SavedLook.IsValid()) return nullptr;
    Homestead::Simulation Candidate;
    if (bEstateMap) PrepareEstateSimulation(Candidate);
    const auto Decoded = Candidate.Deserialize(TCHAR_TO_UTF8(*Save->SimulationData));
    if (!Decoded) { bReadIncompatible = Decoded.code == Homestead::ResultCode::UnsupportedVersion; return nullptr; }
    return Save;
}

bool AHomesteadController::SaveSlot(const FString& Slot, bool Quiet)
{
    if (bTestResetRequired) { Notify(TEXT("Choose an explicit test reset before saving a new woodland."), true); return false; }
    if (!bWorldReady) { Notify(TEXT("The world is not ready; no save files were changed."), true); return false; }
    if (!bSaveRoutingReady) { Notify(TEXT("Save routing is unavailable. No save files were accessed."), true); return false; }
    UHomesteadSave* Save = Cast<UHomesteadSave>(UGameplayStatics::CreateSaveGameObject(UHomesteadSave::StaticClass()));
    if (!Save) { Notify(TEXT("Could not create a save record."), true); return false; }
    Save->WorldId = WorldId;
    Save->HairStyle = Appearance.HairStyle; Save->MetaHair = Appearance.MetaHair;
    Save->HairColor = Appearance.HairColor;
    Save->SkinTone = Appearance.SkinTone;
    Save->EyeColor = Appearance.EyeColor;
    Save->TunicColor = Appearance.TunicColor;
    Save->Outfit = Appearance.Outfit;
    Save->BodyPreset = Appearance.BodyPreset;
    Save->SimulationData = UTF8_TO_TCHAR(Sim.Serialize().c_str());
    if (Save->SimulationData.IsEmpty())
    { Notify(TEXT("World serialization failed; previous saves are untouched."), true); return false; }
    Save->PlayerLocation = GetPawn() ? GetPawn()->GetActorLocation() : PendingLocation;
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    Save->ViewRotation = Avatar ? Avatar->GameplayViewRotation() : GetControlRotation();
    Save->SavedAtUtc = FDateTime::UtcNow().ToUnixTimestamp();
    Save->CameraSensitivity = Sensitivity;
    Save->InvertCameraY = bInvertY;
    Save->MusicVolume = MusicVolume;
    Save->AmbienceVolume = AmbienceVolume;
    Save->EffectsVolume = EffectsVolume;
    Save->HotbarSlots = HotbarSlots;
    Save->SelectedHotbarSlot = SelectedHotbarSlot;
    Save->HotbarLayout = UHomesteadSave::CurrentHotbarLayout;
    Save->SaveLabel = CurrentSaveLabel();
    TArray<uint8> Data;
    const FString Path = SavePath(Slot);
    const FString Temporary = Path + TEXT(".tmp");
    if (!UGameplayStatics::SaveGameToMemory(Save, Data))
    {
        Notify(TEXT("The game could not serialize this save. Previous saves are untouched."), true);
        return false;
    }
    TArray<uint8> Envelope;
    Envelope.SetNumUninitialized(Data.Num() + 12);
    const char Magic[] = "HOMESAV1";
    const uint32 Checksum = FCrc::MemCrc32(Data.GetData(), Data.Num());
    FMemory::Memcpy(Envelope.GetData(), Magic, 8);
    FMemory::Memcpy(Envelope.GetData() + 8, &Checksum, sizeof(Checksum));
    FMemory::Memcpy(Envelope.GetData() + 12, Data.GetData(), Data.Num());
    if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true)
        || !FFileHelper::SaveArrayToFile(Envelope, *Temporary)
        || !ReadSave(Temporary))
    {
        Notify(TEXT("Save failed. Existing saves were not replaced; check disk space and permissions."), true);
        return false;
    }
    if (IFileManager::Get().FileExists(*Path)
        && IFileManager::Get().Copy(*(Path + TEXT(".bak")), *Path, true, true) != COPY_OK)
    {
        Notify(TEXT("Could not back up the previous save. It has not been replaced."), true);
        return false;
    }
    if (!IFileManager::Get().Move(*Path, *Temporary, true, true, false, true))
    {
        Notify(TEXT("Could not finish saving. The previous backup is retained."), true);
        return false;
    }
    if (!Quiet) Notify(TEXT("Your homestead is saved."));
    LastSuccessfulSave = FDateTime::UtcNow();
    LatestSaveLabel = Save->SaveLabel;
    return true;
}

bool AHomesteadController::ApplySave(const UHomesteadSave& Save)
{
    Homestead::Simulation Candidate = Sim;
    const auto Result = Candidate.Deserialize(TCHAR_TO_UTF8(*Save.SimulationData));
    if (!Result) { Notify(Result); return false; }
    const auto Region = Candidate.SetActiveWorldRegion({Save.PlayerLocation.X, Save.PlayerLocation.Y});
    if (!Region) { Notify(Region); return false; }
    FHomesteadAppearance Look;
    Look.HairStyle = Save.HairStyle; Look.MetaHair = Save.MetaHair >= 0 ? Save.MetaHair : HomesteadLook::MetaHairForLegacy(Save.HairStyle); Look.HairColor = Save.HairColor;
    Look.SkinTone = Save.SkinTone; Look.EyeColor = Save.EyeColor;
    Look.TunicColor = Save.TunicColor; Look.Outfit = Save.Outfit; Look.BodyPreset = Save.BodyPreset;
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    FString Error;
    if (Avatar && !Avatar->PrepareEquipment(Candidate.GetState(), Look, Error))
    {
        LoadProblem = TEXT("This save is valid, but its clothing content is unavailable. Nothing was loaded. ") + Error;
        bTestResetRequired = !bHasPlayableSession;
        Notify(LoadProblem, true);
        return false;
    }
    if (!Landscape || !Landscape->Refresh(Candidate))
    {
        bWorldReady = false;
        if (Avatar) Avatar->ClearPreparedEquipment();
        Notify(TEXT("Saved woodland terrain could not be prepared. The save was not applied."), true);
        return false;
    }
    const Homestead::Simulation Previous = Sim;
    Sim = MoveTemp(Candidate);
    ActiveChestId.Reset();
    if (Avatar && !Avatar->ApplyPreparedEquipment(Error))
    {
        Sim = Previous;
        bWorldReady = Landscape->Refresh(Sim);
        Avatar->ClearPreparedEquipment();
        LoadProblem = TEXT("Save loading was canceled because its prepared appearance could not be displayed. ") + Error;
        bTestResetRequired = !bHasPlayableSession;
        Notify(LoadProblem, true);
        return false;
    }
    bTestResetRequired = false;
    bWorldReady = true;
    bHasPlayableSession = true;
    LoadProblem.Reset();
    if (Avatar)
    {
        Avatar->CancelAction(true);
        Avatar->CancelSprint();
    }
    WorldId = Save.WorldId;
    LastSuccessfulSave = FDateTime::FromUnixTimestamp(Save.SavedAtUtc);
    LatestSaveLabel = Save.SaveLabel;
    Appearance.HairStyle = Save.HairStyle; Appearance.MetaHair = Save.MetaHair >= 0 ? Save.MetaHair : HomesteadLook::MetaHairForLegacy(Save.HairStyle);
    Appearance.HairColor = Save.HairColor;
    Appearance.SkinTone = Save.SkinTone;
    Appearance.EyeColor = Save.EyeColor;
    Appearance.TunicColor = Save.TunicColor;
    Appearance.Outfit = Save.Outfit;
    Appearance.BodyPreset = Save.BodyPreset;
    SanitizeHotbar(Save.HotbarSlots, Save.SelectedHotbarSlot, Save.HotbarLayout);
    PendingLocation = Save.PlayerLocation;
    PendingRotation = Save.ViewRotation;
    bFreshTerrainSpawn = false;
    bPendingSpawn = true;
    bWasFailed = false;
    CaptureSessionCheckpoint(PendingLocation, PendingRotation);
    Music->SetVolumeMultiplier(MusicLevel());
    Ambience->SetVolumeMultiplier(AmbienceVolume);
    Creek->SetVolumeMultiplier(AmbienceVolume * CreekGain);
    EndPlacement();
    CloseBook();
    RefreshRemaining = 0;
    GrantPlaytestKit(false);
    return true;
}

void AHomesteadController::GrantPlaytestKit(bool bNewGame)
{
    const TCHAR* Command = FCommandLine::Get();
    for (const TCHAR* Automation : {TEXT("unattended"), TEXT("HomesteadSmokeTest"), TEXT("HomesteadVisualPlaytest"),
        TEXT("HomesteadShippingQA"), TEXT("HomesteadSaveAudit"), TEXT("HomesteadPreviewProfile")})
        if (FParse::Param(Command, Automation) || FString(Command).Contains(FString(TEXT("-")) + Automation + TEXT("=")))
            return;
    if (bSaveRoutingTestPending || !StartupProbeDirectory.IsEmpty()) return;
    // On the estate her first tools are hafted from salvage; handing them over would skip that.
    if (Sim.GetState().fixedEstate) return;
    const FVector Facing = PendingRotation.Vector();
    const auto Result = Sim.GrantStarterKit({PendingLocation.X, PendingLocation.Y}, {Facing.X, Facing.Y}, bNewGame);
    if (!Result)
    {
        UE_LOG(LogTemp, Warning, TEXT("Playtest kit was not granted: %s"), UTF8_TO_TCHAR(Result.message.c_str()));
        return;
    }
    for (const auto Tool : {Homestead::Item::Billhook, Homestead::Item::Hatchet, Homestead::Item::Scythe,
        Homestead::Item::Pickaxe, Homestead::Item::DiggingStick, Homestead::Item::WateringCan, Homestead::Item::Seeds})
    {
        const int32 Value = static_cast<int32>(Tool);
        if (HotbarSlots.Contains(Value)) continue;
        const int32 Empty = HotbarSlots.IndexOfByKey(-1);
        if (Empty != INDEX_NONE) HotbarSlots[Empty] = Value;
    }
    if (Landscape) Landscape->Refresh(Sim);
    UE_LOG(LogTemp, Display, TEXT("Playtest kit granted (new game %d): %s"), bNewGame, UTF8_TO_TCHAR(Result.message.c_str()));
}

bool AHomesteadController::LoadLatest(bool RecoveryOnly)
{
    if (!bSaveRoutingReady) { Notify(TEXT("Save routing is unavailable. No save files were accessed."), true); return false; }
    const TArray<FString> Slots = RecoveryOnly
        ? TArray<FString>{TEXT("Homestead_Recovery"), TEXT("Homestead_Auto_0"), TEXT("Homestead_Auto_1"), TEXT("Homestead_Auto_2"), TEXT("Homestead_Manual")}
        : TArray<FString>{TEXT("Homestead_Manual"), TEXT("Homestead_Auto_0"), TEXT("Homestead_Auto_1"), TEXT("Homestead_Auto_2"), TEXT("Homestead_Recovery")};
    UHomesteadSave* Best = nullptr;
    bool Corrupt = false;
    bool Incompatible = false;
    for (const auto& Slot : Slots)
    {
        for (const FString& Suffix : { FString(), FString(TEXT(".bak")) })
        {
            const FString Path = SavePath(Slot) + Suffix;
            if (!IFileManager::Get().FileExists(*Path)) continue;
            UHomesteadSave* Save = ReadSave(Path);
            if (!Save)
            {
                Incompatible |= bReadIncompatible;
                Corrupt |= !bReadIncompatible;
                UE_LOG(LogTemp, Warning, TEXT("Cannot read save: %s"), *Path);
                continue;
            }
            Homestead::Simulation Candidate;
    if (bEstateMap) PrepareEstateSimulation(Candidate);
            const auto Decoded = Candidate.Deserialize(TCHAR_TO_UTF8(*Save->SimulationData));
            if (!Decoded || Candidate.GetState().failed) continue;
            if (RecoveryOnly && (Save->WorldId != WorldId || Candidate.GetState().hunger < 20
                || Candidate.GetState().energy < 20)) continue;
            if (RecoveryOnly && Slot == TEXT("Homestead_Recovery"))
            {
                if (!ApplySave(*Save)) return false;
                Notify(TEXT("Returned to your sheltered recovery checkpoint."));
                return true;
            }
            if (!Best || Save->SavedAtUtc > Best->SavedAtUtc) Best = Save;
        }
    }
    if (Best)
    {
        if (!ApplySave(*Best)) return false;
        if (!StartupProbeDirectory.IsEmpty()) StartupProbeLoadedState = UTF8_TO_TCHAR(Sim.Serialize().c_str());
        if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadSaveAudit")) && GEngine && GEngine->GameViewport)
            UE_LOG(LogTemp, Display, TEXT("SAVE_LOAD_AUDIT world=%s simulation_md5=%s look=%d,%d,%d,%d,%d,%d,%d view_mode=%d shader_complexity=%d"),
                *WorldId, *FMD5::HashAnsiString(UTF8_TO_TCHAR(Sim.Serialize().c_str())),
                Appearance.HairStyle, Appearance.HairColor, Appearance.SkinTone, Appearance.EyeColor,
                Appearance.TunicColor, Appearance.Outfit, Appearance.BodyPreset,
                GEngine->GameViewport->ViewModeIndex, static_cast<int32>(GEngine->GameViewport->EngineShowFlags.ShaderComplexity));
        Notify(Corrupt ? TEXT("Recovered a valid save. An unreadable save was skipped; backups are retained.") : TEXT("Welcome back to your homestead."), Corrupt);
        return true;
    }
    if (Corrupt || Incompatible)
    {
        LoadProblem = Incompatible && !Corrupt
            ? TEXT("These test saves use an incompatible version. Start a new seeded woodland to use this build; old files are retained.")
            : TEXT("No usable save could be read. Data is corrupt or incompatible; nothing was loaded. You can retry loading or explicitly reset this test world.");
        Notify(LoadProblem, true);
        // Do not let a fresh startup silently autosave over an unsuccessful load.
        bTestResetRequired = !RecoveryOnly && !bHasPlayableSession;
    }
    return false;
}

void AHomesteadController::RetryCheckpoint()
{
    if (LoadLatest(true)) return;
    if (SessionWorld != WorldId)
    { Notify(TEXT("No checkpoint belongs to this world. You can start a new test woodland or quit from Settings."), true); return; }
    Homestead::Simulation Candidate = Sim;
    const auto Result = Candidate.Deserialize(TCHAR_TO_UTF8(*SessionCheckpoint));
    if (!Result) { Notify(Result); return; }
    const auto Region = Candidate.SetActiveWorldRegion({SessionLocation.X, SessionLocation.Y});
    if (!Region) { Notify(Region); return; }
    if (!Landscape->Refresh(Candidate)) { bWorldReady = false; Notify(TEXT("Checkpoint terrain could not be prepared."), true); return; }
    Sim = MoveTemp(Candidate);
    ActiveChestId.Reset();
    bWorldReady = true;
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
    {
        Avatar->CancelAction(true);
        Avatar->CancelSprint();
    }
    Appearance = SessionAppearance;
    PendingLocation = SessionLocation;
    PendingRotation = SessionRotation;
    bFreshTerrainSpawn = false;
    bPendingSpawn = true;
    bWasFailed = false;
    RefreshRemaining = 0;
    Notify(TEXT("Returned to this session's checkpoint. No usable recovery save was available."));
}

void AHomesteadController::CaptureSessionCheckpoint(FVector Location, FRotator Rotation)
{
    SessionCheckpoint = UTF8_TO_TCHAR(Sim.Serialize().c_str());
    SessionAppearance = Appearance;
    SessionWorld = WorldId;
    SessionLocation = Location;
    SessionRotation = Rotation;
}

void AHomesteadController::NewGame()
{
    Homestead::Simulation Candidate = Sim;
    const FGuid Seed = FGuid::NewGuid();
    const auto Result = bEstateMap
        ? Candidate.NewEstateGame(Homestead::ProvisionalEstateLayout(), Homestead::ProvisionalEstatePlacements())
        : Candidate.NewGame((static_cast<uint64>(Seed.A) << 32) | Seed.B);
    if (!Result) { Notify(Result); return; }
    if (!Landscape->Refresh(Candidate)) { bWorldReady = false; Notify(TEXT("The new woodland could not be prepared. Your current session is retained."), true); return; }
    Sim = MoveTemp(Candidate);
    ActiveChestId.Reset();
    bWorldReady = true;
    bTestResetRequired = false;
    bHasPlayableSession = true;
    LastSuccessfulSave = FDateTime();
    LoadProblem.Reset();
    Appearance = FHomesteadAppearance();
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadHeroineTrialVitruvian01")))
    {
        Appearance.HairStyle = 1; Appearance.MetaHair = HomesteadLook::MetaHairForLegacy(1);
    }
    ResetHotbar();
    WorldId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    PendingLocation = FVector(-1000, 0, 180);
    PendingRotation = FRotator(-15, 15, 0);
    bFreshTerrainSpawn = true;
    if (bEstateMap) SetEstateSpawn();
    CaptureSessionCheckpoint(PendingLocation, PendingRotation);
    bPendingSpawn = true;
    bWasFailed = false;
    RefreshRemaining = 0;
    AutosaveRemaining = AutosaveMinutes * 60.0f;
    EndPlacement();
    GrantPlaytestKit(true);
    if (bEstateMap) { BeginNewGameSetup(); return; }
    OpenBook(3);
    Notify(TEXT("A new seeded woodland. Choose where to build; previous save files are still available."));
}
void AHomesteadController::QuickSave()
{
    if (bAutomatedInputOnly) ++TestQuickSaves;
    if (!IsFailed()) SaveSlot(TEXT("Homestead_Manual"));
}
void AHomesteadController::QuickLoad()
{
    if (bAutomatedInputOnly) ++TestQuickLoads;
    if (!LoadLatest())
    {
        if (bTestResetRequired)
        {
            if (NativeMenu.IsValid()) NativeMenu->RequestTestResetPrompt();
            OpenBook(4);
        }
        else if (LoadProblem.IsEmpty()) Notify(TEXT("There is no usable save to load yet."), true);
    }
}

FString AHomesteadController::SavePath(const FString& Slot) const
{
    return FPaths::Combine(SaveRoute.Directory, Slot + TEXT(".sav"));
}

FString AHomesteadController::PreviewLabel() const
{
    return SaveRoute.Mode == TEXT("preview") ? TEXT("Preview: ") + SaveRoute.Profile + TEXT(" (isolated saves)") : FString();
}

void AHomesteadController::InitializeAudio()
{
    bAudioEnabled = !FParse::Param(FCommandLine::Get(), TEXT("nosound"));
    GrassStepA = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/GrassStepA.GrassStepA"));
    GrassStepB = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/GrassStepB.GrassStepB"));
    WoodTapA = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/WoodTapA.WoodTapA"));
    WoodTapB = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/WoodTapB.WoodTapB"));
    CraftStrikeA = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/CraftStrikeA.CraftStrikeA"));
    CraftStrikeB = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/CraftStrikeB.CraftStrikeB"));
    CraftStrikeC = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/CraftStrikeC.CraftStrikeC"));
    UIClick = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/UIClick.UIClick"));
    for (const TCHAR* Chop : {TEXT("ChopA"), TEXT("ChopB"), TEXT("ChopC")})
        if (USoundBase* Cue = LoadObject<USoundBase>(nullptr,
            *FString::Printf(TEXT("/Game/SurvivalGame/Audio/Effects/%s.%s"), Chop, Chop), nullptr, LOAD_NoWarn | LOAD_Quiet))
            ChopStrokes.Add(Cue);
    TreeFallThud = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/TreeFall.TreeFall"),
        nullptr, LOAD_NoWarn | LOAD_Quiet);
    auto LoadPool = [](TArray<TObjectPtr<USoundBase>>& Pool, const TCHAR* Prefix, int32 Count)
    {
        Pool.Reset();
        for (int32 Index = 0; Index < Count; ++Index)
        {
            const FString Name = FString::Printf(TEXT("%s_%02d"), Prefix, Index);
            if (USoundBase* Step = LoadObject<USoundBase>(nullptr,
                    *FString::Printf(TEXT("/Game/SurvivalGame/Audio/Effects/%s.%s"), *Name, *Name)))
                Pool.Add(Step);
        }
    };
    LoadPool(BareWalkSteps, TEXT("BareStepWalk"), 6);
    LoadPool(BareRunSteps, TEXT("BareStepRun"), 4);
    if (!GrassStepA || !GrassStepB || !WoodTapA || !WoodTapB
        || !CraftStrikeA || !CraftStrikeB || !CraftStrikeC || !UIClick || BareWalkSteps.Num() != 6 || BareRunSteps.Num() != 4)
        UE_LOG(LogTemp, Warning, TEXT("Some feedback sounds are missing; rerun the asset/bootstrap pipeline."));
    if (USoundWave* Forest = LoadObject<USoundWave>(nullptr, TEXT("/Game/SurvivalGame/Audio/Ambience/ForestAmbience.ForestAmbience")))
    {
        Forest->bLooping = true;
        Ambience->SetSound(Forest);
        Ambience->SetVolumeMultiplier(AmbienceVolume);
        if (bAudioEnabled) Ambience->FadeIn(3, 1);
    }
    else UE_LOG(LogTemp, Warning, TEXT("Forest ambience is not imported. Run Scripts/bootstrap_unreal.py."));
    if (USoundWave* Brook = LoadObject<USoundWave>(nullptr, TEXT("/Game/SurvivalGame/Audio/Ambience/CreekLoop.CreekLoop")))
    {
        Brook->bLooping = true;
        Creek->SetSound(Brook);
        Creek->SetVolumeMultiplier(AmbienceVolume * CreekGain);
    }
    else UE_LOG(LogTemp, Warning, TEXT("Creek loop is not imported. Run Scripts/bootstrap_unreal.py."));
    // Kevin MacLeod tracks (CC BY 4.0), shuffled with no immediate repeat. Loudness is the gated,
    // K-weighted level measured from each source file (dBFS); every track is matched to the same
    // level, which sits about 14 dB under the old harp-only mix at the default 65% setting, so music
    // stays under the woodland ambience and footsteps instead of dominating them.
    struct FTrack { const TCHAR* Name; float Loudness; };
    static constexpr FTrack Tracks[] = {
        {TEXT("EveningHarp"), -21.0f}, {TEXT("AscendingTheVale"), -21.3f}, {TEXT("TellerOfTheTales"), -23.4f},
        {TEXT("MeditationImpromptu02"), -23.6f}, {TEXT("AtRest"), -28.6f}};
    constexpr float TargetLoudnessAtFullVolume = -35.0f;
    MusicTracks.Reset();
    MusicTrackGains.Reset();
    MusicTrackNames.Reset();
    for (const FTrack& Track : Tracks)
    {
        if (USoundBase* Score = LoadObject<USoundBase>(nullptr,
                *FString::Printf(TEXT("/Game/SurvivalGame/Audio/Music/%s.%s"), Track.Name, Track.Name)))
        {
            MusicTracks.Add(Score);
            MusicTrackNames.Add(Track.Name);
            MusicTrackGains.Add(FMath::Pow(10.0f, (TargetLoudnessAtFullVolume - Track.Loudness) / 20.0f));
        }
        else UE_LOG(LogTemp, Warning, TEXT("Music track %s is not imported and is skipped. Run Scripts/bootstrap_unreal.py."), Track.Name);
    }
    // A fresh launch avoids opening with the track the previous launch last started (a user
    // preference, independent of homestead saves). Order comes from process entropy.
    FString LastTrack;
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    FConfigFile Disk;
    if (Branch && Disk.Combine(Branch->IniPath)) Disk.GetString(AudioSettingsSection, LastMusicTrackKey, LastTrack);
    MusicBag.Reset(MusicTracks.Num(), MusicTrackNames.IndexOfByKey(LastTrack),
        FPlatformTime::Cycles64() ^ static_cast<uint64>(FDateTime::UtcNow().GetTicks()) ^ FPlatformProcess::GetCurrentProcessId());
    if (MusicTracks.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("No music tracks are available; ambience and effects continue."));
    }
    else
    {
        Music->OnAudioFinished.AddDynamic(this, &AHomesteadController::MusicFinished);
    }
}

float AHomesteadController::MusicLevel() const
{
    return MusicVolume * (MusicTrackGains.IsValidIndex(MusicTrack) ? MusicTrackGains[MusicTrack] : 1.0f);
}

void AHomesteadController::StartNextMusicTrack()
{
    MusicTrack = MusicBag.Next();
    if (!MusicTracks.IsValidIndex(MusicTrack)) return;
    Music->SetSound(MusicTracks[MusicTrack].Get());
    Music->SetVolumeMultiplier(MusicLevel());
    UE_LOG(LogTemp, Display, TEXT("MUSIC_TRACK started=%s catalog=%d"), *MusicTrackNames[MusicTrack], MusicTracks.Num());
    if (const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr)
    {
        FConfigFile Property;
        Property.SetString(AudioSettingsSection, LastMusicTrackKey, *MusicTrackNames[MusicTrack]);
        if (!Property.UpdateSinglePropertyInSection(*Branch->IniPath, LastMusicTrackKey, AudioSettingsSection))
            UE_LOG(LogTemp, Warning, TEXT("Could not record the last music track; the next launch may repeat it."));
    }
}

void AHomesteadController::PlayEffect(USoundBase* Cue, float Gain)
{
    if (Cue && bAudioEnabled && EffectsVolume > 0)
        UGameplayStatics::PlaySound2D(this, Cue, EffectsVolume * Gain, FMath::FRandRange(0.96f, 1.04f));
}

void AHomesteadController::PlayFootstep(bool bLeftFoot, bool bRun)
{
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const double Now = GetWorld()->GetTimeSeconds();
    if (!Avatar || bBookOpen || bPlanning || IsFailed() || !Avatar->GetCharacterMovement()->IsMovingOnGround()
        || Avatar->GetVelocity().Size2D() < 12 || Now - LastFootstepTime < 0.18)
        return;
    const auto& Pool = bRun ? BareRunSteps : BareWalkSteps;
    if (Pool.IsEmpty()) return;
    LastFootstepTime = Now;
    ++Footsteps;
    int32 Pick = FMath::RandRange(0, Pool.Num() - 1);
    if (Pool.Num() > 1 && Pick == LastBareStep) Pick = (Pick + 1) % Pool.Num();
    LastBareStep = Pick;
    UE_LOG(LogHomesteadFootsteps, Verbose, TEXT("Footstep %s %s t=%.3f"), bLeftFoot ? TEXT("L") : TEXT("R"),
        bRun ? TEXT("run") : TEXT("walk"), Now);
    // Bare feet on soft soil are quiet: about 10 dB under the old shod grass step while walking,
    // a little firmer when running, with a small level variation so repeats don't stand out.
    PlayEffect(Pool[Pick].Get(), (bRun ? 0.07f : 0.04f) * FMath::FRandRange(0.85f, 1.15f));
}

void AHomesteadController::MusicFinished()
{
    bMusicFading = false;
    MusicElapsed = 0;
    MusicGapRemaining = FMath::FRandRange(55.0f, 110.0f);
}
