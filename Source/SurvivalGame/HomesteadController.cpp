#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadWorld.h"
#include "HomesteadSave.h"
#include "Engine/GameViewportClient.h"
#include "Misc/SecureHash.h"
#include "HomesteadSmokeTest.h"
#include "HomesteadVisualPlaytest.h"
#include "HomesteadTestPaths.h"
#include "Components/AudioComponent.h"
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
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"

namespace
{
FString Text(const char* Value) { return UTF8_TO_TCHAR(Value); }
bool Edible(Homestead::Item Item)
{
    return Item == Homestead::Item::Berries || Item == Homestead::Item::RoastedRoots || Item == Homestead::Item::HerbedRoots;
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
        return Controller.IsValid() && !Controller->MenuPhysicalInput(Event.GetEffectingButton(), IE_Pressed);
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
    Music = CreateDefaultSubobject<UAudioComponent>(TEXT("Music"));
    Music->bAutoActivate = false;
    Music->bAllowSpatialization = false;
    Ambience = CreateDefaultSubobject<UAudioComponent>(TEXT("Ambience"));
    Ambience->bAutoActivate = false;
    Ambience->bAllowSpatialization = false;
}

void AHomesteadController::BeginPlay()
{
    Super::BeginPlay();
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
    if (!PrepareStartupProbe()) return;
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
    WorldId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Landscape = GetWorld()->SpawnActor<AHomesteadWorld>();
    if (!Landscape)
    {
        UE_LOG(LogTemp, Error, TEXT("Unable to create the homestead world."));
        Notify(TEXT("The world could not be created. Check the game log."), true);
        return;
    }
    Landscape->Initialize(Sim.GetState());
    SessionCheckpoint = UTF8_TO_TCHAR(Sim.Serialize().c_str());
    const bool SmokeTest = FParse::Param(FCommandLine::Get(), TEXT("HomesteadSmokeTest"));
    const bool VisualPlaytest = FParse::Param(FCommandLine::Get(), TEXT("HomesteadVisualPlaytest"));
#if !UE_BUILD_SHIPPING
    bAutomatedInputOnly = SmokeTest || VisualPlaytest || bSaveRoutingTestPending;
#endif
    bAutomatedInputOnly |= !StartupProbeDirectory.IsEmpty();
    UE_LOG(LogTemp, Display, TEXT("SAVE_ROUTING version=1 mode=%s profile=%s directory=\"%s\" automation_input=%d smoke_actor=%d visual_actor=%d"),
        *SaveRoute.Mode, *SaveRoute.Profile, *SaveRoute.Directory, bAutomatedInputOnly, SmokeTest, VisualPlaytest);
    const bool Loaded = !SmokeTest && !VisualPlaytest && !bSaveRoutingTestPending && LoadLatest();
    if (!Loaded) OpenBook(3);
    if (!StartupProbeDirectory.IsEmpty() && !Loaded) { FinishStartupProbe(TEXT("The isolated prepared save did not load.")); return; }
    InitializeAudio();
#if !UE_BUILD_SHIPPING
    if (SmokeTest) GetWorld()->SpawnActor<AHomesteadSmokeTest>();
    else if (VisualPlaytest) GetWorld()->SpawnActor<AHomesteadVisualPlaytest>();
#endif
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
    if (Intent != EHomesteadPromptDevice::None) bGamepad = Intent == EHomesteadPromptDevice::Gamepad;
    if (NativeMenu.IsValid() && (bBookOpen || IsFailed()))
    {
        if (bMenuSaveInProgress) return true;
        if (Params.Event == IE_Pressed && Params.Key == EKeys::F5) { QuickSave(); return true; }
        if (Params.Event == IE_Pressed && Params.Key == EKeys::F9) { QuickLoad(); return true; }
        const auto Menu = NativeMenu;
        return Menu->HandleKey(Params.Key, Params.Event, Params.AmountDepressed);
    }
    return Super::InputKey(Params);
}

bool AHomesteadController::MenuPhysicalInput(FKey Key, EInputEvent Event, float Amount)
{
    InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Key, Event, Amount, false, FPlatformTime::Cycles64()));
    return !bAutomatedInputOnly;
}

bool AHomesteadController::MenuPointerIntent(float X, float Y)
{
    MenuPhysicalInput(EKeys::MouseX, IE_Axis, X);
    MenuPhysicalInput(EKeys::MouseY, IE_Axis, Y);
    return !bAutomatedInputOnly && !bGamepad;
}

void AHomesteadController::ShowNativeMenu()
{
    if (!GEngine || !GEngine->GameViewport) return;
    if (!NativeMenu.IsValid())
    {
        FlushPressedKeys();
        NativeMenu = SNew(SHomesteadMenu).Controller(this);
        GEngine->GameViewport->AddViewportWidgetContent(NativeMenu.ToSharedRef(), 100);
        MenuPointerInput = MakeShared<FHomesteadMenuPointerInput>(this);
        FSlateApplication::Get().RegisterInputPreProcessor(MenuPointerInput);
    }
    bShowMouseCursor = true;
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
    FlushPressedKeys();
    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());
}

void AHomesteadController::EndPlay(const EEndPlayReason::Type Reason)
{
    HideNativeMenu();
    Super::EndPlay(Reason);
}

