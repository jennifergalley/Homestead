#include "HomesteadVisualPlaytest.h"
#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadWorld.h"
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
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"
#include "HardwareInfo.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "PhysicsEngine/BodySetup.h"
#include "MaterialShared.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformMemory.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonSerializer.h"
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
    PresentationSettings.Add(FString::Printf(TEXT("process_used_physical_bytes[%s]=%llu"),
        Phase, static_cast<unsigned long long>(FPlatformMemory::GetStats().UsedPhysical)));
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
    bTreeRoute = !bWaterRoute && !bClearRoute && !bWeedRoute && !bPresentationDiagnostics;
    if (bTreeRoute) PrepareTreeEncounter();
    if (bTreeRoute || bPresentationDiagnostics)
    {
        RecordGroveInventory();
        RecordGrassGroundInventory();
    }
    RecordPresentationSettings(TEXT("start"));
    LastWallTime = FPlatformTime::Seconds();
}

void AHomesteadVisualPlaytest::RecordGroveInventory()
{
    TArray<UStaticMeshComponent*> Parts;
    if (PC->Landscape) PC->Landscape->GetComponents(Parts);
    TArray<UStaticMeshComponent*> Trees;
    for (auto* Part : Parts)
        if (Part->ComponentHasTag(TEXT("AuthoredTreeSmall02Grove"))) Trees.Add(Part);
    TArray<FString> Rows;
    Rows.Add(TEXT("component,x,y,z,yaw,triangles,slots,scale_error,ground_error_cm,canopy_radius_cm,home_margin_cm,resource_margin_cm,structure_margin_cm,plot_margin_cm,stream_margin_cm,tree_spacing_margin_cm,materials_ready,collision_ready,encounter"));
    bool Valid = Trees.Num() >= 8 && Trees.Num() <= 16;
    int64 TotalTriangles = 0;
    for (auto* Tree : Trees)
    {
        UStaticMesh* Mesh = Tree->GetStaticMesh();
        const auto* Data = Mesh ? Mesh->GetRenderData() : nullptr;
        const int32 Triangles = Data && Data->LODResources.Num() ? Data->LODResources[0].GetNumTriangles() : 0;
        const FBox Bounds = Mesh ? Mesh->GetBoundingBox() : FBox(ForceInit);
        const double Radius = FVector2D(FMath::Max(FMath::Abs(Bounds.Min.X), FMath::Abs(Bounds.Max.X)),
            FMath::Max(FMath::Abs(Bounds.Min.Y), FMath::Abs(Bounds.Max.Y))).Size();
        const FVector Root = Tree->GetComponentLocation();
        const FVector2D Position(Root);
        const double HomeMargin = FVector2D::Distance(Position, FVector2D(-1000, 0)) - Radius - 650;
        const double StreamMargin = FMath::Abs(Root.X - Homestead::StreamX(Root.Y)) - Radius - 120;
        double ResourceMargin = 1e9, StructureMargin = 1e9, PlotMargin = 1e9, SpacingMargin = 1e9;
        for (const auto& Node : PC->State().resources)
            ResourceMargin = FMath::Min(ResourceMargin,
                FVector2D::Distance(Position, FVector2D(Node.position.x, Node.position.y)) - Radius - 130);
        for (const auto& Structure : PC->State().structures)
        {
            const auto Center = Homestead::CellCenter(Structure.cellX, Structure.cellY);
            StructureMargin = FMath::Min(StructureMargin,
                FVector2D::Distance(Position, FVector2D(Center.x, Center.y)) - Radius - 225);
        }
        for (const auto& Plot : PC->State().plots)
        {
            const auto Center = Homestead::CellCenter(Plot.cellX, Plot.cellY);
            PlotMargin = FMath::Min(PlotMargin,
                FVector2D::Distance(Position, FVector2D(Center.x, Center.y)) - Radius - 175);
        }
        for (const auto* Other : Trees)
            if (Other != Tree)
                SpacingMargin = FMath::Min(SpacingMargin,
                    FVector2D::Distance(Position, FVector2D(Other->GetComponentLocation())) - 2 * Radius);
        const double ScaleError = (Tree->GetComponentScale() - FVector::OneVector).Size();
        const double GroundError = Root.Z - AHomesteadWorld::GroundHeight(Root.X, Root.Y);
        const auto* Body = Mesh ? Mesh->GetBodySetup() : nullptr;
        const bool CollisionReady = Body && Body->AggGeom.SphylElems.Num() == 1
            && Body->AggGeom.GetElementCount() == 1 && Body->CollisionTraceFlag == CTF_UseSimpleAsComplex
            && Tree->IsQueryCollisionEnabled() && Tree->GetOwner()->GetActorEnableCollision()
            && Tree->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block;
        bool MaterialsReady = Mesh && Mesh->GetStaticMaterials().Num() == 3;
        for (int32 Slot = 0; Mesh && Slot < Mesh->GetStaticMaterials().Num(); ++Slot)
        {
            const auto* Interface = Tree->GetMaterial(Slot);
            auto* Material = Interface ? Interface->GetMaterial() : nullptr;
            auto* Resource = Material ? Material->GetMaterialResource(GMaxRHIShaderPlatform) : nullptr;
            MaterialsReady &= Material && Material->GetPathName().StartsWith(TEXT("/Game/Trials/TreeSmall02_20260921_01/Materials/"))
                && Resource && Resource->GetGameThreadShaderMap() && Resource->IsGameThreadShaderMapComplete();
        }
        Valid &= Triangles == 231785 && Mesh
            && Mesh->GetPathName() == TEXT("/Game/Trials/TreeSmall02_20260921_01/Meshes/SM_TreeSmall02_LOD2.SM_TreeSmall02_LOD2")
            && ScaleError < 0.001 && FMath::Abs(GroundError) < 0.1
            && HomeMargin >= -0.1 && ResourceMargin >= -0.1 && StructureMargin >= -0.1
            && PlotMargin >= -0.1 && StreamMargin >= -0.1 && SpacingMargin >= -0.1
            && FVector2D::Distance(Position, FVector2D(-1000, 0)) <= 3000.1
            && MaterialsReady && CollisionReady;
        TotalTriangles += Triangles;
        Rows.Add(FString::Printf(TEXT("%s,%.6f,%.6f,%.6f,%.6f,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%d,%d,%d"),
            *Tree->GetPathName(), Root.X, Root.Y, Root.Z, Tree->GetComponentRotation().Yaw, Triangles,
            Mesh ? Mesh->GetStaticMaterials().Num() : 0, ScaleError, GroundError, Radius, HomeMargin,
            ResourceMargin, StructureMargin, PlotMargin, StreamMargin, SpacingMargin, MaterialsReady, CollisionReady,
            Tree->ComponentHasTag(TEXT("AuthoredTreeSmall02"))));
    }
    Valid &= TotalTriangles <= 3708560;
    if (!FFileHelper::SaveStringArrayToFile(Rows, *FPaths::Combine(OutputDirectory, TEXT("grove-inventory.csv"))))
        Valid = false;
    Observations.Add(FString::Printf(TEXT("%sGrove inventory: trees=%d triangles=%lld valid=%d; shared qualified LOD2, not a whole-scene GPU cost."),
        Valid ? TEXT("") : TEXT("FAILED "), Trees.Num(), static_cast<long long>(TotalTriangles), Valid));
}

