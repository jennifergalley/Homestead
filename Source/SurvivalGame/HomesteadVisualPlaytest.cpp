#include "HomesteadVisualPlaytest.h"
#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWateringTool.h"
#include "HomesteadHatchet.h"
#include "HomesteadTestPaths.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "HardwareInfo.h"
#include "Components/SkeletalMeshComponent.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

AHomesteadVisualPlaytest::AHomesteadVisualPlaytest()
{
    PrimaryActorTick.bCanEverTick = HomesteadAutomatedActorsEnabled();
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void AHomesteadVisualPlaytest::Tap(FKey Key)
{
    if (!Key.IsValid()) return;
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1));
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0));
}

void AHomesteadVisualPlaytest::ApplyAxes(FVector2D Move, FVector2D Look)
{
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftX, IE_Axis, Move.X, 1));
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftY, IE_Axis, Move.Y, 1));
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_RightX, IE_Axis, Look.X, 1));
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_RightY, IE_Axis, Look.Y, 1));
}

void AHomesteadVisualPlaytest::RecordPresentationSettings(const TCHAR* Phase)
{
    PresentationSettings.Add(FString::Printf(TEXT("[%s] rhi=%s render_offscreen=%d forced_windowed=%d"),
        Phase, *FHardwareInfo::GetHardwareInfo(NAME_RHI),
        FParse::Param(FCommandLine::Get(), TEXT("RenderOffscreen")),
        FParse::Param(FCommandLine::Get(), TEXT("windowed"))));
    if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
    {
        const FViewport* Viewport = GEngine->GameViewport->Viewport;
        const FIntPoint Size = Viewport->GetSizeXY(), Target = Viewport->GetRenderTargetTextureSizeXY();
        PresentationSettings.Add(FString::Printf(TEXT("actual_viewport=%d,%d output_target=%d,%d actual_window_mode=%d (0=exclusive,1=borderless,2=windowed)"),
            Size.X, Size.Y, Target.X, Target.Y, static_cast<int32>(Viewport->GetWindowMode())));
    }
    else PresentationSettings.Add(TEXT("actual_viewport=unavailable"));
    if (const auto* Avatar = Cast<AHomesteadCharacter>(PC->GetPawn()))
    {
        const auto* Component = Avatar->GetMesh();
        PresentationSettings.Add(FString::Printf(TEXT("heroine_mesh=%s relative_scale=%s"),
            *GetPathNameSafe(Component->GetSkeletalMeshAsset()), *Component->GetRelativeScale3D().ToString()));
        for (int32 Index = 0; Index < Component->GetNumMaterials(); ++Index)
            PresentationSettings.Add(FString::Printf(TEXT("heroine_material[%d]=%s base_material=%s"), Index,
                *GetPathNameSafe(Component->GetMaterial(Index)),
                *GetPathNameSafe(Component->GetMaterial(Index) ? Component->GetMaterial(Index)->GetMaterial() : nullptr)));
    }
    if (const auto* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
    {
        float Normalized = 0, Scale = 0, Minimum = 0, Maximum = 0;
        Settings->GetResolutionScaleInformationEx(Normalized, Scale, Minimum, Maximum);
        const FIntPoint Resolution = Settings->GetScreenResolution();
        PresentationSettings.Add(FString::Printf(TEXT("user_settings_resolution=%d,%d user_settings_mode=%d user_settings_vsync=%d user_settings_dynamic_resolution=%d user_settings_frame_limit=%.3f user_settings_scale=%.3f"),
            Resolution.X, Resolution.Y, static_cast<int32>(Settings->GetFullscreenMode()),
            Settings->IsVSyncEnabled(), Settings->IsDynamicResolutionEnabled(), Settings->GetFrameRateLimit(), Scale));
    }
    else PresentationSettings.Add(TEXT("user_settings=unavailable"));
    for (const TCHAR* Name : {TEXT("r.VSync"), TEXT("t.MaxFPS"), TEXT("r.FullScreenMode"),
        TEXT("r.ScreenPercentage"), TEXT("r.ScreenPercentage.Default"), TEXT("r.ScreenPercentage.Default.Desktop.Mode"),
        TEXT("r.DynamicRes.OperationMode"), TEXT("r.AntiAliasingMethod"), TEXT("sg.AntiAliasingQuality"),
        TEXT("r.TemporalAA.Upsampling"), TEXT("r.TSR.History.ScreenPercentage"),
        TEXT("r.DefaultFeature.MotionBlur"), TEXT("r.MotionBlurQuality"), TEXT("r.Shadow.Virtual.Enable"),
        TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.ReflectionMethod"),
        TEXT("rhi.SyncInterval"), TEXT("r.D3D12.UseAllowTearing")})
    {
        const auto* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
        PresentationSettings.Add(Variable
            ? FString::Printf(TEXT("cvar %s=%s flags=0x%08x"), Name, *Variable->GetString(), Variable->GetFlags())
            : FString::Printf(TEXT("cvar %s=unregistered"), Name));
    }
}

void AHomesteadVisualPlaytest::Prepare()
{
    OutputDirectory = HomesteadTestOutputDirectory();
    if (const auto* Avatar = Cast<AHomesteadCharacter>(PC->GetPawn()))
    {
        AddTickPrerequisiteComponent(Avatar->GetWateringTool());
        AddTickPrerequisiteComponent(Avatar->GetHatchet());
    }
    IFileManager::Get().MakeDirectory(*FPaths::Combine(OutputDirectory, TEXT("Frames")), true);
    bForageRenewal = FParse::Param(FCommandLine::Get(), TEXT("HomesteadForageRenewal"));
    if (bForageRenewal) { PrepareRenewal(); return; }
    bEndurance = FParse::Param(FCommandLine::Get(), TEXT("HomesteadEndurance"));
    if (bEndurance) { PrepareEndurance(); return; }
    Telemetry.Add(TEXT("frame,seconds,pass,x,y,z,speed,yaw,view_yaw,left_toe_x,left_toe_y,left_toe_z,right_toe_x,right_toe_y,right_toe_z,walk_weight,gait_rate,left_hand_x,left_hand_y,left_hand_z,right_hand_x,right_hand_y,right_hand_z,walk_phase,gather_weight,gather_phase,gather_starts,forage_x,forage_y,water_weight,water_phase,water_starts,tool_visible,tool_x,tool_y,tool_z,can_pitch,water_stock,plot_moisture,tool_scale,tool_radius,plot_weeds"));
    bWaterRoute = FParse::Param(FCommandLine::Get(), TEXT("HomesteadWateringPlaytest"));
    bClearRoute = FParse::Param(FCommandLine::Get(), TEXT("HomesteadClearingPlaytest"));
    Telemetry[0] += TEXT(",clear_weight,clear_phase,clear_starts,hatchet_visible,hatchet_pitch,hatchet_scale,hatchet_radius,hatchet_x,hatchet_y,hatchet_z,branch_stock,fiber_stock,resource_cleared,energy");
    bWeedRoute = FParse::Param(FCommandLine::Get(), TEXT("HomesteadWeedingPlaytest"));
    bPresentationDiagnostics = FParse::Param(FCommandLine::Get(), TEXT("HomesteadPresentationDiagnostics"));
    if (bPresentationDiagnostics && (bWaterRoute || bClearRoute || bWeedRoute))
    {
        Observations.Add(TEXT("FAILED Presentation diagnostics cannot be combined with an action route."));
        Finish();
        return;
    }
    Observations.Add(bWeedRoute
        ? TEXT("Weeding uses an explicitly copied preexisting functional-test save, including saved appearance/location. Prior setup used fixture teleports and ordinary sleep. After F9 loading, this recorded approach/action uses normal mapped controls, no debug teleport/time/state edits. See fixture.json for source and hash.")
        : TEXT("Observational visual playtest: normal mapped controls; no teleports, state edits, or time skips."));
    Observations.Add(TEXT("Frames are sampled at 8 Hz. Screenshot readback can disturb pacing; do not use this run as a frame-rate benchmark."));
    if (bWaterRoute)
        Observations.Add(TEXT("Watering starts from the normal new clearing: mapped gathering, crafting, stream refill, tilling and planting. No fixture/save injection. Ordinary crafting still advances its existing game time. Setup is sampled at 1 Hz; final action at requested 8 Hz."));
    if (bClearRoute)
        Observations.Add(TEXT("Sapling clearing starts from the normal new clearing: mapped supply gathering, hatchet crafting, walking and X. No injected save, teleport or debug state/time edit; ordinary crafting retains its existing time cost. Setup sampled at1 Hz, action at requested8 Hz."));
    Passes = {
        {TEXT("close-notes"), 1, {}, {}, EKeys::Gamepad_Special_Right},
        {TEXT("idle"), 3},
        {TEXT("slow-walk"), 4, FVector2D(0, 0.5f)},
        {TEXT("stop-from-slow-walk"), 2},
        {TEXT("full-walk"), 3, FVector2D(0, 1)},
        {TEXT("turn-while-moving"), 3, FVector2D(0.7f, 0.45f)},
        {TEXT("stop-from-turn"), 2},
        {TEXT("orbit-standing-character"), 4, {}, FVector2D(0.65f, 0)},
        {TEXT("open-pack"), 0.5f, {}, {}, EKeys::Gamepad_Special_Right},
        {TEXT("open-look"), 3, {}, {}, EKeys::Gamepad_LeftShoulder},
        {TEXT("portrait-idle"), 3, {}, {}, EKeys::Gamepad_RightThumbstick},
        {TEXT("portrait-orbit"), 4, {}, FVector2D(0.45f, 0)},
        {TEXT("return-to-world"), 1, {}, {}, EKeys::Gamepad_FaceButton_Right},
        {TEXT("walk-to-forage"), 30, {}, {}, FKey(), true},
        {TEXT("settle-forage"), 0.65f},
        {TEXT("view-picking-side"), 1.15f, {}, FVector2D(-0.65f, 0)},
        {TEXT("before-pick"), 0.8f},
        {TEXT("gather"), 3},
        {TEXT("after-gather"), 2}
    };
    if (bWeedRoute)
        Passes = {
            {TEXT("load-disclosed-test-world"), 1, {}, {}, EKeys::F9},
            {TEXT("walk-to-garden-staging"), 90},
            {TEXT("approach-weedy-plot"), 30},
            {TEXT("settle-weed"), 0.8f},
            {TEXT("view-weeding-side"), 1.15f, {}, FVector2D(-0.65f, 0)},
            {TEXT("before-weed"), 1},
            {TEXT("weed"), 3.5f, {}, {}, EKeys::Gamepad_FaceButton_Left},
            {TEXT("after-weed"), 2}
        };
    if (bPresentationDiagnostics)
    {
        Passes = {
            {TEXT("close-notes"), 1, {}, {}, EKeys::Gamepad_Special_Right},
            {TEXT("warm-up-no-capture"), 3},
            {TEXT("timing-idle"), 2},
            {TEXT("timing-walk"), 6, FVector2D(0, 0.6f)},
            {TEXT("timing-turn"), 6, FVector2D(0.5f, 0.6f), FVector2D(0.35f, 0)},
            {TEXT("timing-sweep"), 6, {}, FVector2D(0.65f, 0)},
            {TEXT("capture-idle"), 2},
            {TEXT("capture-walk"), 4, FVector2D(0, 0.6f)},
            {TEXT("capture-turn"), 4, FVector2D(0.5f, 0.6f), FVector2D(0.35f, 0)},
            {TEXT("capture-sweep"), 6, {}, FVector2D(0.65f, 0)}
        };
        PresentationTimings.Reserve(4096);
        PresentationTimings.Add(TEXT("seconds,pass,wall_frame_ms,engine_delta_ms,captures_requested_before_tick,capture_requested_this_tick,x,y,speed,view_yaw"));
        PresentationSettings.Add(TEXT("Opt-in observer only. Normal mapped game inputs; fresh test-sandbox world, no save injection, teleports or time edits."));
        PresentationSettings.Add(TEXT("Output is GPU-rendered game framebuffer, NOT physical scanout. Actual DXGI Present flags/interval, DWM composition and VRR engagement are not observed."));
        PresentationSettings.Add(TEXT("Timing records instrumented actor-tick wall intervals, NOT GPU duration or present timestamps. Only timing-* precedes all screenshot requests; capture-* is readback-disturbed and visits different positions, not a controlled performance A/B."));
        PresentationSettings.Add(TEXT("Runtime CVars and user settings are recorded separately. output_target is not the internal temporal-upscaler input resolution; auto/default resolution policy may require further evidence."));
    }
    RecordPresentationSettings(TEXT("start"));
    LastWallTime = FPlatformTime::Seconds();
}

void AHomesteadVisualPlaytest::Capture(const FString& Label)
{
    const auto* Avatar = Cast<AHomesteadCharacter>(PC->GetPawn());
    if (!Avatar) return;
    const FVector Position = Avatar->GetActorLocation();
    const FVector Left = Avatar->GetMesh()->GetBoneLocation(TEXT("ball_l"));
    const FVector Right = Avatar->GetMesh()->GetBoneLocation(TEXT("ball_r"));
    const FVector LeftHand = Avatar->GetMesh()->GetBoneLocation(TEXT("hand_l"));
    const FVector RightHand = Avatar->GetMesh()->GetBoneLocation(TEXT("hand_r"));
    const auto* Animation = Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance());
    const auto* Tool = Avatar->GetWateringTool();
    const FVector Grip = Tool->GripPosition();
    double Moisture = -1, Weeds = -1;
    for (const auto& Plot : PC->State().plots) if (Plot.id == WaterPlotId) { Moisture = Plot.moisture; Weeds = Plot.weeds; }
    Telemetry.Add(FString::Printf(TEXT("%d,%.4f,%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.4f,%.4f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.4f,%.4f,%.4f,%u,%.3f,%.3f,%.4f,%.4f,%u,%d,%.3f,%.3f,%.3f,%.3f,%d,%.6f,%.3f,%.3f,%.6f"),
        CaptureIndex, Elapsed, *Label, Position.X, Position.Y, Position.Z, Avatar->GetVelocity().Size2D(),
        Avatar->GetActorRotation().Yaw, PC->GetControlRotation().Yaw,
        Left.X, Left.Y, Left.Z, Right.X, Right.Y, Right.Z,
        Animation ? Animation->WalkWeight() : -1, Animation ? Animation->GaitRate() : -1,
        LeftHand.X, LeftHand.Y, LeftHand.Z, RightHand.X, RightHand.Y, RightHand.Z,
        Animation ? Animation->WalkPhase() : -1,
        Animation ? Animation->GatherWeight() : -1, Animation ? Animation->GatherPhase() : -1,
        Animation ? Animation->GatherStarts() : 0, ForageTarget.X, ForageTarget.Y,
        Animation ? Animation->WaterWeight() : 0, Animation ? Animation->WaterPhase() : 0,
        Animation ? Animation->WaterStarts() : 0, Tool->IsPresented(), Grip.X, Grip.Y, Grip.Z,
        Tool->GetComponentRotation().Pitch, PC->Simulation().Count(Homestead::Item::Water), Moisture,
        Tool->GetComponentScale().X, Tool->Bounds.SphereRadius, Weeds));
    const auto* Hatchet = Avatar->GetHatchet();
    const FVector HatchetGrip = Hatchet->GripPosition();
    int32 Cleared = -1;
    for (const auto& Node : PC->State().resources) if (Node.id == ForageId) Cleared = Node.cleared;
    Telemetry.Last() += FString::Printf(TEXT(",%.4f,%.4f,%u,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d,%.6f"),
        Animation ? Animation->ClearWeight() : 0, Animation ? Animation->ClearPhase() : 0, Animation ? Animation->ClearStarts() : 0,
        Hatchet->IsPresented(), Hatchet->GetComponentRotation().Pitch, Hatchet->GetComponentScale().X, Hatchet->Bounds.SphereRadius,
        HatchetGrip.X, HatchetGrip.Y, HatchetGrip.Z, PC->Simulation().Count(Homestead::Item::Branch),
        PC->Simulation().Count(Homestead::Item::Fiber), Cleared, PC->State().energy);
    if (Animation && bClearRoute && (Label == TEXT("clear") || Label == TEXT("after-clear")))
    {
        bObservedClear |= Animation->ClearWeight() > 0.5f;
        bObservedHatchet |= Hatchet->IsPresented() && Hatchet->GetComponentRotation().Pitch < -30;
        bClearRecovered |= bObservedClear && Animation->ClearWeight() < 0.001f && !Hatchet->IsPresented();
    }
    if (Animation && (Label == TEXT("gather") || Label == TEXT("after-gather")
        || Label == TEXT("weed") || Label == TEXT("after-weed")))
    {
        bObservedGather |= Animation->GatherWeight() > 0.5f;
        bGatherRecovered |= bObservedGather && Animation->GatherWeight() < 0.001f && Animation->GatherPhase() >= 1.59f;
    }
    if (Animation && bWaterRoute && (Label == TEXT("water") || Label == TEXT("after-water")))
    {
        bObservedWater |= Animation->WaterWeight() > 0.5f;
        bObservedTool |= Tool->IsPresented() && Tool->GetComponentRotation().Pitch < -20;
        bWaterRecovered |= bObservedWater && Animation->WaterWeight() < 0.001f && !Tool->IsPresented();
    }
    const FString Name = FString::Printf(TEXT("frame-%05d.png"), CaptureIndex++);
    FScreenshotRequest::RequestScreenshot(FPaths::Combine(OutputDirectory, TEXT("Frames"), Name), false, false);
}