void AHomesteadController::MenuPage(int32 TargetPage)
{
    if (!bMenuSaveInProgress) OpenBook(TargetPage);
}
void AHomesteadController::MenuSelect(int32 Row)
{
    Selection = FMath::Clamp(Row, 0, FMath::Max(0, Rows().Num() - 1));
}
void AHomesteadController::MenuActivate()
{
    if (bMenuSaveInProgress || !bBookOpen) return;
    if (IsFailed() && Page != 4 && Page != 3 && Page != 5)
    { Notify(TEXT("Retry a checkpoint before changing possessions or appearance."), true); return; }
    if (IsFailed() && Page == 4 && Rows().IsValidIndex(Selection) && Rows()[Selection].Id == 0)
    { Notify(TEXT("A failed state cannot replace your checkpoint. Retry or quit without saving."), true); return; }
    ActivateRow();
}
void AHomesteadController::MenuStore() { if (!bMenuSaveInProgress) Secondary(); }
void AHomesteadController::MenuTake() { if (!bMenuSaveInProgress) Withdraw(); }
void AHomesteadController::MenuBack() { if (!bMenuSaveInProgress) CloseBook(); }
void AHomesteadController::MenuRetry()
{
    if (bMenuSaveInProgress) return;
    RetryCheckpoint();
    if (!IsFailed()) CloseBook();
}
void AHomesteadController::MenuRequestExit() { if (NativeMenu.IsValid()) NativeMenu->RequestExit(); }
FString AHomesteadController::MenuSaveStatus() const
{
    const FString When = LastSuccessfulSave.GetTicks() > 0
        ? LastSuccessfulSave.ToString(TEXT("%Y-%m-%d %H:%M:%S UTC")) : TEXT("not known in this session");
    return FString::Printf(TEXT("%s\nLast successful save: %s"),
        PreviewLabel().IsEmpty() ? TEXT("Current homestead") : *PreviewLabel(), *When);
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
    if (Saved) UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
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
    InputComponent->BindKey(EKeys::G, IE_Pressed, this, &AHomesteadController::Withdraw);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed, this, &AHomesteadController::Withdraw);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AHomesteadController::Back);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &AHomesteadController::Back);
    InputComponent->BindKey(EKeys::I, IE_Pressed, this, &AHomesteadController::ToggleBook);
    InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AHomesteadController::ToggleBook);
    InputComponent->BindKey(EKeys::Gamepad_Special_Right, IE_Pressed, this, &AHomesteadController::ToggleBook);
    InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AHomesteadController::OpenCraft);
    InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AHomesteadController::OpenBuild);
    InputComponent->BindKey(EKeys::H, IE_Pressed, this, &AHomesteadController::OpenJournal);
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

bool AHomesteadController::HasHeroine() const
{
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    return Avatar && Avatar->HasHeroine();
}

