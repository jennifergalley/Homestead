#include "HomesteadLab.h"
#include "Simulation/HomesteadAudioLevels.h"

#include "Animation/AnimSequence.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadAnimInspector.h"
#include "HomesteadCharacter.h"
#include "HomesteadLampLook.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "RenderUtils.h"
#include "Simulation/HomesteadSimulation.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
TAutoConsoleVariable<int32> CVarCharacterLab(TEXT("homestead.CharacterLab"), 0,
    TEXT("1 = the next game start (or Play-In-Editor) opens the character lab instead of the woodland."));

// Floor slab: 4 km square, recentred under the player whenever she is 1 km from its centre.
constexpr float FloorSizeCm = 400000.0f;
constexpr float RecentreDistanceCm = 100000.0f;
}

bool HomesteadLab::Requested()
{
    return CVarCharacterLab.GetValueOnGameThread() != 0
        || FParse::Param(FCommandLine::Get(), TEXT("HomesteadCharacterLab"));
}

AHomesteadLabWorld::AHomesteadLabWorld()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("LabRoot"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    // World-aligned grid (homestead_agent.lab_assets): 10 cm / 1 m / 10 m lines fixed in the world,
    // so foot sliding and stride length read directly against it at any floor size.
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GridAsset(TEXT("/Game/Lab/Materials/M_LabGrid.M_LabGrid"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackGrid(
        TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
    Cube = CubeAsset.Object;
    Grid = GridAsset.Succeeded() ? GridAsset.Object : FallbackGrid.Object;

    Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LabFloor"));
    Floor->SetupAttachment(RootComponent);
    Floor->SetStaticMesh(Cube);
    Floor->SetMaterial(0, Grid);
    Floor->SetRelativeScale3D(FVector(FloorSizeCm / 100.0f, FloorSizeCm / 100.0f, 1.0f));
    Floor->SetRelativeLocation(FVector(0, 0, -50));
    Floor->SetMobility(EComponentMobility::Movable);

    Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("LabSun"));
    Sun->SetupAttachment(RootComponent);
    Sun->SetMobility(EComponentMobility::Movable);
    Sun->bAtmosphereSunLight = true;
    Sun->SetIntensity(46000.0f);

    Sky = CreateDefaultSubobject<USkyLightComponent>(TEXT("LabSky"));
    Sky->SetupAttachment(RootComponent);
    Sky->SetMobility(EComponentMobility::Movable);
    Sky->bRealTimeCapture = true;

    auto* Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("LabAtmosphere"));
    Atmosphere->SetupAttachment(RootComponent);
    auto* Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("LabHaze"));
    Fog->SetupAttachment(RootComponent);
    Fog->SetFogDensity(0.002f);
    Fog->SetStartDistance(3000.0f);

    // Exposure matches the woodland's so skin and cloth read the same as in play.
    auto* Exposure = CreateDefaultSubobject<UPostProcessComponent>(TEXT("LabExposure"));
    Exposure->SetupAttachment(RootComponent);
    Exposure->bUnbound = true;
    Exposure->Settings.bOverride_AutoExposureMethod = true;
    Exposure->Settings.AutoExposureMethod = AEM_Histogram;
    Exposure->Settings.bOverride_AutoExposureMinBrightness = true;
    Exposure->Settings.AutoExposureMinBrightness = 0.0f;
    Exposure->Settings.bOverride_AutoExposureMaxBrightness = true;
    Exposure->Settings.AutoExposureMaxBrightness = 16.0f;
    Exposure->Settings.bOverride_AutoExposureBias = true;
    Exposure->Settings.AutoExposureBias = -0.15f;
    Exposure->Settings.bOverride_LocalExposureHighlightContrastScale = true;
    Exposure->Settings.LocalExposureHighlightContrastScale = 0.5f;
    Exposure->Settings.bOverride_LocalExposureShadowContrastScale = true;
    Exposure->Settings.LocalExposureShadowContrastScale = 0.8f;

    // Test course, away from the spawn so the spawn area stays perfectly flat. Lanes along +X:
    // 10, 20 and 30 degree ramps up to a 150 cm plateau and back down, then 10 cm and 20 cm steps.
    const float Width = 400.0f, Thickness = 30.0f, Rise = 150.0f, Plateau = 300.0f;
    const float Angles[] = {10.0f, 20.0f, 30.0f};
    for (int32 Lane = 0; Lane < 3; ++Lane)
    {
        const float Y = -1200.0f + Lane * 600.0f;
        const float A = FMath::DegreesToRadians(Angles[Lane]);
        const float Run = Rise / FMath::Tan(A);
        const float Length = Rise / FMath::Sin(A);
        const FVector UpNormal(-FMath::Sin(A), 0, FMath::Cos(A));
        const FVector DownNormal(FMath::Sin(A), 0, FMath::Cos(A));
        AddBlock(FVector(CourseX + Run * 0.5f, Y, Rise * 0.5f) - UpNormal * Thickness * 0.5f,
            FVector(Length, Width, Thickness), FRotator(Angles[Lane], 0, 0));
        AddBlock(FVector(CourseX + Run + Plateau * 0.5f, Y, Rise * 0.5f), FVector(Plateau, Width, Rise), FRotator::ZeroRotator);
        AddBlock(FVector(CourseX + Run * 1.5f + Plateau, Y, Rise * 0.5f) - DownNormal * Thickness * 0.5f,
            FVector(Length, Width, Thickness), FRotator(-Angles[Lane], 0, 0));
    }
    const float StepHeights[] = {10.0f, 20.0f};
    const int32 StepCounts[] = {6, 4};
    for (int32 Lane = 0; Lane < 2; ++Lane)
    {
        const float Y = 600.0f + Lane * 600.0f, Depth = 45.0f, Top = 200.0f;
        const int32 Count = StepCounts[Lane];
        for (int32 Step = 1; Step <= Count; ++Step)
        {
            const float Start = CourseX + (Step - 1) * Depth;
            const float End = CourseX + Count * Depth + Top + (Count - Step + 1) * Depth;
            const float Height = Step * StepHeights[Lane];
            AddBlock(FVector((Start + End) * 0.5f, Y, Height * 0.5f), FVector(End - Start, Width, Height), FRotator::ZeroRotator);
        }
    }
    SetSunHour(Hour);
}

