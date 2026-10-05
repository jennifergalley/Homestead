// AHomesteadShopkeeper's MetaHuman: Mr. Trethewey's body, face, grooms, Blender-fitted period
// garments, the pencil behind his ear, and the LOD sync that keeps them together. Mirrors the
// heroine's stack (HomesteadCharacterAppearance.cpp) at NPC cost: nothing animates off screen.
#include "HomesteadShopkeeper.h"

#include "Animation/AnimSequence.h"
#include "Components/LODSyncComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GroomAsset.h"
#include "GroomBindingAsset.h"
#include "GroomComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"

namespace ShopkeeperMetaHuman
{
const TCHAR* const Root = TEXT("/Game/Characters/Clerk_MH");
const TCHAR* const FaceAnimClass = TEXT("/Game/Characters/Clerk_MH/Common/Face/ABP_Face.ABP_Face_C");
// His own counter-leaning idle (Content/Python/homestead_agent/clerk_counter_idle.py); the
// heroine's standing idle plays through skeleton remapping if it is missing.
const TCHAR* const Idles[] = {
    TEXT("/Game/Characters/Clerk_MH/Animations/AN_ClerkMH_CounterIdle.AN_ClerkMH_CounterIdle"),
    TEXT("/Game/Characters/Heroine_MH/Animations/AN_HeroineMH_ActiveIdle.AN_HeroineMH_ActiveIdle")};
// Must match GARMENTS in Scripts/Characters/import_clerk_garments.py.
const TCHAR* const GarmentNames[] = {
    TEXT("SKM_ClerkTrousers"), TEXT("SKM_ClerkBoots"), TEXT("SKM_ClerkShirt"),
    TEXT("SKM_ClerkWaistcoat"), TEXT("SKM_ClerkNeckerchief"), TEXT("SKM_ClerkApron")};
const TCHAR* const PencilMesh = TEXT("Assembled/Clerk/Garments/SM_ClerkPencil");
const FName PencilBone(TEXT("head"));
// Pencil resting on his right ear in his bind pose, in component space (cm; +Y forward, his left +X):
// Assets/Characters/ClerkClothing/ClerkPencil/clerk_pencil_fit.json, Y negated from Blender. Its
// long axis (mesh +X) points forward and ~15 degrees down.
const FVector PencilBindLocation(-10.179, -0.674, 168.538);
const FVector PencilBindAxisX(0.0, 0.9659, -0.2588);
const FVector PencilBindAxisZ(-1.0, 0.0, 0.0);

// The pencil's transform relative to the head bone, from its component-space bind transform and the
// head bone's reference pose, so it follows his head through the idle.
FTransform PencilRelativeToHead(const USkeletalMesh& Mesh)
{
    const FReferenceSkeleton& Skeleton = Mesh.GetRefSkeleton();
    FTransform HeadBind = FTransform::Identity;
    for (int32 Bone = Skeleton.FindBoneIndex(PencilBone); Bone != INDEX_NONE; Bone = Skeleton.GetParentIndex(Bone))
        HeadBind = HeadBind * Skeleton.GetRefBonePose()[Bone];
    const FTransform Bind(FRotationMatrix::MakeFromXZ(PencilBindAxisX, PencilBindAxisZ).ToQuat(), PencilBindLocation);
    return Bind.GetRelativeTransform(HeadBind);
}

struct FGroomSpec
{
    const TCHAR* Component;
    const TCHAR* Groom;
    TArray<const TCHAR*> Materials;
};

template <typename T>
T* Load(const FString& RelativePath, bool bRequired = true)
{
    const FString Name = FPaths::GetBaseFilename(RelativePath);
    const FString Path = FString::Printf(TEXT("%s/%s.%s"), Root, *RelativePath, *Name);
    T* Asset = LoadObject<T>(nullptr, *Path, nullptr, bRequired ? LOAD_None : LOAD_NoWarn | LOAD_Quiet);
    if (!Asset && bRequired) UE_LOG(LogHomesteadShopkeeper, Error, TEXT("Shopkeeper MetaHuman asset is missing: %s"), *Path);
    return Asset;
}
}