void AHomesteadController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Landscape) return;
    if (!StartupProbeDirectory.IsEmpty()) TickStartupProbe();
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
        const float Ground = AHomesteadWorld::GroundHeight(PendingLocation.X, PendingLocation.Y);
        PendingLocation.Z = FMath::Max(PendingLocation.Z, Ground + 100.0f);
        GetPawn()->SetActorLocation(PendingLocation, false, nullptr, ETeleportType::TeleportPhysics);
        LastStepPosition = PendingLocation;
        StepDistance = 0;
        SetControlRotation(PendingRotation);
        if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
        {
            if (!Avatar->ApplyAppearance(Appearance))
                Notify(TEXT("Character assets could not be applied. The technical stand-in remains; see the log."), true);
            Avatar->SetAppearancePreview(bBookOpen && Page == 6);
        }
        bPendingSpawn = false;
    }
    if (APawn* ControlledPawn = GetPawn())
    {
        const FVector Position = ControlledPawn->GetActorLocation();
        if (FMath::Abs(Position.X) > 3900 || FMath::Abs(Position.Y) > 3900 || Position.Z < -1000)
        {
            FVector Safe(FMath::Clamp(Position.X, -3850.0, 3850.0), FMath::Clamp(Position.Y, -3850.0, 3850.0), 0);
            Safe.Z = AHomesteadWorld::GroundHeight(Safe.X, Safe.Y) + 100;
            ControlledPawn->SetActorLocation(Safe, false, nullptr, ETeleportType::TeleportPhysics);
            Notify(TEXT("The prototype ends here. Back to the clearing."));
        }
    }

    Sim.Advance(DeltaSeconds, PlayerPoint(), bBookOpen || bPlanning);
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
        if (!bBookOpen && !bPlanning && !IsFailed() && Avatar->GetCharacterMovement()->IsMovingOnGround()
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
    RefreshRemaining -= DeltaSeconds;
    if (RefreshRemaining <= 0)
    {
        Landscape->Refresh(Sim.GetState());
        UpdateFocus();
        RefreshRemaining = 0.25f;
    }
    if (!bBookOpen && !bPlanning && !IsFailed())
    {
        AutosaveRemaining -= DeltaSeconds;
        if (AutosaveRemaining <= 0)
        {
            SaveSlot(FString::Printf(TEXT("Homestead_Auto_%d"), AutoSaveIndex), true);
            AutoSaveIndex = (AutoSaveIndex + 1) % 3;
            AutosaveRemaining = 240;
        }
    }
    if (bAudioEnabled && Music->Sound)
    {
        if (Music->IsPlaying())
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
                Music->SetVolumeMultiplier(MusicVolume);
                Music->FadeIn(4, 1);
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
    for (const auto& Plot : State().plots)
        Consider(EFocus::Plot, Plot.id, Homestead::CellCenter(Plot.cellX, Plot.cellY));
    for (const auto& Structure : State().structures)
    {
        EFocus Kind = EFocus::None;
        if (Structure.kind == Homestead::Piece::Fire) Kind = EFocus::Fire;
        if (Structure.kind == Homestead::Piece::Bed) Kind = EFocus::Bed;
        if (Structure.kind == Homestead::Piece::Chest) Kind = EFocus::Chest;
        if (Kind != EFocus::None) Consider(Kind, Structure.id, Homestead::CellCenter(Structure.cellX, Structure.cellY));
    }
    if (Focus == EFocus::None && Homestead::IsNearWater(Position)) Focus = EFocus::Water;
}

FString AHomesteadController::FocusTitle() const
{
    switch (Focus)
    {
    case EFocus::Resource:
        for (const auto& Node : State().resources)
            if (Node.id == FocusId)
                return Text(Homestead::ResourceName(Node.kind)) + (Sim.CanHarvest(Node.id) ? TEXT("") : TEXT("  (renewing)"));
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
    case EFocus::Fire: return TEXT("Cookfire");
    case EFocus::Bed: return TEXT("Bedroll");
    case EFocus::Chest: return TEXT("Storage chest");
    case EFocus::Water: return TEXT("Fresh stream water");
    default: break;
    }
    return TEXT("The clearing");
}

FString AHomesteadController::FocusActions() const
{
    const FString A = bGamepad ? TEXT("[A]") : TEXT("[E]");
    const FString X = bGamepad ? TEXT("[X]") : TEXT("[F]");
    switch (Focus)
    {
    case EFocus::Resource: return A + TEXT(" Gather   ") + X + TEXT(" Clear");
    case EFocus::Plot:
        for (const auto& Plot : State().plots)
            if (Plot.id == FocusId)
                return Plot.planted
                    ? A + (Plot.growth >= 1 ? TEXT(" Harvest") : TEXT(" Water")) + TEXT("   ") + X + TEXT(" Weed")
                    : A + TEXT(" Plant roots   ") + X + TEXT(" Plant berry seeds");
        break;
    case EFocus::Fire: return A + TEXT(" Cook   ") + X + TEXT(" Add a branch");
    case EFocus::Bed: return A + TEXT(" Sleep 8 hours");
    case EFocus::Chest: return A + TEXT(" Open pack / storage");
    case EFocus::Water: return A + TEXT(" Fill watering can");
    default: return X + TEXT(" Till ground   ") + (bGamepad ? TEXT("[Menu] Field book") : TEXT("[I] Field book"));
    }
    return FString();
}

void AHomesteadController::Notify(const Homestead::Result& Result, USoundBase* SuccessCue)
{
    Notify(UTF8_TO_TCHAR(Result.message.c_str()), !Result.ok);
    if (Result.ok && SuccessCue) PlayEffect(SuccessCue);
    RefreshRemaining = 0;
}

void AHomesteadController::Notify(const FString& Message, bool Error)
{
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
    if (bPlanning)
    {
        const auto Result = Sim.Place(BuildKind, BuildCellX, BuildCellY, BuildRotation, Position);
        Notify(Result, WoodTapA);
        if (Result.ok) Sim.AdvanceGameHours(0.1, Position);
        return;
    }
    UpdateFocus();
    switch (Focus)
    {
    case EFocus::Resource:
    {
        bool Forage = false;
        for (const auto& Node : State().resources)
            if (Node.id == FocusId) { Forage = Node.kind != Homestead::ResourceKind::Sapling; break; }
        const auto Result = Sim.Harvest(FocusId, Position);
        Notify(Result, GrassStepA);
        if (Result.ok && Forage)
            if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->PlayGather();
        break;
    }
    case EFocus::Plot:
        for (const auto& Plot : State().plots)
        {
            if (Plot.id != FocusId) continue;
            const bool Planted = Plot.planted;
            const bool Mature = Plot.growth >= 1;
            const auto Result = !Planted ? Sim.Plant(FocusId, Position) :
                Mature ? Sim.HarvestCrop(FocusId, Position) : Sim.Water(FocusId, Position);
            Notify(Result, GrassStepB);
            if (Result.ok && Planted && !Mature)
                if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->PlayWater();
            break;
        }
        break;
    case EFocus::Fire: OpenBook(1); Selection = 3; break;
    case EFocus::Bed:
    {
        Notify(Sim.Sleep(8, Position));
        if (!IsFailed())
        {
            SaveSlot(FString::Printf(TEXT("Homestead_Auto_%d"), AutoSaveIndex), true);
            AutoSaveIndex = (AutoSaveIndex + 1) % 3;
            if (Sim.IsSheltered(Position) && State().hunger >= 35 && State().warmth >= 45)
                SaveSlot(TEXT("Homestead_Recovery"), true);
        }
        break;
    }
    case EFocus::Chest: OpenBook(0); break;
    case EFocus::Water: Notify(Sim.FillWater(Position)); break;
    default: Notify(TEXT("Walk closer to a plant, resource, or work area.")); break;
    }
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
    UpdateFocus();
    if (Focus == EFocus::Resource)
    {
        bool Sapling = false;
        for (const auto& Node : State().resources)
            if (Node.id == FocusId) { Sapling = Node.kind == Homestead::ResourceKind::Sapling; break; }
        const auto Result = Sim.Clear(FocusId, PlayerPoint());
        Notify(Result, WoodTapB);
        if (Result.ok && Sapling)
            if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->PlayClear();
    }
    else if (Focus == EFocus::Plot)
    {
        for (const auto& Plot : State().plots)
        {
            if (Plot.id != FocusId) continue;
            const bool Planted = Plot.planted;
            const auto Result = Planted ? Sim.Weed(FocusId, PlayerPoint())
                : Sim.Plant(FocusId, PlayerPoint(), Homestead::CropKind::Berries);
            Notify(Result, GrassStepA);
            if (Result.ok && Planted)
                if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->PlayGather();
            break;
        }
    }
    else if (Focus == EFocus::Fire) Notify(Sim.AddFuel(FocusId, PlayerPoint()), WoodTapA);
    else
    {
        const auto Position = PlayerPoint();
        const FVector Forward = GetPawn() ? GetPawn()->GetActorForwardVector() : FVector::ForwardVector;
        const int X = FMath::FloorToInt((Position.x + Forward.X * 190) / Homestead::CellSize);
        const int Y = FMath::FloorToInt((Position.y + Forward.Y * 190) / Homestead::CellSize);
        Notify(Sim.Till(X, Y, Position), GrassStepB);
    }
}

void AHomesteadController::OpenBook(int32 TargetPage)
{
    EndPlacement();
    bBookOpen = true;
    Page = FMath::Clamp(TargetPage, 0, 6);
    Selection = 0;
    bConfirmRestart = false;
    PlayEffect(UIClick, 0.08f);
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
    {
        Avatar->CancelAction();
        Avatar->GetCharacterMovement()->StopMovementImmediately();
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
    bConfirmRestart = false;
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->SetAppearancePreview(false);
    if (IsFailed()) ShowNativeMenu();
    else HideNativeMenu();
}
void AHomesteadController::ToggleBook() { if (IsFailed()) return; if (bBookOpen) CloseBook(); else OpenBook(0); }
void AHomesteadController::OpenCraft() { if (!IsFailed()) OpenBook(1); }
void AHomesteadController::OpenBuild() { if (!IsFailed()) OpenBook(2); }
void AHomesteadController::OpenJournal() { if (!IsFailed()) OpenBook(3); }
void AHomesteadController::Back()
{
    if (IsFailed()) { RetryCheckpoint(); return; }
    if (bBookOpen) CloseBook();
    else if (bPlanning) EndPlacement();
    else OpenBook(4);
}
void AHomesteadController::PreviousPage()
{
    if (bPlanning) { RotatePlacement(); return; }
    if (!bBookOpen) { OpenBook(1); return; }
    Page = (Page + 6) % 7; Selection = 0; bConfirmRestart = false;
    PlayEffect(UIClick, 0.08f);
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->SetAppearancePreview(Page == 6);
}
void AHomesteadController::NextPage()
{
    if (bPlanning) { RotatePlacement(); return; }
    if (!bBookOpen) { OpenBook(2); return; }
    Page = (Page + 1) % 7; Selection = 0; bConfirmRestart = false;
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
            Result.Add({ Index, Text(Homestead::RecipeName(Recipe)),
                FString::Printf(TEXT("Needs: %s"), *Text(Homestead::RecipeRequirements(Recipe))), TEXT("craft") });
        }
    }
    else if (Page == 2)
    {
        for (int Index = 0; Index < static_cast<int>(Homestead::Piece::Count); ++Index)
        {
            const auto Piece = static_cast<Homestead::Piece>(Index);
            Result.Add({ Index, Text(Homestead::PieceName(Piece)),
                FString::Printf(TEXT("Needs: %s"), *Text(Homestead::PieceRequirements(Piece))), TEXT("plan") });
        }
    }
    else if (Page == 3)
    {
        Result.Add({0, TEXT("A place to spend the night"), TEXT("Start with the plants and materials around the clearing.")});
        Result.Add({1, TEXT("1. Find a little breakfast"), TEXT("Gather berries, then eat them from the Pack page.")});
        Result.Add({2, TEXT("2. Make your first tools"), TEXT("Branches, loose stones and reeds supply wood, stone and fiber.")});
        Result.Add({3, TEXT("3. Make a home"), TEXT("Craft a hatchet. Clear saplings, then place a floor, walls, doorway and roof.")});
        Result.Add({4, TEXT("4. Tend a little garden"), TEXT("Craft a digging stick. Till with F/X; bare plots offer roots with A/E or berry seeds with X/F.")});
        Result.Add({5, TEXT("5. Water and weed"), TEXT("Fill a watering can at the stream. F/X removes weeds from a plot.")});
        Result.Add({6, TEXT("6. Cook and rest"), TEXT("Fuel a cookfire with branches. Roast roots; sleep in a sheltered bedroll.")});
        Result.Add({7, TEXT("Make this place your own"), TEXT("The Look page offers three hairstyles, two cosmetic outfits, and color choices.")});
    }
    else if (Page == 4)
    {
        Result.Add({0, TEXT("Save progress"), TEXT("Write a manual save, retaining the previous backup.")});
        Result.Add({1, TEXT("Load latest save"), TEXT("Resume the newest valid manual or automatic save.")});
        Result.Add({2, FString::Printf(TEXT("Day length: %.0f minutes"), State().dayMinutes), TEXT("Cycle 30 / 60 / 120 real minutes per complete game day.")});
        Result.Add({3, FString::Printf(TEXT("Camera sensitivity: %.1f"), Sensitivity), TEXT("Cycle a comfortable turn speed.")});
        Result.Add({4, FString::Printf(TEXT("Invert camera Y: %s"), bInvertY ? TEXT("On") : TEXT("Off")), TEXT("Change vertical look direction.")});
        Result.Add({5, FString::Printf(TEXT("Music volume: %d%%"), FMath::RoundToInt(MusicVolume * 100)), TEXT("Cycle volume; nature continues between pieces.")});
        Result.Add({6, FString::Printf(TEXT("Ambience volume: %d%%"), FMath::RoundToInt(AmbienceVolume * 100)), TEXT("Wind and woodland ambience.")});
        Result.Add({7, FString::Printf(TEXT("Effects volume: %d%%"), FMath::RoundToInt(EffectsVolume * 100)), TEXT("Footsteps, gathering, crafting, and interface sounds.")});
        Result.Add({8, TEXT("Start a new clearing"), TEXT("Open a confirmation before replacing this test session. Cancel keeps your current clearing.")});
        Result.Add({9, TEXT("Save and quit"), TEXT("Save this homestead and close the game. Failed saves leave the game open.")});
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
    }
    else if (Page == 6)
    {
        Result.Add({0, FString::Printf(TEXT("Hair: %s"), HomesteadLook::HairStyleName(Appearance.HairStyle)), TEXT("Long waves, a straight bob, or a practical ponytail.")});
        Result.Add({1, FString::Printf(TEXT("Hair color: %s"), HomesteadLook::HairColorName(Appearance.HairColor)), TEXT("A small chestnut-based color palette.")});
        Result.Add({2, FString::Printf(TEXT("Skin: %s"), HomesteadLook::SkinToneName(Appearance.SkinTone)), TEXT("Prototype tone adjustments; deeper presets follow.")});
        Result.Add({3, FString::Printf(TEXT("Eyes: %s"), HomesteadLook::EyeColorName(Appearance.EyeColor)), TEXT("Iris color changes preserve the whites and pupils.")});
        Result.Add({4, FString::Printf(TEXT("Tunic dye: %s"), HomesteadLook::TunicColorName(Appearance.TunicColor)), TEXT("A color choice for the current original outfit.")});
        Result.Add({5, FString::Printf(TEXT("Outfit: %s"), HomesteadLook::OutfitName(Appearance.Outfit)), TEXT("Cosmetic linen choices; an apron is not winter insulation.")});
        Result.Add({6, FString::Printf(TEXT("Preset: %s"), HomesteadLook::BodyPresetName(Appearance.BodyPreset)), TEXT("Preferred, leaner/defined Willow, or softer/full Hazel.")});
        Result.Add({7, TEXT("An early character creator"), TEXT("Three complete presets. Detailed face/body sliders and clothing physics follow later.")});
    }
    else
    {
        Result.Add({0, TEXT("Music: Evening Fall (Harp)"), TEXT("Kevin MacLeod - incompetech.com")});
        Result.Add({1, TEXT("Creative Commons Attribution 4.0"), TEXT("https://creativecommons.org/licenses/by/4.0/")});
        Result.Add({2, TEXT("Music playback"), TEXT("Converted for game playback; playback fades applied.")});
        Result.Add({3, TEXT("Forest ambience"), TEXT("TinyWorlds - OpenGameArt - CC0")});
        Result.Add({4, TEXT("Brown Mud Leaves 01"), TEXT("Rob Tuytel - Poly Haven - CC0")});
        Result.Add({5, TEXT("Rock Moss Set 02"), TEXT("Kless Gyzen - Poly Haven - CC0")});
        Result.Add({6, TEXT("Complete credits"), TEXT("See docs/asset-credits.md in the project or packaged build.")});
        Result.Add({7, TEXT("Interaction and footstep sounds"), TEXT("Kenney - Impact Sounds and Interface Sounds - CC0")});
        Result.Add({8, TEXT("Character foundation"), TEXT("MakeHuman Community / MPFB graphical assets - CC0; original outfit and motion.")});
    }
    return Result;
}