void AHomesteadLabWorld::AddBlock(const FVector& Center, const FVector& SizeCm, const FRotator& Rotation)
{
    auto* Block = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("LabCourse%d"), Course.Num()));
    Block->SetupAttachment(RootComponent);
    Block->SetStaticMesh(Cube);
    Block->SetMaterial(0, Grid);
    Block->SetRelativeLocationAndRotation(Center, Rotation);
    Block->SetRelativeScale3D(SizeCm / 100.0f);
    Course.Add(Block);
}

void AHomesteadLabWorld::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Pawn) return;
    const FVector Here = Pawn->GetActorLocation();
    const FVector Centre = Floor->GetComponentLocation();
    if (FVector::Dist2D(Here, Centre) > RecentreDistanceCm)
    {
        // Snap to whole metres so the world-aligned grid doesn't shift.
        Floor->SetWorldLocation(FVector(FMath::RoundToDouble(Here.X / 100.0) * 100.0,
            FMath::RoundToDouble(Here.Y / 100.0) * 100.0, Centre.Z));
    }
    if (Sun->GetCastRaytracedShadow() != (IsRayTracingEnabled() ? ECastRayTracedShadow::Enabled : ECastRayTracedShadow::Disabled))
        Sun->SetCastRaytracedShadows(IsRayTracingEnabled() ? ECastRayTracedShadow::Enabled : ECastRayTracedShadow::Disabled);
}

void AHomesteadLabWorld::SetSunHour(float NewHour)
{
    // Same sun path as the woodland (AHomesteadWorld::UpdateLighting).
    Hour = FMath::Fmod(FMath::Fmod(NewHour, 24.0f) + 24.0f, 24.0f);
    const float Elevation = FMath::Sin((Hour - 6.0f) / 24.0f * 2.0f * PI);
    Sun->SetRelativeRotation(FRotator(-Elevation * 65.0f, (Hour - 6.0f) * 15.0f - 70.0f, 0));
    Sun->SetIntensity(Elevation > -0.05f ? 46000.0f : 0.0f);
}

float AHomesteadLabWorld::SurfaceHeight(float X, float Y) const
{
    FHitResult Hit;
    const FCollisionQueryParams Params(SCENE_QUERY_STAT(HomesteadLabSurface), false);
    return GetWorld()->LineTraceSingleByChannel(Hit, FVector(X, Y, 5000), FVector(X, Y, -1000), ECC_WorldStatic, Params)
        ? static_cast<float>(Hit.ImpactPoint.Z) : 0.0f;
}