void AHomesteadVisualPlaytest::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bFinished) return;
    if (!bReady)
    {
        Elapsed += DeltaSeconds;
        PC = Cast<AHomesteadController>(UGameplayStatics::GetPlayerController(this, 0));
        if (!PC || !PC->GetPawn() || Elapsed < 8) return;
#if WITH_EDITOR
        if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling()) return;
#endif
        bReady = true;
        Prepare();
        if (bFinished) return;
        Elapsed = 0;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadShippingQA"))
        && IFileManager::Get().FileExists(*FPaths::Combine(OutputDirectory, TEXT("stop-qa.txt"))))
    {
        Observations.Add(TEXT("FAILED Shipping QA cancelled by its owned supervisor."));
        Finish();
        return;
    }
    if (bForageRenewal) { TickRenewal(DeltaSeconds); return; }
    if (bEndurance) { TickEndurance(DeltaSeconds); return; }
    const double Now = FPlatformTime::Seconds();
    const double RawWallDelta = Now - LastWallTime;
    const float WallDelta = FMath::Min(static_cast<float>(RawWallDelta), 1.0f);
    LastWallTime = Now;
    Elapsed += WallDelta;
    if (bWeedRoute) { TickWeeding(WallDelta); return; }
    if (bWaterRoute || bClearRoute) { TickWatering(WallDelta); return; }
    if (!Passes.IsValidIndex(PassIndex) || Elapsed > 100) { Finish(); return; }
    FPass& Pass = Passes[PassIndex];
    if (!bEntered)
    {
        bEntered = true;
        PassElapsed = 0;
        CaptureElapsed = 0;
        Tap(Pass.Press);
        if (Pass.WalkToForage)
        {
            double Best = 1.e20;
            const auto Position = PC->PlayerPoint();
            for (const auto& Node : PC->State().resources)
            {
                if (!PC->Simulation().CanHarvest(Node.id)
                    || (Node.kind != Homestead::ResourceKind::BerryBush && Node.kind != Homestead::ResourceKind::Flowers)) continue;
                const double Distance = FMath::Square(Node.position.x - Position.x) + FMath::Square(Node.position.y - Position.y);
                if (Distance < Best)
                {
                    Best = Distance;
                    ForageId = Node.id;
                    ForageTarget = FVector2D(Node.position.x, Node.position.y);
                }
            }
            FoodBefore = PC->Simulation().Count(Homestead::Item::Berries);
            HerbBefore = PC->Simulation().Count(Homestead::Item::Flowers);
            Observations.Add(FString::Printf(TEXT("Forage target id=%d position=%s"), ForageId, *ForageTarget.ToString()));
        }
        if (Pass.Label == TEXT("gather"))
        {
            if (bReachedForage && PC->IsResourceFocused(ForageId)) Tap(EKeys::Gamepad_FaceButton_Bottom);
            else Observations.Add(TEXT("Forage approach did not reach its target in time; gather was not faked."));
        }
        if (Pass.Label == TEXT("settle-forage"))
        {
            Tap(EKeys::Gamepad_RightThumbstick);
            Tap(EKeys::Gamepad_RightThumbstick);
        }
        Observations.Add(FString::Printf(TEXT("BEGIN %.2fs %s: %s"), Elapsed, *Pass.Label, *PC->FocusTitle()));
    }
    FVector2D Move = Pass.Move;
    FVector2D Look = Pass.Look;
    if (Pass.WalkToForage && ForageId >= 0)
    {
        const auto Position = PC->PlayerPoint();
        const FVector2D Offset = ForageTarget - FVector2D(Position.x, Position.y);
        const float DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(Offset.Y, Offset.X));
        const float Difference = FMath::FindDeltaAngleDegrees(static_cast<float>(PC->GetControlRotation().Yaw), DesiredYaw);
        Look.X = FMath::Clamp(Difference / 45.0f, -0.7f, 0.7f);
        Move.Y = FMath::Abs(Difference) < 40 ? (Offset.Size() < 90 ? 0.4f : 0.6f) : 0;
        if (Offset.Size() < 55 && PC->IsResourceFocused(ForageId))
        {
            Move = Look = FVector2D::ZeroVector;
            bReachedForage = true;
            PassElapsed = Pass.Duration;
        }
    }
    ApplyAxes(Move, Look);
    PassElapsed += WallDelta;
    CaptureElapsed += WallDelta;
    const bool RequestCapture = CaptureElapsed >= 0.125f
        && (!bPresentationDiagnostics || Pass.Label.StartsWith(TEXT("capture-")));
    if (bPresentationDiagnostics)
    {
        const FVector Position = PC->GetPawn()->GetActorLocation();
        PresentationTimings.Add(FString::Printf(TEXT("%.6f,%s,%.6f,%.6f,%d,%d,%.3f,%.3f,%.3f,%.3f"),
            Elapsed, *Pass.Label, RawWallDelta * 1000, DeltaSeconds * 1000,
            CaptureIndex, RequestCapture, Position.X, Position.Y, PC->GetPawn()->GetVelocity().Size2D(),
            PC->GetControlRotation().Yaw));
    }
    if (RequestCapture)
    {
        Capture(Pass.Label);
        CaptureElapsed = 0;
    }
    if (PassElapsed >= Pass.Duration)
    {
        ApplyAxes({}, {});
        ++PassIndex;
        bEntered = false;
    }
}