void AHomesteadVisualPlaytest::RecordGrassGroundInventory()
{
    TArray<UHierarchicalInstancedStaticMeshComponent*> Batches;
    if (PC->Landscape) PC->Landscape->GetComponents(Batches);
    const FString Prefix = TEXT("/Game/Trials/GrassGround_20260921_01");
    const auto MaterialReady = [&](UMaterialInterface* Interface, const TCHAR* Name)
    {
        auto* Material = Interface ? Interface->GetMaterial() : nullptr;
        auto* Resource = Material ? Material->GetMaterialResource(GMaxRHIShaderPlatform) : nullptr;
        return Material && Material->GetPathName() == Prefix + TEXT("/Materials/") + Name + TEXT(".") + Name
            && Resource && Resource->GetGameThreadShaderMap() && Resource->IsGameThreadShaderMapComplete();
    };
    TArray<FString> Rows;
    Rows.Add(TEXT("mesh,instance,x,y,z,yaw,triangles,scale_error,ground_error_cm,home_margin_cm,resource_margin_cm,structure_margin_cm,plot_margin_cm,grass_weight,materials_ready,nonblocking"));
    bool Valid = PC->Landscape != nullptr;
    int32 Clumps = 0, Triangles = 0, AuthoredBatches = 0;
    for (auto* Batch : Batches)
    {
        if (!Batch->ComponentHasTag(TEXT("AuthoredGrassMedium01"))) continue;
        ++AuthoredBatches;
        UStaticMesh* Mesh = Batch->GetStaticMesh();
        const auto* Data = Mesh ? Mesh->GetRenderData() : nullptr;
        const int32 Count = Data && Data->LODResources.Num() == 1 ? Data->LODResources[0].GetNumTriangles() : 0;
        const FBox Bounds = Mesh ? Mesh->GetBoundingBox() : FBox(ForceInit);
        const FVector Anchor(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z);
        const bool Nonblocking = Batch->GetCollisionEnabled() == ECollisionEnabled::NoCollision
            && !Batch->GetGenerateOverlapEvents() && !Batch->CanEverAffectNavigation();
        const bool Ready = Mesh && Mesh->GetStaticMaterials().Num() == 1
            && Batch->IsRenderStateCreated() && MaterialReady(Batch->GetMaterial(0), TEXT("M_GrassMedium01"));
        const TCHAR* Names[] = { TEXT("mid_b"), TEXT("small_b"), TEXT("tall_a"), TEXT("tiny_a") };
        const int32 Expected[] = { 1257, 653, 290, 79 };
        bool Known = false;
        for (int32 Index = 0; Index < 4; ++Index)
            Known |= Mesh && Mesh->GetPathName() == Prefix + TEXT("/Meshes/SM_GrassMedium01_") + Names[Index]
                + TEXT(".SM_GrassMedium01_") + Names[Index] && Count == Expected[Index];
        Valid &= Known && Ready && Nonblocking && Batch->GetNumMaterials() == 1;
        for (int32 Index = 0; Index < Batch->GetInstanceCount(); ++Index)
        {
            FTransform Transform;
            if (!Batch->GetInstanceTransform(Index, Transform, true)) { Valid = false; continue; }
            const FVector Position = Transform.TransformPosition(Anchor);
            const double ScaleError = (Transform.GetScale3D() - FVector::OneVector).Size();
            const double GroundError = Position.Z - AHomesteadWorld::GroundHeight(Position.X, Position.Y);
            const double HomeMargin = FVector2D(Position.X + 1000, Position.Y).Size() - 670;
            double ResourceMargin = 1e9, StructureMargin = 1e9, PlotMargin = 1e9;
            for (const auto& Node : PC->State().resources)
                ResourceMargin = FMath::Min(ResourceMargin,
                    FVector2D::Distance(FVector2D(Position), FVector2D(Node.position.x, Node.position.y)) - 150);
            for (const auto& Structure : PC->State().structures)
            {
                const auto Center = Homestead::CellCenter(Structure.cellX, Structure.cellY);
                StructureMargin = FMath::Min(StructureMargin,
                    FVector2D::Distance(FVector2D(Position), FVector2D(Center.x, Center.y)) - 245);
            }
            for (const auto& Plot : PC->State().plots)
            {
                const auto Center = Homestead::CellCenter(Plot.cellX, Plot.cellY);
                PlotMargin = FMath::Min(PlotMargin,
                    FVector2D::Distance(FVector2D(Position), FVector2D(Center.x, Center.y)) - 195);
            }
            const float Weight = AHomesteadWorld::GrassGroundWeight(Position.X, Position.Y);
            Valid &= ScaleError < 0.001 && FMath::Abs(GroundError) < 0.1 && HomeMargin >= -0.1
                && ResourceMargin >= -0.1 && StructureMargin >= -0.1 && PlotMargin >= -0.1
                && Weight >= 0.399 && HomeMargin <= 1930.1;
            ++Clumps; Triangles += Count;
            Rows.Add(FString::Printf(TEXT("%s,%d,%.6f,%.6f,%.6f,%.6f,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%d,%d"),
                Mesh ? *Mesh->GetPathName() : TEXT("missing"), Index, Position.X, Position.Y, Position.Z,
                Transform.Rotator().Yaw, Count, ScaleError, GroundError, HomeMargin, ResourceMargin,
                StructureMargin, PlotMargin, Weight, Ready, Nonblocking));
        }
    }
    Valid &= AuthoredBatches == 4 && Clumps == 512 && Triangles > 0 && Triangles <= 650000;
    if (!FFileHelper::SaveStringArrayToFile(Rows, *FPaths::Combine(OutputDirectory, TEXT("grass-inventory.csv")))) Valid = false;
    auto* Ground = PC->Landscape ? PC->Landscape->Ground.Get() : nullptr;
    const auto* Section = Ground ? Ground->GetProcMeshSection(0) : nullptr;
    bool TerrainValid = Section && Section->bEnableCollision && Ground->IsQueryCollisionEnabled()
        && Section->ProcVertexBuffer.Num() == 103041 && Section->ProcIndexBuffer.Num() == 614400
        && MaterialReady(Ground->GetMaterial(0), TEXT("M_GrassGroundBlend"));
    double MaxPositionError = 0, MaxNormalError = 0, MaxUvError = 0, MaxWeightError = 0;
    if (TerrainValid)
    {
        for (int32 Index = 0; Index < Section->ProcVertexBuffer.Num(); ++Index)
        {
            const auto& Vertex = Section->ProcVertexBuffer[Index];
            const float X = -4000 + (Index % 321) * 25, Y = -4000 + (Index / 321) * 25;
            const FVector Expected(X, Y, AHomesteadWorld::GroundHeight(X, Y));
            const float DX = (AHomesteadWorld::GroundHeight(X + 1, Y) - AHomesteadWorld::GroundHeight(X - 1, Y)) * 0.5f;
            const float DY = (AHomesteadWorld::GroundHeight(X, Y + 1) - AHomesteadWorld::GroundHeight(X, Y - 1)) * 0.5f;
            MaxPositionError = FMath::Max(MaxPositionError, (FVector(Vertex.Position) - Expected).Size());
            MaxNormalError = FMath::Max(MaxNormalError, (FVector(Vertex.Normal) - FVector(-DX, -DY, 1).GetSafeNormal()).Size());
            MaxUvError = FMath::Max(MaxUvError, (FVector2D(Vertex.UV0) - FVector2D(X / 300.0f, Y / 300.0f)).Size());
            MaxWeightError = FMath::Max(MaxWeightError, FMath::Abs(Vertex.Color.R / 255.0 - AHomesteadWorld::GrassGroundWeight(X, Y)));
        }
        int32 Offset = 0;
        for (int32 Y = 0; Y < 320; ++Y)
            for (int32 X = 0; X < 320; ++X)
            {
                const uint32 A = Y * 321 + X;
                for (uint32 Index : { A, A + 321, A + 1, A + 1, A + 321, A + 322 })
                    TerrainValid &= Section->ProcIndexBuffer[Offset++] == Index;
            }
        TerrainValid &= MaxPositionError < 0.001 && MaxNormalError < 0.001 && MaxUvError < 0.001 && MaxWeightError <= 1.0 / 255;
    }
    TArray<FString> GroundRows;
    GroundRows.Add(TEXT("terrain_valid,vertices,triangles,max_position_error_cm,max_normal_error,max_uv_error,max_weight_error"));
    GroundRows.Add(FString::Printf(TEXT("%d,%d,%d,%.9f,%.9f,%.9f,%.9f"), TerrainValid,
        Section ? Section->ProcVertexBuffer.Num() : 0, Section ? Section->ProcIndexBuffer.Num() / 3 : 0,
        MaxPositionError, MaxNormalError, MaxUvError, MaxWeightError));
    if (!FFileHelper::SaveStringArrayToFile(GroundRows, *FPaths::Combine(OutputDirectory, TEXT("grass-ground-inventory.csv")))) TerrainValid = false;
    Observations.Add(FString::Printf(TEXT("%sGrass/ground inventory: clumps=%d triangles=%d batches=%d placement_valid=%d terrain_valid=%d; current actor/mesh/material observations, not GPU timing."),
        Valid && TerrainValid ? TEXT("") : TEXT("FAILED "), Clumps, Triangles, AuthoredBatches, Valid, TerrainValid));
}