void AHomesteadLabWorld::PlaceProp(EProp Kind, FVector2D At)
{
    for (UStaticMeshComponent* Part : PropBase) if (IsValid(Part)) Part->DestroyComponent();
    for (UStaticMeshComponent* Part : PropProduce) if (IsValid(Part)) Part->DestroyComponent();
    PropBase.Reset();
    PropProduce.Reset();
    Prop = Kind;
    PropAt = At;
    // Same meshes, offsets, yaws and scales as AHomesteadWorld::BuildResource.
    auto Add = [this, At](const TCHAR* Path, FVector2D Offset, float Yaw, float Scale, bool bProduce,
        UMaterialInterface* Material = nullptr)
    {
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, Path);
        if (!Mesh) { UE_LOG(LogTemp, Warning, TEXT("LabProp: missing %s"), Path); return; }
        const FBox Bounds = Mesh->GetBoundingBox();
        const FRotator Rotation(0, Yaw, 0);
        const FVector Anchor(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z);
        const FVector Ground(At.X + Offset.X, At.Y + Offset.Y, SurfaceHeight(At.X + Offset.X, At.Y + Offset.Y));
        auto* Part = NewObject<UStaticMeshComponent>(this);
        Part->SetupAttachment(RootComponent);
        Part->SetMobility(EComponentMobility::Movable);
        Part->SetStaticMesh(Mesh);
        if (Material) Part->SetMaterial(0, Material);
        Part->SetWorldTransform(FTransform(Rotation, Ground - Rotation.RotateVector(Anchor * Scale), FVector(Scale)));
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->RegisterComponent();
        (bProduce ? PropProduce : PropBase).Add(Part);
    };
    const TCHAR* Meshes = TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/");
    if (Kind == EProp::Sticks)
    {
        const TCHAR* Names[] = {TEXT("SM_DryBranchesMedium01_a"), TEXT("SM_DryBranchesMedium01_b"), TEXT("SM_DryBranchesMedium01_c")};
        for (int32 I = 0; I < 3; ++I)
            Add(*FString::Printf(TEXT("%s%s.%s"), Meshes, Names[I], Names[I]), FVector2D(I * 9 - 9, I * 7 - 7), I * 35 + 20,
                AHomesteadCharacter::CarriedStickScale, true);
    }
    else if (Kind == EProp::Stones)
    {
        auto* RockMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SurvivalGame/Materials/M_Rock.M_Rock"));
        for (int32 I = 0; I < 3; ++I)
        {
            UStaticMesh* HandStone = AHomesteadCharacter::LoadHandStone(I);
            const TCHAR* Path = TEXT("/Game/SurvivalGame/Environment/MossRocks.MossRocks");
            const FString StonePath = HandStone ? HandStone->GetPathName() : FString(Path);
            auto* Rock = HandStone ? HandStone : LoadObject<UStaticMesh>(nullptr, Path);
            const float Size = Rock ? Rock->GetBoundingBox().GetSize().GetMax() : 0.0f;
            if (Size > 0)
                Add(*StonePath, FVector2D(I * 17 - 17, I % 2 * 14), I * 79,
                    AHomesteadCharacter::StonePileSize(I, HandStone != nullptr) / Size, true, HandStone ? nullptr : RockMaterial);
        }
    }
    else if (Kind == EProp::Reeds)
    {
        Add(TEXT("/Game/SurvivalGame/Environment/Reeds/SM_ReedStubble.SM_ReedStubble"), FVector2D::ZeroVector, 0, 1.0f, false);
        Add(TEXT("/Game/SurvivalGame/Environment/Reeds/SM_ReedClump.SM_ReedClump"), FVector2D::ZeroVector, 0, 1.0f, true);
    }
    else if (Kind == EProp::Roots)
    {
        const TCHAR* Name = TEXT("SM_Shrub04_a");
        for (int32 I = 0; I < 3; ++I)
            Add(*FString::Printf(TEXT("%s%s.%s"), Meshes, Name, Name), FVector2D(I * 8 - 8, I % 2 * 9), I * 120, 1.0f, false);
        auto* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
        auto* Brown = Base ? UMaterialInstanceDynamic::Create(Base, this) : nullptr;
        if (Brown) Brown->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.65f, 0.43f, 0.19f));
        for (int32 I = 0; I < 2 && Sphere; ++I)
        {
            auto* Root = NewObject<UStaticMeshComponent>(this);
            Root->SetupAttachment(RootComponent);
            Root->SetStaticMesh(Sphere);
            if (Brown) Root->SetMaterial(0, Brown);
            Root->SetWorldLocation(FVector(At.X + I * 11 - 5, At.Y + I * 4, SurfaceHeight(At.X, At.Y) + 4));
            Root->SetWorldScale3D(FVector(0.11f, 0.11f, 0.08f));
            Root->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Root->RegisterComponent();
            PropProduce.Add(Root);
        }
    }
    else if (Kind == EProp::Berries)
    {
        for (int32 I = 0; I < 3; ++I)
        {
            const TCHAR* Name = I == 1 ? TEXT("SM_Shrub04_a") : TEXT("SM_Shrub04_c");
            Add(*FString::Printf(TEXT("%s%s.%s"), Meshes, Name, Name), FVector2D((I - 1) * 10, I % 2 * 10 - 5), I * 113, 1.0f, false);
        }
        auto* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
        auto* Red = Base ? UMaterialInstanceDynamic::Create(Base, this) : nullptr;
        if (Red) Red->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.42f, 0.025f, 0.055f));
        for (int32 I = 0; I < 8 && Sphere; ++I)
        {
            const float Angle = I * 2.399f;
            auto* Berry = NewObject<UStaticMeshComponent>(this);
            Berry->SetupAttachment(RootComponent);
            Berry->SetStaticMesh(Sphere);
            if (Red) Berry->SetMaterial(0, Red);
            Berry->SetWorldLocation(FVector(At.X + FMath::Cos(Angle) * 15, At.Y + FMath::Sin(Angle) * 11,
                SurfaceHeight(At.X, At.Y) + 16 + I % 3 * 4));
            Berry->SetWorldScale3D(FVector(3.8f / 100.0f));
            Berry->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Berry->RegisterComponent();
            PropProduce.Add(Berry);
        }
    }
}

void AHomesteadLabWorld::TakePropPart(int32 Index)
{
    if (PropProduce.IsValidIndex(Index) && IsValid(PropProduce[Index])) PropProduce[Index]->SetVisibility(false);
}

void AHomesteadLabWorld::TakeAllProp()
{
    for (int32 Index = 0; Index < PropProduce.Num(); ++Index) TakePropPart(Index);
}