FString AHomesteadController::BookTitle() const
{
    switch (Page)
    {
    case 0: return TEXT("Your pack");
    case 1: return TEXT("Crafting recipes");
    case 2: return TEXT("Building plans");
    default: return TEXT("Field book");
    }
}

FString AHomesteadController::BookSummary() const
{
    switch (Page)
    {
    case 0: return TEXT("Carried counts are in your pack; Chest counts are in nearby storage.");
    case 1: return TEXT("Recipes show what you can make, not what you carry.");
    case 2: return TEXT("Choose a plan to preview placement. Materials are spent when you place it.");
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
        if (Result.ok) Sim.AdvanceGameHours(0.05, PlayerPoint());
    }
    else if (Page == 2) BeginPlacement(static_cast<Homestead::Piece>(Id));
    else if (Page == 6)
    {
        FHomesteadAppearance Next = Appearance;
        switch (Id)
        {
        case 0: Next.HairStyle = (Next.HairStyle + 1) % 3; break;
        case 1: Next.HairColor = (Next.HairColor + 1) % 4; break;
        case 2: Next.SkinTone = (Next.SkinTone + 1) % 4; break;
        case 3: Next.EyeColor = (Next.EyeColor + 1) % 4; break;
        case 4: Next.TunicColor = (Next.TunicColor + 1) % 4; break;
        case 5: Next.Outfit = (Next.Outfit + 1) % 2; break;
        case 6: Next.BodyPreset = (Next.BodyPreset + 1) % 3; break;
        default: Notify(TEXT("More character presets and clothes are planned. These controls are a first prototype.")); return;
        }
        auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
        if (!Avatar || !Avatar->ApplyAppearance(Next))
        {
            Notify(TEXT("That appearance could not be applied. Your saved selection has not changed."), true);
            return;
        }
        Appearance = Next;
        PlayEffect(UIClick, 0.08f);
    }
    else if (Page == 4)
    {
        switch (Id)
        {
        case 0: QuickSave(); break;
        case 1: QuickLoad(); break;
        case 2: Notify(Sim.SetDayMinutes(State().dayMinutes < 60 ? 60 : State().dayMinutes < 120 ? 120 : 30)); break;
        case 3: Sensitivity = Sensitivity >= 1.8f ? 0.6f : Sensitivity + 0.2f; break;
        case 4: bInvertY = !bInvertY; break;
        case 5:
            MusicVolume = MusicVolume >= 0.99f ? 0 : FMath::Min(1.0f, MusicVolume + 0.2f);
            Music->SetVolumeMultiplier(MusicVolume);
            break;
        case 6:
            AmbienceVolume = AmbienceVolume >= 0.99f ? 0 : FMath::Min(1.0f, AmbienceVolume + 0.2f);
            Ambience->SetVolumeMultiplier(AmbienceVolume);
            break;
        case 7: EffectsVolume = EffectsVolume >= 0.99f ? 0 : FMath::Min(1.0f, EffectsVolume + 0.2f); break;
        case 8: if (bConfirmRestart) NewGame(); else bConfirmRestart = true; break;
        case 9:
            MenuRequestExit();
            break;
        case 10:
            if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
            {
                float Normalized = 0, Scale = 100, Minimum = 0, Maximum = 100;
                Settings->GetResolutionScaleInformationEx(Normalized, Scale, Minimum, Maximum);
                Settings->SetResolutionScaleValueEx(Scale > 99 ? 85 : Scale > 84 ? 70 : 100);
                Settings->ApplyNonResolutionSettings();
                if (!FParse::Param(FCommandLine::Get(), TEXT("HomesteadSmokeTest")))
                {
                    Settings->SaveSettings();
                    FConfigFile Disk;
                    float Persisted = -1;
                    const float Requested = Scale > 99 ? 85 : Scale > 84 ? 70 : 100;
                    if (!Disk.Combine(GGameUserSettingsIni)
                        || !Disk.GetFloat(TEXT("ScalabilityGroups"), TEXT("sg.ResolutionQuality"), Persisted)
                        || !FMath::IsNearlyEqual(Persisted, Requested, 0.1f))
                    {
                        Settings->SetResolutionScaleValueEx(Scale);
                        Settings->ApplyNonResolutionSettings();
                        Notify(TEXT("Could not save 3D resolution scale. The previous preference was restored."), true);
                    }
                }
            }
            else Notify(TEXT("Video settings are unavailable in this session."), true);
            break;
        case 11: ToggleVerticalSync(); break;
        default: break;
        }
    }
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
    bPlanning = true;
    BuildKind = Kind;
    BuildRotation = 0;
    const auto Position = PlayerPoint();
    const FVector Forward = GetPawn() ? GetPawn()->GetActorForwardVector() : FVector::ForwardVector;
    BuildCellX = FMath::FloorToInt((Position.x + Forward.X * 350) / Homestead::CellSize);
    BuildCellY = FMath::FloorToInt((Position.y + Forward.Y * 350) / Homestead::CellSize);
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->SetPlanning(true);
    if (Landscape) Landscape->SetPlacementPreview(true, BuildKind, BuildCellX, BuildCellY, BuildRotation);
}

