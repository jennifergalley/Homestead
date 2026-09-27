#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadWorld.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWateringTool.h"
#include "HomesteadHatchet.h"
#include "HomesteadDiggingStick.h"
#include "HomesteadKnife.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/LODSyncComponent.h"
#include "GroomComponent.h"
#include "GroomAsset.h"
#include "GroomBindingAsset.h"
#include "HAL/IConsoleManager.h"
#include "RenderCore.h"
#include "HomesteadLab.h"

namespace
{
TAutoConsoleVariable<int32> CVarMetaHumanHeroine(TEXT("homestead.MetaHumanHeroine"), 1,
    TEXT("1 = MetaHuman heroine (default), 0 = legacy heroine rollback. "
         "Read when the heroine's appearance is applied."));

// Hair inherits more of her motion as she speeds up from a walk to a full sprint.
TAutoConsoleVariable<float> CVarHairLinearWalk(TEXT("homestead.HairLinearWalk"), 0.5f,
    TEXT("MetaHuman hair linear velocity scale at walking speed and below."));
TAutoConsoleVariable<float> CVarHairLinearSprint(TEXT("homestead.HairLinearSprint"), 0.65f,
    TEXT("MetaHuman hair linear velocity scale at full sprint."));
TAutoConsoleVariable<float> CVarHairAngularWalk(TEXT("homestead.HairAngularWalk"), 0.4f,
    TEXT("MetaHuman hair angular velocity scale at walking speed and below."));
TAutoConsoleVariable<float> CVarHairAngularSprint(TEXT("homestead.HairAngularSprint"), 0.45f,
    TEXT("MetaHuman hair angular velocity scale at full sprint."));

// Resting tool carries: degrees the head tips down from level (the wrist supplies RestWristDegrees
// of it). Small values carry the tool nearly parallel to the ground. Negative = authored default.
TAutoConsoleVariable<float> CVarCarryHatchet(TEXT("homestead.CarryHatchet"), -1.0f, TEXT("Hatchet carry tilt (deg)."));
TAutoConsoleVariable<float> CVarCarryHoe(TEXT("homestead.CarryHoe"), -1.0f, TEXT("Stone hoe carry tilt (deg)."));
TAutoConsoleVariable<float> CVarCarryMachete(TEXT("homestead.CarryMachete"), -1.0f, TEXT("Machete carry tilt (deg)."));
TAutoConsoleVariable<float> CVarCarryKnife(TEXT("homestead.CarryKnife"), -1.0f, TEXT("Knife carry tilt (deg)."));


const TCHAR* const MetaHumanRoot = TEXT("/Game/Characters/Heroine_MH");

template <typename T>
T* LoadMetaHumanAsset(const FString& RelativePath)
{
    const FString Name = FPaths::GetBaseFilename(RelativePath);
    const FString Path = FString::Printf(TEXT("%s/%s.%s"), MetaHumanRoot, *RelativePath, *Name);
    T* Asset = LoadObject<T>(nullptr, *Path);
    if (!Asset) UE_LOG(LogTemp, Error, TEXT("MetaHuman heroine asset is missing: %s"), *Path);
    return Asset;
}

struct FMetaHumanGroomSpec
{
    const TCHAR* Component;
    const TCHAR* Groom;
    TArray<const TCHAR*> Materials;
};

FTransform RefComponentTransform(const FReferenceSkeleton& Skeleton, int32 Bone)
{
    FTransform Result = FTransform::Identity;
    for (; Bone != INDEX_NONE; Bone = Skeleton.GetParentIndex(Bone))
        Result = Result * Skeleton.GetRefBonePose()[Bone];
    return Result;
}

// A handle held in the right hand's closed grip (FHandGrip), relative to hand_r: across the palm
// at the finger crease, the tool's +Z toward the thumb side and its -Y (a blade's edge) along the
// knuckles, the way a machete or hatchet is held. Tools author their pivot at the grip centre.
FTransform HandGripTransform(const USkeletalMesh& Mesh)
{
    const FReferenceSkeleton& Skeleton = Mesh.GetRefSkeleton();
    const int32 Hand = Skeleton.FindBoneIndex(TEXT("hand_r"));
    const int32 Middle = Skeleton.FindBoneIndex(TEXT("middle_01_r"));
    const int32 Index = Skeleton.FindBoneIndex(TEXT("index_01_r"));
    const int32 Pinky = Skeleton.FindBoneIndex(TEXT("pinky_01_r"));
    if (Hand == INDEX_NONE || Middle == INDEX_NONE || Index == INDEX_NONE || Pinky == INDEX_NONE)
        return FTransform::Identity;
    const FTransform HandT = RefComponentTransform(Skeleton, Hand);
    const FVector Knuckle = RefComponentTransform(Skeleton, Middle).GetLocation();
    const FVector Along = (Knuckle - HandT.GetLocation()).GetSafeNormal();
    FVector Across = RefComponentTransform(Skeleton, Index).GetLocation() - RefComponentTransform(Skeleton, Pinky).GetLocation();
    Across = (Across - Along * FVector::DotProduct(Across, Along)).GetSafeNormal();
    // Into the palm (Across x Along points out of the back of the hand).
    const FVector Palm = FVector::CrossProduct(Along, Across).GetSafeNormal();
    // Seated in the palm just below the knuckles so the index knuckle clears the haft.
    const FVector Centre = HandT.GetLocation() + (Knuckle - HandT.GetLocation()) * 0.75f + Palm * 3.3f;
    const FTransform Grip(FRotationMatrix::MakeFromZY(Across, -Along).ToQuat(), Centre);
    return Grip.GetRelativeTransform(HandT);
}
}

bool AHomesteadCharacter::UsesMetaHumanHeroine()
{
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadMetaHuman"))) return true;
    // Smoke/automation routes assert the legacy wardrobe and material contracts.
    return CVarMetaHumanHeroine.GetValueOnGameThread() != 0 && !GIsAutomationTesting
        && !FParse::Param(FCommandLine::Get(), TEXT("HomesteadSmokeTest"))
        && !FParse::Param(FCommandLine::Get(), TEXT("HomesteadLegacyHeroine"));
}

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

bool AHomesteadCharacter::LoadHeroineAssets()
{
    if (bAttemptedAssetLoad) return bHeroineAssetsValid;
    bAttemptedAssetLoad = true;
    LongHairMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Trials/HeroineWave_20260921_01/Meshes/SK_Heroine_LongWave.SK_Heroine_LongWave"));
    BobHairMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/SurvivalGame/Characters/ModularClothing/JoinedBob/SK_Heroine_Bob.SK_Heroine_Bob"));
    IdleAnimation = LoadObject<UAnimSequence>(nullptr,
        TEXT("/Game/Trials/HeroineIdle_20260924_15/Animations/AN_Heroine_LivingIdle02.AN_Heroine_LivingIdle02"));
    const bool CMUWalk = FParse::Param(FCommandLine::Get(), TEXT("HomesteadTrialCMUWalk01"));
    const bool LevelHead = FParse::Param(FCommandLine::Get(), TEXT("HomesteadTrialCMULevelHead"));
    if (LevelHead && !CMUWalk)
    {
        UE_LOG(LogTemp, Error, TEXT("The upright-head motion variant requires the licensed CMU walk trial."));
        return false;
    }
    WalkAnimation = LoadObject<UAnimSequence>(nullptr, CMUWalk
        ? (LevelHead
            ? TEXT("/Game/Trials/HeroineCMUWalk_20260924_04/Animations/AN_Heroine_CMUNormalWalk02.AN_Heroine_CMUNormalWalk02")
            : TEXT("/Game/Trials/HeroineCMUWalk_20260924_03/Animations/AN_Heroine_CMUNormalWalk01.AN_Heroine_CMUNormalWalk01"))
        : TEXT("/Game/SurvivalGame/Characters/Heroine/Animations/AN_Heroine_GroundedWalk.AN_Heroine_GroundedWalk"));
    if (CMUWalk)
        SlowWalkAnimation = LoadObject<UAnimSequence>(nullptr,
            LevelHead
                ? TEXT("/Game/Trials/HeroineCMUWalk_20260924_04/Animations/AN_Heroine_CMUSlowWalk02.AN_Heroine_CMUSlowWalk02")
                : TEXT("/Game/Trials/HeroineCMUWalk_20260924_03/Animations/AN_Heroine_CMUSlowWalk01.AN_Heroine_CMUSlowWalk01"));
    SprintAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Trials/HeroineSprint_20260924_01/Animations/AN_Heroine_Sprint.AN_Heroine_Sprint"));
    GatherAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/SurvivalGame/Characters/Heroine/Animations/AN_Heroine_Gather.AN_Heroine_Gather"));
    WaterAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Trials/HomesteadWork_20260923_01/Animations/AN_Heroine_WaterRefined.AN_Heroine_WaterRefined"));
    ClearAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Trials/HomesteadWork_20260923_01/Animations/AN_Heroine_Chop.AN_Heroine_Chop"));
    KnifeCutAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Trials/HeroineKnife_20260924_01/Animations/AN_Heroine_KnifeCut.AN_Heroine_KnifeCut"));
    TillAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Trials/HomesteadWork_20260923_01/Animations/AN_Heroine_Till.AN_Heroine_Till"));
    if (!LongHairMesh || !BobHairMesh || !IdleAnimation || !WalkAnimation
        || (CMUWalk && !SlowWalkAnimation) || !SprintAnimation
        || !GatherAnimation || !WaterAnimation || !ClearAnimation || !KnifeCutAnimation || !TillAnimation)
    {
        UE_LOG(LogTemp, Error, TEXT("Heroine mesh or motion assets are missing. Run Scripts/Build-Game.ps1; the labeled stand-in remains visible."));
        return false;
    }
    if (!LongHairMesh->GetSkeleton() || LongHairMesh->GetSkeleton() != BobHairMesh->GetSkeleton()
        || IdleAnimation->GetSkeleton() != LongHairMesh->GetSkeleton()
        || WalkAnimation->GetSkeleton() != LongHairMesh->GetSkeleton()
        || (CMUWalk && SlowWalkAnimation->GetSkeleton() != LongHairMesh->GetSkeleton())
        || SprintAnimation->GetSkeleton() != LongHairMesh->GetSkeleton()
        || GatherAnimation->GetSkeleton() != LongHairMesh->GetSkeleton()
        || WaterAnimation->GetSkeleton() != LongHairMesh->GetSkeleton()
        || ClearAnimation->GetSkeleton() != LongHairMesh->GetSkeleton()
        || KnifeCutAnimation->GetSkeleton() != LongHairMesh->GetSkeleton()
        || TillAnimation->GetSkeleton() != LongHairMesh->GetSkeleton())
    {
        UE_LOG(LogTemp, Error, TEXT("Heroine meshes and clips do not share a skeleton."));
        return false;
    }
    WardrobeMeshes = {LongHairMesh, BobHairMesh};
    const TCHAR* MoreMeshes[] = {
        TEXT("/Game/SurvivalGame/Characters/Heroine/SK_Heroine_Ponytail.SK_Heroine_Ponytail"),
        TEXT("/Game/Trials/HeroineWave_20260921_01/Meshes/SK_Heroine_LongWave_Apron.SK_Heroine_LongWave_Apron"),
        TEXT("/Game/SurvivalGame/Characters/ModularClothing/JoinedBob/SK_Heroine_Bob_Apron.SK_Heroine_Bob_Apron"),
        TEXT("/Game/SurvivalGame/Characters/Heroine/SK_Heroine_Ponytail_Apron.SK_Heroine_Ponytail_Apron")
    };
    for (const TCHAR* Path : MoreMeshes)
    {
        USkeletalMesh* Variant = LoadObject<USkeletalMesh>(nullptr, Path);
        if (!Variant || Variant->GetSkeleton() != LongHairMesh->GetSkeleton())
        {
            UE_LOG(LogTemp, Error, TEXT("A wardrobe variant is missing or incompatible: %s"), Path);
            return false;
        }
        WardrobeMeshes.Add(Variant);
    }
    const TCHAR* Bodies[] = {TEXT("Willow"), TEXT("Hazel")};
    const TCHAR* Styles[] = {TEXT("LongWave"), TEXT("Bob"), TEXT("Ponytail")};
    for (const TCHAR* Body : Bodies)
    {
        for (int32 Outfit = 0; Outfit < 2; ++Outfit)
        {
            for (const TCHAR* Style : Styles)
            {
                const FString Name = FString::Printf(TEXT("SK_Heroine_%s_%s%s"), Body, Style, Outfit ? TEXT("_Apron") : TEXT(""));
                const TCHAR* Directory = FCString::Strcmp(Style, TEXT("LongWave")) == 0
                    ? TEXT("/Game/Trials/HeroineWave_20260921_01/Meshes")
                    : FCString::Strcmp(Style, TEXT("Bob")) == 0
                        ? TEXT("/Game/SurvivalGame/Characters/ModularClothing/JoinedBob")
                        : TEXT("/Game/SurvivalGame/Characters/Heroine");
                const FString Path = FString::Printf(TEXT("%s/%s.%s"), Directory, *Name, *Name);
                USkeletalMesh* Preset = LoadObject<USkeletalMesh>(nullptr, *Path);
                if (!Preset || Preset->GetSkeleton() != LongHairMesh->GetSkeleton())
                {
                    UE_LOG(LogTemp, Error, TEXT("A complete character preset is missing or incompatible: %s"), *Path);
                    return false;
                }
                WardrobeMeshes.Add(Preset);
            }
        }
    }
    bHeroineAssetsValid = true;
    return true;
}

float AHomesteadCharacter::InferMeshYaw(const USkeletalMesh& Asset) const
{
    const FReferenceSkeleton& Skeleton = Asset.GetRefSkeleton();
    auto FindBone = [&Skeleton](std::initializer_list<const TCHAR*> Names)
    {
        for (const TCHAR* Name : Names)
            if (const int32 Index = Skeleton.FindBoneIndex(Name); Index != INDEX_NONE) return Index;
        return static_cast<int32>(INDEX_NONE);
    };
    const int32 LeftFoot = FindBone({TEXT("foot_l"), TEXT("foot.L"), TEXT("LeftFoot")});
    const int32 LeftToe = FindBone({TEXT("ball_l"), TEXT("toe_l"), TEXT("toe.L"), TEXT("LeftToeBase")});
    const int32 RightFoot = FindBone({TEXT("foot_r"), TEXT("foot.R"), TEXT("RightFoot")});
    const int32 RightToe = FindBone({TEXT("ball_r"), TEXT("toe_r"), TEXT("toe.R"), TEXT("RightToeBase")});
    if (LeftFoot == INDEX_NONE || LeftToe == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot infer heroine facing from foot/toe bones; verify the imported mesh orientation."));
        return 0;
    }
    auto Position = [&Skeleton](int32 Bone)
    {
        FTransform Transform = Skeleton.GetRefBonePose()[Bone];
        for (int32 Parent = Skeleton.GetParentIndex(Bone); Parent != INDEX_NONE; Parent = Skeleton.GetParentIndex(Parent))
            Transform = Transform * Skeleton.GetRefBonePose()[Parent];
        return Transform.GetLocation();
    };
    // Feet toe out (about 10 degrees on the MetaHuman), so average both feet to cancel the splay;
    // a single foot turned the whole body off the direction of travel.
    FVector Forward = Position(LeftToe) - Position(LeftFoot);
    if (RightFoot != INDEX_NONE && RightToe != INDEX_NONE)
        Forward += Position(RightToe) - Position(RightFoot);
    Forward.Z = 0;
    if (Forward.SizeSquared() < 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Heroine foot reference does not define a horizontal forward axis."));
        return 0;
    }
    return -Forward.Rotation().Yaw;
}