bool AHomesteadLabWorld::PropIntact() const
{
    for (UStaticMeshComponent* Part : PropProduce) if (!IsValid(Part) || !Part->IsVisible()) return false;
    return !PropProduce.IsEmpty();
}

void AHomesteadLabController::BeginPlay()
{
    Super::BeginPlay();
    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());
    World = GetWorld()->SpawnActor<AHomesteadLabWorld>();
    auto LoadPool = [](TArray<TObjectPtr<USoundBase>>& Pool, const TCHAR* Prefix, int32 Count)
    {
        for (int32 Index = 0; Index < Count; ++Index)
        {
            const FString Name = FString::Printf(TEXT("%s_%02d"), Prefix, Index);
            if (USoundBase* Step = LoadObject<USoundBase>(nullptr,
                    *FString::Printf(TEXT("/Game/SurvivalGame/Audio/Effects/%s.%s"), *Name, *Name)))
                Pool.Add(Step);
        }
    };
    LoadPool(WalkSteps, TEXT("BareStepWalk"), 6);
    LoadPool(RunSteps, TEXT("BareStepRun"), 4);
    LabTeleport(0, 0);
#if !UE_BUILD_SHIPPING
    // The Animation Inspector is a development tool (Scripts\Inspect-Animation.ps1); Shipping never spawns it.
    if (HomesteadAnimInspector::Requested()) GetWorld()->SpawnActor<AHomesteadAnimInspector>();
#endif
    UE_LOG(LogTemp, Display, TEXT("CHARACTER_LAB ready: flat grid floor, course at x=%.0f; console: LabAction, LabProp, LabSun, LabTeleport, LabCourse, slomo."),
        AHomesteadLabWorld::CourseX);
}

