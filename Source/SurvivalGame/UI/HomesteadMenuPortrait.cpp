#include "HomesteadMenuPortrait.h"
#include "../HomesteadCharacter.h"
#include "../HomesteadController.h"
#include "../HomesteadWardrobePresentation.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SceneComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

AHomesteadMenuPortrait::AHomesteadMenuPortrait()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.0f / 24.0f;
    SetActorEnableCollision(false);
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("PortraitRoot"));
    Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PortraitBody"));
    Body->SetupAttachment(RootComponent);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetGenerateOverlapEvents(false);
    Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Body->SetCastShadow(false);
    Body->SetLightingChannels(false, false, true);
    const FName Names[] = {TEXT("PortraitTunic"), TEXT("PortraitApron"), TEXT("PortraitFeet")};
    for (FName Name : Names)
    {
        auto* Part = CreateDefaultSubobject<USkeletalMeshComponent>(Name);
        Part->SetupAttachment(Body);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
        Part->SetCastShadow(false);
        Part->SetLightingChannels(false, false, true);
        Garments.Add(Part);
    }
    Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortraitCapture"));
    Capture->SetupAttachment(RootComponent);
    Capture->SetRelativeLocation(FVector(270, 0, 85));
    Capture->SetRelativeRotation(FRotator(0, 180, 0));
    Capture->FOVAngle = 20;
    Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    Capture->bCaptureEveryFrame = false;
    Capture->bCaptureOnMovement = false;
    Capture->bAlwaysPersistRenderingState = true;
    Capture->ShowFlags.SetAtmosphere(false);
    Capture->ShowFlags.SetFog(false);
    Capture->ShowFlags.SetMotionBlur(false);
    Capture->ShowFlags.SetDepthOfField(false);
    Capture->ShowFlags.SetSkyLighting(false);
    Capture->ShowFlags.SetGlobalIllumination(false);
    Capture->ShowFlags.SetReflectionEnvironment(false);
    Capture->ShowFlags.SetLocalExposure(false);
    Capture->ShowFlags.SetEyeAdaptation(true);
    Capture->PostProcessBlendWeight = 1;
    auto& Exposure = Capture->PostProcessSettings;
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
    Exposure.bOverride_DepthOfFieldFstop = true;
    Exposure.DepthOfFieldFstop = 2.8f;
    Exposure.bOverride_BloomIntensity = true;
    Exposure.BloomIntensity = 0;
    Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortraitLight"));
    Light->SetupAttachment(RootComponent);
    Light->SetRelativeLocation(FVector(180, -120, 200));
    Light->SetIntensityUnits(ELightUnits::Lumens);
    Light->SetIntensity(3000);
    Light->SetAttenuationRadius(700);
    Light->SetCastShadows(false);
    Light->SetLightingChannels(false, false, true);
    FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortraitFillLight"));
    FillLight->SetupAttachment(RootComponent);
    FillLight->SetRelativeLocation(FVector(110, 155, 65));
    FillLight->SetIntensityUnits(ELightUnits::Lumens);
    FillLight->SetIntensity(1800);
    FillLight->SetAttenuationRadius(650);
    FillLight->SetCastShadows(false);
    FillLight->SetLightingChannels(false, false, true);
}