void AHomesteadVisualPlaytest::Finish()
{
    if (bFinished) return;
    bFinished = true;
    ApplyAxes({}, {});
    const bool Gathered = PC->Simulation().Count(Homestead::Item::Berries) > FoodBefore
        || PC->Simulation().Count(Homestead::Item::Flowers) > HerbBefore;
    if (bPresentationDiagnostics)
        Observations.Add(FString::Printf(TEXT("Presentation diagnostic route completed=%d; captured frames=%d; physical scanout not observed"),
            PassIndex >= Passes.Num() && !Passes.IsEmpty(), CaptureIndex));
    else if (bClearRoute)
        Observations.Add(FString::Printf(TEXT("Cleared actual sapling=%d; action observed=%d; swung hatchet observed=%d; recovered and hidden=%d"),
            bCleared, bObservedClear, bObservedHatchet, bClearRecovered));
    else if (bWeedRoute)
        Observations.Add(FString::Printf(TEXT("Weeded existing planted plot=%d; action observed=%d; recovered to idle=%d"),
            bWeeded, bObservedGather, bGatherRecovered));
    else if (bWaterRoute)
        Observations.Add(FString::Printf(TEXT("Watered real planted plot=%d; action observed=%d; tilted tool observed=%d; recovered and hidden=%d"),
            bWatered, bObservedWater, bObservedTool, bWaterRecovered));
    else
    {
        Observations.Add(FString::Printf(TEXT("Forage target reached=%d; resources actually gathered=%d"), bReachedForage, Gathered));
        Observations.Add(FString::Printf(TEXT("Picking action observed=%d; recovered to idle=%d"), bObservedGather, bGatherRecovered));
    }
    Observations.Add(TEXT("This observational capture is not a visual-quality pass or a replacement for human feel/listening review."));
    bool Saved = FFileHelper::SaveStringToFile(FString::Join(Telemetry, TEXT("\n")) + TEXT("\n"),
        *FPaths::Combine(OutputDirectory, TEXT("telemetry.csv")));
    Saved = FFileHelper::SaveStringToFile(FString::Join(Observations, TEXT("\n")) + TEXT("\n"),
        *FPaths::Combine(OutputDirectory, TEXT("observations.txt"))) && Saved;
    if (bPresentationDiagnostics)
    {
        Saved = FFileHelper::SaveStringToFile(FString::Join(PresentationTimings, TEXT("\n")) + TEXT("\n"),
            *FPaths::Combine(OutputDirectory, TEXT("presentation-timings.csv"))) && Saved;
    }
    RecordPresentationSettings(TEXT("end"));
    Saved = FFileHelper::SaveStringToFile(FString::Join(PresentationSettings, TEXT("\n")) + TEXT("\n"),
        *FPaths::Combine(OutputDirectory, TEXT("presentation-settings.txt"))) && Saved;
    if (!Saved) UE_LOG(LogTemp, Error, TEXT("Visual playtest could not persist all evidence files."));
    UE_LOG(LogTemp, Display, TEXT("Visual playtest captured %d frames in %s"), CaptureIndex, *OutputDirectory);
    const bool Complete = bPresentationDiagnostics ? PassIndex >= Passes.Num() && !Passes.IsEmpty() && CaptureIndex > 0
        : bClearRoute ? bCleared && bObservedClear && bObservedHatchet && bClearRecovered
        : bWeedRoute ? bWeeded && bObservedGather && bGatherRecovered
        : bWaterRoute ? bWatered && bObservedWater && bObservedTool && bWaterRecovered
        : bReachedForage && Gathered && bObservedGather && bGatherRecovered;
    const bool Cancelled = FParse::Param(FCommandLine::Get(), TEXT("HomesteadShippingQA"))
        && IFileManager::Get().FileExists(*FPaths::Combine(OutputDirectory, TEXT("stop-qa.txt")));
    FPlatformMisc::RequestExitWithStatus(false, Saved && Complete && !Cancelled ? 0 : 1);
}