void AHomesteadLabController::LabAction(const FString& Name)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar) return;
    const FVector Ahead = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 100.0f;
    const Homestead::Point Target{Ahead.X, Ahead.Y};
    using EProp = AHomesteadLabWorld::EProp;
    if (Name.Equals(TEXT("Gather"), ESearchCase::IgnoreCase))
    {
        // Like the game, forage produce leaves the ground as soon as the harvest commits.
        Avatar->PlayGather();
        if (World && World->PropKind() != EProp::Sticks) World->TakeAllProp();
    }
    else if (Name.Equals(TEXT("Sticks"), ESearchCase::IgnoreCase))
    {
        if (World && (World->PropKind() != EProp::Sticks || !World->PropIntact())) LabProp(TEXT("Sticks"));
        bHoldingStickPile = World && Avatar->PlayGatherSticks(World->PropLocation());
        HeldPartsFirst = 1;
        HeldPartsCount = 1;
    }
    else if (Name.Equals(TEXT("Stones"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("Roots"), ESearchCase::IgnoreCase)
        || Name.Equals(TEXT("Berries"), ESearchCase::IgnoreCase))
    {
        const bool Stones = Name.Equals(TEXT("Stones"), ESearchCase::IgnoreCase);
        const bool Berries = Name.Equals(TEXT("Berries"), ESearchCase::IgnoreCase);
        const EProp Kind = Stones ? EProp::Stones : Berries ? EProp::Berries : EProp::Roots;
        if (World && (World->PropKind() != Kind || !World->PropIntact())) LabProp(Name);
        FVector2D Pile = World ? World->PropLocation() : FVector2D(Target.x, Target.y);
        if (Berries)
        {
            const FVector2D Toward = FVector2D(Avatar->GetActorLocation()) - Pile;
            if (Toward.Size() > 1.0f) Pile += Toward.GetSafeNormal() * 22.0f;
        }
        bHoldingStickPile = World && Avatar->PlayKneelGather(
            Stones ? EHomesteadKneelGather::Stones : EHomesteadKneelGather::Pouch, Pile, Berries);
        HeldPartsFirst = Stones ? 1 : 0;
        HeldPartsCount = Berries ? 4 : 1;
    }
    else if (Name.Equals(TEXT("Reeds"), ESearchCase::IgnoreCase))
    {
        if (World && (World->PropKind() != EProp::Reeds || !World->PropIntact())) LabProp(Name);
        bHoldingStickPile = World && Avatar->PlayKneelGather(EHomesteadKneelGather::Reeds, World->PropLocation());
        HeldPartsFirst = 0;
        HeldPartsCount = 1;
    }
    else if (Name.Equals(TEXT("Eat"), ESearchCase::IgnoreCase)) Avatar->PlayEat(true);
    else if (Name.Equals(TEXT("Pull"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("Pick"), ESearchCase::IgnoreCase))
    {
        // Crop harvests (kneel_harvest / the pouch pick) with a turnip or strawberries in her hand.
        const bool Pick = Name.Equals(TEXT("Pick"), ESearchCase::IgnoreCase);
        const TCHAR* Path = Pick
            ? TEXT("/Game/SurvivalGame/Environment/Props/CropStrawberry/SM_CropStrawberry_Harvest.SM_CropStrawberry_Harvest")
            : TEXT("/Game/SurvivalGame/Environment/Props/CropTurnip/SM_CropTurnip_Harvest.SM_CropTurnip_Harvest");
        const FVector Crown = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 45.0f;
        Avatar->PlayHarvest(Homestead::Point{Crown.X, Crown.Y}, Pick, LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet));
    }
    else if (Name.Equals(TEXT("Craft"), ESearchCase::IgnoreCase)) Avatar->PlayLabCraft(3);
    else if (Name.Equals(TEXT("Water"), ESearchCase::IgnoreCase)) Avatar->PlayWater(Target);
    else if (Name.Equals(TEXT("Fill"), ESearchCase::IgnoreCase)) Avatar->PlayFillPail(Target);
    else if (Name.Equals(TEXT("Chop"), ESearchCase::IgnoreCase)) Avatar->PlayClear(Target);
    else if (Name.Equals(TEXT("Knife"), ESearchCase::IgnoreCase)) Avatar->PlayKnifeCut(Target);
    else if (Name.Equals(TEXT("Till"), ESearchCase::IgnoreCase)) Avatar->PlayTill(Target);
    else if (Name.Equals(TEXT("Machete"), ESearchCase::IgnoreCase)) Avatar->PlayMacheteHack(Target);
    else if (Name.Equals(TEXT("Fell"), ESearchCase::IgnoreCase)) Avatar->PlayFell(Target, 2);
    // The estate tools (each held first, as the hotbar would): the weed pull on both knees, one scythe
    // sweep, two pickaxe or axe strikes at a rock or stump 70 cm ahead, and the billhook's hack.
    else if (Name.Equals(TEXT("Weeds"), ESearchCase::IgnoreCase))
    {
        const FVector Tuft = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 45.0f;
        Avatar->PlayPullWeeds(Homestead::Point{Tuft.X, Tuft.Y});
    }
    else if (Name.Equals(TEXT("Mow"), ESearchCase::IgnoreCase))
    {
        Avatar->SetLabHeldTool(Homestead::Item::Scythe);
        Avatar->PlayStrike(Target, Homestead::Item::Scythe, 1, -1.0f);
    }
    else if (Name.Equals(TEXT("Pickaxe"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("AxeStrike"), ESearchCase::IgnoreCase))
    {
        const auto Tool = Name.Equals(TEXT("Pickaxe"), ESearchCase::IgnoreCase) ? Homestead::Item::Pickaxe : Homestead::Item::Hatchet;
        Avatar->SetLabHeldTool(Tool);
        const FVector Rock = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 70.0f;
        Avatar->PlayStrike(Homestead::Point{Rock.X, Rock.Y}, Tool, 2, 30.0f);
    }
    else if (Name.Equals(TEXT("Billhook"), ESearchCase::IgnoreCase))
    {
        Avatar->SetLabHeldTool(Homestead::Item::Billhook);
        Avatar->PlayMacheteHack(Target, Homestead::Item::Billhook);
    }
    else if (Name.Equals(TEXT("LampDown"), ESearchCase::IgnoreCase))
    {
        // She sets the lamp she's holding (LabHold Lamp) on the floor at arm's length.
        ClearGroundLamp();
        Avatar->SetLabHeldTool(Homestead::Item::OilLamp);
        const FVector Spot = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 45.0f;
        LampSpot = FVector(Spot.X, Spot.Y, Avatar->GetActorLocation().Z - Avatar->GetSimpleCollisionHalfHeight());
        LampKneel = Avatar->PlayLampKneel({Spot.X, Spot.Y}, true) ? 1 : 0;
    }
    else if (Name.Equals(TEXT("LampUp"), ESearchCase::IgnoreCase))
    {
        // Takes up the lamp on the floor ahead (placing one there first if there's none).
        if (GroundLamp.IsEmpty())
        {
            const FVector Spot = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 45.0f;
            LampSpot = FVector(Spot.X, Spot.Y, Avatar->GetActorLocation().Z - Avatar->GetSimpleCollisionHalfHeight());
            PlaceGroundLamp(LampSpot);
        }
        Avatar->SetLabHeldTool(Homestead::Item::OilLamp);
        LampKneel = Avatar->PlayLampKneel({LampSpot.X, LampSpot.Y}, false) ? 2 : 0;
    }
    else UE_LOG(LogTemp, Warning, TEXT("LabAction takes Gather, Sticks, Stones, Roots, Berries, Reeds, Pull, Pick, Eat, Craft, Water, Fill, Chop, Knife, Till, Machete, Fell, Weeds, Mow, Pickaxe, AxeStrike, Billhook, LampDown or LampUp."));
}

void AHomesteadLabController::PlaceGroundLamp(const FVector& At)
{
    ClearGroundLamp();
    if (!World) return;
    auto* Stand = NewObject<USceneComponent>(World, MakeUniqueObjectName(World, USceneComponent::StaticClass(), TEXT("LabLamp")));
    Stand->SetupAttachment(World->GetRootComponent());
    Stand->SetMobility(EComponentMobility::Movable);
    Stand->SetWorldLocation(At);
    Stand->RegisterComponent();
    GroundLamp.Add(Stand);
    const TArray<UStaticMeshComponent*> Parts = HomesteadLampLook::AddParts(World, Stand, FVector::ZeroVector, TEXT("LabLampPart"));
    for (UStaticMeshComponent* Part : Parts) if (Part) GroundLamp.Add(Part);
    UPointLightComponent* Light = HomesteadLampLook::AddLight(World, Stand, FVector::ZeroVector, TEXT("LabLampLight"));
    if (Light) GroundLamp.Add(Light);
    if (!Parts.IsEmpty()) HomesteadLampLook::SetLit(Parts.Last(), Light, true, 0.0f, Parts.Num() >= 3 ? Parts[1] : nullptr);
}

void AHomesteadLabController::ClearGroundLamp()
{
    for (USceneComponent* Part : GroundLamp) if (IsValid(Part)) Part->DestroyComponent();
    GroundLamp.Reset();
}

void AHomesteadLabController::LabProp(const FString& Name)
{
    const APawn* Avatar = GetPawn();
    if (!Avatar || !World) return;
    using EProp = AHomesteadLabWorld::EProp;
    const EProp Kind = Name.Equals(TEXT("Sticks"), ESearchCase::IgnoreCase) ? EProp::Sticks
        : Name.Equals(TEXT("Stones"), ESearchCase::IgnoreCase) ? EProp::Stones
        : Name.Equals(TEXT("Berries"), ESearchCase::IgnoreCase) ? EProp::Berries
        : Name.Equals(TEXT("Roots"), ESearchCase::IgnoreCase) ? EProp::Roots
        : Name.Equals(TEXT("Reeds"), ESearchCase::IgnoreCase) ? EProp::Reeds : EProp::None;
    if (Kind == EProp::None && !Name.Equals(TEXT("None"), ESearchCase::IgnoreCase))
        UE_LOG(LogTemp, Warning, TEXT("LabProp takes Sticks, Stones, Berries, Roots, Reeds or None."));
    const FVector At = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 45.0f;
    World->PlaceProp(Kind, FVector2D(At));
    bHoldingStickPile = false;
}

void AHomesteadLabController::LabHold(const FString& Name)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar) return;
    using Homestead::Item;
    const TPair<const TCHAR*, Item> Tools[] = {{TEXT("Knife"), Item::Knife}, {TEXT("Hatchet"), Item::Hatchet},
        {TEXT("DiggingStick"), Item::DiggingStick}, {TEXT("Pail"), Item::WateringCan}, {TEXT("Machete"), Item::Machete},
        {TEXT("Lamp"), Item::OilLamp}, {TEXT("Scythe"), Item::Scythe}, {TEXT("Billhook"), Item::Billhook},
        {TEXT("Pickaxe"), Item::Pickaxe}, {TEXT("FishingPole"), Item::FishingPole}};
    for (const auto& Tool : Tools)
        if (Name.Equals(Tool.Key, ESearchCase::IgnoreCase)) { Avatar->SetLabHeldTool(Tool.Value); return; }
    Avatar->SetLabHeldTool(Item::Count);
    if (!Name.Equals(TEXT("None"), ESearchCase::IgnoreCase))
        UE_LOG(LogTemp, Warning, TEXT("LabHold takes Knife, Hatchet, DiggingStick, Pail, Machete, Lamp, Scythe, Billhook, Pickaxe, FishingPole or None."));
}

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadLabFish, Log, All);