bool AHomesteadCharacter::LoadMetaHumanStack()
{
    if (bAttemptedMetaHumanLoad) return bMetaHumanAssetsValid;
    bAttemptedMetaHumanLoad = true;
    MetaHumanBody = LoadMetaHumanAsset<USkeletalMesh>(TEXT("Assembled/Heroine/Body/SKM_MHC_Heroine_BodyMesh"));
    // Walk and sprint are Game Animation Sample loops (retargeted by homestead_agent.gasp_locomotion);
    // the work actions are retargeted from the legacy heroine with RTG_HeroineLegacy_To_MH.
    UAnimSequence* Clips[] = {
        LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_LivingIdle02")),
        LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/GASP/AN_HeroineMH_GASP_Walk")),
        LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/GASP/AN_HeroineMH_GASP_Run")),
        LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_Gather")),
        LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_WaterRefined")),
        LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_Chop")),
        LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_KnifeCut")),
        LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_Till")),
    };
    if (!MetaHumanBody || !MetaHumanBody->GetSkeleton()) return false;
    for (UAnimSequence* Clip : Clips)
    {
        if (!Clip || Clip->GetSkeleton() != MetaHumanBody->GetSkeleton())
        {
            UE_LOG(LogTemp, Error, TEXT("A MetaHuman heroine clip is missing or not on the MetaHuman body skeleton."));
            return false;
        }
    }
    IdleAnimation = Clips[0];
    // Optional: the grounded resting stance authored with homestead_agent.active_idle.
    if (auto* ActiveIdle = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_ActiveIdle")))
        if (ActiveIdle->GetSkeleton() == MetaHumanBody->GetSkeleton()) IdleAnimation = ActiveIdle;
    WalkAnimation = Clips[1];
    SlowWalkAnimation = nullptr;
    SprintAnimation = Clips[2];
    GatherAnimation = Clips[3];
    WaterAnimation = Clips[4];
    ClearAnimation = Clips[5];
    KnifeCutAnimation = Clips[6];
    TillAnimation = Clips[7];
    // Optional: the two-handed stone hoe authored with homestead_agent.hoe_till.
    if (auto* Hoe = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_HoeTill")))
        if (Hoe->GetSkeleton() == MetaHumanBody->GetSkeleton()) { TillAnimation = Hoe; bHoeTill = true; }
    // Optional: authored with homestead_agent.kneel_gather; without it sticks use the plain gather.
    GatherSticksAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_KneelGatherSticks"));
    if (GatherSticksAnimation && GatherSticksAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton())
        GatherSticksAnimation = nullptr;
    const TCHAR* StickMeshes[] = {
        TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_DryBranchesMedium01_b.SM_DryBranchesMedium01_b"),
        TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_DryBranchesMedium01_c.SM_DryBranchesMedium01_c")};
    CarriedSticks.Reset();
    for (int32 Index = 0; Index < 2; ++Index)
        if (UStaticMesh* Branch = LoadObject<UStaticMesh>(nullptr, StickMeshes[Index]))
        {
            auto* Stick = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("CarriedStick%d"), Index));
            Stick->SetupAttachment(GetMesh());
            Stick->SetStaticMesh(Branch);
            Stick->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Stick->SetVisibility(false);
            Stick->RegisterComponent();
            CarriedSticks.Add(Stick);
        }
    GatherPouchAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_KneelGatherPouch"));
    if (GatherPouchAnimation && GatherPouchAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton())
        GatherPouchAnimation = nullptr;
    // Optional: authored with homestead_agent.machete_hack.
    MacheteAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_MacheteHack"));
    if (MacheteAnimation && MacheteAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton())
        MacheteAnimation = nullptr;
    // Optional: authored with homestead_agent.axe_fell.
    FellAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_AxeFell"));
    if (FellAnimation && FellAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton())
        FellAnimation = nullptr;
    auto MakeProp = [this](const TCHAR* Name, UStaticMesh* PropMesh)
    {
        auto* Prop = NewObject<UStaticMeshComponent>(this, Name);
        Prop->SetupAttachment(GetMesh());
        Prop->SetStaticMesh(PropMesh);
        Prop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Prop->SetVisibility(false);
        Prop->RegisterComponent();
        return Prop;
    };
    CarriedStones.Reset();
    UStaticMesh* ClusterRock = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/SurvivalGame/Environment/MossRocks.MossRocks"));
    UMaterialInterface* RockMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SurvivalGame/Materials/M_Rock.M_Rock"));
    for (int32 Index = 0; Index < 2; ++Index)
    {
        // She carries pile parts 1 and 2.
        UStaticMesh* HandStone = LoadHandStone(Index + 1);
        if (!HandStone && !ClusterRock) break;
        auto* Stone = MakeProp(*FString::Printf(TEXT("CarriedStone%d"), Index), HandStone ? HandStone : ClusterRock);
        if (!HandStone && RockMaterial) Stone->SetMaterial(0, RockMaterial);
        CarriedStones.Add(Stone);
    }
    // Authored forage props when imported (Blender recipes); simple tinted shapes otherwise.
    ForageBerryMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/SurvivalGame/Environment/Props/BerryCluster/SM_BerryCluster.SM_BerryCluster"));
    ForageRootMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/SurvivalGame/Environment/Props/WildRoot/SM_WildRoot.SM_WildRoot"));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    CarriedForage = MakeProp(TEXT("CarriedForage"), ForageBerryMesh ? ForageBerryMesh.Get() : Sphere);
    EatenFood = MakeProp(TEXT("EatenFood"), ForageBerryMesh ? ForageBerryMesh.Get() : Sphere);
    // Optional: authored with homestead_agent.kneel_reeds; without it reeds use the standing knife cut.
    GatherReedsAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_KneelCutReeds"));
    if (GatherReedsAnimation && GatherReedsAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton())
        GatherReedsAnimation = nullptr;
    if (UStaticMesh* Reeds = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/SurvivalGame/Environment/Reeds/SM_ReedClump.SM_ReedClump")))
    {
        CarriedReeds = MakeProp(TEXT("CarriedReeds"), Reeds);
        CarriedReeds->SetCastShadow(true);
    }
    // Optional: authored with homestead_agent.kneel_plant.
    GatherPlantAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_KneelPlant"));
    if (GatherPlantAnimation && GatherPlantAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton())
        GatherPlantAnimation = nullptr;
    if (UStaticMesh* Seeds = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/SurvivalGame/Environment/Props/Seeds/SM_Seeds.SM_Seeds")))
        CarriedSeed = MakeProp(TEXT("CarriedSeed"), Seeds);
    // Optional: authored with homestead_agent.eat_berry.
    EatAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_Eat"));
    if (EatAnimation && EatAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton())
        EatAnimation = nullptr;
    if (UStaticMesh* Pouch = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/SurvivalGame/Environment/Props/ForagePouch/SM_ForagePouch.SM_ForagePouch")))
    {
        ForagePouch = MakeProp(TEXT("ForagePouch"), Pouch);
        // A flat pouch fitted to her right hip and thigh (Scripts/Blender/Recipes/forage_pouch_fit.py),
        // authored about the belt cord where its thong wraps it, in the reference pose.
        ForagePouch->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("pelvis"));
        const FReferenceSkeleton& Skeleton = MetaHumanBody->GetRefSkeleton();
        const int32 Pelvis = Skeleton.FindBoneIndex(TEXT("pelvis"));
        const int32 Thigh = Skeleton.FindBoneIndex(TEXT("thigh_r"));
        const int32 Calf = Skeleton.FindBoneIndex(TEXT("calf_r"));
        PouchPivotRef = FVector(-16.79f, 4.01f, 103.09f);
        if (Pelvis != INDEX_NONE && Thigh != INDEX_NONE && Calf != INDEX_NONE)
        {
            PelvisRefPose = RefComponentTransform(Skeleton, Pelvis);
            HipRef = RefComponentTransform(Skeleton, Thigh).GetLocation();
            ThighDirRef = (RefComponentTransform(Skeleton, Calf).GetLocation() - HipRef).GetSafeNormal();
            ForagePouch->SetRelativeTransform(FTransform(PouchPivotRef).GetRelativeTransform(PelvisRefPose));
            if (!PouchSwingHandle.IsValid())
                PouchSwingHandle = GetMesh()->RegisterOnBoneTransformsFinalizedDelegate(
                    FOnBoneTransformsFinalizedMultiCast::FDelegate::CreateUObject(this, &AHomesteadCharacter::UpdatePouchSwing));
        }
        ForagePouch->SetVisibility(true);
    }
    // The rawhide cord belt the pouch hangs from, tied on the shorts' waistband with the knot in
    // front. Its path is fitted to the shorts (Scripts/Blender/Recipes/cord_belt_fit.py) and
    // authored about its pivot in the skeleton's reference pose, so it rides the pelvis from there.
    if (UStaticMesh* Belt = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/SurvivalGame/Environment/Props/CordBelt/SM_CordBelt.SM_CordBelt")))
    {
        CordBelt = MakeProp(TEXT("CordBelt"), Belt);
        CordBelt->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("pelvis"));
        const FReferenceSkeleton& Skeleton = MetaHumanBody->GetRefSkeleton();
        const int32 Pelvis = Skeleton.FindBoneIndex(TEXT("pelvis"));
        const FTransform BeltPivot(FVector(0.0f, 2.25f, 103.28f));
        CordBelt->SetRelativeTransform(Pelvis == INDEX_NONE ? FTransform::Identity
            : BeltPivot.GetRelativeTransform(RefComponentTransform(Skeleton, Pelvis)));
        CordBelt->SetVisibility(true);
    }
    if (UStaticMesh* Machete = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/SurvivalGame/Environment/Props/Machete/SM_Machete.SM_Machete")))
    {
        HeldMachete = MakeProp(TEXT("HeldMachete"), Machete);
        HeldMachete->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("hand_r"));
        MacheteGrip = HandGripTransform(*MetaHumanBody);
        HeldMachete->SetRelativeTransform(MacheteGrip);
        HeldMachete->SetCastShadow(true);
    }
    // Blender hand tools, each authored with its pivot at the main hand's grip, the handle along
    // +Z and the working edge toward -Y (docs/blender-assets.md). Offsets adapt tools held elsewhere.
    HeldProps.Reset();
    HeldToolSpecs.Reset();
    const FTransform Grip = HandGripTransform(*MetaHumanBody);
    struct FHeldToolAsset { Homestead::Item Tool; const TCHAR* Path; float CarryDegrees; bool bHangs; FTransform Offset; };
    // The digging stick's pivot is its upper grip with the point toward -Z; she trail-carries it at
    // the balance point instead, point forward and down, the way a spear or staff is carried.
    const FTransform StickTrail(FQuat(FVector::XAxisVector, PI), FVector(0, 0, -25));
    const FHeldToolAsset Assets[] = {
        {Homestead::Item::Knife, TEXT("FlintKnife/SM_FlintKnife"), 30, false, FTransform::Identity},
        {Homestead::Item::Hatchet, TEXT("FlintHatchet/SM_FlintHatchet"), 20, false, FTransform::Identity},
        // Tools ride nearly level in a relaxed hand, heads a little low. The stone hoe (blade at the
        // far end) is carried out in front from the top of its haft; the digging stick is the fallback.
        {Homestead::Item::DiggingStick, TEXT("StoneHoe/SM_StoneHoe"), 24, false, FTransform::Identity},
        {Homestead::Item::DiggingStick, TEXT("DiggingStick/SM_DiggingStick"), 34, false, StickTrail},
        {Homestead::Item::WateringCan, TEXT("WaterPail/SM_WaterPail"), 20, true, FTransform::Identity},
        // Authored in the machete's frame (grip pivot, blade +Z, edge -Y), so the hack fits it as is.
        {Homestead::Item::Billhook, TEXT("Billhook/SM_Billhook"), MacheteCarryDegrees, false, FTransform::Identity},
    };
    for (const FHeldToolAsset& Asset : Assets)
    {
        const FString Name = FPaths::GetBaseFilename(Asset.Path);
        auto* PropMesh = LoadObject<UStaticMesh>(nullptr,
            *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s.%s"), Asset.Path, *Name));
        if (!PropMesh) continue;
        // One prop per tool: the first authored mesh found wins.
        if (HeldToolSpecs.ContainsByPredicate([&Asset](const FHeldToolSpec& Spec) { return Spec.Tool == Asset.Tool; })) continue;
        auto* Prop = MakeProp(*FString::Printf(TEXT("Held_%s"), *Name), PropMesh);
        Prop->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("hand_r"));
        Prop->SetRelativeTransform(Asset.Offset * Grip);
        Prop->SetUsingAbsoluteRotation(Asset.bHangs);
        Prop->SetCastShadow(true);
        HeldProps.Add(Prop);
        HeldToolSpecs.Add({Asset.Tool, Asset.CarryDegrees, Asset.bHangs, Asset.Offset * Grip});
    }

    USkeletalMesh* FaceMesh = LoadMetaHumanAsset<USkeletalMesh>(TEXT("Assembled/Heroine/Face/SKM_MHC_Heroine_FaceMesh"));
    UClass* FaceAnimClass = LoadObject<UClass>(nullptr,
        TEXT("/Game/Characters/Heroine_MH/Common/Face/ABP_Face.ABP_Face_C"));
    USkeletalMesh* OutfitMesh = LoadMetaHumanAsset<USkeletalMesh>(TEXT("Assembled/Heroine/PrimitiveOutfit/SKM_PrimitiveOutfit"));
    // The homespun tank top and shorts are fitted to the un-culled body (BodyFull); the stock
    // MetaHuman outfit stays as the fallback on the body culled beneath it.
    const bool bPrimitiveOutfit = OutfitMesh && OutfitMesh->GetSkeleton() == MetaHumanBody->GetSkeleton();
    if (bPrimitiveOutfit)
    {
        if (USkeletalMesh* FullBody = LoadMetaHumanAsset<USkeletalMesh>(TEXT("Assembled/Heroine/BodyFull/SKM_MHC_Heroine_BodyFull"));
            FullBody && FullBody->GetSkeleton() == MetaHumanBody->GetSkeleton())
            MetaHumanBody = FullBody;
    }
    else OutfitMesh = LoadMetaHumanAsset<USkeletalMesh>(TEXT("Assembled/Heroine/Clothing/MHC_Heroine_Outfits"));
    if (!FaceMesh || !FaceAnimClass || !OutfitMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("MetaHuman heroine face, face animation or outfit is missing."));
        return false;
    }

    USkeletalMeshComponent* Body = GetMesh();
    auto MakeSkinned = [this, Body](const TCHAR* Name, USkeletalMesh* Asset)
    {
        auto* Component = NewObject<USkeletalMeshComponent>(this, Name);
        Component->SetupAttachment(Body);
        Component->SetSkeletalMesh(Asset);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->bUseAttachParentBound = true;
        Component->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        return Component;
    };
    // ABP_Face copies the attached parent's pose, then runs RigLogic for the face rig.
    MetaHumanFace = MakeSkinned(TEXT("MetaHumanFace"), FaceMesh);
    MetaHumanFace->SetAnimInstanceClass(FaceAnimClass);
    MetaHumanFace->AddTickPrerequisiteComponent(Body);
    MetaHumanFace->RegisterComponent();

    MetaHumanOutfit = MakeSkinned(TEXT("MetaHumanOutfit"), OutfitMesh);
    const TCHAR* OutfitMaterials[] = {
        TEXT("Assembled/Heroine/Clothing/MI_WI_DefaultGarment_M_DG_bodyShapeB_Shirt"),
        TEXT("Assembled/Heroine/Clothing/MI_WI_DefaultGarment_M_DG_bodyShapeB_Short")};
    for (int32 Index = 0; !bPrimitiveOutfit && Index < UE_ARRAY_COUNT(OutfitMaterials); ++Index)
        if (auto* Material = LoadMetaHumanAsset<UMaterialInterface>(OutfitMaterials[Index]))
            MetaHumanOutfit->SetMaterial(Index, Material);
    MetaHumanOutfit->RegisterComponent();
    MetaHumanOutfit->SetLeaderPoseComponent(Body);
    MetaHumanGarments.Reset();
    for (const TCHAR* Name : {TEXT("MetaHumanTop"), TEXT("MetaHumanLegs"), TEXT("MetaHumanCoat"), TEXT("MetaHumanFeet")})
    {
        auto* Garment = MakeSkinned(Name, nullptr);
        Garment->SetVisibility(false);
        Garment->RegisterComponent();
        Garment->SetLeaderPoseComponent(Body);
        MetaHumanGarments.Add(Garment);
    }
    if (MetaHumanWorn.Num() != MetaHumanGarments.Num()) MetaHumanWorn.Init(INDEX_NONE, MetaHumanGarments.Num());

    const FMetaHumanGroomSpec Grooms[] = {
        {TEXT("MetaHumanHair"), TEXT("Hair_L_Straight"),
            {TEXT("MI_WI_Hair_L_Straight_Hair"), TEXT("MI_WI_Hair_L_Straight_Hair_Cards"), TEXT("MI_WI_Hair_L_Straight_Hair_Helmet")}},
        {TEXT("MetaHumanEyebrows"), TEXT("Eyebrows_M_SlightArch"),
            {TEXT("MI_WI_Eyebrows_M_SlightArch_Hair"), TEXT("MI_WI_Eyebrows_M_SlightArch_Facial_Hair")}},
        {TEXT("MetaHumanEyelashes"), TEXT("Eyelashes_L_ThickCurl"), {TEXT("MI_WI_Eyelashes_L_ThickCurl_Hair")}},
    };
    for (const FMetaHumanGroomSpec& Spec : Grooms)
    {
        const FString Base = FString::Printf(TEXT("Assembled/Heroine/Grooms/%s"), Spec.Groom);
        auto* Asset = LoadMetaHumanAsset<UGroomAsset>(Base);
        auto* Binding = LoadMetaHumanAsset<UGroomBindingAsset>(Base + TEXT("_Binding"));
        if (!Asset || !Binding) return false;
        auto* Groom = NewObject<UGroomComponent>(this, Spec.Component);
        Groom->SetupAttachment(MetaHumanFace);
        Groom->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Groom->SetGroomAsset(Asset, Binding);
        for (int32 Index = 0; Index < Spec.Materials.Num(); ++Index)
            if (auto* Material = LoadMetaHumanAsset<UMaterialInterface>(
                FString::Printf(TEXT("Assembled/Heroine/Grooms/%s"), Spec.Materials[Index])))
                Groom->SetMaterial(Index, Material);
        if (FCString::Strcmp(Spec.Component, TEXT("MetaHumanHair")) == 0)
        {
            // Calmer than the stock groom: less inherited body motion and more damping, so turns don't fling it.
            // UpdateHairMotion raises the inherited motion a little at a sprint.
            FHairSimulationSettings& Sim = Groom->SimulationSettings;
            Sim.SimulationSetup.LinearVelocityScale = CVarHairLinearWalk.GetValueOnGameThread();
            Sim.SimulationSetup.AngularVelocityScale = CVarHairAngularWalk.GetValueOnGameThread();
            HairSprintBlend = 0;
            MetaHumanHair = Groom;
            Sim.bOverrideSettings = true;
            Sim.SolverSettings.bEnableSimulation = true;
            Sim.ExternalForces.GravityVector = FVector(0, 0, -981);
            Sim.ExternalForces.AirDrag = 1.0f;
            Sim.MaterialConstraints.BendDamping = 0.05f;
            Sim.MaterialConstraints.BendStiffness = 0.15f;
            Sim.MaterialConstraints.StretchDamping = 0.0f;
            Sim.MaterialConstraints.StretchStiffness = 1.0f;
            Sim.MaterialConstraints.StaticFriction = 0.5f;
            Sim.MaterialConstraints.KineticFriction = 0.5f;
            Sim.MaterialConstraints.StrandsViscosity = 1.0f;
            Sim.MaterialConstraints.CollisionRadius = 5.0f;
        }
        Groom->RegisterComponent();
        MetaHumanGrooms.Add(Groom);
    }

    // Mirrors the assembled BP_Heroine LODSync: body and face drive, garments and grooms follow.
    MetaHumanLODSync = NewObject<ULODSyncComponent>(this, TEXT("MetaHumanLODSync"));
    MetaHumanLODSync->NumLODs = 4;
    MetaHumanLODSync->ComponentsToSync = {
        FComponentSync(Body->GetFName(), ESyncOption::Drive),
        FComponentSync(MetaHumanFace->GetFName(), ESyncOption::Drive),
        FComponentSync(MetaHumanOutfit->GetFName(), ESyncOption::Passive)};
    for (USkeletalMeshComponent* Garment : MetaHumanGarments)
        MetaHumanLODSync->ComponentsToSync.Add(FComponentSync(Garment->GetFName(), ESyncOption::Passive));
    for (UGroomComponent* Groom : MetaHumanGrooms)
    {
        MetaHumanLODSync->ComponentsToSync.Add(FComponentSync(Groom->GetFName(), ESyncOption::Passive));
        FLODMappingData GroomMapping;
        GroomMapping.Mapping = {1, 3, 5, 7};
        MetaHumanLODSync->CustomLODMapping.Add(Groom->GetFName(), GroomMapping);
    }
    MetaHumanLODSync->RegisterComponent();
    bMetaHumanAssetsValid = true;
    return true;
}

bool AHomesteadCharacter::ApplyMetaHumanStack()
{
    if (!LoadMetaHumanStack())
    {
        UE_LOG(LogTemp, Error, TEXT("MetaHuman heroine is unavailable; the labeled stand-in remains visible."));
        return false;
    }
    for (USkeletalMeshComponent* Garment : GarmentComponents)
    {
        Garment->SetVisibility(false);
        Garment->SetLeaderPoseComponent(nullptr);
        Garment->EmptyOverrideMaterials();
        Garment->SetSkeletalMesh(nullptr);
    }
    USkeletalMeshComponent* Body = GetMesh();
    if (Body->GetSkeletalMeshAsset() != MetaHumanBody)
    {
        Body->EmptyOverrideMaterials();
        Body->SetSkeletalMesh(MetaHumanBody);
        Body->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
        Body->SetRelativeRotation(FRotator(0, InferMeshYaw(*MetaHumanBody), 0));
        Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Body->SetAnimInstanceClass(UHomesteadAnimInstance::StaticClass());
        MetaHumanOutfit->SetLeaderPoseComponent(Body, true);
    }
    AppearanceMaterials.Reset();
    StandIn->SetVisibility(false);
    Body->SetVisibility(true);
    bMetaHumanActive = true;
    if (!bSprintActive) GetCharacterMovement()->MaxWalkSpeed = WalkSpeed();
    ApplyMetaHumanLook();
    ApplyMetaHumanGarments();
    bHeroineReady = true;
    return true;
}

namespace
{
struct FMetaHumanGarmentSpec
{
    Homestead::WearableDefinition Definition;
    int32 Slot; // Index into MetaHumanGarments: 0 top, 1 legs, 2 coat, 3 feet.
    const TCHAR* Asset;
    float LiftCm; // Sole thickness (Assets/Characters/Footwear report character_offset_cm).
};
// Garments fitted to the MetaHuman body in Blender (Assets/Characters/Garments and Footwear) and
// imported by Scripts/Characters/import_heroine_garments.py.
const FMetaHumanGarmentSpec MetaHumanGarmentSpecs[] = {
    {Homestead::WearableDefinition::LinenShirt, 0, TEXT("SKM_LinenTee"), 0},
    {Homestead::WearableDefinition::LinenLongShirt, 0, TEXT("SKM_LinenLongShirt"), 0},
    {Homestead::WearableDefinition::Trousers, 1, TEXT("SKM_WoolTrousers"), 0},
    {Homestead::WearableDefinition::FurCoat, 2, TEXT("SKM_FurCoat"), 0},
    {Homestead::WearableDefinition::FurBoots, 3, TEXT("SKM_FurBoots"), 1.2f},
    {Homestead::WearableDefinition::WovenSandals, 3, TEXT("SKM_WovenSandals"), 1.05f},
    {Homestead::WearableDefinition::TurnShoes, 3, TEXT("SKM_TurnShoes"), 0.5f},
};
const FMetaHumanGarmentSpec* FindMetaHumanGarment(int32 Definition)
{
    for (const auto& Spec : MetaHumanGarmentSpecs)
        if (static_cast<int32>(Spec.Definition) == Definition) return &Spec;
    return nullptr;
}
int32 MetaHumanGarmentSlot(Homestead::WearableDefinition Definition)
{
    const auto* Spec = FindMetaHumanGarment(static_cast<int32>(Definition));
    return Spec ? Spec->Slot : INDEX_NONE;
}
// Shows or hides every section of one material slot, on every LOD.
void ShowMaterialSlot(USkeletalMeshComponent& Component, FName SlotName, bool bShow)
{
    USkeletalMesh* Mesh = Component.GetSkeletalMeshAsset();
    FSkeletalMeshRenderData* Render = Mesh ? Mesh->GetResourceForRendering() : nullptr;
    if (!Render) return;
    const int32 Material = Mesh->GetMaterials().IndexOfByPredicate(
        [SlotName](const FSkeletalMaterial& Slot) { return Slot.MaterialSlotName == SlotName; });
    if (Material == INDEX_NONE) return;
    for (int32 Lod = 0; Lod < Render->LODRenderData.Num(); ++Lod)
    {
        const auto& Sections = Render->LODRenderData[Lod].RenderSections;
        for (int32 Section = 0; Section < Sections.Num(); ++Section)
            if (Sections[Section].MaterialIndex == Material)
                Component.ShowMaterialSection(Material, Section, bShow, Lod);
    }
}
}

const USkeletalMesh* AHomesteadCharacter::MetaHumanGarmentMesh(int32 Slot) const
{
    const USkeletalMeshComponent* Garment = MetaHumanGarments.IsValidIndex(Slot) ? MetaHumanGarments[Slot].Get() : nullptr;
    return Garment && Garment->IsVisible() ? Garment->GetSkeletalMeshAsset() : nullptr;
}

void AHomesteadCharacter::ApplyMetaHumanGarments()
{
    if (!bMetaHumanActive || !MetaHumanOutfit || MetaHumanGarments.IsEmpty()) return;
    FootwearLift = 0;
    for (int32 Slot = 0; Slot < MetaHumanGarments.Num(); ++Slot)
    {
        USkeletalMeshComponent* Garment = MetaHumanGarments[Slot];
        const FMetaHumanGarmentSpec* Spec = MetaHumanWorn.IsValidIndex(Slot) ? FindMetaHumanGarment(MetaHumanWorn[Slot]) : nullptr;
        USkeletalMesh* Fitted = Spec ? LoadMetaHumanAsset<USkeletalMesh>(
            FString::Printf(TEXT("Assembled/Heroine/Garments/%s"), Spec->Asset)) : nullptr;
        if (Spec && (!Fitted || Fitted->GetSkeleton() != MetaHumanBody->GetSkeleton()))
        {
            UE_LOG(LogTemp, Warning, TEXT("MetaHuman garment %s is not imported; she wears the base layer there."), Spec->Asset);
            Fitted = nullptr;
        }
        if (Garment->GetSkeletalMeshAsset() != Fitted)
        {
            Garment->EmptyOverrideMaterials();
            Garment->SetSkeletalMesh(Fitted, false);
            if (Fitted) Garment->SetLeaderPoseComponent(GetMesh(), true);
        }
        Garment->SetVisibility(Fitted != nullptr);
        if (Fitted && Spec) FootwearLift = FMath::Max(FootwearLift, Spec->LiftCm);
    }
    // The homespun tank top and shorts stay on as the base layer, hidden where a garment covers them.
    ShowMaterialSlot(*MetaHumanOutfit, TEXT("M_PrimitiveTankTop"), MetaHumanGarmentMesh(0) == nullptr);
    ShowMaterialSlot(*MetaHumanOutfit, TEXT("M_PrimitiveShorts"), MetaHumanGarmentMesh(1) == nullptr);
    GetMesh()->SetRelativeLocation(FVector(0, 0, FootwearLift - GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
}

void AHomesteadCharacter::ApplyMetaHumanLook()
{
    if (!MetaHumanHair || !MetaHumanLook.IsValid()) return;
    const int32 Style = MetaHumanLook.MetaHair;
    if (Style != AppliedMetaHair)
    {
        // Style 0 is the assembled groom; the rest are stock MetaHuman grooms copied into the
        // project and bound to her face (homestead_agent.metahuman_hair).
        const FString Groom = HomesteadLook::MetaHairGroom(Style);
        const FString Base = Style == 0 ? FString::Printf(TEXT("Assembled/Heroine/Grooms/%s"), *Groom)
            : FString::Printf(TEXT("Common/Optional/Grooms/GroomAssets/Hair/%s/%s"), *Groom, *Groom);
        auto* Asset = LoadMetaHumanAsset<UGroomAsset>(Base);
        auto* Binding = LoadMetaHumanAsset<UGroomBindingAsset>(Base + TEXT("_Binding"));
        if (!Asset || !Binding)
        {
            UE_LOG(LogTemp, Error, TEXT("MetaHuman hairstyle %s is unavailable; keeping the current one."), *Groom);
            return;
        }
        MetaHumanHair->SetGroomAsset(Asset, Binding);
        AppliedMetaHair = Style;
        bHairHasLastHead = false;
    }
    // Every groom's slots share the heroine's hair materials; the pigment follows her hair colour.
    const TPair<const TCHAR*, const TCHAR*> Slots[] = {
        {TEXT("MI_Hair"), TEXT("MI_WI_Hair_L_Straight_Hair")},
        {TEXT("MI_Hair_Cards"), TEXT("MI_WI_Hair_L_Straight_Hair_Cards")},
        {TEXT("MI_Hair_Helmet"), TEXT("MI_WI_Hair_L_Straight_Hair_Helmet")}};
    const FVector2D Pigment = HomesteadLook::HairPigment(MetaHumanLook.HairColor);
    for (const auto& Slot : Slots)
    {
        const int32 Index = MetaHumanHair->GetMaterialIndex(Slot.Key);
        if (Index == INDEX_NONE) continue;
        auto* Material = LoadMetaHumanAsset<UMaterialInterface>(FString::Printf(TEXT("Assembled/Heroine/Grooms/%s"), Slot.Value));
        if (!Material) continue;
        auto* Dynamic = UMaterialInstanceDynamic::Create(Material, MetaHumanHair);
        Dynamic->SetScalarParameterValue(TEXT("hairMelanin"), Pigment.X);
        Dynamic->SetScalarParameterValue(TEXT("hairRedness"), Pigment.Y);
        MetaHumanHair->SetMaterial(Index, Dynamic);
    }
    for (UGroomComponent* Groom : MetaHumanGrooms)
    {
        if (!Groom || Groom == MetaHumanHair || Groom->GetName() != TEXT("MetaHumanEyebrows")) continue;
        for (int32 Index = 0; Index < Groom->GetNumMaterials(); ++Index)
            if (UMaterialInterface* Material = Groom->GetMaterial(Index))
            {
                auto* Dynamic = Cast<UMaterialInstanceDynamic>(Material);
                if (!Dynamic) Dynamic = Groom->CreateDynamicMaterialInstance(Index, Material);
                if (Dynamic)
                {
                    Dynamic->SetScalarParameterValue(TEXT("hairMelanin"), Pigment.X);
                    Dynamic->SetScalarParameterValue(TEXT("hairRedness"), Pigment.Y);
                }
            }
    }
    ApplyMetaHumanSkinAndEyes();
}

namespace
{
// Post-bake multiply on the baked MetaHuman skin: natural, warm, deep, light.
FLinearColor MetaHumanSkinMultiply(int32 Tone)
{
    static const FLinearColor Values[] = {FLinearColor::White, FLinearColor(0.9f, 0.76f, 0.6f),
        FLinearColor(0.45f, 0.31f, 0.21f), FLinearColor(1.06f, 1.03f, 1.01f)};
    return Values[FMath::Clamp(Tone, 0, 3)];
}
struct FMetaHumanIris { float PrimaryHue, PrimaryValue, SecondaryHue, SecondaryValue, Saturation; };
// Custom MetaHuman iris (MI_Eye*_Homestead, "Use Custom Iris" on). Hue runs blue (0) through
// green (0.5) to amber-brown (0.9): blue, green, hazel, grey.
FMetaHumanIris MetaHumanIris(int32 Eye)
{
    static const FMetaHumanIris Values[] = {
        {0.1f, 0.8f, 0.15f, 0.5f, 1.2f}, {0.5f, 0.7f, 0.45f, 0.45f, 1.25f},
        {0.92f, 0.55f, 0.62f, 0.45f, 1.2f}, {0.2f, 0.7f, 0.2f, 0.5f, 0.0f}};
    return Values[FMath::Clamp(Eye, 0, 3)];
}
UMaterialInstanceDynamic* DynamicFrom(UMeshComponent& Component, int32 Index, UMaterialInterface* Parent)
{
    UMaterialInterface* Current = Component.GetMaterial(Index);
    auto* Dynamic = Cast<UMaterialInstanceDynamic>(Current);
    if (Dynamic && (!Parent || Dynamic->Parent == Parent)) return Dynamic;
    Dynamic = UMaterialInstanceDynamic::Create(Parent ? Parent : Current, &Component);
    Component.SetMaterial(Index, Dynamic);
    return Dynamic;
}
}

void AHomesteadCharacter::ApplyMetaHumanSkinAndEyes()
{
    if (!MetaHumanFace || !MetaHumanLook.IsValid()) return;
    const FLinearColor Skin = MetaHumanSkinMultiply(MetaHumanLook.SkinTone);
    const auto TintSkin = [&Skin](UMeshComponent& Component, int32 Index)
    {
        if (!Component.GetMaterial(Index)) return;
        if (auto* Dynamic = DynamicFrom(Component, Index, nullptr))
            Dynamic->SetVectorParameterValue(TEXT("Basecolor Global Multiply Post-Bake"), Skin);
    };
    const TArray<FName> FaceSlots = MetaHumanFace->GetMaterialSlotNames();
    for (int32 Index = 0; Index < FaceSlots.Num(); ++Index)
        if (FaceSlots[Index].ToString().StartsWith(TEXT("head_"))) TintSkin(*MetaHumanFace, Index);
    if (USkeletalMeshComponent* Body = GetMesh(); Body && Body->GetSkeletalMeshAsset() == MetaHumanBody)
        for (int32 Index = 0; Index < Body->GetNumMaterials(); ++Index) TintSkin(*Body, Index);

    // The baked eye materials ignore iris colour; MI_Eye*_Homestead switch to the procedural iris.
    const FMetaHumanIris Iris = MetaHumanIris(MetaHumanLook.EyeColor);
    const TPair<const TCHAR*, const TCHAR*> Eyes[] = {
        {TEXT("eyeLeft_shader_shader"), TEXT("Assembled/Heroine/Face/Materials/MI_EyeL_Homestead")},
        {TEXT("eyeRight_shader_shader"), TEXT("Assembled/Heroine/Face/Materials/MI_EyeR_Homestead")}};
    for (const auto& Eye : Eyes)
    {
        const int32 Index = MetaHumanFace->GetMaterialIndex(Eye.Key);
        auto* Parent = Index == INDEX_NONE ? nullptr : LoadMetaHumanAsset<UMaterialInterface>(Eye.Value);
        if (!Parent) continue;
        auto* Dynamic = DynamicFrom(*MetaHumanFace, Index, Parent);
        Dynamic->SetScalarParameterValue(TEXT("Iris Primary Color Hue"), Iris.PrimaryHue);
        Dynamic->SetScalarParameterValue(TEXT("Iris Primary Color Value"), Iris.PrimaryValue);
        Dynamic->SetScalarParameterValue(TEXT("Iris Secondary Color Hue"), Iris.SecondaryHue);
        Dynamic->SetScalarParameterValue(TEXT("Iris Secondary Color Value"), Iris.SecondaryValue);
        Dynamic->SetScalarParameterValue(TEXT("Iris Global Saturation"), Iris.Saturation);
    }
}

bool AHomesteadCharacter::ApplyAppearance(const FHomesteadAppearance& Appearance)
{
    if (ActiveEquipment.Ready)
    {
        UE_LOG(LogTemp, Error, TEXT("Owned wardrobe appearance requires PrepareEquipment with authoritative state."));
        return false;
    }
    if (!Appearance.IsValid() || !LoadHeroineAssets())
    {
        UE_LOG(LogTemp, Error, TEXT("Appearance change rejected: invalid selection or unavailable assets."));
        return false;
    }
    USkeletalMesh* Desired = WardrobeMeshes[Appearance.BodyPreset * 6 + Appearance.Outfit * 3 + Appearance.HairStyle].Get();
    for (const auto& Slot : Desired->GetMaterials())
    {
        const FString Name = Slot.MaterialSlotName.ToString();
        const UMaterialInterface* Surface = Slot.MaterialInterface;
        if (!Surface) { UE_LOG(LogTemp, Error, TEXT("Character material %s is missing."), *Name); return false; }
        TArray<FMaterialParameterInfo> VectorParameters;
        TArray<FGuid> VectorIds;
        Surface->GetAllVectorParameterInfo(VectorParameters, VectorIds);
        auto HasVector = [&VectorParameters](FName Parameter)
        {
            return VectorParameters.ContainsByPredicate([Parameter](const FMaterialParameterInfo& Info) { return Info.Name == Parameter; });
        };
        const bool NeedsTint = Name == TEXT("M_Heroine_Skin") || Name == TEXT("M_Heroine_Eyebrows")
            || Name == TEXT("M_Heroine_MossLinen") || Name.StartsWith(TEXT("M_Heroine_Hair_"));
        if (NeedsTint && !HasVector(TEXT("ColorTint")))
        {
            UE_LOG(LogTemp, Error, TEXT("Character material %s has no ColorTint parameter. Reimport its material contract."), *Name);
            return false;
        }
        if (Name == TEXT("M_Heroine_LightEyes"))
        {
            TArray<FMaterialParameterInfo> ScalarParameters;
            TArray<FGuid> ScalarIds;
            Surface->GetAllScalarParameterInfo(ScalarParameters, ScalarIds);
            if (!HasVector(TEXT("IrisColor"))
                || !ScalarParameters.ContainsByPredicate([](const FMaterialParameterInfo& Info) { return Info.Name == TEXT("IrisMix"); }))
            {
                UE_LOG(LogTemp, Error, TEXT("Character eye material is missing its iris-only color controls."));
                return false;
            }
        }
    }
    CancelAction(true);
    MetaHumanLook = Appearance;
    if (UsesMetaHumanHeroine()) return ApplyMetaHumanStack();
    USkeletalMeshComponent* VisualMesh = GetMesh();
    if (VisualMesh->GetSkeletalMeshAsset() != Desired)
    {
        VisualMesh->EmptyOverrideMaterials();
        VisualMesh->SetSkeletalMesh(Desired);
        VisualMesh->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
        VisualMesh->SetRelativeRotation(FRotator(0, InferMeshYaw(*Desired), 0));
        VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        VisualMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        VisualMesh->SetAnimInstanceClass(UHomesteadAnimInstance::StaticClass());
        AppearanceMaterials.Reset();
        for (int32 Index = 0; Index < VisualMesh->GetNumMaterials(); ++Index)
            AppearanceMaterials.Add(VisualMesh->CreateDynamicMaterialInstance(Index));
    }
    const TArray<FName> Names = VisualMesh->GetMaterialSlotNames();
    for (int32 Index = 0; Index < Names.Num() && Index < AppearanceMaterials.Num(); ++Index)
    {
        auto* Material = AppearanceMaterials[Index].Get();
        if (!Material) { UE_LOG(LogTemp, Error, TEXT("Heroine material slot %d is unavailable."), Index); return false; }
        const FString Name = Names[Index].ToString();
        if (Name == TEXT("M_Heroine_Skin"))
            Material->SetVectorParameterValue(TEXT("ColorTint"), HomesteadLook::SkinTint(Appearance.SkinTone));
        else if (Name == TEXT("M_Heroine_Hair_long01_Neutral") || Name == TEXT("M_Heroine_Hair_bob01_Neutral"))
            Material->SetVectorParameterValue(TEXT("ColorTint"), HomesteadLook::NeutralHairTint(Appearance.HairColor));
        else if (Name.StartsWith(TEXT("M_Heroine_Hair_")) || Name == TEXT("M_Heroine_Eyebrows"))
            Material->SetVectorParameterValue(TEXT("ColorTint"), HomesteadLook::HairTint(Appearance.HairColor));
        else if (Name == TEXT("M_Heroine_MossLinen"))
            Material->SetVectorParameterValue(TEXT("ColorTint"), HomesteadLook::TunicTint(Appearance.TunicColor));
        else if (Name == TEXT("M_Heroine_LightEyes"))
        {
            Material->SetVectorParameterValue(TEXT("IrisColor"), HomesteadLook::IrisColor(Appearance.EyeColor));
            Material->SetScalarParameterValue(TEXT("IrisMix"), Appearance.EyeColor == 0 ? 0.0f : 1.0f);
        }
    }
    StandIn->SetVisibility(false);
    VisualMesh->SetVisibility(true);
    bHeroineReady = true;
    return true;
}

bool AHomesteadCharacter::PrepareEquipment(const Homestead::State& CandidateState,
    const FHomesteadAppearance& Look, FString& Error)
{
    ClearPreparedEquipment();
    PendingMetaHumanLook = Look;
    PendingMetaHumanWorn.Init(INDEX_NONE, 4);
    for (const auto& Item : CandidateState.wearables)
        if (Item.owner == Homestead::WearableOwner::Equipped)
            if (const int32 Slot = MetaHumanGarmentSlot(Item.definition); Slot != INDEX_NONE)
                PendingMetaHumanWorn[Slot] = static_cast<int32>(Item.definition);
    if (!LoadHeroineAssets())
    {
        Error = TEXT("Original heroine skeleton or animations are unavailable.");
        UE_LOG(LogTemp, Error, TEXT("Wardrobe preparation failed: %s"), *Error);
        return false;
    }
    return HomesteadWardrobePresentation::Prepare(this, CandidateState, Look,
        *LongHairMesh, PreparedEquipment, Error);
}

void AHomesteadCharacter::ClearPreparedEquipment()
{
    PreparedEquipment = {};
}

bool AHomesteadCharacter::ApplyPreparedEquipment(FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || !PreparedEquipment.Ready)
    {
        Error = TEXT("No retained wardrobe presentation is ready to apply.");
        UE_LOG(LogTemp, Error, TEXT("Wardrobe apply rejected: %s"), *Error);
        return false;
    }
    CancelAction(true);
    if (UsesMetaHumanHeroine())
    {
        // The trial keeps the authoritative equipment record but presents the MetaHuman's own outfit.
        ActiveEquipment = MoveTemp(PreparedEquipment);
        ClearPreparedEquipment();
        MetaHumanLook = PendingMetaHumanLook;
        MetaHumanWorn = PendingMetaHumanWorn;
        if (ApplyMetaHumanStack()) return true;
        Error = TEXT("MetaHuman heroine assets are unavailable.");
        return false;
    }
    USkeletalMeshComponent* VisualMesh = GetMesh();
    HomesteadWardrobePresentation::ApplySurface(PreparedEquipment.Base, *VisualMesh);
    VisualMesh->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
    VisualMesh->SetRelativeRotation(FRotator(0, InferMeshYaw(*PreparedEquipment.Base.Mesh), 0));
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    if (!VisualMesh->GetAnimInstance() || VisualMesh->GetAnimInstance()->GetClass() != UHomesteadAnimInstance::StaticClass())
        VisualMesh->SetAnimInstanceClass(UHomesteadAnimInstance::StaticClass());
    const int32 Slots[] = {0, 2, 3};
    for (int32 Index = 0; Index < GarmentComponents.Num(); ++Index)
    {
        USkeletalMeshComponent* Component = GarmentComponents[Index];
        const auto* Surface = PreparedEquipment.Garments.FindByPredicate(
            [Slot = Slots[Index]](const FHomesteadEquipmentSurface& Item) { return Item.Slot == Slot; });
        if (Surface)
        {
            HomesteadWardrobePresentation::ApplySurface(*Surface, *Component);
            Component->SetLeaderPoseComponent(VisualMesh, true, false);
            Component->SetVisibility(true);
        }
        else
        {
            Component->SetVisibility(false);
            Component->SetLeaderPoseComponent(nullptr);
            Component->EmptyOverrideMaterials();
            Component->SetSkeletalMesh(nullptr);
        }
    }
    AppearanceMaterials.Reset();
    ActiveEquipment = MoveTemp(PreparedEquipment);
    ClearPreparedEquipment();
    StandIn->SetVisibility(false);
    VisualMesh->SetVisibility(true);
    bHeroineReady = true;
    return true;
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
    bSprintActive = bSprintHeld && !Blocked && Moving
        && (Lab || PC->State().energy > 10.0) && SprintAnimation != nullptr;
    Movement->MaxWalkSpeed = bSprintActive ? SprintSpeed() : WalkSpeed();
    if (bSprintActive && !Lab)
    {
        const auto Result = PC->SpendSprintEnergy(DeltaSeconds);
        if (!Result.ok || PC->State().energy <= 10.0)
        {
            if (!Result.ok && PC->State().energy > 10.0)
                UE_LOG(LogTemp, Error, TEXT("Sprint Energy update failed: %s"),
                    UTF8_TO_TCHAR(Result.message.c_str()));
            CancelSprint();
        }
    }
    if (bAppearancePreview) UpdateAppearanceFraming();
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
    }
    UpdateStickAlignment(DeltaSeconds);
    UpdateHairMotion(DeltaSeconds);
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

FRotator AHomesteadCharacter::ChooseStartingView(const AHomesteadWorld& Landscape, FRotator Preferred)
{
    const FVector Origin = GetActorLocation() + CameraArm->TargetOffset;
    const FVector Focus = GetActorLocation() + FVector(0, 0, 45);
    int32 BestScore = MAX_int32, InitialObstructions = -1, BestObstructions = -1;
    FRotator Best = Preferred;
    for (int32 Index = 0; Index < 24; ++Index)
    {
        const int32 Offset = Index == 0 ? 0 : (Index % 2 ? (Index + 1) / 2 : -Index / 2);
        const FRotator Rotation(Preferred.Pitch, Preferred.Yaw + Offset * 15, 0);
        const FVector Desired = Origin - Rotation.Vector() * CameraArm->TargetArmLength
            + FRotationMatrix(Rotation).TransformVector(CameraArm->SocketOffset);
        FHitResult Hit;
        const FCollisionQueryParams Query(SCENE_QUERY_STAT(HomesteadStartingView), false, this);
        const bool Blocked = GetWorld()->SweepSingleByChannel(Hit, Origin, Desired, FQuat::Identity,
            CameraArm->ProbeChannel, FCollisionShape::MakeSphere(CameraArm->ProbeSize), Query);
        const FVector CameraPosition = Blocked ? Hit.Location : Desired;
        const int32 Obstructions = Landscape.StartingViewObstructions(Focus, CameraPosition);
        if (Index == 0) InitialObstructions = Obstructions;
        const int32 Score = Obstructions * 1000
            + FMath::RoundToInt(FMath::Max(0.0, 250.0 - FVector::Dist(Origin, CameraPosition)));
        if (Score < BestScore)
        {
            BestScore = Score;
            BestObstructions = Obstructions;
            Best = Rotation;
        }
    }
    InitialViewEvidence = FString::Printf(
        TEXT("fresh_start=1 preferred_yaw=%.1f chosen_yaw=%.1f bounds_hits_before=%d bounds_hits_after=%d; bounds heuristic, not pixel visibility"),
        Preferred.Yaw, Best.Yaw, InitialObstructions, BestObstructions);
    return Best;
}

void AHomesteadCharacter::PlayGather()
{
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestGather();
    else
        UE_LOG(LogTemp, Error, TEXT("Gather succeeded but the heroine gathering animation instance is unavailable."));
}

UStaticMesh* AHomesteadCharacter::LoadHandStone(int32 Index)
{
    static const TCHAR* Names[] = {TEXT("SM_HandStone_A"), TEXT("SM_HandStone_B"), TEXT("SM_HandStone_C")};
    if (Index < 0 || Index >= UE_ARRAY_COUNT(Names)) return nullptr;
    const FString Path = FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/HandStones/%s.%s"), Names[Index], Names[Index]);
    return LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

bool AHomesteadCharacter::PlayGatherSticks(TOptional<FVector2D> Pile)
{
    return PlayKneelGather(EHomesteadKneelGather::Sticks, Pile);
}

bool AHomesteadCharacter::PlayKneelGather(EHomesteadKneelGather Kind, TOptional<FVector2D> Pile, bool bBerries)
{
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    KneelKind = Kind;
    bForageBerries = bBerries;
    const bool bPropsReady = Kind == EHomesteadKneelGather::Sticks ? CarriedSticks.Num() >= 2
        : Kind == EHomesteadKneelGather::Stones ? CarriedStones.Num() >= 2
        : Kind == EHomesteadKneelGather::Reeds ? CarriedReeds && GetHeldProp(Homestead::Item::Knife)
        : Kind == EHomesteadKneelGather::Plant ? CarriedSeed != nullptr
        : CarriedForage != nullptr;
    if (!GetGatherSticksAnimation() || !bPropsReady || !Animation)
    {
        const bool bQuiet = Kind == EHomesteadKneelGather::Reeds || Kind == EHomesteadKneelGather::Plant;
        KneelKind = EHomesteadKneelGather::Sticks;
        if (!bQuiet) PlayGather();
        return false;
    }
    if (Kind == EHomesteadKneelGather::Pouch)
    {
        UStaticMesh* ForageMesh = bBerries ? ForageBerryMesh.Get() : ForageRootMesh.Get();
        if (ForageMesh) CarriedForage->SetStaticMesh(ForageMesh);
        else
        {
            // Placeholder until the authored props are imported: a berry-red or root-brown ball.
            CarriedForage->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
            if (auto* Tint = CarriedForage->CreateDynamicMaterialInstance(0,
                LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"))))
                Tint->SetVectorParameterValue(TEXT("Color"), bBerries ? FLinearColor(0.42f, 0.025f, 0.055f) : FLinearColor(0.65f, 0.43f, 0.19f));
        }
    }
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    StickStage = 0;
    bStickPileOnGround = true;
    bStickGatherStarted = false;
    SticksLifted = 0;
    StickAlignRemaining = 0;
    if (Pile)
    {
        // Her right hand picks both sticks up about 32 cm ahead and 26 cm to her right; reed stems
        // are gathered 34 cm ahead and 10 cm to her right, beside the forward knee (kneel_reeds.STEMS). Turn and settle her during the
        // first step so that spot lands on the pile.
        // Her forefinger presses the seed in 32 cm ahead and 9 cm to her right (kneel_plant.SPOT, as baked).
        const bool bReeds = Kind == EHomesteadKneelGather::Reeds, bPlant = Kind == EHomesteadKneelGather::Plant;
        const float GrabForward = bReeds ? 34.0f : bPlant ? 32.0f : 32.0f;
        const float GrabRight = bReeds ? 10.0f : bPlant ? 9.0f : 26.0f;
        const FVector Here = GetActorLocation();
        const FVector2D ToPile = *Pile - FVector2D(Here);
        if (ToPile.Size() > 1.0f && ToPile.Size() < 150.0f)
        {
            const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(ToPile.Y, ToPile.X))
                - FMath::RadiansToDegrees(FMath::Atan2(GrabRight, GrabForward));
            const FVector Desired = FVector(Pile->X, Pile->Y, Here.Z)
                - FRotator(0, Yaw, 0).RotateVector(FVector(GrabForward, GrabRight, 0));
            StickAlignFrom = GetActorTransform();
            StickAlignTo = FTransform(FRotator(0, Yaw, 0), Desired);
            StickAlignRemaining = StickAlignSeconds;
        }
    }
    Animation->RequestGatherSticks();
    return true;
}

void AHomesteadCharacter::UpdatePouchSwing()
{
    USkeletalMeshComponent* Body = GetMesh();
    if (!ForagePouch || !Body || !ForagePouch->IsVisible()) return;
    const TArray<FTransform>& Pose = Body->GetComponentSpaceTransforms();
    const int32 Pelvis = Body->GetBoneIndex(TEXT("pelvis"));
    const int32 Thigh = Body->GetBoneIndex(TEXT("thigh_r"));
    const int32 Calf = Body->GetBoneIndex(TEXT("calf_r"));
    if (!Pose.IsValidIndex(Pelvis) || !Pose.IsValidIndex(Thigh) || !Pose.IsValidIndex(Calf)) return;
    // The thigh's direction as her pelvis sees it, in the reference pose's frame.
    const FVector Dir = PelvisRefPose.TransformVectorNoScale(Pose[Pelvis].InverseTransformVectorNoScale(
        Pose[Calf].GetLocation() - Pose[Thigh].GetLocation())).GetSafeNormal();
    const auto Swing = [this, &Dir](const FVector& Axis, float Alpha, float MinDegrees, float MaxDegrees)
    {
        const FVector From = FVector::VectorPlaneProject(ThighDirRef, Axis).GetSafeNormal();
        const FVector To = FVector::VectorPlaneProject(Dir, Axis).GetSafeNormal();
        if (From.IsNearlyZero() || To.IsNearlyZero()) return FQuat::Identity;
        const float Angle = FMath::Atan2(FVector::DotProduct(FVector::CrossProduct(From, To), Axis), FVector::DotProduct(From, To));
        return FQuat(Axis, FMath::Clamp(Angle * Alpha, FMath::DegreesToRadians(MinDegrees), FMath::DegreesToRadians(MaxDegrees)));
    };
    // Forward and back it pivots on the belt (the thigh swings under it about the same lateral
    // axis, so it stays flush); out and in it rides the thigh about the hip.
    const FQuat Flex = Swing(FVector::XAxisVector, 0.9f, -40.0f, 55.0f);
    const FQuat Abduct = Swing(FVector::YAxisVector, 1.0f, -20.0f, 20.0f);
    const FTransform AboutHip(Abduct, HipRef - Abduct.RotateVector(HipRef));
    const FTransform Placed = FTransform(Flex, PouchPivotRef) * AboutHip;
    ForagePouch->SetRelativeTransform(Placed.GetRelativeTransform(PelvisRefPose));
}

void AHomesteadCharacter::UpdateHairMotion(float DeltaSeconds)
{
    if (!MetaHumanHair) return;
    // Strands left over from a hitch, a pause or a snap of the head fly out stiff; start them fresh instead.
    // Actors don't tick while paused, so a pause shows up as a gap in real time between ticks.
    const double Now = GetWorld()->GetRealTimeSeconds();
    const FTransform Head = MetaHumanFace ? MetaHumanFace->GetSocketTransform(TEXT("head")) : GetActorTransform();
    bool bReset = false;
    if (bHairHasLastHead)
    {
        const float Step = FVector::Dist(Head.GetLocation(), HairLastHead.GetLocation());
        const float Turn = FMath::RadiansToDegrees(Head.GetRotation().AngularDistance(HairLastHead.GetRotation()));
        bReset = DeltaSeconds > 0.1f || Now - HairLastRealTime > 0.25 || Step > 25.0f || Turn > 40.0f;
    }
    if (bReset)
    {
        MetaHumanHair->ResetSimulation();
        UE_LOG(LogTemp, Verbose, TEXT("Homestead hair simulation reset"));
    }
    HairLastHead = Head;
    HairLastRealTime = Now;
    bHairHasLastHead = true;
    const float Walk = WalkSpeed(), Sprint = SprintSpeed();
    float Target = FMath::Clamp((GetVelocity().Size2D() - Walk) / FMath::Max(Sprint - Walk, 1.0f), 0.0f, 1.0f);
    // Swinging an axe or machete throws her head and shoulders about as much as a sprint does.
    if (const auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Target = FMath::Max(Target, 0.5f * FMath::Max(Animation->FellWeight(), Animation->MacheteWeight()));
    HairSprintBlend = FMath::FInterpTo(HairSprintBlend, Target, DeltaSeconds, 3.0f);
    auto& Setup = MetaHumanHair->SimulationSettings.SimulationSetup;
    Setup.LinearVelocityScale = FMath::Lerp(CVarHairLinearWalk.GetValueOnGameThread(),
        CVarHairLinearSprint.GetValueOnGameThread(), HairSprintBlend);
    Setup.AngularVelocityScale = FMath::Lerp(CVarHairAngularWalk.GetValueOnGameThread(),
        CVarHairAngularSprint.GetValueOnGameThread(), HairSprintBlend);
}

void AHomesteadCharacter::UpdateStickAlignment(float DeltaSeconds)
{
    if (StickAlignRemaining <= 0) return;
    StickAlignRemaining = FMath::Max(0.0f, StickAlignRemaining - DeltaSeconds);
    const float Alpha = FMath::SmoothStep(0.0f, 1.0f, 1.0f - StickAlignRemaining / StickAlignSeconds);
    const FVector Location = FMath::Lerp(StickAlignFrom.GetLocation(), StickAlignTo.GetLocation(), Alpha);
    const FQuat Rotation = FQuat::Slerp(StickAlignFrom.GetRotation(), StickAlignTo.GetRotation(), Alpha);
    SetActorLocationAndRotation(FVector(Location.X, Location.Y, GetActorLocation().Z), Rotation, true);
}

// Stick moments in AN_HeroineMH_KneelGatherSticks (seconds; homestead_agent.kneel_gather STICK_EVENTS).
// Stones share this clip.
namespace GatherSticksTiming
{
constexpr float Pick1 = 38.0f / 30.0f, Stack1 = 54.0f / 30.0f, Pick2 = 70.0f / 30.0f, Stack2 = 86.0f / 30.0f,
    Stow = 112.0f / 30.0f;
}
// Moments in AN_HeroineMH_KneelGatherPouch (seconds; homestead_agent.kneel_pouch POUCH_EVENTS).
namespace GatherPouchTiming
{
constexpr float Pick1 = 36.0f / 30.0f, Stow1 = 56.0f / 30.0f, Pick2 = 74.0f / 30.0f, Stow2 = 94.0f / 30.0f;
}
// Moments in AN_HeroineMH_KneelCutReeds (seconds; homestead_agent.kneel_reeds EVENTS): her left
// fist closes on the stems, the knife cuts them free.
namespace GatherReedsTiming
{
constexpr float Grab = 38.0f / 30.0f, Cut = 72.0f / 30.0f;
}

// Moments in AN_HeroineMH_KneelPlant (seconds; homestead_agent.kneel_plant EVENTS).
namespace GatherPlantTiming
{
constexpr float Pick = 36.0f / 30.0f, Press = 58.0f / 30.0f, Covered = 102.0f / 30.0f;
}

bool AHomesteadCharacter::PlayPlant(Homestead::Point Target)
{
    return PlayKneelGather(EHomesteadKneelGather::Plant, FVector2D(Target.x, Target.y));
}

bool AHomesteadCharacter::IsCuttingReeds() const
{
    const auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    return KneelKind == EHomesteadKneelGather::Reeds && Animation && Animation->IsGatheringSticks();
}

void AHomesteadCharacter::UpdateCarriedSticks()
{
    const bool bPouch = KneelKind == EHomesteadKneelGather::Pouch;
    const bool bStones = KneelKind == EHomesteadKneelGather::Stones;
    const bool bReeds = KneelKind == EHomesteadKneelGather::Reeds;
    const bool bPlant = KneelKind == EHomesteadKneelGather::Plant;
    const auto& Props = bStones ? CarriedStones : CarriedSticks;
    if (bPlant ? !CarriedSeed : bReeds ? !CarriedReeds : bPouch ? !CarriedForage : Props.Num() < 2) return;
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    const bool Active = Animation && Animation->IsGatheringSticks();
    const float Time = Active ? Animation->GatherSticksPhase() : 0.0f;
    // Reeds come off the clump all at once, with the cut.
    // Reeds come off the clump all at once, with the cut; a planted square stays bare until covered.
    const float Pick1 = bPlant ? GatherPlantTiming::Covered : bReeds ? GatherReedsTiming::Cut
        : bPouch ? GatherPouchTiming::Pick1 : GatherSticksTiming::Pick1;
    const float Pick2 = bPlant ? GatherPlantTiming::Covered : bReeds ? GatherReedsTiming::Cut
        : bPouch ? GatherPouchTiming::Pick2 : GatherSticksTiming::Pick2;
    if (bReeds && Animation)
        Animation->SetLeftHandGrip(Active && Time >= GatherReedsTiming::Grab - 0.1f ? 1.0f : 0.0f);
    int32 Stage = 0;
    if (bPlant) Stage = Active && Time >= GatherPlantTiming::Pick && Time < GatherPlantTiming::Press ? 1 : 0;
    else if (bReeds) Stage = Active && Time >= GatherReedsTiming::Cut ? 1 : 0;
    else if (bPouch)
    {
        using namespace GatherPouchTiming;
        Stage = !Active ? 0 : Time >= Stow2 ? 4 : Time >= Pick2 ? 3 : Time >= Stow1 ? 2 : Time >= Pick1 ? 1 : 0;
    }
    else
    {
        using namespace GatherSticksTiming;
        Stage = !Active || Time >= Stow ? 0 : Time >= Stack2 ? 4 : Time >= Pick2 ? 3 : Time >= Stack1 ? 2 : Time >= Pick1 ? 1 : 0;
    }
    bStickGatherStarted |= Active;
    if (Active) SticksLifted = Time >= Pick2 ? 2 : Time >= Pick1 ? 1 : 0;
    // The ground produce leaves with the second pickup (or if the gather ends early).
    if (bStickPileOnGround && ((Active && Time >= Pick2) || (bStickGatherStarted && !Active)))
        bStickPileOnGround = false;
    if (Stage == StickStage) return;
    StickStage = Stage;
    USkeletalMeshComponent* Body = GetMesh();
    // Hand frame: fingers, across the knuckles (index -> pinky reversed) and out of the palm.
    const auto HandFrame = [Body](FVector& Hand, FVector& Fingers, FVector& Across, FVector& Palm)
    {
        Hand = Body->GetSocketLocation(TEXT("hand_r"));
        Fingers = (Body->GetSocketLocation(TEXT("middle_01_r")) - Hand).GetSafeNormal();
        Across = (Body->GetSocketLocation(TEXT("index_01_r")) - Body->GetSocketLocation(TEXT("pinky_01_r"))).GetSafeNormal();
        Palm = FVector::CrossProduct(Fingers, Across).GetSafeNormal();
    };
    // Places a prop so its bounds centre (or authored pivot) lands on Centre, attached to Bone.
    const auto Put = [Body](UStaticMeshComponent* Prop, FVector Centre, FRotator Rotation, FVector Scale, FName Bone, bool bCentreBounds)
    {
        Prop->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        const FVector Offset = bCentreBounds ? Rotation.RotateVector(Prop->GetStaticMesh()->GetBounds().Origin * Scale) : FVector::ZeroVector;
        Prop->SetWorldLocationAndRotation(Centre - Offset, Rotation);
        Prop->SetWorldScale3D(Scale);
        Prop->AttachToComponent(Body, FAttachmentTransformRules::KeepWorldTransform, Bone);
        Prop->SetVisibility(true);
    };
    if (bPlant)
    {
        if (Stage != 1)
        {
            CarriedSeed->SetVisibility(false);
            return;
        }
        // Between the pinched thumb and forefinger tips.
        const FVector Index = Body->GetSocketLocation(TEXT("index_03_r"));
        const FVector Thumb = Body->GetSocketLocation(TEXT("thumb_03_r"));
        Put(CarriedSeed, (Index + Thumb) * 0.5f, Body->GetSocketRotation(TEXT("hand_r")), FVector(1.0f), TEXT("hand_r"), false);
        return;
    }
    if (bReeds)
    {
        if (Stage != 1)
        {
            CarriedReeds->SetVisibility(false);
            return;
        }
        // Upright through her left fist: the stems run out of the thumb side, cut ends a hand's
        // width below it, the clump narrowed to a gathered bunch.
        const FVector HandL = Body->GetSocketLocation(TEXT("hand_l"));
        const FVector FingersL = (Body->GetSocketLocation(TEXT("middle_01_l")) - HandL).GetSafeNormal();
        const FVector UpL = (Body->GetSocketLocation(TEXT("index_01_l")) - Body->GetSocketLocation(TEXT("pinky_01_l"))).GetSafeNormal();
        const FVector PalmL = -FVector::CrossProduct(FingersL, UpL).GetSafeNormal();
        const FVector Fist = HandL + FingersL * 7.0f + PalmL * 3.0f;
        Put(CarriedReeds, Fist - UpL * 14.0f, FRotationMatrix::MakeFromZX(UpL, FingersL).Rotator(),
            FVector(0.28f, 0.28f, 0.85f), TEXT("hand_l"), false);
        return;
    }
    if (bPouch)
    {
        if (Stage != 1 && Stage != 3)
        {
            CarriedForage->SetVisibility(false);
            return;
        }
        FVector Hand, Fingers, Across, Palm;
        HandFrame(Hand, Fingers, Across, Palm);
        const bool bAuthored = CarriedForage->GetStaticMesh() == (bForageBerries ? ForageBerryMesh.Get() : ForageRootMesh.Get())
            && CarriedForage->GetStaticMesh() != nullptr;
        // Pinched between thumb and fingers: authored props hang from their pinch pivot, a root
        // points along the fingers.
        const FVector Pinch = Hand + Fingers * 8.0f + Palm * 2.5f;
        const FRotator Rotation = bForageBerries ? FRotationMatrix::MakeFromZX(-Fingers, Across).Rotator()
            : FRotationMatrix::MakeFromZX(Fingers, Across).Rotator();
        const FVector Scale = bAuthored ? FVector(1.0f) : bForageBerries ? FVector(0.045f) : FVector(0.06f, 0.06f, 0.12f);
        Put(CarriedForage, bAuthored ? Pinch : Pinch + (bForageBerries ? FVector::ZeroVector : Fingers * 3.0f), Rotation, Scale,
            TEXT("hand_r"), !bAuthored);
        return;
    }
    // Stones match the woodland pile's components 1 and 2 (StonePileSize).
    const auto StoneScale = [](UStaticMeshComponent* Stone, int32 Index)
    {
        const float Size = Stone->GetStaticMesh()->GetBoundingBox().GetSize().GetMax();
        const bool bHandStone = Stone->GetStaticMesh() == LoadHandStone(Index + 1);
        return FVector(StonePileSize(Index + 1, bHandStone) / FMath::Max(Size, 1.0f));
    };
    // Grip: sticks run across the fingers; a stone sits in the palm.
    auto Grip = [&, this](int32 Index)
    {
        FVector Hand, Fingers, Across, Palm;
        HandFrame(Hand, Fingers, Across, Palm);
        UStaticMeshComponent* Prop = Props[Index];
        if (bStones)
        {
            const FVector Scale = StoneScale(Prop, Index);
            const float Radius = Prop->GetStaticMesh()->GetBoundingBox().GetSize().GetMax() * Scale.X * 0.4f;
            Put(Prop, Hand + Fingers * 6.0f + Palm * (Radius + 1.5f), FRotationMatrix::MakeFromYZ(Across, Palm).Rotator(),
                Scale, TEXT("hand_r"), true);
            return;
        }
        // Branch meshes run along their local Y.
        Put(Prop, Hand + Fingers * 6.0f + Palm * 3.0f, FRotationMatrix::MakeFromYZ(Across, Palm).Rotator(),
            FVector(CarriedStickScale), TEXT("hand_r"), true);
    };
    // Stack: sticks lie level on the left forearm across her body, like carried firewood hugged
    // against the belly; stones nest in the crook of the arm, side by side along the forearm.
    auto Stack = [&, this](int32 Index)
    {
        const FVector Elbow = Body->GetSocketLocation(TEXT("lowerarm_l"));
        const FVector Wrist = Body->GetSocketLocation(TEXT("hand_l"));
        UStaticMeshComponent* Prop = Props[Index];
        if (bStones)
        {
            const FVector Scale = StoneScale(Prop, Index);
            const float Radius = Prop->GetStaticMesh()->GetBoundingBox().GetSize().GetMax() * Scale.X * 0.4f;
            const FVector Centre = FMath::Lerp(Elbow, Wrist, Index == 0 ? 0.3f : 0.62f)
                + FVector(0, 0, Radius + 3.0f) + GetActorForwardVector() * (Index == 0 ? 2.0f : 4.0f);
            Put(Prop, Centre, FRotator(0, GetActorRotation().Yaw + Index * 70.0f, 0), Scale, TEXT("lowerarm_l"), true);
            return;
        }
        const float Height = Index == 0 ? 5.0f : 9.0f, Twist = Index == 0 ? -8.0f : 10.0f;
        const FVector Along = GetActorRightVector().GetSafeNormal2D().RotateAngleAxis(Twist, FVector::UpVector);
        const FVector Centre = FMath::Lerp(Elbow, Wrist, 0.55f) + FVector(0, 0, Height) + GetActorForwardVector() * 3.0f;
        Put(Prop, Centre, FRotationMatrix::MakeFromYZ(Along, FVector::UpVector).Rotator(), FVector(CarriedStickScale),
            TEXT("lowerarm_l"), true);
    };
    if (Stage == 0)
        for (UStaticMeshComponent* Prop : Props) Prop->SetVisibility(false);
    else if (Stage == 1) Grip(0);
    else if (Stage == 2) Stack(0);
    else if (Stage == 3) Grip(1);
    else Stack(1);
}
void AHomesteadCharacter::PlayWater()
{
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    WaterYaw.Reset();
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestWater();
    else
        UE_LOG(LogTemp, Error, TEXT("Watering succeeded but its animation instance is unavailable."));
}

void AHomesteadCharacter::PlayWater(Homestead::Point Target)
{
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    const FVector2D Delta(Target.x - GetActorLocation().X, Target.y - GetActorLocation().Y);
    if (!FMath::IsFinite(Delta.X) || !FMath::IsFinite(Delta.Y) || Delta.SizeSquared() < 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Water committed without a valid presentation target; using heroine facing."));
        PlayWater();
        return;
    }
    WaterYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestWater();
    else
        UE_LOG(LogTemp, Error, TEXT("Watering succeeded but its animation instance is unavailable."));
}

void AHomesteadCharacter::CancelAction(bool Immediate)
{
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->CancelAction(Immediate);
    for (UStaticMeshComponent* Stick : CarriedSticks) Stick->SetVisibility(false);
    StickStage = 0;
    bStickPileOnGround = false;
    StickAlignRemaining = 0;
    WateringTool->SetHiddenInGame(true, true);
    Hatchet->SetHiddenInGame(true, true);
    DiggingStick->SetHiddenInGame(true, true);
    Knife->SetHiddenInGame(true, true);
    ClearYaw.Reset();
    TillYaw.Reset();
    bFellApproach = false;
    WaterYaw.Reset();
}

void AHomesteadCharacter::PlayClear()
{
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    ClearYaw.Reset();
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestClear();
    else
        UE_LOG(LogTemp, Error, TEXT("Sapling clearing succeeded but its animation instance is unavailable."));
}

void AHomesteadCharacter::PlayClear(Homestead::Point Target)
{
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    const FVector2D Delta(Target.x - GetActorLocation().X, Target.y - GetActorLocation().Y);
    if (!FMath::IsFinite(Delta.X) || !FMath::IsFinite(Delta.Y) || Delta.SizeSquared() < 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Chop committed without a valid presentation target; using heroine facing."));
        PlayClear();
        return;
    }

    ClearYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestClear();
    else
        UE_LOG(LogTemp, Error, TEXT("Chopping succeeded but its animation instance is unavailable."));
}

void AHomesteadCharacter::PlayKnifeCut(Homestead::Point Target)
{
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    const FVector2D Delta(Target.x - GetActorLocation().X, Target.y - GetActorLocation().Y);
    if (FMath::IsFinite(Delta.X) && FMath::IsFinite(Delta.Y) && Delta.SizeSquared() >= 1)
        ClearYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Knife work committed without a valid presentation target; using heroine facing."));
        ClearYaw.Reset();
    }
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestKnifeCut();
    else
        UE_LOG(LogTemp, Error, TEXT("Knife work succeeded but its distinct animation instance is unavailable."));
}

void AHomesteadCharacter::UpdateHeldTools(float DeltaSeconds)
{
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    if (!Animation || !bMetaHumanActive) return;
    const auto* PC = Cast<AHomesteadController>(Controller);
    // At rest a selected tool rides in her hand; it gives way to authored actions and menus.
    Homestead::Item Presented = Homestead::Item::Count;
    if (PC)
    {
        if (!bAppearancePreview && !PC->IsPlanning() && !PC->IsFailed())
            Presented = PC->IsBookOpen() ? PC->SelectedCarriedTool() : PC->PresentedTool();
    }
    else if (InCharacterLab() && LabHeldTool) Presented = *LabHeldTool;
    const bool HandsFree = Animation->ActionWeight() < 0.01f && Animation->EatWeight() < 0.01f;
    const bool Hacking = Animation->MacheteWeight() > 0.01f;
    const bool Felling = Animation->FellWeight() > 0.01f;
    const bool CuttingReeds = IsCuttingReeds();
    const bool Hoeing = bHoeTill && Animation->TillWeight() > 0.01f;
    float Grip = 0, Carry = RestWristDegrees;
    // At rest a tool's handle crosses the palm diagonally (heel of the hand to the index knuckle),
    // which tips its head forward and down with the wrist nearly straight. Authored actions set
    // the tool's angle themselves, so the tilt eases out while one plays.
    const float TiltTarget = HandsFree && !Hacking && !Felling && !CuttingReeds && !Hoeing ? 1.0f : 0.0f;
    HeldToolTilt = FMath::FInterpConstantTo(HeldToolTilt, TiltTarget, DeltaSeconds, 1.0f / 0.15f);
    const auto Tilt = [this](const FTransform& Rest, float Degrees)
    {
        // Grip-local X is the palm normal; a positive turn about it tips the head toward the fingertips.
        return FTransform(FQuat(FVector::XAxisVector, FMath::DegreesToRadians(Degrees * HeldToolTilt))) * Rest;
    };
    if (HeldMachete)
    {
        const bool Held = (Hacking && HackTool == Homestead::Item::Machete) || (HandsFree && Presented == Homestead::Item::Machete);
        HeldMachete->SetVisibility(Held);
        if (Held)
        {
            Grip = 1;
            const float MacheteCarry = CVarCarryMachete.GetValueOnGameThread() >= 0
                ? CVarCarryMachete.GetValueOnGameThread() : MacheteCarryDegrees;
            HeldMachete->SetRelativeTransform(Tilt(MacheteGrip, MacheteCarry - RestWristDegrees));
        }
    }
    for (int32 Index = 0; Index < HeldProps.Num(); ++Index)
    {
        UStaticMeshComponent* Prop = HeldProps[Index];
        const FHeldToolSpec& Spec = HeldToolSpecs[Index];
        const bool Held = Spec.Tool == Homestead::Item::Hatchet && Felling
            || Spec.Tool == Homestead::Item::Knife && CuttingReeds
            || Spec.Tool == Homestead::Item::DiggingStick && Hoeing
            || (Hacking && Spec.Tool == HackTool)
            || (HandsFree && !Hacking && Presented == Spec.Tool);
        Prop->SetVisibility(Held);
        if (!Held) continue;
        Grip = 1;
        const bool StoneHoe = Spec.Tool == Homestead::Item::DiggingStick && Prop->GetStaticMesh()
            && Prop->GetStaticMesh()->GetName() == TEXT("SM_StoneHoe");
        const float Tuned = Spec.Tool == Homestead::Item::Hatchet ? CVarCarryHatchet.GetValueOnGameThread()
            : Spec.Tool == Homestead::Item::Knife ? CVarCarryKnife.GetValueOnGameThread()
            : StoneHoe ? CVarCarryHoe.GetValueOnGameThread() : -1.0f;
        const float CarryDegrees = Tuned >= 0 ? Tuned : Spec.CarryDegrees;
        // The authored saw stroke drives the wrist; the resting carry deviation would skew the blade.
        Carry = CuttingReeds || Hoeing ? 0.0f : Hacking ? RestWristDegrees : FMath::Min(CarryDegrees, RestWristDegrees);
        // Resting carries that differ from the working grip: the hatchet and knife hang edge-down
        // (turned about the haft) and the hoe is carried blade-low in front, turned end for end from
        // how she works it. The turn eases out with the tilt when an authored action takes over.
        FQuat Flip = FQuat::Identity;
        float Slide = 0;
        if (Spec.Tool == Homestead::Item::Hatchet || Spec.Tool == Homestead::Item::Knife)
            Flip = FQuat(FVector::ZAxisVector, PI);
        else if (StoneHoe)
        {
            // Carried, her hand rides near the top of the haft so its end clears her hip.
            Flip = FQuat(FVector::XAxisVector, PI);
            Slide = -26.0f;
        }
        // The hoe keeps its carried grip through the tilling clip (hoe_till.py is authored for
        // it), so nothing turns in her hand as she starts or stops.
        const float TurnWeight = StoneHoe ? 1.0f : HeldToolTilt;
        const FTransform Turn = FTransform(FVector(0, 0, Slide * TurnWeight))
            * FTransform(FQuat::Slerp(FQuat::Identity, Flip, TurnWeight));
        const float Lean = CarryDegrees - (StoneHoe ? FMath::Min(CarryDegrees, RestWristDegrees) : Carry);
        const FTransform HeldPose = StoneHoe
            ? FTransform(FQuat(FVector::XAxisVector, FMath::DegreesToRadians(Lean))) * Spec.Rest
            : Tilt(Spec.Rest, Lean);
        if (!Spec.bHangs) Prop->SetRelativeTransform(Turn * HeldPose);
        if (Spec.bHangs) UpdateHangingPail(*Prop, DeltaSeconds);
    }
    if (!HeldProps.ContainsByPredicate([](const UStaticMeshComponent* Prop) { return Prop->IsVisible(); })) bPailHandValid = false;
    Animation->SetRightHandGrip(Grip, Carry);
    // She ticks after the pose is final (TG_PostUpdateWork), so the felling haft is laid through
    // both fists here, over the one-handed placement just set.
    UpdateFellingHatchet();
}

void AHomesteadCharacter::UpdateFellingHatchet()
{
    const auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    UStaticMeshComponent* Prop = GetHeldProp(Homestead::Item::Hatchet);
    if (!Animation || !Prop || !Prop->IsVisible()) return;
    const float Weight = Animation->FellWeight();
    if (Weight <= 0.01f) return;
    // The left fist holds the knob and the right closes just above it (axe_fell.py): the haft
    // runs up from the left grip centre, the edge along the left knuckles.
    USkeletalMeshComponent* Body = GetMesh();
    const auto GripCentre = [Body](const TCHAR* Side, FVector& Along)
    {
        const FVector Hand = Body->GetSocketLocation(*FString::Printf(TEXT("hand_%s"), Side));
        const FVector Knuckle = Body->GetSocketLocation(*FString::Printf(TEXT("middle_01_%s"), Side));
        const FVector Across = Body->GetSocketLocation(*FString::Printf(TEXT("index_01_%s"), Side))
            - Body->GetSocketLocation(*FString::Printf(TEXT("pinky_01_%s"), Side));
        Along = (Knuckle - Hand).GetSafeNormal();
        const bool bLeft = Side[0] == TEXT('l');
        const FVector Palm = (bLeft ? FVector::CrossProduct(Across, Along) : FVector::CrossProduct(Along, Across)).GetSafeNormal();
        return Hand + (Knuckle - Hand) * 0.75f + Palm * 3.3f;
    };
    FVector AlongL;
    const FVector Knob = GripCentre(TEXT("l"), AlongL);
    // Both fists stay together at the base of the haft (axe_fell.py), so their spacing can't set
    // the line; each closed fist's pinky-to-index axis runs along the haft.
    const FVector AcrossL = (Body->GetSocketLocation(TEXT("index_01_l")) - Body->GetSocketLocation(TEXT("pinky_01_l"))).GetSafeNormal();
    const FVector AcrossR = (Body->GetSocketLocation(TEXT("index_01_r")) - Body->GetSocketLocation(TEXT("pinky_01_r"))).GetSafeNormal();
    const FVector Haft = (AcrossL + AcrossR).GetSafeNormal().IsNearlyZero() ? AcrossL : (AcrossL + AcrossR).GetSafeNormal();
    const FVector Edge = (AlongL - Haft * FVector::DotProduct(AlongL, Haft)).GetSafeNormal();
    if (Edge.IsNearlyZero()) return;
    // Hatchet convention: head along +Z, edge toward -Y.
    const FTransform TwoHanded(FRotationMatrix::MakeFromZY(Haft, -Edge).ToQuat(), Knob, Prop->GetComponentScale());
    const FTransform OneHanded = Prop->GetComponentTransform();
    FTransform Blended;
    Blended.Blend(OneHanded, TwoHanded, FMath::SmoothStep(0.0f, 1.0f, Weight));
    Prop->SetWorldTransform(Blended);
}

bool AHomesteadCharacter::PlayEat(bool bBerry)
{
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    if (!bMetaHumanActive || !EatAnimation || !EatenFood || !Animation || Animation->IsEating()) return false;
    bEatBerry = bBerry;
    Animation->RequestEat();
    return true;
}

void AHomesteadCharacter::UpdateEating()
{
    if (!EatenFood) return;
    const auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    const float Time = Animation && Animation->IsEating() ? Animation->EatPhase() : -1.0f;
    const bool bInHand = Time >= EatPick && Time < EatBite;
    if (bInHand == bEatFoodInHand) return;
    bEatFoodInHand = bInHand;
    if (!bInHand)
    {
        EatenFood->SetVisibility(false);
        return;
    }
    // Pinched between thumb and fingertips, like the forage she stows (UpdateCarriedSticks).
    USkeletalMeshComponent* Body = GetMesh();
    const FVector Hand = Body->GetSocketLocation(TEXT("hand_r"));
    const FVector Fingers = (Body->GetSocketLocation(TEXT("middle_01_r")) - Hand).GetSafeNormal();
    const FVector Across = (Body->GetSocketLocation(TEXT("index_01_r")) - Body->GetSocketLocation(TEXT("pinky_01_r"))).GetSafeNormal();
    UStaticMesh* FoodMesh = bEatBerry ? ForageBerryMesh.Get() : ForageRootMesh.Get();
    const bool bAuthored = FoodMesh != nullptr;
    if (bAuthored) EatenFood->SetStaticMesh(FoodMesh);
    // A small bite: a few berries off the cluster, or a short piece of root.
    const FVector Scale = bAuthored ? FVector(bEatBerry ? 0.65f : 0.45f) : FVector(0.035f);
    const FRotator Rotation = bEatBerry ? FRotationMatrix::MakeFromZX(-Fingers, Across).Rotator()
        : FRotationMatrix::MakeFromZX(Fingers, Across).Rotator();
    // Between the pinched thumb and fingertips (the last knuckles plus a little toward the tips).
    const FVector Index = Body->GetSocketLocation(TEXT("index_03_r"));
    const FVector Thumb = Body->GetSocketLocation(TEXT("thumb_03_r"));
    const FVector Pinch = (Index + Thumb) * 0.5f + ((Index - Hand).GetSafeNormal() + (Thumb - Hand).GetSafeNormal()).GetSafeNormal() * 1.2f;
    EatenFood->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    EatenFood->SetWorldLocationAndRotation(Pinch, Rotation);
    EatenFood->SetWorldScale3D(Scale);
    EatenFood->AttachToComponent(Body, FAttachmentTransformRules::KeepWorldTransform, TEXT("hand_r"));
    EatenFood->SetVisibility(true);
}

void AHomesteadCharacter::SetLabHeldTool(Homestead::Item Tool)
{
    if (Tool == Homestead::Item::Count) LabHeldTool.Reset();
    else LabHeldTool = Tool;
}

UStaticMeshComponent* AHomesteadCharacter::GetHeldProp(Homestead::Item Tool) const
{
    if (!bMetaHumanActive) return nullptr;
    if (Tool == Homestead::Item::Machete) return HeldMachete;
    for (int32 Index = 0; Index < HeldToolSpecs.Num(); ++Index)
        if (HeldToolSpecs[Index].Tool == Tool) return HeldProps[Index];
    return nullptr;
}

void AHomesteadCharacter::UpdateHangingPail(UStaticMeshComponent& Pail, float DeltaSeconds)
{
    // A pendulum hanging from the bail: the hand's horizontal acceleration swings the pail the
    // other way, then gravity (a ~25 cm pendulum) and a little damping settle it plumb.
    const FVector Hand = Pail.GetComponentLocation();
    const float Dt = FMath::Clamp(DeltaSeconds, 1.0f / 240.0f, 1.0f / 20.0f);
    FVector2D Push = FVector2D::ZeroVector;
    if (bPailHandValid)
    {
        const FVector Velocity = (Hand - PailHandLast) / Dt;
        const FVector Smoothed = FMath::Lerp(PailHandVelocity, Velocity, FMath::Min(1.0f, Dt * 20.0f));
        const FVector Acceleration = (Smoothed - PailHandVelocity) / Dt;
        PailHandVelocity = Smoothed;
        Push = FVector2D(-Acceleration.X, -Acceleration.Y) / 980.0f;
        Push = Push.ClampAxes(-0.5f, 0.5f);
    }
    else
    {
        PailSwing = PailSwingRate = FVector2D::ZeroVector;
        PailHandVelocity = FVector::ZeroVector;
    }
    PailHandLast = Hand;
    bPailHandValid = true;
    constexpr float Stiffness = 980.0f / 25.0f, Damping = 3.5f;
    PailSwingRate += (-(PailSwing - Push) * Stiffness - PailSwingRate * Damping) * Dt;
    PailSwing = (PailSwing + PailSwingRate * Dt).ClampAxes(-0.7f, 0.7f);
    // The bail runs fore and aft in her fist; the spout faces out to her right.
    const FQuat Yaw(FVector::UpVector, FMath::DegreesToRadians(GetActorRotation().Yaw + 90.0f));
    const FQuat Swing = FQuat::FindBetweenNormals(-FVector::UpVector, FVector(PailSwing.X, PailSwing.Y, -1.0f).GetSafeNormal());
    Pail.SetWorldRotation(Swing * Yaw);
}

bool AHomesteadCharacter::PlayMacheteHack(Homestead::Point Target)
{
    return PlayMacheteHack(Target, Homestead::Item::Machete);
}

bool AHomesteadCharacter::PlayMacheteHack(Homestead::Point Target, Homestead::Item Tool)
{
    if (!bMetaHumanActive || !MacheteAnimation || !GetHeldProp(Tool)) return false;
    HackTool = Tool;
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    if (!Animation) return false;
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    const FVector2D Delta(Target.x - GetActorLocation().X, Target.y - GetActorLocation().Y);
    if (FMath::IsFinite(Delta.X) && FMath::IsFinite(Delta.Y) && Delta.SizeSquared() >= 1)
    {
        ClearYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
        BeginStanceStep(GetActorLocation(), *ClearYaw);
    }
    else ClearYaw.Reset();
    Animation->RequestMacheteHack();
    return true;
}

void AHomesteadCharacter::BeginStanceStep(const FVector& To, float Yaw)
{
    FellStepFrom = GetActorLocation();
    FellStepTo = To;
    FellStepFromYaw = GetActorRotation().Yaw;
    FellStepToYaw = Yaw;
    FellStepRemaining = FellStepSeconds;
}

bool AHomesteadCharacter::CanFell() const
{
    return bMetaHumanActive && FellAnimation && GetHeldProp(Homestead::Item::Hatchet);
}

bool AHomesteadCharacter::PlayFell(Homestead::Point Target, int32 Strokes, float TrunkRadius)
{
    if (!CanFell()) return false;
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    if (!Animation) return false;
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    const FVector2D Delta(Target.x - GetActorLocation().X, Target.y - GetActorLocation().Y);
    if (FMath::IsFinite(Delta.X) && FMath::IsFinite(Delta.Y) && Delta.SizeSquared() >= 1)
    {
        const float TreeYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
        ClearYaw = TreeYaw;
        FellStepRemaining = 0;
        if (TrunkRadius > 0)
        {
            // The trunk's centre sits past the bit along its travel, 2 cm of bite in.
            const float Bite = FMath::Max(0.0f, TrunkRadius - 2.0f);
            const float Left = FellBitLeft + FellCutLeft * Bite, Forward = FellBitForward + FellCutForward * Bite;
            const float Standoff = FMath::Sqrt(Left * Left + Forward * Forward);
            // The bit lands to one side of straight ahead (her right for the right-shoulder chop),
            // so she faces a little past the tree to the other side.
            ClearYaw = TreeYaw + FMath::RadiansToDegrees(FMath::Atan2(Left, Forward));
            const FVector2D To = FVector2D(Target.x, Target.y) - Delta.GetSafeNormal() * Standoff;
            UE_LOG(LogTemp, Verbose, TEXT("Fell: trunk (%.0f, %.0f) r%.1f from (%.0f, %.0f) stance (%.0f, %.0f) yaw %.1f"), Target.x, Target.y, TrunkRadius, GetActorLocation().X, GetActorLocation().Y, To.X, To.Y, *ClearYaw);
            if (FVector2D::Distance(To, FVector2D(GetActorLocation())) > 35.0f && Delta.Size() > Standoff)
            {
                // Too far for a stance step: walk up to the trunk, then settle and swing.
                bFellApproach = true;
                FellApproachTo = To;
                FellApproachYaw = *ClearYaw;
                FellApproachTime = 0;
                FellApproachStrokes = Strokes;
                return true;
            }
            BeginStanceStep(FVector(To.X, To.Y, GetActorLocation().Z), *ClearYaw);
        }
        else BeginStanceStep(GetActorLocation(), *ClearYaw);
    }
    else ClearYaw.Reset();
    Animation->RequestFell(Strokes);
    return true;
}

void AHomesteadCharacter::UpdateFellApproach(float DeltaSeconds)
{
    if (!bFellApproach) return;
    FellApproachTime += DeltaSeconds;
    const FVector2D Remaining = FellApproachTo - FVector2D(GetActorLocation());
    const float Distance = Remaining.Size();
    // Close enough for the stance step to finish the placement (or blocked): address the trunk.
    if (Distance > 12.0f && FellApproachTime < 2.5f)
    {
        const float Scale = FMath::Clamp(Distance / 70.0f, 0.4f, 1.0f);
        AddMovementInput(FVector(Remaining.GetSafeNormal(), 0.0), Scale);
        return;
    }
    bFellApproach = false;
    UE_LOG(LogTemp, Verbose, TEXT("Fell: approach ended at (%.0f, %.0f) after %.2fs, %.0f cm short"), GetActorLocation().X, GetActorLocation().Y, FellApproachTime, Distance);
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    if (!Animation || !CanFell()) return;
    GetCharacterMovement()->StopMovementImmediately();
    ClearYaw = FellApproachYaw;
    BeginStanceStep(FVector(FellApproachTo.X, FellApproachTo.Y, GetActorLocation().Z), FellApproachYaw);
    Animation->RequestFell(FellApproachStrokes);
}

void AHomesteadCharacter::PlayTill(Homestead::Point Target)
{
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    const FVector2D Delta(Target.x - GetActorLocation().X, Target.y - GetActorLocation().Y);
    if (!FMath::IsFinite(Delta.X) || !FMath::IsFinite(Delta.Y) || Delta.SizeSquared() < 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Till committed without a valid presentation target; using heroine facing."));
        TillYaw.Reset();
    }
    else
        TillYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestTill();
    else
        UE_LOG(LogTemp, Error, TEXT("Tilling succeeded but its animation instance is unavailable."));
}

void AHomesteadCharacter::CreateMappings()
{
    if (Mapping) return;
    Mapping = NewObject<UInputMappingContext>(this);
    auto MakeAction = [this](EInputActionValueType Type)
    {
        UInputAction* Action = NewObject<UInputAction>(this);
        Action->ValueType = Type;
        return Action;
    };
    MoveAction = MakeAction(EInputActionValueType::Axis2D);
    SprintAction = MakeAction(EInputActionValueType::Boolean);
    MouseLookAction = MakeAction(EInputActionValueType::Axis2D);
    StickLookAction = MakeAction(EInputActionValueType::Axis2D);
    ZoomAction = MakeAction(EInputActionValueType::Axis1D);

    auto AddMove = [this](FKey Key, bool Vertical, bool Negative)
    {
        FEnhancedActionKeyMapping& Entry = Mapping->MapKey(MoveAction, Key);
        if (Negative) Entry.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
        if (Vertical)
        {
            auto* Swizzle = NewObject<UInputModifierSwizzleAxis>(Mapping);
            Swizzle->Order = EInputAxisSwizzle::YXZ;
            Entry.Modifiers.Add(Swizzle);
        }
    };
    AddMove(EKeys::W, true, false);
    AddMove(EKeys::S, true, true);
    AddMove(EKeys::A, false, true);
    AddMove(EKeys::D, false, false);
    auto& MoveStick = Mapping->MapKey(MoveAction, EKeys::Gamepad_Left2D);
    MoveStick.Modifiers.Add(NewObject<UInputModifierDeadZone>(Mapping));
    Mapping->MapKey(MouseLookAction, EKeys::Mouse2D);
    Mapping->MapKey(SprintAction, EKeys::LeftShift);
    Mapping->MapKey(SprintAction, EKeys::RightShift);
    Mapping->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);
    auto& LookStick = Mapping->MapKey(StickLookAction, EKeys::Gamepad_Right2D);
    LookStick.Modifiers.Add(NewObject<UInputModifierDeadZone>(Mapping));
}

void AHomesteadCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    CreateMappings();
    if (auto* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHomesteadCharacter::Move);
        Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AHomesteadCharacter::BeginSprint);
        Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AHomesteadCharacter::EndSprint);
        Input->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AHomesteadCharacter::EndSprint);
        Input->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AHomesteadCharacter::MouseLook);
        Input->BindAction(StickLookAction, ETriggerEvent::Triggered, this, &AHomesteadCharacter::StickLook);
        Input->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AHomesteadCharacter::ZoomInput);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Homestead requires EnhancedInputComponent; check DefaultInput.ini."));
    }
}

