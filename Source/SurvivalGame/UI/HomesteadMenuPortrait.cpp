#include "HomesteadMenuPortrait.h"
#include "../HomesteadCharacter.h"
#include "../HomesteadWardrobePresentation.h"
#include "Components/SceneComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"

AHomesteadMenuPortrait::AHomesteadMenuPortrait()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.0f / 15.0f;
    SetActorEnableCollision(false);
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("PortraitRoot"));
    Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PortraitBody"));
    Body->SetupAttachment(RootComponent);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetGenerateOverlapEvents(false);
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
    Capture->SetRelativeLocation(FVector(300, 0, 85));
    Capture->SetRelativeRotation(FRotator(0, 180, 0));
    Capture->FOVAngle = 38;
    Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    Capture->bCaptureEveryFrame = false;
    Capture->bCaptureOnMovement = false;
    Capture->ShowFlags.SetAtmosphere(false);
    Capture->ShowFlags.SetFog(false);
    Capture->ShowFlags.SetMotionBlur(false);
    Capture->ShowFlags.SetEyeAdaptation(false);
    Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortraitLight"));
    Light->SetupAttachment(RootComponent);
    Light->SetRelativeLocation(FVector(180, -120, 200));
    Light->SetIntensity(8000);
    Light->SetAttenuationRadius(700);
    Light->SetCastShadows(false);
    Light->SetLightingChannels(false, false, true);
}

bool AHomesteadMenuPortrait::Refresh(AHomesteadCharacter& Character)
{
    const auto* Source = Character.GetMesh();
    if (!Source || !Source->GetSkeletalMeshAsset()) return false;
    if (!Target)
    {
        Target = NewObject<UTextureRenderTarget2D>(this);
        Target->ClearColor = FLinearColor(0.025f, 0.05f, 0.038f, 1);
        Target->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
        Target->InitAutoFormat(512, 640);
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
    Body->SetLeaderPoseComponent(Character.GetMesh(), true, false);
    MeshRotation = Source->GetRelativeRotation();
    Body->SetRelativeRotation(MeshRotation + FRotator(0, Yaw, 0));
    Body->SetRelativeScale3D(Source->GetRelativeScale3D());
    Capture->ShowOnlyComponent(Body);
    bCapturePending = true;
    return true;
}

void AHomesteadMenuPortrait::Orbit(float Degrees)
{
    Yaw = FMath::Fmod(Yaw + Degrees, 360.0f);
    Body->SetRelativeRotation(MeshRotation + FRotator(0, Yaw, 0));
    bCapturePending = true;
}

void AHomesteadMenuPortrait::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bCapturePending && Target)
    {
        Capture->CaptureScene();
        bCapturePending = false;
    }
}