void AHomesteadController::EndPlacement()
{
    bPlanning = false;
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->SetPlanning(false);
    if (Landscape) Landscape->SetPlacementPreview(false, BuildKind, BuildCellX, BuildCellY, BuildRotation);
}

void AHomesteadController::NudgePlacement(FVector2D Axis)
{
    if (!bPlanning || Axis.SizeSquared() < 0.2) return;
    const double Now = GetWorld()->GetRealTimeSeconds();
    if (Now - LastNudgeTime < 0.18) return;
    LastNudgeTime = Now;
    const FRotator View(0, GetControlRotation().Yaw, 0);
    const FVector Direction = FRotationMatrix(View).GetUnitAxis(EAxis::X) * Axis.Y
        + FRotationMatrix(View).GetUnitAxis(EAxis::Y) * Axis.X;
    const int StepX = FMath::Abs(Direction.X) > FMath::Abs(Direction.Y) ? (Direction.X > 0 ? 1 : -1) : 0;
    const int StepY = StepX == 0 ? (Direction.Y > 0 ? 1 : -1) : 0;
    const auto Target = Homestead::CellCenter(BuildCellX + StepX, BuildCellY + StepY);
    const auto Position = PlayerPoint();
    if (FMath::Square(Target.x - Position.x) + FMath::Square(Target.y - Position.y) > FMath::Square(700.0)) return;
    BuildCellX += StepX;
    BuildCellY += StepY;
    Landscape->SetPlacementPreview(true, BuildKind, BuildCellX, BuildCellY, BuildRotation);
}