bool AHomesteadShopkeeper::BuildMetaHuman()
{
    using namespace ShopkeeperMetaHuman;
    USkeletalMesh* BodyMesh = Load<USkeletalMesh>(TEXT("Assembled/Clerk/Body/SKM_MHC_Clerk_BodyMesh"));
    USkeletalMesh* FaceMesh = Load<USkeletalMesh>(TEXT("Assembled/Clerk/Face/SKM_MHC_Clerk_FaceMesh"));
    UClass* FaceAnim = LoadObject<UClass>(nullptr, FaceAnimClass);
    if (!BodyMesh || !FaceMesh || !FaceAnim) return false;

    Body->SetSkeletalMeshAsset(BodyMesh);
    // An NPC: his pose only needs evaluating when someone can see him.
    Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
    UAnimSequence* Idle = nullptr;
    for (const TCHAR* Path : Idles)
        if ((Idle = LoadObject<UAnimSequence>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet)) != nullptr) break;
    if (Idle)
    {
        Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Body->PlayAnimation(Idle, true);
    }
    else UE_LOG(LogHomesteadShopkeeper, Warning, TEXT("Shopkeeper has no idle clip; he will hold the reference pose."));

    auto MakeSkinned = [this](const TCHAR* Name, USkeletalMesh* Asset)
    {
        auto* Component = NewObject<USkeletalMeshComponent>(this, Name);
        Component->SetupAttachment(Body);
        Component->SetSkeletalMeshAsset(Asset);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->bUseAttachParentBound = true;
        Component->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
        return Component;
    };
    // ABP_Face copies the body's pose, then runs RigLogic for the face rig.
    Face = MakeSkinned(TEXT("Face"), FaceMesh);
    Face->SetAnimInstanceClass(FaceAnim);
    Face->AddTickPrerequisiteComponent(Body);
    Face->RegisterComponent();

    Garments.Reset();
    for (const TCHAR* Name : GarmentNames)
    {
        USkeletalMesh* Mesh = Load<USkeletalMesh>(FString::Printf(TEXT("Assembled/Clerk/Garments/%s"), Name));
        if (!Mesh) continue;
        auto* Garment = MakeSkinned(Name, Mesh);
        Garment->RegisterComponent();
        Garment->SetLeaderPoseComponent(Body);
        Garments.Add(Garment);
    }

    // The mutton-chop beard and dense brows rendered as white/gold glitter at talk distance whatever
    // their melanin, spec or strand LOD (2026-10-04 PIE), so he is clean-shaven and the face
    // texture's baked brows show. Their assets stay assembled for a later groom fix.
    const FGroomSpec Specs[] = {
        {TEXT("Hair"), TEXT("Hair_S_SlickBack"),
            {TEXT("MI_WI_Hair_S_SlickBack_Hair"), TEXT("MI_WI_Hair_S_SlickBack_Hair_Cards"), TEXT("MI_WI_Hair_S_SlickBack_Hair_Helmet")}},
        {TEXT("Eyelashes"), TEXT("Eyelashes_S_Sparse"), {TEXT("MI_WI_Eyelashes_S_Sparse_Hair")}},
    };
    Grooms.Reset();
    for (const FGroomSpec& Spec : Specs)
    {
        const FString Base = FString::Printf(TEXT("Assembled/Clerk/Grooms/%s"), Spec.Groom);
        auto* Asset = Load<UGroomAsset>(Base);
        auto* Binding = Load<UGroomBindingAsset>(Base + TEXT("_Binding"));
        if (!Asset || !Binding) continue;
        auto* Groom = NewObject<UGroomComponent>(this, Spec.Component);
        Groom->SetupAttachment(Face);
        Groom->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        // Short, oiled hair and whiskers: no strand simulation for a man standing at a counter.
        Groom->SimulationSettings.bOverrideSettings = true;
        Groom->SimulationSettings.SolverSettings.bEnableSimulation = false;
        Groom->SetGroomAsset(Asset, Binding);
        for (int32 Index = 0; Index < Spec.Materials.Num(); ++Index)
            if (auto* Material = Load<UMaterialInterface>(FString::Printf(TEXT("Assembled/Clerk/Grooms/%s"), Spec.Materials[Index])))
                Groom->SetMaterial(Index, Material);
        Groom->RegisterComponent();
        Grooms.Add(Groom);
    }

    if (UStaticMesh* PencilAsset = Load<UStaticMesh>(PencilMesh))
    {
        Pencil = NewObject<UStaticMeshComponent>(this, TEXT("Pencil"));
        Pencil->SetupAttachment(Body, PencilBone);
        Pencil->SetStaticMesh(PencilAsset);
        Pencil->SetRelativeTransform(PencilRelativeToHead(*BodyMesh));
        Pencil->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Pencil->SetGenerateOverlapEvents(false);
        Pencil->bUseAttachParentBound = true;
        Pencil->RegisterComponent();
    }

    // Mirrors the assembled BP_Clerk LODSync: body and face drive, garments and grooms follow.
    LODSync = NewObject<ULODSyncComponent>(this, TEXT("LODSync"));
    LODSync->NumLODs = 4;
    LODSync->ComponentsToSync = {
        FComponentSync(Body->GetFName(), ESyncOption::Drive),
        FComponentSync(Face->GetFName(), ESyncOption::Drive)};
    for (USkeletalMeshComponent* Garment : Garments)
        LODSync->ComponentsToSync.Add(FComponentSync(Garment->GetFName(), ESyncOption::Passive));
    for (UGroomComponent* Groom : Grooms)
    {
        LODSync->ComponentsToSync.Add(FComponentSync(Groom->GetFName(), ESyncOption::Passive));
        FLODMappingData GroomMapping;
        GroomMapping.Mapping = TArray<int32>{1, 3, 5, 7};
        LODSync->CustomLODMapping.Add(Groom->GetFName(), GroomMapping);
    }
    LODSync->RegisterComponent();
    UE_LOG(LogHomesteadShopkeeper, Log, TEXT("Shopkeeper MetaHuman built: %d garments, %d grooms, idle %s."),
        Garments.Num(), Grooms.Num(), Idle ? *Idle->GetName() : TEXT("none"));
    return true;
}