void AHomesteadVisualPlaytest::RecordCameraForeground()
{
    if (!PC->Landscape) return;
    FVector Camera;
    FRotator Rotation;
    PC->GetPlayerViewPoint(Camera, Rotation);
    int32 Width = 0, Height = 0;
    PC->GetViewportSize(Width, Height);
    auto Evidence = MakeShared<FJsonObject>();
    Evidence->SetStringField(TEXT("camera"), Camera.ToString());
    Evidence->SetStringField(TEXT("rotation"), Rotation.ToString());
    Evidence->SetNumberField(TEXT("gameHour"), PC->State().hour);
    Evidence->SetNumberField(TEXT("width"), Width);
    Evidence->SetNumberField(TEXT("height"), Height);
    Evidence->SetStringField(TEXT("limits"), TEXT("Actual spring-arm state and independent matching camera-channel sweep; bounds and complex-collision ray counts are not pixel coverage. Inspect the real frame for surface visibility."));
    const auto* Boom = PC->GetPawn()->FindComponentByClass<USpringArmComponent>();
    if (Boom)
    {
        Evidence->SetBoolField(TEXT("cameraCollisionTest"), Boom->bDoCollisionTest);
        Evidence->SetNumberField(TEXT("probeChannel"), Boom->ProbeChannel.GetValue());
        Evidence->SetNumberField(TEXT("probeRadius"), Boom->ProbeSize);
        Evidence->SetBoolField(TEXT("collisionFixApplied"), Boom->IsCollisionFixApplied());
        Evidence->SetStringField(TEXT("unfixedCamera"), Boom->GetUnfixedCameraPosition().ToString());
        FHitResult Hit;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(CameraForeground), false, PC->GetPawn());
        const bool Blocked = GetWorld()->SweepSingleByChannel(Hit, Boom->PreviousArmOrigin,
            Boom->GetUnfixedCameraPosition(), FQuat::Identity, Boom->ProbeChannel,
            FCollisionShape::MakeSphere(Boom->ProbeSize), Query);
        Evidence->SetBoolField(TEXT("matchingCameraSweepBlocked"), Blocked);
        Evidence->SetStringField(TEXT("sweepComponent"), Hit.GetComponent() ? Hit.GetComponent()->GetPathName() : TEXT(""));
        Evidence->SetStringField(TEXT("sweepLocation"), Hit.Location.ToString());
        Evidence->SetBoolField(TEXT("sweepStartPenetrating"), Hit.bStartPenetrating);
    }
    TArray<TSharedPtr<FJsonValue>> Rows;
    const auto Inspect = [&](const TMap<int32, FHomesteadWorldVisual>& Visuals, const TCHAR* VisualRole)
    {
        for (const auto& Pair : Visuals)
            for (const auto& Component : Pair.Value.Components)
            {
                auto* Part = Cast<UStaticMeshComponent>(Component);
                const UStaticMesh* Mesh = Part ? Part->GetStaticMesh() : nullptr;
                if (!Mesh || !Part->IsVisible() || Part->bHiddenInGame
                    || Part->Bounds.GetBox().ComputeSquaredDistanceToPoint(Camera) > FMath::Square(400.0)) continue;
                const FTransform Transform = Part->GetComponentTransform();
                const FBox Bounds = Mesh->GetBoundingBox();
                int32 Hits = 0, SurfaceHits = 0;
                for (int32 Y = 0; Y < 9; ++Y)
                    for (int32 X = 0; X < 16; ++X)
                    {
                        FVector Origin, Direction;
                        if (!PC->DeprojectScreenPositionToWorld((X + 0.5f) * Width / 16, (Y + 0.5f) * Height / 9, Origin, Direction)) continue;
                        const FVector Start = Transform.InverseTransformPosition(Origin);
                        const FVector End = Transform.InverseTransformPosition(Origin + Direction * 400);
                        Hits += FMath::LineBoxIntersection(Bounds, Start, End, End - Start) ? 1 : 0;
                        FHitResult Surface;
                        FCollisionQueryParams Query(SCENE_QUERY_STAT(ForegroundSurface), true);
                        if (Part->IsQueryCollisionEnabled()
                            && Part->LineTraceComponent(Surface, Origin, Origin + Direction * 400, Query)) ++SurfaceHits;
                    }
                if (!Hits) continue;
                auto Row = MakeShared<FJsonObject>();
                Row->SetNumberField(TEXT("resourceId"), Pair.Key);
                for (const auto& Node : PC->State().resources)
                    if (Node.id == Pair.Key) Row->SetNumberField(TEXT("resourceKind"), static_cast<int32>(Node.kind));
                Row->SetStringField(TEXT("role"), VisualRole);
                Row->SetStringField(TEXT("component"), Part->GetPathName());
                Row->SetStringField(TEXT("mesh"), Mesh->GetPathName());
                Row->SetStringField(TEXT("transform"), Transform.ToString());
                Row->SetStringField(TEXT("meshBoundsMin"), Bounds.Min.ToString());
                Row->SetStringField(TEXT("meshBoundsMax"), Bounds.Max.ToString());
                Row->SetStringField(TEXT("cameraInMeshSpace"), Transform.InverseTransformPosition(Camera).ToString());
                Row->SetNumberField(TEXT("boundsRayHitsOf144"), Hits);
                Row->SetNumberField(TEXT("complexCollisionRayHitsOf144"), SurfaceHits);
                Row->SetBoolField(TEXT("queryEnabled"), Part->IsQueryCollisionEnabled());
                Row->SetNumberField(TEXT("cameraResponse"), Part->GetCollisionResponseToChannel(ECC_Camera));
                Row->SetNumberField(TEXT("pawnResponse"), Part->GetCollisionResponseToChannel(ECC_Pawn));
                FLinearColor Tint;
                const bool HasTint = Part->GetMaterial(0) && Part->GetMaterial(0)->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Tint")), Tint);
                Row->SetStringField(TEXT("tint"), HasTint ? Tint.ToString() : TEXT("unavailable"));
                Rows.Add(MakeShared<FJsonValueObject>(Row));
            }
    };
    Inspect(PC->Landscape->ResourceVisuals, TEXT("base"));
    Inspect(PC->Landscape->ResourceProduceVisuals, TEXT("produce"));
    Evidence->SetArrayField(TEXT("nearbyVisibleResourceBounds"), Rows);
    FString Text;
    if (!FJsonSerializer::Serialize(Evidence, TJsonWriterFactory<>::Create(&Text))
        || !FFileHelper::SaveStringToFile(Text, *FPaths::Combine(OutputDirectory, TEXT("camera-foreground.json"))))
        Observations.Add(TEXT("FAILED camera foreground evidence persistence."));
}