void AHomesteadLabController::LabFish(const FString& Pose)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    if (!Animation) return;
    using Homestead::Item;
    const TPair<const TCHAR*, Item> Fish[] = {{TEXT("Trout"), Item::RiverTrout}, {TEXT("Salmon"), Item::RiverSalmon},
        {TEXT("Perch"), Item::LakePerch}, {TEXT("Carp"), Item::LakeCarp}, {TEXT("Mackerel"), Item::SeaMackerel},
        {TEXT("Bass"), Item::SeaBass}};
    for (const auto& Kind : Fish)
        if (Pose.Equals(Kind.Key, ESearchCase::IgnoreCase)) { Avatar->SetFishingCatch(Kind.Value); return; }
    const TPair<const TCHAR*, EHomesteadFishingPose> Poses[] = {{TEXT("None"), EHomesteadFishingPose::None},
        {TEXT("Cast"), EHomesteadFishingPose::Cast}, {TEXT("Wait"), EHomesteadFishingPose::Wait},
        {TEXT("Bite"), EHomesteadFishingPose::Bite}, {TEXT("Fight"), EHomesteadFishingPose::Fight},
        {TEXT("Catch"), EHomesteadFishingPose::Catch}, {TEXT("Miss"), EHomesteadFishingPose::Miss}};
    FishStep = 0;
    if (!Pose.Equals(TEXT("None"), ESearchCase::IgnoreCase)) Avatar->SetLabHeldTool(Item::FishingPole);
    FishSplashes = Animation->FishCastSplashes();
    FishLifts = Animation->FishCatchLifts();
    if (Pose.Equals(TEXT("Auto"), ESearchCase::IgnoreCase))
    {
        FishStep = 1;
        FishNextStep = 0;
        return;
    }
    if (Pose.Equals(TEXT("Strike"), ESearchCase::IgnoreCase))
    {
        Animation->SetFishingPose(EHomesteadFishingPose::Fight);
        Animation->PlayFishingStrike();
        return;
    }
    for (const auto& Entry : Poses)
        if (Pose.Equals(Entry.Key, ESearchCase::IgnoreCase)) { Animation->SetFishingPose(Entry.Value); return; }
    UE_LOG(LogHomesteadLabFish, Warning, TEXT("LabFish takes Cast, Wait, Bite, Fight, Strike, Catch, Miss, None, Auto or a fish (Trout, Salmon, Perch, Carp, Mackerel, Bass)."));
}