void AHomesteadCharacter::Move(const FInputActionValue& Value)
{
    AHomesteadController* PC = Cast<AHomesteadController>(Controller);
    if (!PC && !InCharacterLab()) return;
    if (PC && (!PC->IsWorldReady() || PC->IsBookOpen() || PC->IsShopScreenOpen() || PC->IsFailed())) return;
    const FVector2D Axis = Value.Get<FVector2D>();
    if (!Axis.IsNearlyZero()) CancelAction();
    const FRotator Facing(0, Controller->GetControlRotation().Yaw, 0);
    AddMovementInput(FRotationMatrix(Facing).GetUnitAxis(EAxis::X), Axis.Y);
    AddMovementInput(FRotationMatrix(Facing).GetUnitAxis(EAxis::Y), Axis.X);
}

void AHomesteadCharacter::BeginSprint(const FInputActionValue&)
{
    const auto* PC = Cast<AHomesteadController>(Controller);
    bSprintHeld = PC ? PC->IsWorldReady() && !PC->IsBookOpen() && !PC->IsPlanning() && !PC->IsFailed()
        : InCharacterLab();
}

void AHomesteadCharacter::EndSprint(const FInputActionValue&)
{
    CancelSprint();
}

void AHomesteadCharacter::CancelSprint()
{
    bSprintHeld = false;
    bSprintActive = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed();
}

