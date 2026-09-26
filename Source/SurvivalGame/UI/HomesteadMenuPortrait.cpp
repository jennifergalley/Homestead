#include "HomesteadMenuPortrait.h"
#include "../HomesteadCharacter.h"
#include "../HomesteadController.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

namespace
{
constexpr int32 PortraitWidth = 512;
constexpr int32 PortraitHeight = 1024;

void PortraitShowFlags(USceneCaptureComponent2D& Capture)
{
    Capture.PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Capture.bCaptureEveryFrame = false;
    Capture.bCaptureOnMovement = false;
    Capture.bAlwaysPersistRenderingState = true;
    Capture.FOVAngle = 20;
    Capture.ShowFlags.SetAtmosphere(false);
    Capture.ShowFlags.SetFog(false);
    Capture.ShowFlags.SetMotionBlur(false);
    Capture.ShowFlags.SetDepthOfField(false);
    Capture.ShowFlags.SetSkyLighting(false);
    Capture.ShowFlags.SetGlobalIllumination(false);
    Capture.ShowFlags.SetReflectionEnvironment(false);
    Capture.ShowFlags.SetLocalExposure(false);
    Capture.ShowFlags.SetEyeAdaptation(true);
    Capture.PostProcessBlendWeight = 1;
    auto& Exposure = Capture.PostProcessSettings;
    Exposure.bOverride_AutoExposureMethod = true;
    Exposure.AutoExposureMethod = AEM_Manual;
    Exposure.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Exposure.AutoExposureApplyPhysicalCameraExposure = true;
    Exposure.bOverride_AutoExposureBias = true;
    Exposure.AutoExposureBias = 0;
    Exposure.bOverride_CameraISO = true;
    Exposure.CameraISO = 400;
    Exposure.bOverride_CameraShutterSpeed = true;
    Exposure.CameraShutterSpeed = 15;
    Exposure.bOverride_BloomIntensity = true;
    Exposure.BloomIntensity = 0;
}
}

AHomesteadMenuPortrait::AHomesteadMenuPortrait()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.0f / 24.0f;
    SetActorEnableCollision(false);
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("PortraitRoot"));
    Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortraitCapture"));
    Capture->SetupAttachment(RootComponent);
    PortraitShowFlags(*Capture);
    Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    // The same view again for her silhouette: scene colour's alpha is the inverse of coverage.
    CoverageCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortraitCoverage"));
    CoverageCapture->SetupAttachment(Capture);
    PortraitShowFlags(*CoverageCapture);
    CoverageCapture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
    // Soft studio key and fill on lighting channel 2, which only she joins while the book is open.
    Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortraitLight"));
    Light->SetupAttachment(RootComponent);
    Light->SetRelativeLocation(FVector(180, -120, 110));
    Light->SetIntensityUnits(ELightUnits::Lumens);
    Light->SetIntensity(14000);
    Light->SetAttenuationRadius(700);
    Light->SetCastShadows(false);
    Light->SetLightingChannels(false, false, true);
    FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortraitFillLight"));
    FillLight->SetupAttachment(RootComponent);
    FillLight->SetRelativeLocation(FVector(110, 155, -25));
    FillLight->SetIntensityUnits(ELightUnits::Lumens);
    FillLight->SetIntensity(8000);
    FillLight->SetAttenuationRadius(650);
    FillLight->SetCastShadows(false);
    FillLight->SetLightingChannels(false, false, true);
}

bool AHomesteadMenuPortrait::Refresh(AHomesteadCharacter& Character)
{
    if (!Character.GetMesh() || !Character.GetMesh()->GetSkeletalMeshAsset()) return false;
    if (!ColorTarget)
    {
        ColorTarget = NewObject<UTextureRenderTarget2D>(this);
        ColorTarget->ClearColor = FLinearColor::Transparent;
        ColorTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
        ColorTarget->InitAutoFormat(PortraitWidth, PortraitHeight);
        Capture->TextureTarget = ColorTarget;
        CoverageTarget = NewObject<UTextureRenderTarget2D>(this);
        CoverageTarget->ClearColor = FLinearColor(0, 0, 0, 1);
        CoverageTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
        CoverageTarget->InitAutoFormat(PortraitWidth, PortraitHeight);
        CoverageCapture->TextureTarget = CoverageTarget;
    }
    if (!CompositeMaterial)
        CompositeMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/SurvivalGame/UI/M_PortraitCutout.M_PortraitCutout"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (CompositeMaterial && !Composite)
    {
        Composite = UMaterialInstanceDynamic::Create(CompositeMaterial, this);
        Composite->SetTextureParameterValue(TEXT("Color"), ColorTarget);
        Composite->SetTextureParameterValue(TEXT("Coverage"), CoverageTarget);
    }
    if (Subject.Get() != &Character)
    {
        RestoreLighting();
        Subject = &Character;
        AnimatedSeconds = 0;
        if (const auto* Idle = Character.GetIdleAnimation())
            StartPhase = FMath::Fmod(static_cast<float>(GetWorld()->GetTimeSeconds()), Idle->GetPlayLength());
    }
    // Everything she is showing right now: body, face, outfit, hair, pouch, belt and held tool.
    Capture->ClearShowOnlyComponents();
    CoverageCapture->ClearShowOnlyComponents();
    TArray<UPrimitiveComponent*> Parts;
    Character.GetComponents(Parts);
    const FTransform Frame(FRotator(0, Character.GetActorRotation().Yaw, 0), Character.GetActorLocation());
    FBox Bounds(ForceInit);
    for (UPrimitiveComponent* Part : Parts)
    {
        if (!Part || Part->IsA<UCapsuleComponent>()) continue;
        // Hidden parts stay listed: a tool she takes up or a garment she puts on while the book
        // is open shows up without rebuilding the list.
        Capture->ShowOnlyComponent(Part);
        CoverageCapture->ShowOnlyComponent(Part);
        if (!LitParts.ContainsByPredicate([Part](const auto& Lit) { return Lit.Key.Get() == Part; }))
        {
            LitParts.Add({Part, Part->LightingChannels});
            Part->SetLightingChannels(false, false, true);
        }
        if (Part->IsVisible() && !Part->bHiddenInGame
            && (Part->IsA<USkeletalMeshComponent>() || Part->IsA<UStaticMeshComponent>()))
            Bounds += Part->Bounds.GetBox().TransformBy(Frame.Inverse());
    }
    const float HalfHeight = Character.GetCapsuleComponent() ? Character.GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.0f;
    if (!Bounds.IsValid || Bounds.GetExtent().ContainsNaN())
        Bounds = FBox(FVector(-40, -40, -HalfHeight), FVector(40, 40, HalfHeight));
    // Skeletal bounds are generous; keep the frame on her rather than on padding.
    Bounds.Min.Z = FMath::Max(Bounds.Min.Z, -HalfHeight - 4.0);
    Bounds.Max.Z = FMath::Min(Bounds.Max.Z, HalfHeight + 20.0);
    SubjectCenter = Bounds.GetCenter();
    SubjectExtent = Bounds.GetExtent();
    SubjectExtent.X = FMath::Min(SubjectExtent.X, 60.0);
    SubjectExtent.Y = FMath::Min(SubjectExtent.Y, 70.0);
    FollowSubject();
    UpdateCaptureFraming();
    bCapturePending = true;
    return true;
}