void AHomesteadVisualPlaytest::PrepareTreeEncounter()
{
    TArray<UStaticMeshComponent*> Parts;
    if (PC->Landscape) PC->Landscape->GetComponents(Parts);
    TArray<UStaticMeshComponent*> Trees;
    for (auto* Part : Parts)
        if (Part->ComponentHasTag(TEXT("AuthoredTreeSmall02"))) Trees.Add(Part);
    if (Trees.Num() != 1)
    {
        Observations.Add(FString::Printf(TEXT("FAILED expected one ordinary-world authored tree, observed %d."), Trees.Num()));
        return;
    }
    auto* Tree = Trees[0];
    UStaticMesh* Mesh = Tree->GetStaticMesh();
    auto* Body = Mesh ? Mesh->GetBodySetup() : nullptr;
    bTreeReady = Mesh && Mesh->GetPathName() == TEXT("/Game/Trials/TreeSmall02_20260921_01/Meshes/SM_TreeSmall02_LOD2.SM_TreeSmall02_LOD2")
        && Tree->GetComponentScale().Equals(FVector::OneVector, 0.001)
        && Mesh->GetStaticMaterials().Num() == 3 && Body && Body->AggGeom.SphylElems.Num() == 1
        && Body->AggGeom.GetElementCount() == 1 && Body->CollisionTraceFlag == CTF_UseSimpleAsComplex
        && Tree->GetCollisionEnabled() != ECollisionEnabled::NoCollision
        && Tree->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block;
    const FVector Root = Tree->GetComponentLocation();
    const double GroundError = Root.Z - AHomesteadWorld::GroundHeight(Root.X, Root.Y);
    bTreeReady &= FMath::Abs(GroundError) < 0.1;
    for (int32 Index = 0; Mesh && Index < Mesh->GetStaticMaterials().Num(); ++Index)
    {
        const auto* Interface = Tree->GetMaterial(Index);
        auto* Material = Interface ? Interface->GetMaterial() : nullptr;
        auto* Resource = Material ? Material->GetMaterialResource(GMaxRHIShaderPlatform) : nullptr;
        const bool Ready = Material && Material->GetPathName().StartsWith(TEXT("/Game/Trials/TreeSmall02_20260921_01/Materials/"))
            && Resource && Resource->GetGameThreadShaderMap() && Resource->IsGameThreadShaderMapComplete();
        bTreeReady &= Ready;
        PresentationSettings.Add(FString::Printf(TEXT("tree_material[%d]=%s imported_slot=%s shader_map_complete=%d"),
            Index, *GetPathNameSafe(Material), *Mesh->GetStaticMaterials()[Index].ImportedMaterialSlotName.ToString(), Ready));
    }
    PresentationSettings.Add(FString::Printf(TEXT("tree_mesh=%s root=%s scale=%s ground_error_cm=%.6f ready=%d"),
        *GetPathNameSafe(Mesh), *Root.ToString(), *Tree->GetComponentScale().ToString(), GroundError, bTreeReady));
    if (!bTreeReady)
    {
        Observations.Add(TEXT("FAILED tree material/geometry/grounding/collision readiness."));
        return;
    }
    ObservedTree = Tree;
    TreeCenter = FVector2D(Tree->GetComponentTransform().TransformPosition(Body->AggGeom.SphylElems[0].Center));
    TreeContactSamples.Add(TEXT("seconds,x,y,distance_cm,input_x,input_y,inward_intent,inward_velocity_cm_s,radial_progress_cm_s,total_speed_cm_s,hit_component,geometry_blocking_flag,start_penetrating,hit_distance_cm,blocked_seconds,geometry_hit,tree_query,pawn_query,tree_blocks_pawn,pawn_blocks_tree,tree_object_type,pawn_object_type,tree_actor_collision,pawn_actor_collision"));
    Observations.Add(FString::Printf(TEXT("Ordinary tree encounter: root=%s trunk_center_xy=%s radius_cm=%.6f capsule_cylinder_cm=%.6f"),
        *Root.ToString(), *TreeCenter.ToString(), Body->AggGeom.SphylElems[0].Radius, Body->AggGeom.SphylElems[0].Length));
    Passes.Append({
        {TEXT("walk-to-authored-tree"), 18},
        {TEXT("view-authored-tree"), 3},
        {TEXT("walk-into-authored-trunk"), 9},
        {TEXT("retreat-from-authored-trunk"), 3}
    });
}