void AHomesteadCharacter::ApplyLook(FVector2D Value, float Scale)
{
    AHomesteadController* PC = Cast<AHomesteadController>(Controller);
    if (!PC && InCharacterLab())
    {
        AddControllerYawInput(Value.X * Scale);
        AddControllerPitchInput(Value.Y * Scale);
        return;
    }
    if (!PC || (PC->IsBookOpen() && PC->BookPage() != 6) || PC->IsShopScreenOpen() || PC->IsFailed()) return;
    AddControllerYawInput(Value.X * Scale * PC->Sensitivity);
    AddControllerPitchInput(Value.Y * Scale * PC->Sensitivity * (PC->bInvertY ? -1.0f : 1.0f));
}

void AHomesteadCharacter::MouseLook(const FInputActionValue& Value)
{
    ApplyLook(Value.Get<FVector2D>(), 0.6f);
}

void AHomesteadCharacter::StickLook(const FInputActionValue& Value)
{
    ApplyLook(Value.Get<FVector2D>(), GetWorld()->GetDeltaSeconds() * 95.0f);
}

void AHomesteadCharacter::SetPlanning(bool Enabled)
{
    bPlanning = Enabled;
    if (Enabled)
    {
        CancelSprint();
        CancelAction(true);
        GetCharacterMovement()->StopMovementImmediately();
    }
}