void AHomesteadController::RotatePlacement()
{
    if (!bPlanning) return;
    BuildRotation = (BuildRotation + 1) % 4;
    Landscape->SetPlacementPreview(true, BuildKind, BuildCellX, BuildCellY, BuildRotation);
}

FString AHomesteadController::PlacementLabel() const
{
    return FString::Printf(TEXT("%s  |  %s"), *Text(Homestead::PieceName(BuildKind)), *Text(Homestead::PieceRequirements(BuildKind)));
}

void AHomesteadController::CycleZoom()
{
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->CycleZoom();
}

UHomesteadSave* AHomesteadController::ReadSave(const FString& Filename) const
{
    TArray<uint8> Data;
    if (IFileManager::Get().FileSize(*Filename) > 4 * 1024 * 1024) return nullptr;
    if (!FFileHelper::LoadFileToArray(Data, *Filename)) return nullptr;
    const char Magic[] = "HOMESAV1";
    if (Data.Num() < 16 || FMemory::Memcmp(Data.GetData(), Magic, 8) != 0) return nullptr;
    uint32 ExpectedCrc = 0;
    FMemory::Memcpy(&ExpectedCrc, Data.GetData() + 8, sizeof(ExpectedCrc));
    if (FCrc::MemCrc32(Data.GetData() + 12, Data.Num() - 12) != ExpectedCrc) return nullptr;
    Data.RemoveAt(0, 12, EAllowShrinking::No);
    UHomesteadSave* Save = Cast<UHomesteadSave>(UGameplayStatics::LoadGameFromMemory(Data));
    FGuid ParsedWorld;
    if (!Save || Save->Version < 1 || Save->Version > 4 || Save->PlayerLocation.ContainsNaN() || Save->ViewRotation.ContainsNaN()
        || !FGuid::Parse(Save->WorldId, ParsedWorld) || !ParsedWorld.IsValid()
        || FMath::Abs(Save->PlayerLocation.X) > 4000 || FMath::Abs(Save->PlayerLocation.Y) > 4000
        || FMath::Abs(Save->PlayerLocation.Z) > 5000 || !FMath::IsFinite(Save->CameraSensitivity)
        || Save->CameraSensitivity < 0.2 || Save->CameraSensitivity > 3
        || !FMath::IsFinite(Save->MusicVolume) || Save->MusicVolume < 0 || Save->MusicVolume > 1
        || !FMath::IsFinite(Save->AmbienceVolume) || Save->AmbienceVolume < 0 || Save->AmbienceVolume > 1
        || !FMath::IsFinite(Save->EffectsVolume) || Save->EffectsVolume < 0 || Save->EffectsVolume > 1)
        return nullptr;
    FHomesteadAppearance SavedLook;
    SavedLook.HairStyle = Save->HairStyle;
    SavedLook.HairColor = Save->HairColor;
    SavedLook.SkinTone = Save->SkinTone;
    SavedLook.EyeColor = Save->EyeColor;
    SavedLook.TunicColor = Save->TunicColor;
    SavedLook.Outfit = Save->Outfit;
    SavedLook.BodyPreset = Save->BodyPreset;
    if (!SavedLook.IsValid()) return nullptr;
    Homestead::Simulation Candidate;
    if (!Candidate.Deserialize(TCHAR_TO_UTF8(*Save->SimulationData))) return nullptr;
    return Save;
}

