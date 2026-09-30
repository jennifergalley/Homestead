#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWateringTool.h"
#include "HomesteadHatchet.h"
#include "HomesteadDiggingStick.h"
#include "HomesteadKnife.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

AHomesteadCharacter::AHomesteadCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    GetCapsuleComponent()->InitCapsuleSize(32.0f, 86.0f);
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0, 300, 0);
    GetCharacterMovement()->MaxWalkSpeed = 180.0f;
    GetCharacterMovement()->MaxAcceleration = 700.0f;
    GetCharacterMovement()->MaxStepHeight = 42.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 900.0f;

    CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    CameraArm->SetupAttachment(RootComponent);
    CameraArm->TargetArmLength = 470;
    CameraArm->SocketOffset = FVector(0, 45, 55);
    CameraArm->bUsePawnControlRotation = true;
    CameraArm->bEnableCameraLag = true;
    CameraArm->CameraLagSpeed = 12;
    CameraArm->ProbeSize = 12;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraArm);
    Camera->FieldOfView = 75;
    static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> CameraFoliageCollection(
        TEXT("/Game/SurvivalGame/Environment/CameraSafeFoliage/MPC_CameraSafeFoliage.MPC_CameraSafeFoliage"));
    CameraFoliageParameters = CameraFoliageCollection.Object;

    // An explicit fallback, never a silent substitute for a missing character asset.
    StandIn = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TechnicalStandIn"));
    StandIn->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> StandInMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (StandInMesh.Succeeded())
    {
        StandIn->SetStaticMesh(StandInMesh.Object);
    }
    StandIn->SetRelativeScale3D(FVector(0.42, 0.42, 1.55));
    StandIn->SetRelativeLocation(FVector(0, 0, -8));
    StandIn->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WateringTool = CreateDefaultSubobject<UHomesteadWateringTool>(TEXT("ContextualWateringCan"));
    WateringTool->SetupAttachment(GetMesh(), TEXT("hand_r"));
    Hatchet = CreateDefaultSubobject<UHomesteadHatchet>(TEXT("ContextualHatchet"));
    Hatchet->SetupAttachment(GetMesh(), TEXT("hand_r"));
    DiggingStick = CreateDefaultSubobject<UHomesteadDiggingStick>(TEXT("ContextualDiggingStick"));
    DiggingStick->SetupAttachment(GetMesh(), TEXT("hand_r"));
    Knife = CreateDefaultSubobject<UHomesteadKnife>(TEXT("HeldKnife"));
    Knife->SetupAttachment(GetMesh(), TEXT("hand_r"));
    const FName GarmentNames[] = {TEXT("EquippedTunic"), TEXT("EquippedApron"), TEXT("EquippedFootwear")};
    for (FName Name : GarmentNames)
    {
        auto* Garment = CreateDefaultSubobject<USkeletalMeshComponent>(Name);
        Garment->SetupAttachment(GetMesh());
        Garment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Garment->SetGenerateOverlapEvents(false);
        Garment->bUseAttachParentBound = true;
        Garment->SetVisibility(false);
        GarmentComponents.Add(Garment);
    }
}

void AHomesteadCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    RestoreNearClip();
    Super::EndPlay(Reason);
}

void AHomesteadCharacter::BeginPlay()
{
    Super::BeginPlay();
    if (!ActiveEquipment.Ready)
    {
        if (auto* PC = Cast<AHomesteadController>(Controller))
            ApplyAppearance(PC->GetAppearance());
        else if (InCharacterLab())
            ApplyAppearance(FHomesteadAppearance());
    }
    CreateMappings();
    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (ULocalPlayer* LP = PC->GetLocalPlayer())
        {
            if (auto* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                Subsystem->AddMappingContext(Mapping, 0);
            }
        }
    }
}

void AHomesteadCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    auto* PC = Cast<AHomesteadController>(Controller);
    const bool Lab = !PC && InCharacterLab();
    auto* Movement = GetCharacterMovement();
    const auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    const bool Blocked = (Lab ? false : !PC || !PC->IsWorldReady() || PC->IsBookOpen() || PC->IsShopScreenOpen()
        || PC->IsPlanning() || PC->IsFailed()) || bPlanning || bAppearancePreview
        || !Movement->IsMovingOnGround() || !bHeroineReady
        || (Animation && Animation->ActionWeight() > 0.01f);
    if (Blocked) CancelSprint();
    const bool Moving = Movement->GetCurrentAcceleration().SizeSquared2D() > 1.0f
        && GetVelocity().SizeSquared2D() > 144.0f;
    // Too tired to go on (work and the hours awake wore her down; running itself is free): the
    // toggle goes off with one gentle notice, and she walks. It never turns itself back on.
    if (bSprintOn && !Lab && PC && !PC->Simulation().CanSprint())
    {
        ResetSprint();
        PC->SprintTooTired();
    }
    bSprintActive = bSprintOn && !Blocked && Moving
        && (Lab || PC->Simulation().CanSprint()) && SprintAnimation != nullptr;
    Movement->MaxWalkSpeed = bSprintActive ? SprintSpeed() : WalkSpeed();
    if (bAppearancePreview) UpdateAppearanceFraming();
    UpdatePendingKneel();
    UpdateCarriedSticks();
    UpdateEating();
    UpdateHeldTools(DeltaSeconds);
    UpdateFellApproach(DeltaSeconds);
    if (FellStepRemaining > 0)
    {
        // Settle into the work stance while she addresses the trunk or bush.
        FellStepRemaining = FMath::Max(0.0f, FellStepRemaining - DeltaSeconds);
        const float Alpha = FMath::SmoothStep(0.0f, 1.0f, 1.0f - FellStepRemaining / FellStepSeconds);
        FVector Step = FMath::Lerp(FellStepFrom, FellStepTo, Alpha);
        Step.Z = GetActorLocation().Z;
        const float Yaw = FellStepFromYaw + FMath::FindDeltaAngleDegrees(FellStepFromYaw, FellStepToYaw) * Alpha;
        SetActorLocationAndRotation(Step, FRotator(0, Yaw, 0), true);
        if (bStanceStepFollowsGround)
            SettleOnGround();
    }
    UpdateStickAlignment(DeltaSeconds);
    UpdateHairMotion(DeltaSeconds);
    if (!Lab) UpdateRoomCamera(DeltaSeconds);
    if (CameraSnapFrames > 0 && --CameraSnapFrames == 0) CameraArm->bEnableCameraLag = true;
    if (!bAppearancePreview && CameraFoliageParameters && Camera && GetWorld())
    {
        const FVector CameraPosition = Camera->GetComponentLocation();
        const FVector HeroTarget = GetActorLocation() + FVector(0, 0, 65);
        if (!CameraPosition.ContainsNaN() && !HeroTarget.ContainsNaN()
            && FVector::DistSquared(CameraPosition, HeroTarget) > 1.0)
            if (auto* Parameters = GetWorld()->GetParameterCollectionInstance(
                CameraFoliageParameters))
            {
                Parameters->SetVectorParameterValue(TEXT("CameraPosition"),
                    FLinearColor(CameraPosition));
                Parameters->SetVectorParameterValue(TEXT("HeroTargetPosition"),
                    FLinearColor(HeroTarget));
            }
    }
}