void AHomesteadCharacter::Zoom(float Amount)
{
    CameraArm->TargetArmLength = FMath::Clamp(CameraArm->TargetArmLength - Amount * 80.0f, 250.0f, 1000.0f);
}

void AHomesteadCharacter::CycleZoom()
{
    CameraArm->TargetArmLength = bAppearancePreview
        ? (CameraArm->TargetArmLength < 240 ? 320 : 190)
        : (CameraArm->TargetArmLength < 500 ? 740 : 380);
}

void AHomesteadCharacter::ZoomInput(const FInputActionValue& Value)
{
    Zoom(Value.Get<float>());
}

void AHomesteadCharacter::SetAppearancePreview(bool Enabled)
{
    if (Enabled == bAppearancePreview || !Controller) return;
    if (Enabled)
    {
        SavedViewRotation = Controller->GetControlRotation();
        SavedCameraDistance = CameraArm->TargetArmLength;
        CameraArm->TargetArmLength = 280;
        UpdateAppearanceFraming();
        // Face her from the front, but swing around a trunk or wall that would pull the arm in close.
        const float Front = GetActorRotation().Yaw + 180;
        float Yaw = Front;
        if (UWorld* World = GetWorld())
        {
            const FVector Pivot = CameraArm->GetComponentLocation();
            FCollisionQueryParams Query(SCENE_QUERY_STAT(AppearancePreview), false, this);
            const float Swings[] = {0, 25, -25, 50, -50, 80, -80, 115, -115};
            for (const float Swing : Swings)
            {
                const FRotator View(-6, Front + Swing, 0);
                const FVector End = Pivot - View.Vector() * CameraArm->TargetArmLength
                    + View.Quaternion().RotateVector(CameraArm->SocketOffset);
                if (!World->SweepTestByChannel(Pivot, End, FQuat::Identity, ECC_Camera,
                    FCollisionShape::MakeSphere(CameraArm->ProbeSize), Query))
                {
                    Yaw = Front + Swing;
                    break;
                }
            }
        }
        Controller->SetControlRotation(FRotator(-6, Yaw, 0));
    }
    else
    {
        CameraArm->TargetArmLength = SavedCameraDistance;
        CameraArm->SocketOffset = FVector(0, 45, 55);
        CameraArm->TargetOffset = FVector::ZeroVector;
        bAppearanceFaceFocus = false;
        RestoreNearClip();
        Camera->PostProcessSettings.bOverride_AutoExposureBias = false;
        Controller->SetControlRotation(SavedViewRotation);
    }
    bAppearancePreview = Enabled;
}