bool AHomesteadController::SaveSlot(const FString& Slot, bool Quiet)
{
    if (!bSaveRoutingReady) { Notify(TEXT("Save routing is unavailable. No save files were accessed."), true); return false; }
    UHomesteadSave* Save = Cast<UHomesteadSave>(UGameplayStatics::CreateSaveGameObject(UHomesteadSave::StaticClass()));
    if (!Save) { Notify(TEXT("Could not create a save record."), true); return false; }
    Save->WorldId = WorldId;
    Save->HairStyle = Appearance.HairStyle;
    Save->HairColor = Appearance.HairColor;
    Save->SkinTone = Appearance.SkinTone;
    Save->EyeColor = Appearance.EyeColor;
    Save->TunicColor = Appearance.TunicColor;
    Save->Outfit = Appearance.Outfit;
    Save->BodyPreset = Appearance.BodyPreset;
    Save->SimulationData = UTF8_TO_TCHAR(Sim.Serialize().c_str());
    Save->PlayerLocation = GetPawn() ? GetPawn()->GetActorLocation() : PendingLocation;
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    Save->ViewRotation = Avatar ? Avatar->GameplayViewRotation() : GetControlRotation();
    Save->SavedAtUtc = FDateTime::UtcNow().ToUnixTimestamp();
    Save->CameraSensitivity = Sensitivity;
    Save->InvertCameraY = bInvertY;
    Save->MusicVolume = MusicVolume;
    Save->AmbienceVolume = AmbienceVolume;
    Save->EffectsVolume = EffectsVolume;
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
    return true;
}

void AHomesteadController::ApplySave(const UHomesteadSave& Save)
{
    const auto Result = Sim.Deserialize(TCHAR_TO_UTF8(*Save.SimulationData));
    if (!Result) { Notify(Result); return; }
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->CancelAction();
    WorldId = Save.WorldId;
    LastSuccessfulSave = FDateTime::FromUnixTimestamp(Save.SavedAtUtc);
    Appearance.HairStyle = Save.HairStyle;
    Appearance.HairColor = Save.HairColor;
    Appearance.SkinTone = Save.SkinTone;
    Appearance.EyeColor = Save.EyeColor;
    Appearance.TunicColor = Save.TunicColor;
    Appearance.Outfit = Save.Outfit;
    Appearance.BodyPreset = Save.BodyPreset;
    PendingLocation = Save.PlayerLocation;
    PendingRotation = Save.ViewRotation;
    bPendingSpawn = true;
    bWasFailed = false;
    Sensitivity = Save.CameraSensitivity;
    bInvertY = Save.InvertCameraY;
    MusicVolume = Save.MusicVolume;
    AmbienceVolume = Save.AmbienceVolume;
    EffectsVolume = Save.EffectsVolume;
    Music->SetVolumeMultiplier(MusicVolume);
    Ambience->SetVolumeMultiplier(AmbienceVolume);
    EndPlacement();
    CloseBook();
    Landscape->Refresh(State());
    RefreshRemaining = 0;
}