void AHomesteadLabController::TickLabFish()
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    if (!Animation) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Animation->FishCastSplashes() != FishSplashes)
    {
        FishSplashes = Animation->FishCastSplashes();
        UE_LOG(LogHomesteadLabFish, Log, TEXT("LabFish: cast splash beat %u at clip %.2f s."), FishSplashes, Animation->FishingClipTime());
        if (FishStep == 2) { FishStep = 3; FishNextStep = Now + 2.0; }
    }
    if (Animation->FishCatchLifts() != FishLifts)
    {
        FishLifts = Animation->FishCatchLifts();
        UE_LOG(LogHomesteadLabFish, Log, TEXT("LabFish: catch lift beat %u at clip %.2f s."), FishLifts, Animation->FishingClipTime());
        if (FishStep == 6) { FishStep = 7; FishNextStep = Now + 2.5; }
    }
    if (FishStep == 0 || Now < FishNextStep) return;
    // The beats advance steps 2 and 6, as the simulation's cast and catch do; the rest are timed.
    switch (FishStep)
    {
    case 1: Animation->SetFishingPose(EHomesteadFishingPose::Cast); FishStep = 2; FishNextStep = Now + 4.0; break;
    case 2: UE_LOG(LogHomesteadLabFish, Warning, TEXT("LabFish: no cast splash beat arrived.")); FishStep = 0; break;
    case 3: Animation->SetFishingPose(EHomesteadFishingPose::Bite); FishStep = 4; FishNextStep = Now + 1.8; break;
    case 4: Animation->SetFishingPose(EHomesteadFishingPose::Fight); Animation->PlayFishingStrike(); FishStep = 5; FishNextStep = Now + 2.6; break;
    case 5: Animation->SetFishingPose(EHomesteadFishingPose::Catch); FishStep = 6; FishNextStep = Now + 4.0; break;
    case 6: UE_LOG(LogHomesteadLabFish, Warning, TEXT("LabFish: no catch lift beat arrived.")); FishStep = 0; break;
    default: Animation->SetFishingPose(EHomesteadFishingPose::None); FishStep = 0; break;
    }
}