void AHomesteadVisualPlaytest::TickTreeEncounter(const FPass& Pass, float Delta, FVector2D& Move, FVector2D& Look)
{
    const bool Approach = Pass.Label == TEXT("walk-to-authored-tree");
    const bool View = Pass.Label == TEXT("view-authored-tree");
    const bool Contact = Pass.Label == TEXT("walk-into-authored-trunk");
    const bool Retreat = Pass.Label == TEXT("retreat-from-authored-trunk");
    if (!bTreeReady || !(Approach || View || Contact || Retreat)) return;
    const auto Point = PC->PlayerPoint();
    const FVector2D Position(Point.x, Point.y);
    const FVector2D Offset = (Approach ? TreeStaging : TreeCenter) - Position;
    const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Offset.Y, Offset.X));
    const float Difference = FMath::FindDeltaAngleDegrees(static_cast<float>(PC->GetControlRotation().Yaw), Yaw);
    Look.X = FMath::Clamp(Difference / 45.0f, -0.7f, 0.7f);
    Move.Y = View ? 0 : Retreat ? -0.65f : FMath::Abs(Difference) < 25 ? 0.7f : 0;
    if (View && ObservedTree.IsValid())
    {
        FVector CameraPosition;
        FRotator CameraRotation;
        PC->GetPlayerViewPoint(CameraPosition, CameraRotation);
        const FVector ToCenter = ObservedTree->Bounds.Origin - CameraPosition;
        const float DesiredPitch = FMath::RadiansToDegrees(FMath::Atan2(ToCenter.Z, ToCenter.Size2D()));
        const float PitchError = FMath::FindDeltaAngleDegrees(CameraRotation.Pitch, DesiredPitch);
        Look.Y = -FMath::Clamp(PitchError / 30.0f, -0.65f, 0.65f) * (PC->bInvertY ? -1.0f : 1.0f);
        if (PassElapsed + Delta >= Pass.Duration)
        {
            RecordCameraForeground();
            int32 Width = 0, Height = 0;
            PC->GetViewportSize(Width, Height);
            const FBox Bounds = ObservedTree->Bounds.GetBox();
            FVector2D Minimum(DBL_MAX, DBL_MAX), Maximum(-DBL_MAX, -DBL_MAX);
            bTreeFramed = Width > 0 && Height > 0;
            for (int32 Corner = 0; Corner < 8; ++Corner)
            {
                const FVector CornerPoint(Corner & 1 ? Bounds.Max.X : Bounds.Min.X,
                    Corner & 2 ? Bounds.Max.Y : Bounds.Min.Y, Corner & 4 ? Bounds.Max.Z : Bounds.Min.Z);
                FVector2D Screen;
                const bool Projected = PC->ProjectWorldLocationToScreen(CornerPoint, Screen);
                bTreeFramed &= Projected && Screen.X >= 0 && Screen.X <= Width && Screen.Y >= 0 && Screen.Y <= Height;
                if (Projected)
                {
                    Minimum.X = FMath::Min(Minimum.X, Screen.X);
                    Minimum.Y = FMath::Min(Minimum.Y, Screen.Y);
                    Maximum.X = FMath::Max(Maximum.X, Screen.X);
                    Maximum.Y = FMath::Max(Maximum.Y, Screen.Y);
                }
            }
            Observations.Add(FString::Printf(TEXT("Tree bounds inside viewport=%d; screen_min=%s; screen_max=%s; viewport=%d,%d; camera_pitch=%.6f"),
                bTreeFramed, *Minimum.ToString(), *Maximum.ToString(), Width, Height, CameraRotation.Pitch));
        }
    }
    if (Approach && Offset.Size() < 45)
    {
        bReachedTree = true;
        Move = Look = FVector2D::ZeroVector;
        PassElapsed = Pass.Duration;
    }
    auto* Avatar = Cast<AHomesteadCharacter>(PC->GetPawn());
    if (Contact && Avatar && ObservedTree.IsValid())
    {
        const FVector Start = Avatar->GetActorLocation();
        const FVector Direction(Offset.GetSafeNormal(), 0);
        const FVector Input = Avatar->GetLastMovementInputVector();
        const double InwardIntent = FVector::DotProduct(Input, Direction);
        const double InwardVelocity = FVector::DotProduct(Avatar->GetVelocity(), Direction);
        const double Distance = Offset.Size();
        const double RadialProgress = bHaveTreeDistance && Delta > 0 ? (PreviousTreeDistance - Distance) / Delta : 0;
        FHitResult Hit;
        const auto* PawnCapsule = Avatar->GetCapsuleComponent();
        const bool GeometryHit = ObservedTree->SweepComponent(Hit, Start, Start + Direction * 15,
            PawnCapsule->GetComponentQuat(), PawnCapsule->GetCollisionShape());
        const bool TreeQuery = ObservedTree->IsQueryCollisionEnabled(), PawnQuery = PawnCapsule->IsQueryCollisionEnabled();
        const bool TreeBlocksPawn = ObservedTree->GetCollisionResponseToChannel(PawnCapsule->GetCollisionObjectType()) == ECR_Block;
        const bool PawnBlocksTree = PawnCapsule->GetCollisionResponseToChannel(ObservedTree->GetCollisionObjectType()) == ECR_Block;
        const bool TreeActorCollision = ObservedTree->GetOwner() && ObservedTree->GetOwner()->GetActorEnableCollision();
        const bool PawnActorCollision = Avatar->GetActorEnableCollision();
        // Component sweeps use overlap-all filters; check the actual response pair separately.
        const bool HitTree = GeometryHit && !Hit.bStartPenetrating && Hit.GetComponent() == ObservedTree.Get()
            && TreeQuery && PawnQuery && TreeBlocksPawn && PawnBlocksTree && TreeActorCollision && PawnActorCollision;
        const bool RadiallyBlocked = bHaveTreeDistance && HitTree && Move.Y > 0.25 && InwardIntent > 0.25
            && FMath::Abs(InwardVelocity) < 5 && FMath::Abs(RadialProgress) < 5;
        TreeBlockedSeconds = RadiallyBlocked ? TreeBlockedSeconds + Delta : 0;
        TreeContactSamples.Add(FString::Printf(TEXT("%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%s,%d,%d,%.6f,%.6f,%d,%d,%d,%d,%d,%d,%d,%d,%d"),
            Elapsed, Position.X, Position.Y, Distance, Input.X, Input.Y, InwardIntent, InwardVelocity, RadialProgress,
            Avatar->GetVelocity().Size2D(), *GetPathNameSafe(Hit.GetComponent()), Hit.bBlockingHit,
            Hit.bStartPenetrating, Hit.Distance, TreeBlockedSeconds, GeometryHit, TreeQuery, PawnQuery,
            TreeBlocksPawn, PawnBlocksTree, static_cast<int32>(ObservedTree->GetCollisionObjectType()),
            static_cast<int32>(PawnCapsule->GetCollisionObjectType()), TreeActorCollision, PawnActorCollision));
        PreviousTreeDistance = Distance;
        bHaveTreeDistance = true;
        if (TreeBlockedSeconds >= 0.75f)
        {
            bTreeBlocked = true;
            Observations.Add(FString::Printf(TEXT("Actual inward walking blocked at authored trunk: player=%s distance_cm=%.6f inward_intent=%.6f inward_velocity_cm_s=%.6f radial_progress_cm_s=%.6f total_speed_cm_s=%.6f sweep_component=%s"),
                *Position.ToString(), Distance, InwardIntent, InwardVelocity, RadialProgress,
                Avatar->GetVelocity().Size2D(), *GetPathNameSafe(Hit.GetComponent())));
            PassElapsed = Pass.Duration;
        }
    }
    if (Retreat)
    {
        TreeRetreatDistance = FVector2D::Distance(Position, TreeRetreatStart);
        bTreeRetreated |= TreeRetreatDistance > 100;
    }
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
        if (bEndurance) FinishEndurance(TEXT("cancelled"), TEXT("Shipping QA cancelled by its owned supervisor."));
        else Finish();
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
        if (Pass.Label == TEXT("walk-to-authored-tree"))
        {
            const auto Point = PC->PlayerPoint();
            TreeStaging = TreeCenter + (FVector2D(Point.x, Point.y) - TreeCenter).GetSafeNormal() * 750;
        }
        if (Pass.Label == TEXT("retreat-from-authored-trunk"))
        {
            const auto Point = PC->PlayerPoint();
            TreeRetreatStart = FVector2D(Point.x, Point.y);
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
    TickTreeEncounter(Pass, WallDelta, Move, Look);
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
        Observations.Add(FString::Printf(TEXT("Tree ready=%d; ordinary approach=%d; actual trunk blocked walking=%d; ordinary retreat=%d"),
            bTreeReady, bReachedTree, bTreeBlocked, bTreeRetreated));
        Observations.Add(FString::Printf(TEXT("Tree retreat displacement_cm=%.6f; retreat_start=%s; completed_passes=%d; planned_passes=%d"),
            TreeRetreatDistance, *TreeRetreatStart.ToString(), PassIndex, Passes.Num()));
        if (bTreeRoute && !bTreeFramed)
            Observations.Add(TEXT("FAILED full tree bounds were not observed inside the viewport."));
    }
    Observations.Add(TEXT("This observational capture is not a visual-quality pass or a replacement for human feel/listening review."));
    bool Saved = FFileHelper::SaveStringToFile(FString::Join(Telemetry, TEXT("\n")) + TEXT("\n"),
        *FPaths::Combine(OutputDirectory, TEXT("telemetry.csv")));
    Saved = FFileHelper::SaveStringToFile(FString::Join(Observations, TEXT("\n")) + TEXT("\n"),
        *FPaths::Combine(OutputDirectory, TEXT("observations.txt"))) && Saved;
    if (bTreeRoute)
        Saved = FFileHelper::SaveStringToFile(FString::Join(TreeContactSamples, TEXT("\n")) + TEXT("\n"),
            *FPaths::Combine(OutputDirectory, TEXT("tree-contact.csv"))) && Saved;
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
        : bReachedForage && Gathered && bObservedGather && bGatherRecovered
            && (!bTreeRoute || (bTreeReady && bReachedTree && bTreeFramed && bTreeBlocked && bTreeRetreated));
    const bool Cancelled = FParse::Param(FCommandLine::Get(), TEXT("HomesteadShippingQA"))
        && IFileManager::Get().FileExists(*FPaths::Combine(OutputDirectory, TEXT("stop-qa.txt")));
    FPlatformMisc::RequestExitWithStatus(false, Saved && Complete && !Cancelled ? 0 : 1);
}