bool AHomesteadMenuPortrait::Refresh(AHomesteadCharacter& Character)
{
    const auto* Source = Character.GetMesh();
    if (!Source || !Source->GetSkeletalMeshAsset()) return false;
    UAnimSequence* Idle = Character.GetIdleAnimation();
    if (!Idle)
    {
        UE_LOG(LogTemp, Error, TEXT("Menu portrait needs the heroine's admitted idle animation."));
        return false;
    }
    Subject = &Character;
    const auto* Previous = Body->GetSingleNodeInstance();
    const float PreviousPhase = Previous && Previous->GetAnimationAsset() == Idle
        ? Previous->GetCurrentTime() : 0.0f;
    if (!Target)
    {
        Target = NewObject<UTextureRenderTarget2D>(this);
        Target->ClearColor = FLinearColor(0.025f, 0.05f, 0.038f, 1);
        Target->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
        Target->InitAutoFormat(768, 1536);
        Capture->TextureTarget = Target;
    }
    Capture->ClearShowOnlyComponents();
    if (const auto* Presentation = Character.GetEquipmentPresentation())
    {
        HomesteadWardrobePresentation::ApplySurface(Presentation->Base, *Body);
        for (int32 Index = 0; Index < Garments.Num(); ++Index)
        {
            if (Presentation->Garments.IsValidIndex(Index))
            {
                HomesteadWardrobePresentation::ApplySurface(Presentation->Garments[Index], *Garments[Index]);
                Garments[Index]->SetLeaderPoseComponent(Body, true, false);
                Garments[Index]->SetVisibility(true);
                Capture->ShowOnlyComponent(Garments[Index]);
            }
            else
            {
                Garments[Index]->SetVisibility(false);
                Garments[Index]->SetSkeletalMesh(nullptr);
                Garments[Index]->EmptyOverrideMaterials();
            }
        }
    }
    else
    {
        Body->SetSkeletalMesh(Source->GetSkeletalMeshAsset());
        Body->EmptyOverrideMaterials();
        for (int32 Index = 0; Index < Source->GetNumMaterials(); ++Index) Body->SetMaterial(Index, Source->GetMaterial(Index));
        for (const auto& Part : Garments) Part->SetVisibility(false);
    }
    Body->SetLeaderPoseComponent(nullptr);
    Body->PlayAnimation(Idle, true);
    if (auto* Current = Body->GetSingleNodeInstance())
        Current->SetPosition(FMath::Fmod(PreviousPhase, Idle->GetPlayLength()), false);
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Menu portrait could not play its independent idle."));
        return false;
    }
    MeshRotation = Source->GetRelativeRotation();
    Body->SetRelativeRotation(MeshRotation + FRotator(0, Yaw, 0));
    Body->SetRelativeScale3D(Source->GetRelativeScale3D());
    FTransform ReferenceTransform = Body->GetRelativeTransform();
    ReferenceTransform.SetRotation(MeshRotation.Quaternion());
    FBox SubjectBounds = Body->GetSkeletalMeshAsset()->GetBounds().GetBox().TransformBy(ReferenceTransform);
    for (const auto& Part : Garments)
        if (Part->IsVisible() && Part->GetSkeletalMeshAsset())
            SubjectBounds += Part->GetSkeletalMeshAsset()->GetBounds().GetBox().TransformBy(
                Part->GetRelativeTransform() * ReferenceTransform);
    if (!SubjectBounds.IsValid || SubjectBounds.GetExtent().ContainsNaN() || SubjectBounds.GetExtent().Z <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Menu portrait rejected invalid rendered subject bounds."));
        return false;
    }
    SubjectCenter = SubjectBounds.GetCenter();
    SubjectExtent = SubjectBounds.GetExtent();
    UpdateCaptureFraming();
    Capture->ShowOnlyComponent(Body);
    bCapturePending = true;
    return true;
}

void AHomesteadMenuPortrait::Orbit(float Degrees)
{
    Yaw = FMath::Fmod(Yaw + Degrees, 360.0f);
    Body->SetRelativeRotation(MeshRotation + FRotator(0, Yaw, 0));
    UpdateCaptureFraming();
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
    const auto* Animation = Body->GetSingleNodeInstance();
    return Animation ? TOptional<float>(Animation->GetCurrentTime()) : TOptional<float>();
}

void AHomesteadMenuPortrait::UpdateCaptureFraming()
{
    if (!Target || SubjectExtent.Z <= 0) return;
    const float HalfFov = FMath::DegreesToRadians(Capture->FOVAngle * 0.5f);
    const double Aspect = static_cast<double>(Target->SizeX) / Target->SizeY;
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
        const double Radius = FVector2D(SubjectExtent.X, SubjectExtent.Y).Size();
        Distance = FMath::Max(Radius / FMath::Sin(HalfFov),
            SubjectExtent.Z / VerticalTangent + SubjectExtent.X) * 1.08;
    }
    Center = FRotator(0, Yaw, 0).RotateVector(Center);
    Capture->SetRelativeLocation(Center + FVector(Distance, 0, 0));
    Capture->SetRelativeRotation(FRotator(0, 180, 0));
}

void AHomesteadMenuPortrait::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const auto* PC = Subject.IsValid()
        ? Cast<AHomesteadController>(Subject->GetController()) : nullptr;
    const bool bVisible = PC && PC->IsBookOpen()
        && (PC->BookPage() == 0 || PC->BookPage() == 6);
    if ((bCapturePending || bVisible) && Target)
    {
        Capture->CaptureScene();
        bCapturePending = false;
    }
}