bool AHomesteadController::LoadLatest(bool RecoveryOnly)
{
    if (!bSaveRoutingReady) { Notify(TEXT("Save routing is unavailable. No save files were accessed."), true); return false; }
    const TArray<FString> Slots = RecoveryOnly
        ? TArray<FString>{TEXT("Homestead_Recovery"), TEXT("Homestead_Auto_0"), TEXT("Homestead_Auto_1"), TEXT("Homestead_Auto_2"), TEXT("Homestead_Manual")}
        : TArray<FString>{TEXT("Homestead_Manual"), TEXT("Homestead_Auto_0"), TEXT("Homestead_Auto_1"), TEXT("Homestead_Auto_2"), TEXT("Homestead_Recovery")};
    UHomesteadSave* Best = nullptr;
    bool Corrupt = false;
    for (const auto& Slot : Slots)
    {
        for (const FString& Suffix : { FString(), FString(TEXT(".bak")) })
        {
            const FString Path = SavePath(Slot) + Suffix;
            if (!IFileManager::Get().FileExists(*Path)) continue;
            UHomesteadSave* Save = ReadSave(Path);
            if (!Save)
            {
                Corrupt = true;
                UE_LOG(LogTemp, Warning, TEXT("Cannot read save: %s"), *Path);
                continue;
            }
            Homestead::Simulation Candidate;
            const auto Decoded = Candidate.Deserialize(TCHAR_TO_UTF8(*Save->SimulationData));
            if (!Decoded || Candidate.GetState().failed) continue;
            if (RecoveryOnly && (Save->WorldId != WorldId || Candidate.GetState().hunger < 20
                || Candidate.GetState().warmth < 20 || Candidate.GetState().energy < 20)) continue;
            if (RecoveryOnly && Slot == TEXT("Homestead_Recovery"))
            {
                ApplySave(*Save);
                Notify(TEXT("Returned to your sheltered recovery checkpoint."));
                return true;
            }
            if (!Best || Save->SavedAtUtc > Best->SavedAtUtc) Best = Save;
        }
    }
    if (Best)
    {
        ApplySave(*Best);
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
    if (Corrupt) Notify(TEXT("No valid save could be read. Files were preserved; starting a new session."), true);
    return false;
}

void AHomesteadController::RetryCheckpoint()
{
    if (LoadLatest(true)) return;
    const auto Result = Sim.Deserialize(TCHAR_TO_UTF8(*SessionCheckpoint));
    if (!Result) { Notify(Result); return; }
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->CancelAction();
    PendingLocation = FVector(-1000, 0, 180);
    bPendingSpawn = true;
    bWasFailed = false;
    RefreshRemaining = 0;
    Notify(TEXT("Returned to the start of this clearing. No recovery save was available."));
}

void AHomesteadController::NewGame()
{
    Sim.NewGame();
    Appearance = FHomesteadAppearance();
    WorldId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    SessionCheckpoint = UTF8_TO_TCHAR(Sim.Serialize().c_str());
    PendingLocation = FVector(-1000, 0, 180);
    PendingRotation = FRotator(-15, 15, 0);
    bPendingSpawn = true;
    bWasFailed = false;
    RefreshRemaining = 0;
    AutosaveRemaining = 240;
    EndPlacement();
    OpenBook(3);
    Notify(TEXT("A new clearing. Previous save files are still available."));
}
void AHomesteadController::QuickSave()
{
    if (bAutomatedInputOnly) ++TestQuickSaves;
    if (!IsFailed()) SaveSlot(TEXT("Homestead_Manual"));
}
void AHomesteadController::QuickLoad()
{
    if (bAutomatedInputOnly) ++TestQuickLoads;
    if (!LoadLatest()) Notify(TEXT("There is no usable save to load yet."), true);
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
    UIClick = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/UIClick.UIClick"));
    if (!GrassStepA || !GrassStepB || !WoodTapA || !WoodTapB || !UIClick)
        UE_LOG(LogTemp, Warning, TEXT("Some feedback sounds are missing; rerun the asset/bootstrap pipeline."));
    if (USoundWave* Forest = LoadObject<USoundWave>(nullptr, TEXT("/Game/SurvivalGame/Audio/Ambience/ForestAmbience.ForestAmbience")))
    {
        Forest->bLooping = true;
        Ambience->SetSound(Forest);
        Ambience->SetVolumeMultiplier(AmbienceVolume);
        if (bAudioEnabled) Ambience->FadeIn(3, 1);
    }
    else UE_LOG(LogTemp, Warning, TEXT("Forest ambience is not imported. Run Scripts/bootstrap_unreal.py."));
    if (USoundBase* Score = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Music/EveningHarp.EveningHarp")))
    {
        Music->SetSound(Score);
        Music->OnAudioFinished.AddDynamic(this, &AHomesteadController::MusicFinished);
    }
    else UE_LOG(LogTemp, Warning, TEXT("Music is not imported. Run Scripts/bootstrap_unreal.py."));
}

void AHomesteadController::PlayEffect(USoundBase* Cue, float Gain)
{
    if (Cue && bAudioEnabled && EffectsVolume > 0)
        UGameplayStatics::PlaySound2D(this, Cue, EffectsVolume * Gain, FMath::FRandRange(0.96f, 1.04f));
}

void AHomesteadController::MusicFinished()
{
    bMusicFading = false;
    MusicElapsed = 0;
    MusicGapRemaining = FMath::FRandRange(55.0f, 110.0f);
}
