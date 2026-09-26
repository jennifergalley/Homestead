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
    WalkAnimation = Clips[1];
    SlowWalkAnimation = nullptr;
    SprintAnimation = Clips[2];
    GatherAnimation = Clips[3];
    WaterAnimation = Clips[4];
    ClearAnimation = Clips[5];
    KnifeCutAnimation = Clips[6];
    TillAnimation = Clips[7];
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
    if (UStaticMesh* Pouch = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/SurvivalGame/Environment/Props/ForagePouch/SM_ForagePouch.SM_ForagePouch")))
    {
        ForagePouch = MakeProp(TEXT("ForagePouch"), Pouch);
        // Hangs from her right hip, back against the body, neck just below POUCH_OPENING in
        // kneel_pouch.py (pelvis frame of the clip's standing pose).
        ForagePouch->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("pelvis"));
        ForagePouch->SetRelativeTransform(FTransform(FRotator(0.0f, -92.05f, 90.0f), FVector(4.79f, 4.61f, 21.0f)));
        ForagePouch->SetVisibility(true);
    }
    // The rawhide cord belt the pouch hangs from, on the shorts' waistband with the knot in front.
    // The loop is stretched to her hip width and depth so it wraps rather than floating.
    if (UStaticMesh* Belt = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/SurvivalGame/Environment/Props/CordBelt/SM_CordBelt.SM_CordBelt")))
    {
        CordBelt = MakeProp(TEXT("CordBelt"), Belt);
        CordBelt->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("pelvis"));
        CordBelt->SetRelativeTransform(FTransform(FRotator(-90.0f, -2.05f, 0.0f), FVector(4.25f, 1.07f, -0.02f), FVector(1.24f, 1.12f, 1.0f)));
        CordBelt->SetVisibility(true);
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
    bHeroineReady = true;
    return true;
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
    const bool Blocked = (Lab ? false : !PC || !PC->IsWorldReady() || PC->IsBookOpen()
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
        : Kind == EHomesteadKneelGather::Stones ? CarriedStones.Num() >= 2 : CarriedForage != nullptr;
    if (!GetGatherSticksAnimation() || !bPropsReady || !Animation)
    {
        KneelKind = EHomesteadKneelGather::Sticks;
        PlayGather();
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
        // Her right hand picks both sticks up about 32 cm ahead and 26 cm to her right. Turn and
        // settle her during the first step so that spot lands on the pile.
        constexpr float GrabForward = 32.0f, GrabRight = 26.0f;
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

void AHomesteadCharacter::UpdateHairMotion(float DeltaSeconds)
{
    if (!MetaHumanHair) return;
    const float Walk = WalkSpeed(), Sprint = SprintSpeed();
    const float Target = FMath::Clamp((GetVelocity().Size2D() - Walk) / FMath::Max(Sprint - Walk, 1.0f), 0.0f, 1.0f);
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

void AHomesteadCharacter::UpdateCarriedSticks()
{
    const bool bPouch = KneelKind == EHomesteadKneelGather::Pouch;
    const bool bStones = KneelKind == EHomesteadKneelGather::Stones;
    const auto& Props = bStones ? CarriedStones : CarriedSticks;
    if (bPouch ? !CarriedForage : Props.Num() < 2) return;
    const auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    const bool Active = Animation && Animation->IsGatheringSticks();
    const float Time = Active ? Animation->GatherSticksPhase() : 0.0f;
    const float Pick1 = bPouch ? GatherPouchTiming::Pick1 : GatherSticksTiming::Pick1;
    const float Pick2 = bPouch ? GatherPouchTiming::Pick2 : GatherSticksTiming::Pick2;
    int32 Stage = 0;
    if (bPouch)
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
    if (PC && (!PC->IsWorldReady() || PC->IsBookOpen() || PC->IsFailed())) return;
    const FVector2D Axis = Value.Get<FVector2D>();
    if (!Axis.IsNearlyZero()) CancelAction();
    if (bPlanning && PC)
    {
        PC->NudgePlacement(Axis);
        return;
    }
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
    if (!PC || (PC->IsBookOpen() && PC->BookPage() != 6) || PC->IsFailed()) return;
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
        Controller->SetControlRotation(FRotator(-6, GetActorRotation().Yaw + 180, 0));
    }
    else
    {
        CameraArm->TargetArmLength = SavedCameraDistance;
        CameraArm->SocketOffset = FVector(0, 45, 55);
        Camera->PostProcessSettings.bOverride_AutoExposureBias = false;
        Controller->SetControlRotation(SavedViewRotation);
    }
    bAppearancePreview = Enabled;
}

void AHomesteadCharacter::UpdateAppearanceFraming()
{
    const auto* PC = Cast<APlayerController>(Controller);
    if (!PC) return;
    int32 Width = 0, Height = 0;
    PC->GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0) return;
    const float Scale = FMath::Clamp(Height / 1080.0f, 0.4f, 3.0f);
    const float VirtualWidth = Width / Scale;
    const float PanelRight = (32.0f + FMath::Min(500.0f, VirtualWidth * 0.35f)) * Scale;
    const float Offset = -(PanelRight / Width) * CameraArm->TargetArmLength
        * FMath::Tan(FMath::DegreesToRadians(Camera->FieldOfView * 0.5f));
    CameraArm->SocketOffset = FVector(0, Offset, 10);
    const auto* GameController = Cast<AHomesteadController>(Controller);
    Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
    Camera->PostProcessSettings.AutoExposureBias =
        GameController && GameController->Simulation().IsNight() ? 0.5f : 0.0f;
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
