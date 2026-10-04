#include "HomesteadShopkeeper.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogHomesteadShopkeeper);

namespace ShopkeeperStandIn
{
const TCHAR* Mesh = TEXT("/Game/SurvivalGame/Characters/Heroine/SK_Heroine_Ponytail_Apron.SK_Heroine_Ponytail_Apron");
const TCHAR* Idle = TEXT("/Game/SurvivalGame/Characters/Heroine/Animations/AN_Heroine_RelaxedIdle.AN_Heroine_RelaxedIdle");
}

namespace ShopkeeperTurn
{
// Both the legacy heroine meshes and the MetaHuman body face +Y in their own space.
constexpr float MeshYaw = -90.0f;
constexpr float LookRadius = 450.0f;
constexpr float TurnDegreesPerSecond = 120.0f;
// The stand-in stands free and turns as far as her shoulders allow.
constexpr float StandInMaxTurn = 70.0f;
// The clerk's hands rest on the counter about 38 cm in front of him (clerk_counter_idle.py), so a
// small turn keeps them on the counter top instead of sliding off its back edge.
constexpr float LeaningMaxTurn = 10.0f;
constexpr float LeaningTurnDegreesPerSecond = 30.0f;
}

AHomesteadShopkeeper::AHomesteadShopkeeper()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.05f;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(Root);
    Body->SetRelativeRotation(FRotator(0, ShopkeeperTurn::MeshYaw, 0));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetGenerateOverlapEvents(false);
    Body->SetCastShadow(true);
}

void AHomesteadShopkeeper::Place(const FVector& Location, float Yaw)
{
    RestYaw = Yaw;
    SetActorLocationAndRotation(Location, FRotator(0, Yaw, 0));
    if (bBuilt) return;
    bBuilt = true;
    bMetaHuman = BuildMetaHuman();
    if (!bMetaHuman) BuildStandIn();
}

void AHomesteadShopkeeper::BuildStandIn()
{
    UE_LOG(LogHomesteadShopkeeper, Warning, TEXT("Shopkeeper MetaHuman assets are missing; using the stand-in body."));
    if (USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, ShopkeeperStandIn::Mesh)) Body->SetSkeletalMeshAsset(Mesh);
    else UE_LOG(LogHomesteadShopkeeper, Warning, TEXT("Shopkeeper stand-in mesh is missing: %s"), ShopkeeperStandIn::Mesh);
    if (UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, ShopkeeperStandIn::Idle))
    {
        Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Body->PlayAnimation(Idle, true);
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
    const float MaxTurn = bMetaHuman ? ShopkeeperTurn::LeaningMaxTurn : ShopkeeperTurn::StandInMaxTurn;
    const float TurnRate = bMetaHuman ? ShopkeeperTurn::LeaningTurnDegreesPerSecond : ShopkeeperTurn::TurnDegreesPerSecond;
    float TargetYaw = RestYaw;
    if (const APawn* Heroine = UGameplayStatics::GetPlayerPawn(this, 0))
    {
        const FVector ToHer = Heroine->GetActorLocation() - GetActorLocation();
        if (ToHer.Size2D() < ShopkeeperTurn::LookRadius)
        {
            const float Wanted = FMath::RadiansToDegrees(FMath::Atan2(ToHer.Y, ToHer.X));
            TargetYaw = RestYaw + FMath::Clamp(FMath::FindDeltaAngleDegrees(RestYaw, Wanted), -MaxTurn, MaxTurn);
        }
    }
    const float Current = GetActorRotation().Yaw;
    const float Step = FMath::Clamp(FMath::FindDeltaAngleDegrees(Current, TargetYaw),
        -TurnRate * DeltaSeconds, TurnRate * DeltaSeconds);
    SetActorRotation(FRotator(0, Current + Step, 0));
}