void AHomesteadCharacter::SetAppearanceFaceFocus(bool bFace)
{
    bAppearanceFaceFocus = bAppearancePreview && bFace;
}

void AHomesteadCharacter::UpdateAppearanceFraming()
{
    const auto* PC = Cast<APlayerController>(Controller);
    if (!PC) return;
    int32 Width = 0, Height = 0;
    PC->GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0) return;
    // Eye choices bring the camera in close on her face; other rows ease back to full length.
    const float Delta = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
    if (bAppearanceFaceFocus && FaceFocusBlend <= KINDA_SMALL_NUMBER) FaceFocusBodyArm = CameraArm->TargetArmLength;
    FaceFocusBlend = FMath::FInterpTo(FaceFocusBlend, bAppearanceFaceFocus ? 1.0f : 0.0f, Delta, 6.0f);
    if (FaceFocusBlend > KINDA_SMALL_NUMBER || bAppearanceFaceFocus)
    {
        CameraArm->TargetArmLength = FMath::Lerp(FaceFocusBodyArm, 70.0f, FaceFocusBlend);
        CameraArm->TargetOffset = FVector(0, 0, (66.0f + GetFootwearLift()) * FaceFocusBlend);
    }
    else FaceFocusBlend = 0.0f;
    const float Scale = FMath::Clamp(Height / 1080.0f, 0.4f, 3.0f);
    const float VirtualWidth = Width / Scale;
    const float PanelRight = (32.0f + FMath::Min(500.0f, VirtualWidth * 0.35f)) * Scale;
    const float Offset = -(PanelRight / Width) * CameraArm->TargetArmLength
        * FMath::Tan(FMath::DegreesToRadians(Camera->FieldOfView * 0.5f));
    CameraArm->SocketOffset = FVector(0, Offset, 10);
    // Branches and ferns have no collision, so the arm can't avoid them; clip away anything
    // well in front of her instead so the preview always shows her clearly.
    const FVector Pivot = CameraArm->GetComponentLocation() + CameraArm->TargetOffset;
    const float ViewDistance = FVector::Dist(Camera->GetComponentLocation(), Pivot);
    const float NearClip = FMath::Max(10.0f, ViewDistance * 0.6f);
    if (!SavedNearClip.IsSet()) SavedNearClip = GNearClippingPlane;
    if (FMath::Abs(GNearClippingPlane - NearClip) > 1.0f) SetNearClipPlaneGlobals(NearClip);
    const auto* GameController = Cast<AHomesteadController>(Controller);
    Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
    Camera->PostProcessSettings.AutoExposureBias =
        GameController && GameController->Simulation().IsNight() ? 0.5f : 0.0f;
}

void AHomesteadCharacter::RestoreNearClip()
{
    if (!SavedNearClip.IsSet()) return;
    SetNearClipPlaneGlobals(SavedNearClip.GetValue());
    SavedNearClip.Reset();
}

bool AHomesteadCharacter::InCharacterLab() const
{
    return Controller && Controller->IsA<AHomesteadLabController>();
}

FRotator AHomesteadCharacter::GameplayViewRotation() const
{
    return bAppearancePreview ? SavedViewRotation : (Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator);
}

float AHomesteadCharacter::CameraDistance() const
{
    return CameraArm->TargetArmLength;
}
