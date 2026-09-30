#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWorld.h"
#include "HomesteadLampLook.h"

#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/LODSyncComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GroomAsset.h"
#include "GroomBindingAsset.h"
#include "GroomComponent.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "RenderCore.h"

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



DEFINE_LOG_CATEGORY_STATIC(LogHomesteadHair, Log, All);

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

bool AHomesteadCharacter::UsesMetaHumanHeroine()
{
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadMetaHuman"))) return true;
    // Smoke/automation routes assert the legacy wardrobe and material contracts.
    return CVarMetaHumanHeroine.GetValueOnGameThread() != 0 && !GIsAutomationTesting
        && !FParse::Param(FCommandLine::Get(), TEXT("HomesteadSmokeTest"))
        && !FParse::Param(FCommandLine::Get(), TEXT("HomesteadLegacyHeroine"));
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
    // Optional: filling and pouring the carved pail (homestead_agent.pail_fill / pail_pour).
    PailPourAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_PailPour"));
    if (PailPourAnimation && PailPourAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton()) PailPourAnimation = nullptr;
    PailFillAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_PailFill"));
    if (PailFillAnimation && PailFillAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton()) PailFillAnimation = nullptr;
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
    // Optional: authored with homestead_agent.ground_strike and homestead_agent.scythe_mow.
    StrikeAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_GroundStrike"));
    if (StrikeAnimation && StrikeAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton())
        StrikeAnimation = nullptr;
    MowAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_ScytheMow"));
    if (MowAnimation && MowAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton())
        MowAnimation = nullptr;
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
    GatherHarvestAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_KneelHarvest"));
    if (GatherHarvestAnimation && GatherHarvestAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton())
        GatherHarvestAnimation = nullptr;
    // Optional: authored with homestead_agent.kneel_pull_weeds. Missing, weeds keep the pouch kneel.
    PullWeedsAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_KneelPullWeeds"));
    if (PullWeedsAnimation && PullWeedsAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton())
        PullWeedsAnimation = nullptr;
    // Pulled weed fistfuls: meshed like the clump she pulls (PlayPullWeeds), else the garden's nettle tuft.
    PulledWeedDefault = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Game/SurvivalGame/Environment/Props/Nettle/SM_NettlePatch.SM_NettlePatch"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    PulledWeedL = MakeProp(TEXT("PulledWeedL"), PulledWeedDefault.Get());
    PulledWeedR = MakeProp(TEXT("PulledWeedR"), PulledWeedDefault.Get());
    PulledWeedL->SetCastShadow(false);
    PulledWeedR->SetCastShadow(false);
    if (UStaticMesh* Seeds = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/SurvivalGame/Environment/Props/Seeds/SM_Seeds.SM_Seeds")))
        CarriedSeed = MakeProp(TEXT("CarriedSeed"), Seeds);
    // Optional: authored with homestead_agent.eat_berry.
    EatAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_Eat"));
    if (EatAnimation && EatAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton())
        EatAnimation = nullptr;
    // Optional: authored with homestead_agent.craft_hands.
    CraftAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_CraftHands"));
    if (CraftAnimation && CraftAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton())
        CraftAnimation = nullptr;
    // Optional: authored with homestead_agent.lamp_pose.
    LampRaisedAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_LampRaised"));
    if (LampRaisedAnimation && LampRaisedAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton())
        LampRaisedAnimation = nullptr;
    LampSetDownAnimation = LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_LampSetDown"));
    if (LampSetDownAnimation && LampSetDownAnimation->GetSkeleton() != MetaHumanBody->GetSkeleton())
        LampSetDownAnimation = nullptr;
    // The branch she works while crafting, upright through her left fist.
    if (UStaticMesh* Piece = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_DryBranchesMedium01_b.SM_DryBranchesMedium01_b")))
        CraftPiece = MakeProp(TEXT("CraftPiece"), Piece);
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
    // Blender hand tools, each authored with its pivot at the main hand's grip and the handle along
    // +Z (docs/blender-assets.md). The Blender export mirrors Y, so in the engine their working edges
    // face +Y. Offsets adapt tools held elsewhere.
    HeldProps.Reset();
    HeldToolSpecs.Reset();
    const FTransform Grip = HandGripTransform(*MetaHumanBody);
    struct FHeldToolAsset { Homestead::Item Tool; const TCHAR* Path; float CarryDegrees; bool bHangs; FTransform Offset; };
    // The digging stick's pivot is its upper grip with the point toward -Z; she trail-carries it at
    // the balance point instead, point forward and down, the way a spear or staff is carried.
    const FTransform StickTrail(FQuat(FVector::XAxisVector, PI), FVector(0, 0, -25));
    // SM_Scythe (scythe.py): pivot at the lower nib's grip, snath +Z, the snath itself 13 cm along +Y
    // from that grip, blade +X from the heel. In the grip frame the handle runs along +Z (pinky to
    // index, forward in a hanging hand), +Y is back toward the wrist and +X points out to her right,
    // so the snath goes along the fist tipped 25 degrees up, and the blade reaches out along +X.
    // Her fist closes on the wood itself (Jenny, 09-29: at rest the scythe must sit in her hand): the
    // snath's centreline 15 cm below the lower nib and its direction there, from scythe.py snath_point
    // (t = 0.519) relative to the grip. The crooked snath runs about 2 cm off and 6 degrees from the
    // straight line through the nib that the carry used before, so it stood out beside her fingers.
    const FVector SnathInFist(-0.2f, 14.9f, -15.0f);
    const FVector SnathAlong = FVector(0.0035f, -0.1032f, 0.9947f).GetSafeNormal();
    // The carry keeps its old line through the fist (the 15 degrees 'in' it once asked for were squared
    // away by the frame, and it looked right that way).
    const FVector CarryAxis(0.0f, FMath::Sin(FMath::DegreesToRadians(25.0f)), FMath::Cos(FMath::DegreesToRadians(25.0f)));
    const FQuat ScytheTurn = FRotationMatrix::MakeFromZX(CarryAxis, FVector(1, 0, 0)).ToQuat()
        * FRotationMatrix::MakeFromZX(SnathAlong, FVector(1, 0, 0)).ToQuat().Inverse();
    const FTransform ScytheTrail(ScytheTurn, -ScytheTurn.RotateVector(SnathInFist));
    const FHeldToolAsset Assets[] = {
        {Homestead::Item::Knife, TEXT("FlintKnife/SM_FlintKnife"), 30, false, FTransform::Identity},
        // The estate axe (estate_axe.py) and draw hoe (draw_hoe.py) are authored in the flint hatchet's
        // and stone hoe's frames, so the felling, strike and tilling clips fit them unchanged; the
        // flint originals remain as fallbacks.
        {Homestead::Item::Hatchet, TEXT("EstateAxe/SM_EstateAxe"), 20, false, FTransform::Identity},
        {Homestead::Item::Hatchet, TEXT("FlintHatchet/SM_FlintHatchet"), 20, false, FTransform::Identity},
        // Tools ride nearly level in a relaxed hand, heads a little low. The stone hoe (blade at the
        // far end) is carried out in front from the top of its haft; the digging stick is the fallback.
        {Homestead::Item::DiggingStick, TEXT("DrawHoe/SM_DrawHoe"), 24, false, FTransform::Identity},
        {Homestead::Item::DiggingStick, TEXT("StoneHoe/SM_StoneHoe"), 24, false, FTransform::Identity},
        {Homestead::Item::DiggingStick, TEXT("DiggingStick/SM_DiggingStick"), 34, false, StickTrail},
        {Homestead::Item::WateringCan, TEXT("WaterPail/SM_WaterPail"), 20, true, FTransform::Identity},
        // Authored in the machete's frame (grip pivot, blade +Z, edge -Y), so the hack fits it as is.
        {Homestead::Item::Billhook, TEXT("Billhook/SM_Billhook"), MacheteCarryDegrees, false, FTransform::Identity},
        // The pick shares the hatchet's frame (knob grip pivot, head +Z, point -Y). Carried one-handed
        // she holds it partway up the haft, near its balance, head forward and low.
        {Homestead::Item::Pickaxe, TEXT("Pickaxe/SM_Pickaxe"), 20, false, FTransform(FVector(0, 0, -34))},
        // The scythe is carried at the trail by its snath, gripped at the balance 15 cm below the lower
        // nib: the snath runs through her fist with its top end forward and up, and the blade trails
        // behind her at knee height, out to her right.
        {Homestead::Item::Scythe, TEXT("Scythe/SM_Scythe"), RestWristDegrees, false, ScytheTrail},
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
    // The oil lamp hangs from her fist by its bail (oil_lamp.py): a hanger at the grip that the
    // pendulum turns, with the bail's top at the hanger.
    if (!LampHanger)
    {
        LampHanger = NewObject<USceneComponent>(this, TEXT("LampHanger"));
        LampHanger->SetupAttachment(GetMesh(), TEXT("hand_r"));
        LampHanger->SetUsingAbsoluteRotation(true);
        LampHanger->RegisterComponent();
    }
    LampHanger->SetRelativeLocation(Grip.GetLocation());
    if (HeldLampParts.IsEmpty())
    {
        const FVector Hang(0, 0, -HomesteadLampLook::BailTopHeight);
        for (UStaticMeshComponent* Part : HomesteadLampLook::AddParts(this, LampHanger, Hang, TEXT("HeldLamp")))
            HeldLampParts.Add(Part);
        if (!HeldLampParts.IsEmpty() && !HeldLampLight)
            HeldLampLight = HomesteadLampLook::AddLight(this, LampHanger, Hang, TEXT("HeldLampLight"));
        // Held up, the lamp's own fount would shade the ground beneath it from its flame.
        for (UStaticMeshComponent* Part : HeldLampParts) if (Part) { Part->SetVisibility(false); Part->SetCastShadow(false); }
        if (HeldLampLight) HeldLampLight->SetVisibility(false);
    }
    // Pouring water: an engine cylinder with the translucent stream material, stretched from the
    // pail's lip down to the soil while she pours (UpdateWaterPail).
    if (UStaticMesh* Column = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
    {
        PourStream = MakeProp(TEXT("PourStream"), Column);
        PourStream->SetUsingAbsoluteLocation(true);
        PourStream->SetUsingAbsoluteRotation(true);
        PourStream->SetUsingAbsoluteScale(true);
        PourStream->SetCastShadow(false);
        if (auto* Water = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/SurvivalGame/Environment/Props/WaterPail/M_PourStream.M_PourStream"), nullptr, LOAD_NoWarn | LOAD_Quiet))
            PourStream->SetMaterial(0, Water);
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
            // UpdateHairMotion raises the inherited motion a little at a sprint. The material and force overrides
            // below are tuned for this long groom only; ApplyMetaHumanLook turns them off for the other styles.
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
        // Scalp hair stops at groom LOD 4 (cards) instead of the stock 5 and 7. Those are Legacy01 helmet
        // meshes bound by transfer, and Pixie declares them with no mesh at all. Jenny saw rods and fans in
        // far and rear views (09-29). This is an unconfirmed fix: it didn't reproduce in the lab.
        // Brows, lashes and fuzz keep the stock mapping.
        const bool bScalp = Groom == MetaHumanHair;
        GroomMapping.Mapping = bScalp ? TArray<int32>{1, 3, 4, 4} : TArray<int32>{1, 3, 5, 7};
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
        // The component's physics overrides (bend stiffness 0.15, collision radius 5 cm, air drag 1...)
        // were tuned for the long assembled groom and replace every group's authored values. On the stock
        // bobs, updos and ponytail they are 3-20x stiffer, 2-50x wider and 10x draggier than authored, so a
        // few guides get shoved off the scalp and held there as rigid rods and fans (Jenny's playtest,
        // 2026-09-29). Every other style simulates with its own asset physics; the calmer inherited
        // motion (SimulationSetup velocity scales, UpdateHairMotion) still applies to all of them.
        MetaHumanHair->SimulationSettings.bOverrideSettings = Style == 0;
        MetaHumanHair->ResetSimulation();
        UE_LOG(LogHomesteadHair, Log, TEXT("Heroine hairstyle %s: %s simulation settings."), *Groom,
            Style == 0 ? TEXT("tuned override") : TEXT("the groom's own"));
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