UObject* AHomesteadMenuPortrait::BrushResource() const
{
    return Composite ? static_cast<UObject*>(Composite) : static_cast<UObject*>(ColorTarget);
}

void AHomesteadMenuPortrait::Orbit(float Degrees)
{
    Yaw = FMath::Fmod(Yaw + Degrees, 360.0f);
    FollowSubject();
    bCapturePending = true;
}

void AHomesteadMenuPortrait::ToggleCloseup()
{
    bCloseup = !bCloseup;
    UpdateCaptureFraming();
    bCapturePending = true;
}

TOptional<float> AHomesteadMenuPortrait::IdlePhase() const
{
    const auto* Character = Subject.Get();
    const auto* Idle = Character ? Character->GetIdleAnimation() : nullptr;
    if (!Idle || Idle->GetPlayLength() <= 0) return {};
    return FMath::Fmod(StartPhase + AnimatedSeconds, Idle->GetPlayLength());
}

void AHomesteadMenuPortrait::FollowSubject()
{
    if (const auto* Character = Subject.Get())
        SetActorLocationAndRotation(Character->GetActorLocation(),
            FRotator(0, Character->GetActorRotation().Yaw + Yaw, 0));
}

void AHomesteadMenuPortrait::UpdateCaptureFraming()
{
    if (!ColorTarget || SubjectExtent.Z <= 0) return;
    const float HalfFov = FMath::DegreesToRadians(Capture->FOVAngle * 0.5f);
    const double Aspect = static_cast<double>(ColorTarget->SizeX) / ColorTarget->SizeY;
    const double VerticalTangent = FMath::Tan(HalfFov) / Aspect;
    FVector Center = SubjectCenter;
    double Distance = 0;
    if (bCloseup)
    {
        Center.Z += SubjectExtent.Z * 0.68;
        Distance = SubjectExtent.Z * 0.38 * 1.12 / VerticalTangent + SubjectExtent.X;
    }
    else
    {
        // Fit her height; a tool held out to the side may run past the frame edge.
        Distance = FMath::Max(FMath::Min(SubjectExtent.Y, 45.0) / FMath::Tan(HalfFov),
            SubjectExtent.Z / VerticalTangent) * 1.05 + FMath::Min(SubjectExtent.X, 45.0);
    }
    Center.Y = 0;
    Capture->SetRelativeLocation(Center + FVector(Distance, 0, 0));
    Capture->SetRelativeRotation(FRotator(0, 180, 0));
}

void AHomesteadMenuPortrait::RestoreLighting()
{
    for (const auto& Lit : LitParts)
        if (UPrimitiveComponent* Part = Lit.Key.Get())
            Part->SetLightingChannels(Lit.Value.bChannel0, Lit.Value.bChannel1, Lit.Value.bChannel2);
    LitParts.Reset();
}

void AHomesteadMenuPortrait::EndPlay(const EEndPlayReason::Type Reason)
{
    RestoreLighting();
    Super::EndPlay(Reason);
}

void AHomesteadMenuPortrait::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const auto* PC = Subject.IsValid()
        ? Cast<AHomesteadController>(Subject->GetController()) : nullptr;
    const bool bVisible = PC && PC->IsBookOpen()
        && (PC->BookPage() == 0 || PC->BookPage() == 6);
    if (bVisible) AnimatedSeconds += DeltaSeconds;
    if ((bCapturePending || bVisible) && ColorTarget)
    {
        FollowSubject();
        Capture->CaptureScene();
        CoverageCapture->CaptureScene();
        bCapturePending = false;
    }
}
