#include "HomesteadLab.h"

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
#include "HomesteadCharacter.h"
#include "Kismet/GameplayStatics.h"
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
    // World-aligned engine grid: its lines stay fixed in the world, so foot sliding and stride
    // length can be read directly against it.
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GridAsset(
        TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
    Cube = CubeAsset.Object;
    Grid = GridAsset.Object;

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
    UE_LOG(LogTemp, Display, TEXT("CHARACTER_LAB ready: flat grid floor, course at x=%.0f; console: LabAction, LabSun, LabTeleport, LabCourse, slomo."),
        AHomesteadLabWorld::CourseX);
}

void AHomesteadLabController::LabAction(const FString& Name)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar) return;
    const FVector Ahead = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 100.0f;
    const Homestead::Point Target{Ahead.X, Ahead.Y};
    if (Name.Equals(TEXT("Gather"), ESearchCase::IgnoreCase)) Avatar->PlayGather();
    else if (Name.Equals(TEXT("Water"), ESearchCase::IgnoreCase)) Avatar->PlayWater(Target);
    else if (Name.Equals(TEXT("Chop"), ESearchCase::IgnoreCase)) Avatar->PlayClear(Target);
    else if (Name.Equals(TEXT("Knife"), ESearchCase::IgnoreCase)) Avatar->PlayKnifeCut(Target);
    else if (Name.Equals(TEXT("Till"), ESearchCase::IgnoreCase)) Avatar->PlayTill(Target);
    else UE_LOG(LogTemp, Warning, TEXT("LabAction takes Gather, Water, Chop, Knife or Till."));
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
    UGameplayStatics::PlaySound2D(this, Pool[Pick].Get(), 0.8f * (bRun ? 0.07f : 0.04f) * FMath::FRandRange(0.85f, 1.15f),
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
        Lines.Add(FString::Printf(TEXT("Speed %.0f cm/s   Sprint %s   Pos %.0f, %.0f, %.0f"),
            Avatar->GetVelocity().Size2D(), Avatar->IsSprinting() ? TEXT("on") : TEXT("off"), P.X, P.Y, P.Z));
    }
    if (Anim)
        Lines.Add(FString::Printf(TEXT("Walk %.2f  Sprint %.2f  Action %.2f   Foot placement %s"),
            Anim->WalkWeight(), Anim->SprintWeight(), Anim->ActionWeight(),
            Feet && Feet->GetInt() ? TEXT("on") : TEXT("off")));
    Lines.Add(FString::Printf(TEXT("Frame %.1f ms   Sun %.1f h"), SmoothedFrameMs, Lab && Lab->LabWorld() ? Lab->LabWorld()->SunHour() : 0.0f));
    Lines.Add(TEXT("Move WASD / left stick   Sprint Shift / L3   Look mouse / right stick   Zoom wheel"));
    Lines.Add(TEXT("Console: LabAction Gather|Water|Chop|Knife|Till   LabSun <hour>   LabCourse   LabTeleport <x> <y>   slomo <rate>"));
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