void AHomesteadLabController::LabLoop(const FString& Name)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar || Name.IsEmpty() || Name.Equals(TEXT("Off"), ESearchCase::IgnoreCase))
    {
        LoopAction.Reset();
        return;
    }
    LoopAction = Name;
    LoopStart = Avatar->GetActorTransform();
    // One clip plus a second's pause between repeats.
    const bool Kneel = Name.Equals(TEXT("Sticks"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("Stones"), ESearchCase::IgnoreCase)
        || Name.Equals(TEXT("Roots"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("Berries"), ESearchCase::IgnoreCase)
        || Name.Equals(TEXT("Reeds"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("Pull"), ESearchCase::IgnoreCase)
        || Name.Equals(TEXT("Pick"), ESearchCase::IgnoreCase);
    const UAnimSequence* Clip = Kneel ? Avatar->GetGatherSticksAnimation() : nullptr;
    LoopPeriod = (Clip ? Clip->GetPlayLength() : 3.5f) + 1.0f;
    LoopNextStart = GetWorld()->GetTimeSeconds();
}

void AHomesteadLabController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    TickLabFish();
    if (LampKneel != 0)
        if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
        {
            if (Avatar->ConsumeLampContact())
            {
                if (LampKneel == 1) { PlaceGroundLamp(LampSpot); Avatar->SetLabHeldTool(Homestead::Item::Count); }
                else { ClearGroundLamp(); Avatar->SetLabHeldTool(Homestead::Item::OilLamp); }
                LampKneel = 0;
            }
            else if (!Avatar->IsLampKneeling()) LampKneel = 0;
        }
    if (!LoopAction.IsEmpty() && GetWorld()->GetTimeSeconds() >= LoopNextStart)
        if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
        {
            Avatar->CancelAction(true);
            // Back to where the loop began (the stick gather steps her onto the pile), camera untouched.
            Avatar->SetActorLocationAndRotation(LoopStart.GetLocation(), LoopStart.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
            if (World && World->PropKind() != AHomesteadLabWorld::EProp::None) World->PlaceProp(World->PropKind(),
                FVector2D(LoopStart.GetLocation() + LoopStart.GetRotation().GetForwardVector() * 45.0f));
            // The animation ignores new requests until the cancel has blended out.
            LoopPlayAt = GetWorld()->GetTimeSeconds() + 0.25;
            LoopNextStart = LoopPlayAt + LoopPeriod;
        }
    if (!LoopAction.IsEmpty() && LoopPlayAt > 0 && GetWorld()->GetTimeSeconds() >= LoopPlayAt)
    {
        LoopPlayAt = 0;
        LabAction(LoopAction);
    }
    if (!bHoldingStickPile || !World) return;
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    // Same pile timing as the woodland (AHomesteadController::Tick).
    if (Avatar && Avatar->SticksLiftedFromPile() >= 1)
        for (int32 Part = HeldPartsFirst; Part < HeldPartsFirst + HeldPartsCount; ++Part) World->TakePropPart(Part);
    if (!Avatar || !Avatar->IsStickPileOnGround())
    {
        World->TakeAllProp();
        bHoldingStickPile = false;
    }
}

void AHomesteadLabController::LabSun(float Hour)
{
    if (World) World->SetSunHour(Hour);
}

void AHomesteadLabController::LabTeleport(float X, float Y)
{
    APawn* Avatar = GetPawn();
    if (!Avatar || !World) return;
    const float HalfHeight = Avatar->GetSimpleCollisionHalfHeight();
    Avatar->TeleportTo(FVector(X, Y, World->SurfaceHeight(X, Y) + HalfHeight + 2.0f), FRotator::ZeroRotator);
    SetControlRotation(FRotator(-12, 0, 0));
}

void AHomesteadLabController::LabCourse()
{
    LabTeleport(AHomesteadLabWorld::CourseX - 400.0f, -1200.0f);
}

void AHomesteadLabController::PlayFootstep(bool bLeftFoot, bool bRun)
{
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const double Now = GetWorld()->GetTimeSeconds();
    if (!Avatar || !Avatar->GetCharacterMovement()->IsMovingOnGround() || Avatar->GetVelocity().Size2D() < 12
        || Now - LastFootstepTime < 0.18)
        return;
    const auto& Pool = bRun ? RunSteps : WalkSteps;
    if (Pool.IsEmpty()) return;
    LastFootstepTime = Now;
    int32 Pick = FMath::RandRange(0, Pool.Num() - 1);
    if (Pool.Num() > 1 && Pick == LastStep) Pick = (Pick + 1) % Pool.Num();
    LastStep = Pick;
    // Same levels as the game at its default 80% effects volume.
    UGameplayStatics::PlaySound2D(this, Pool[Pick].Get(), 0.8f * (bRun ? Homestead::AudioLevels::Gain::FootstepRun : Homestead::AudioLevels::Gain::FootstepWalk) * FMath::FRandRange(0.85f, 1.15f),
        FMath::FRandRange(0.96f, 1.04f));
}

void AHomesteadLabHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas) return;
    const float Scale = FMath::Clamp(Canvas->ClipY / 1080.0f, 0.5f, 3.0f);
    SmoothedFrameMs = FMath::Lerp(SmoothedFrameMs, FApp::GetDeltaTime() * 1000.0f, 0.05f);
    const auto* Avatar = Cast<AHomesteadCharacter>(GetOwningPawn());
    const auto* Anim = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    const auto* Lab = Cast<AHomesteadLabController>(PlayerOwner);
    const IConsoleVariable* Feet = IConsoleManager::Get().FindConsoleVariable(TEXT("homestead.FootPlacement"));
    TArray<FString> Lines;
    Lines.Add(TEXT("CHARACTER LAB"));
    if (Avatar)
    {
        const FVector P = Avatar->GetActorLocation();
        Lines.Add(FString::Printf(TEXT("Speed %.0f cm/s   Sprint %s (%s)   Pos %.0f, %.0f, %.0f"),
            Avatar->GetVelocity().Size2D(), Avatar->IsSprintOn() ? TEXT("on") : TEXT("off"),
            Avatar->IsSprinting() ? TEXT("running") : TEXT("not running"), P.X, P.Y, P.Z));
    }
    if (Anim)
        Lines.Add(FString::Printf(TEXT("Walk %.2f  Sprint %.2f  Action %.2f   Foot placement %s"),
            Anim->WalkWeight(), Anim->SprintWeight(), Anim->ActionWeight(),
            Feet && Feet->GetInt() ? TEXT("on") : TEXT("off")));
    Lines.Add(FString::Printf(TEXT("Frame %.1f ms   Sun %.1f h"), SmoothedFrameMs, Lab && Lab->LabWorld() ? Lab->LabWorld()->SunHour() : 0.0f));
    Lines.Add(TEXT("Move WASD / left stick   Sprint toggle Shift / L3   Look mouse / right stick   Zoom wheel"));
    Lines.Add(TEXT("Console: LabAction Gather|Sticks|Stones|Roots|Berries|Reeds|Eat|Craft|Water|Fill|Chop|Knife|Till|Machete|Fell   LabHold <tool>|None   LabLoop <action>|Off   LabProp Sticks|Stones|Roots|Berries|Reeds|None   LabSun <hour>   LabCourse   LabTeleport <x> <y>   slomo <rate>"));
    float Y = 24.0f * Scale;
    for (const FString& Line : Lines)
    {
        Canvas->SetDrawColor(FColor(0, 0, 0, 160));
        Canvas->DrawText(GEngine->GetMediumFont(), Line, 26.0f * Scale, Y + 2.0f * Scale, Scale, Scale);
        Canvas->SetDrawColor(FColor(240, 236, 220));
        Canvas->DrawText(GEngine->GetMediumFont(), Line, 24.0f * Scale, Y, Scale, Scale);
        Y += 26.0f * Scale;
    }
}
