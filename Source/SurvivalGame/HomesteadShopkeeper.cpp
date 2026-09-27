#include "HomesteadShopkeeper.h"

#include "Animation/AnimSequence.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

namespace
{
const TCHAR* StandInMesh = TEXT("/Game/SurvivalGame/Characters/Heroine/SK_Heroine_Ponytail_Apron.SK_Heroine_Ponytail_Apron");
const TCHAR* StandInIdle = TEXT("/Game/SurvivalGame/Characters/Heroine/Animations/AN_Heroine_RelaxedIdle.AN_Heroine_RelaxedIdle");
// The legacy heroine meshes face +Y in their own space.
constexpr float MeshYaw = -90.0f;
constexpr float LookRadius = 450.0f;
constexpr float TurnDegreesPerSecond = 120.0f;
}

AHomesteadShopkeeper::AHomesteadShopkeeper()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.05f;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("StandInBody"));
    Body->SetupAttachment(Root);
    Body->SetRelativeRotation(FRotator(0, MeshYaw, 0));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetCastShadow(true);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StandInLabel"));
    Label->SetupAttachment(Root);
    Label->SetRelativeLocation(FVector(0, 0, 196));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetVerticalAlignment(EVRTA_TextBottom);
    Label->SetWorldSize(9.0f);
    Label->SetTextRenderColor(FColor(236, 222, 190));
    Label->SetText(FText::FromString(TEXT("Mrs. Martha Pascoe\n(stand-in body)")));
    Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label->SetCastShadow(false);
}

void AHomesteadShopkeeper::Place(const FVector& Location, float Yaw)
{
    RestYaw = Yaw;
    SetActorLocationAndRotation(Location, FRotator(0, Yaw, 0));
    if (!Body->GetSkeletalMeshAsset())
    {
        if (USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, StandInMesh)) Body->SetSkeletalMeshAsset(Mesh);
        else UE_LOG(LogTemp, Warning, TEXT("Shopkeeper stand-in mesh is missing: %s"), StandInMesh);
        if (UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, StandInIdle))
        {
            Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
            Body->PlayAnimation(Idle, true);
        }
    }
}

void AHomesteadShopkeeper::SetOnDuty(bool bOnDuty)
{
    if (bDuty == bOnDuty) return;
    bDuty = bOnDuty;
    SetActorHiddenInGame(!bOnDuty);
    SetActorTickEnabled(bOnDuty);
}

void AHomesteadShopkeeper::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    float TargetYaw = RestYaw;
    if (const APawn* Heroine = UGameplayStatics::GetPlayerPawn(this, 0))
    {
        const FVector ToHer = Heroine->GetActorLocation() - GetActorLocation();
        if (ToHer.Size2D() < LookRadius)
        {
            // She turns toward the heroine, but not further than her shoulders would allow at the counter.
            const float Wanted = FMath::RadiansToDegrees(FMath::Atan2(ToHer.Y, ToHer.X));
            TargetYaw = RestYaw + FMath::Clamp(FMath::FindDeltaAngleDegrees(RestYaw, Wanted), -70.0f, 70.0f);
        }
    }
    const float Current = GetActorRotation().Yaw;
    const float Step = FMath::Clamp(FMath::FindDeltaAngleDegrees(Current, TargetYaw),
        -TurnDegreesPerSecond * DeltaSeconds, TurnDegreesPerSecond * DeltaSeconds);
    SetActorRotation(FRotator(0, Current + Step, 0));
    // The label always reads toward the camera.
    if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
    {
        const FVector ToCamera = Camera->GetCameraLocation() - Label->GetComponentLocation();
        Label->SetWorldRotation(FRotator(0, FMath::RadiansToDegrees(FMath::Atan2(ToCamera.Y, ToCamera.X)), 0));
    }
}
