#include "HomesteadController.h"
#include "HomesteadControllerText.h"
#include "HomesteadControllerPreferences.h"
#include "HomesteadControllerConfig.h"
#include "HomesteadControllerHelpers.h"
#include "HomesteadEstateGround.h"
#include "Simulation/HomesteadOvergrowth.h"
#include "Simulation/HomesteadCrops.h"
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
#include "Components/CapsuleComponent.h"
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
#include "UI/SHomesteadHudScale.h"
#include "UI/SHomesteadVitals.h"
#include "HomesteadMapComponent.h"
#include "Simulation/HomesteadManor.h"
#include "Simulation/HomesteadLamp.h"
#include "Simulation/HomesteadPail.h"
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

using HomesteadControllerText::Text;
using HomesteadControllerText::SleepClockText;
using HomesteadControllerConfig::ActionHintSection;
using HomesteadControllerConfig::PersistIntProperty;
using HomesteadControllerHelpers::FindPlotWhere;
using HomesteadControllerHelpers::HotbarIcon;
using HomesteadControllerHelpers::IsFoodItem;
using HomesteadControllerHelpers::IsHotbarTool;
using HomesteadControllerHelpers::PlantingCrop;
using HomesteadControllerHelpers::SwingVerb;
using HomesteadControllerHelpers::ToolPrompt;
using HomesteadControllerHelpers::ToolWhereabouts;

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
    // A smoke route on the Estate (-HomesteadEstateSmoke) starts a real new estate game, as a player does.
    const bool EstateSmoke = SmokeTest && bEstateMap;
    if ((!SmokeTest || EstateSmoke) && !VisualPlaytest && !bSaveRoutingTestPending)
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
    if (!Loaded && bEstateMap && !bTestResetRequired && (!SmokeTest || EstateSmoke) && !VisualPlaytest) BeginNewGameSetup();
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

namespace HomesteadWaterProbe
{
// Even-odd test against a closed shoreline spline, sampled every ShoreProbeStepCm.
constexpr float ShoreProbeStepCm = 200.0f;
bool ShoreContains(const USplineComponent& Spline, const FVector2D& Point)
{
    const float Length = Spline.GetSplineLength();
    const int32 Samples = FMath::Clamp(FMath::CeilToInt32(Length / ShoreProbeStepCm), 8, 512);
    bool bInside = false;
    FVector Previous = Spline.GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
    for (int32 Index = 1; Index <= Samples; ++Index)
    {
        const FVector Next = Spline.GetLocationAtDistanceAlongSpline(Length * (Index % Samples) / Samples, ESplineCoordinateSpace::World);
        if ((Previous.Y > Point.Y) != (Next.Y > Point.Y)
            && Point.X < Previous.X + (Point.Y - Previous.Y) * (Next.X - Previous.X) / (Next.Y - Previous.Y))
            bInside = !bInside;
        Previous = Next;
    }
    return bInside;
}
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
        if (bEstateMap)
        {
            // Wait (in place, movement off) for World Partition to stream in the ground under the spawn,
            // then stand her on it. Standalone and packaged games take about a minute to stream it.
            PendingLocation.Z = bFreshTerrainSpawn ? Ground + 150.0f : FMath::Max(PendingLocation.Z, Ground + 100.0f);
            if (!SettleOnGround(PendingLocation, EstateSpawnWait, DeltaSeconds, 180.0f,
                bFreshTerrainSpawn ? TEXT("new game") : TEXT("load"))) return;
        }
        else
        {
            PendingLocation.Z = bFreshTerrainSpawn ? Ground + 100.0f : FMath::Max(PendingLocation.Z, Ground + 100.0f);
        }
        EstateSpawnWait = 0;
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
            Avatar->SnapCamera();
        }
        bPendingSpawn = false;
        if (bFreshTerrainSpawn) CaptureSessionCheckpoint(PendingLocation, PendingRotation);
        bFreshTerrainSpawn = false;
        RefreshMenuPortrait();
    }
    if (bPendingGroundSnap && GetPawn())
    {
        if (!PrepareWorldAt({GroundSnapTarget.X, GroundSnapTarget.Y})) return;
        if (!SettleOnGround(GroundSnapTarget, GroundSnapWait, DeltaSeconds, bEstateMap ? 180.0f : 0.0f, TEXT("teleport"))) return;
        bPendingGroundSnap = false;
        GetPawn()->SetActorLocation(GroundSnapTarget, false, nullptr, ETeleportType::TeleportPhysics);
        if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->SnapCamera();
        LastStepPosition = GroundSnapTarget;
        LastSafeWorldPosition = GroundSnapTarget;
        StepDistance = 0;
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
            Notify(TEXT("Recovered the character above the generated terrain; this traversal needs review."), true);
            if (bEstateMap)
            {
                // She fell through ground that hadn't streamed in yet: hold her until it has, then stand her on it.
                GroundSnapTarget = FVector(Position.X, Position.Y, Surface + 150.0f);
                GroundSnapWait = 0;
                bPendingGroundSnap = true;
                return;
            }
            ControlledPawn->SetActorLocation(FVector(Position.X, Position.Y, Surface + 100),
                false, nullptr, ETeleportType::TeleportPhysics);
        }
        LastSafeWorldPosition = ControlledPawn->GetActorLocation();
    }

    UpdateLamp();
    Sim.Advance(DeltaSeconds, PlayerPoint(), bBookOpen || bPlanning || bTestResetRequired || ShopScreen.IsValid());
    // Out of Energy she dozes off where she stands (the simulation sleeps her on the spot).
    if (Sim.DozeCount() != SeenDozes)
    {
        SeenDozes = Sim.DozeCount();
        if (!IsFailed())
        {
            if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->GetCharacterMovement()->StopMovementImmediately();
            Notify(TEXT("Worn out, you dozed off where you stood. You wake at ") + SleepClockText(State().hour)
                + TEXT(", stiff and only half rested. Sleep in a bed before you're this tired."));
            RefreshRemaining = 0;
        }
    }
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
    if (HeldHarvestPlot != INDEX_NONE)
    {
        const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
        if (!Avatar || Avatar->SticksLiftedFromPile() >= 1 || !Avatar->IsStickPileOnGround()
            || GetWorld()->GetTimeSeconds() - HeldHarvestSince > 6.0)
        {
            if (Landscape) Landscape->ReleaseHarvest();
            HeldHarvestPlot = INDEX_NONE;
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
