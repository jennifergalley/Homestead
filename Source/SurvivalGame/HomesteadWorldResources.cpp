#include "HomesteadWorld.h"
#include "HomesteadCharacter.h"
#include "HomesteadWorldCommon.h"
#include "HomesteadWorldLog.h"
#include "HomesteadWorldLook.h"
#include "Simulation/HomesteadOvergrowth.h"
#include "Simulation/HomesteadRuinDebris.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

namespace HomesteadWorldResourceLook
{
// Estate berry bushes (BerryBush): a wild red currant (SM_WildCurrant, about 1 m across) with three
// sectors of ripe red strings (SM_CurrantProduce, one 120-degree sector each) that go one at a time
// when she picks. Distinct from the bramble look on purpose. Authored by
// Scripts/Blender/Recipes/wild_currant.py and currant_produce.py.
constexpr float CurrantBushScale = 1.0f;
constexpr int32 CurrantProduceSectors = 3;
constexpr float CurrantSectorYawStep = 360.0f / CurrantProduceSectors;
}

using HomesteadWorldCommon::IsMvpWoodlandId;
using HomesteadWorldLook::Stone;
using HomesteadWorldResourceLook::CurrantBushScale;
using HomesteadWorldResourceLook::CurrantProduceSectors;
using HomesteadWorldResourceLook::CurrantSectorYawStep;

void AHomesteadWorld::HideHeldProducePart(int32 Index)
{
    if (HeldProduceId == INDEX_NONE) return;
    if (auto* Produce = ResourceProduceVisuals.Find(HeldProduceId))
        if (Produce->Components.IsValidIndex(Index) && IsValid(Produce->Components[Index]))
            Produce->Components[Index]->SetVisibility(false);
}

UStaticMesh* AHomesteadWorld::ResourceVisualMesh(int32 Id) const
{
    if (const auto* Visual = ResourceVisuals.Find(Id))
        for (const auto& Component : Visual->Components)
            if (const auto* Mesh = Cast<UStaticMeshComponent>(Component.Get()); Mesh && Mesh->GetStaticMesh())
                return Mesh->GetStaticMesh();
    return nullptr;
}

void AHomesteadWorld::ThinResource(int32 Id, float Fraction)
{
    const auto* Visual = ResourceVisuals.Find(Id);
    if (!Visual) return;
    // Already thinned, and the visual hasn't been rebuilt under us: nothing to do.
    const bool bSame = Id == ThinnedResourceId && ThinnedComponents.Num() == Visual->Components.Num()
        && (ThinnedComponents.IsEmpty() || ThinnedComponents[0].Get() == Visual->Components[0].Get());
    if (bSame) return;
    RestoreThinnedResource();
    ThinnedResourceId = Id;
    for (const auto& Component : Visual->Components)
    {
        USceneComponent* Scene = Component.Get();
        ThinnedComponents.Add(Scene);
        ThinnedScales.Add(Scene ? Scene->GetRelativeScale3D() : FVector::OneVector);
        if (Scene) Scene->SetRelativeScale3D(Scene->GetRelativeScale3D() * Fraction);
    }
}

void AHomesteadWorld::RestoreThinnedResource()
{
    for (int32 Index = 0; Index < ThinnedComponents.Num(); ++Index)
        if (USceneComponent* Scene = ThinnedComponents[Index].Get()) Scene->SetRelativeScale3D(ThinnedScales[Index]);
    ForgetThinnedResource();
}

void AHomesteadWorld::ForgetThinnedResource()
{
    ThinnedResourceId = INDEX_NONE;
    ThinnedComponents.Reset();
    ThinnedScales.Reset();
}

void AHomesteadWorld::BuildResource(FHomesteadWorldVisual& Visual, const Homestead::ResourceNode& Node, bool bProduceOnly)
{
    if (Node.cleared)
    {
        return;
    }
    const FVector Base = AtGround(Node.position.x, Node.position.y);
    const uint32 Variation = GetTypeHash(Descriptor.seed) ^ GetTypeHash(Node.key.chunk.x)
        ^ (GetTypeHash(Node.key.chunk.y) * 127u) ^ Node.key.localId;
    FRandomStream Random(static_cast<int32>(Variation));
    auto Part = [&](UStaticMesh* Mesh, FVector Offset, FVector Size, FLinearColor Color,
        FRotator Rotation = FRotator::ZeroRotator, bool bProduce = false)
    {
        if (bProduce == bProduceOnly)
        {
            AddPart(Visual, Mesh, Base + Offset, Size, Color, false, Rotation);
        }
    };
    auto LoadResource = [&](const TCHAR* Name, bool bGrass = false)
    {
        const FString Path = FString(bGrass ? TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/")
            : TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/")) + Name;
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
        if (!Mesh)
        {
            bVisualBuildFailed = true;
            UE_LOG(LogHomesteadWorld, Error, TEXT("Resource %d is missing authored mesh %s"), Node.id, *Path);
        }
        return Mesh;
    };
    // bPivotGround: the mesh is authored part-sunk with its pivot on the ground line, so place the pivot,
    // not the lowest point, on the terrain. bPivotOrigin: place the mesh's own origin on the node rather
    // than its bounds centre, for pieces authored to share one pivot (a bush and its berry sectors).
    auto Authored = [&](UStaticMesh* Mesh, FVector2D Offset, float Yaw, bool bProduce, float Scale = 1.0f, float Lift = 0.0f,
        bool bPivotGround = false, bool bPivotOrigin = false)
    {
        if (bProduce != bProduceOnly || !Mesh) return;
        const FBox Bounds = Mesh->GetBoundingBox();
        if (!Bounds.IsValid || Bounds.Min.ContainsNaN() || Bounds.Max.ContainsNaN())
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Resource %d has invalid authored bounds: %s"), Node.id, *Mesh->GetPathName());
            bVisualBuildFailed = true;
            return;
        }
        const FRotator Rotation(0, Yaw, 0);
        const FVector Anchor(bPivotOrigin ? 0.0 : Bounds.GetCenter().X, bPivotOrigin ? 0.0 : Bounds.GetCenter().Y,
            bPivotGround ? 0.0 : Bounds.Min.Z);
        FVector Ground = AtGround(Base.X + Offset.X, Base.Y + Offset.Y) + FVector(0, 0, Lift);
        // Soft ground cover (weeds, nettles, tall grass) sits on the soil actually drawn under its whole
        // clump, so none of it hovers on a slope or where the Landscape differs from the heightfield.
        const bool bSoil = !bProduce && IsSoilGrounded(Node.kind);
        bool bOnLandscape = true;
        const FVector2D ClumpCentre(Base.X + Offset.X, Base.Y + Offset.Y);
        const FVector2D ClumpHalf(Bounds.GetExtent().X * Scale, Bounds.GetExtent().Y * Scale);
        if (bSoil) Ground.Z = SoilHeight(ClumpCentre, ClumpHalf, Yaw, bOnLandscape) + Lift;
        auto* Component = NewObject<UStaticMeshComponent>(this);
        Component->SetupAttachment(GetRootComponent());
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetStaticMesh(Mesh);
        if (!ApplyCameraSafeFoliageMaterials(*Component))
        {
            Component->DestroyComponent();
            bVisualBuildFailed = true;
            return;
        }
        TagSwayingShrub(*Component);
        Component->SetRelativeTransform(FTransform(Rotation, Ground - Rotation.RotateVector(Anchor * Scale), FVector(Scale)));
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCollisionResponseToAllChannels(ECR_Ignore);
        if (IsMvpWoodlandId(Node.id) && !bProduce
            && (Node.kind == Homestead::ResourceKind::BrambleThin || Node.kind == Homestead::ResourceKind::BrambleThicket))
        {
            // As in the MVP, a bramble stops her until she cuts it; the camera boom and traces pass.
            Component->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
            Component->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
            Component->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
        }
        // The legacy stone mesh takes the textured rock material; authored Nanite stones (hand stones,
        // GraniteHandPile) keep their own, and M_Rock isn't flagged for Nanite.
        if (Node.kind == Homestead::ResourceKind::Stones && RockMaterial && !Mesh->HasValidNaniteData())
            Component->SetMaterial(0, RockMaterial);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCanEverAffectNavigation(false);
        Component->ComponentTags.Append({TEXT("AuthoredResource"), bProduce ? TEXT("ResourceProduce") : TEXT("ResourceBase")});
        if (bStagingResourceBuild)
        {
            Component->SetVisibility(false);
            Component->SetHiddenInGame(true);
        }
        Component->RegisterComponent();
        Visual.Components.Add(Component);
        // Landscape collision not streamed in yet: settle it onto the drawn soil once it is.
        if (bSoil && !bOnLandscape) QueueSoilGrounding(Component, ClumpCentre, ClumpHalf, Yaw, Ground.Z - Lift);
        // Solid clear-out obstacles (stumps, logs, boulders, barrels, crates, middens) block her
        // until they're cleared: a box a little inside the mesh's bounds, removed with the visual.
        // It ignores the camera and visibility traces, so neither the view nor her focus snags on it.
        using Homestead::ResourceKind;
        const ResourceKind Kind = Node.kind;
        const bool bSolid = Kind == ResourceKind::StumpSmall || Kind == ResourceKind::StumpMedium
            || Kind == ResourceKind::StumpLarge || Kind == ResourceKind::StumpAncient || Kind == ResourceKind::FallenLog
            || Kind == ResourceKind::GiantLog || Kind == ResourceKind::Boulder || Kind == ResourceKind::BrokenBarrel
            || Kind == ResourceKind::BrokenCrate || Kind == ResourceKind::RubbishHeap || Kind == ResourceKind::RuinTimbers;
        if (bSolid && !bProduce && !bStagingResourceBuild)
        {
            const FVector Extent = Bounds.GetExtent() * Scale;
            // Stumps are round: an upright capsule, so she can stand close to chop without snagging a corner.
            const bool bRound = Kind == ResourceKind::StumpSmall || Kind == ResourceKind::StumpMedium
                || Kind == ResourceKind::StumpLarge || Kind == ResourceKind::StumpAncient;
            UShapeComponent* Blocker = nullptr;
            if (bRound)
            {
                auto* Capsule = NewObject<UCapsuleComponent>(this);
                const float Radius = FMath::Min(Extent.X, Extent.Y) * 0.85f;
                Capsule->SetCapsuleSize(Radius, FMath::Max(Extent.Z, Radius), false);
                Blocker = Capsule;
            }
            else
            {
                auto* Box = NewObject<UBoxComponent>(this);
                Box->SetBoxExtent(FVector(Extent.X * 0.85f, Extent.Y * 0.85f, Extent.Z), false);
                Blocker = Box;
            }
            Blocker->SetupAttachment(GetRootComponent());
            Blocker->SetMobility(EComponentMobility::Movable);
            const float Rise = bRound ? CastChecked<UCapsuleComponent>(Blocker)->GetUnscaledCapsuleHalfHeight() : Extent.Z;
            const float Bottom = bPivotGround ? static_cast<float>(Bounds.Min.Z) * Scale : 0.0f;
            Blocker->SetRelativeTransform(FTransform(Rotation, Ground + FVector(0, 0, Bottom + Rise)));
            Blocker->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
            Blocker->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
            Blocker->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
            // The ruin's fallen roof timbers used to block as ruin scenery; as a resource they stop
            // only her (a pawn), so nothing else (felled trees, drops, traces) snags on the proxy. The
            // pile is under 60 cm, within her step height: without this she walked up onto it (PIE).
            if (Kind == ResourceKind::RuinTimbers)
            {
                Blocker->SetCollisionResponseToAllChannels(ECR_Ignore);
                Blocker->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
                Blocker->CanCharacterStepUpOn = ECB_No;
            }
            Blocker->SetGenerateOverlapEvents(false);
            Blocker->SetCanEverAffectNavigation(false);
            Blocker->SetHiddenInGame(true);
            Blocker->ComponentTags.Add(TEXT("ClearoutBlocker"));
            Blocker->RegisterComponent();
            Visual.Components.Add(Blocker);
        }
    };

    switch (Node.kind)
    {
    case Homestead::ResourceKind::ForestTree:
        break;
    case Homestead::ResourceKind::Branches:
        if (bProduceOnly)
        {
            // Scale matches the heroine's carried stick props (AHomesteadCharacter::CarriedStickScale),
            // so a stick keeps its size when she picks it up; component 1 (_b) and 2 (_c) are the
            // two she lifts.
            for (int I = 0; I < 3; ++I)
            {
                const TCHAR* Names[] = {TEXT("SM_DryBranchesMedium01_a"), TEXT("SM_DryBranchesMedium01_b"), TEXT("SM_DryBranchesMedium01_c")};
                Authored(LoadResource(Names[I]), FVector2D(I * 9 - 9, I * 7 - 7), I * 35 + 20, true, 0.6f);
            }
        }
        break;
    case Homestead::ResourceKind::Stones:
        if (bProduceOnly)
        {
            for (int I = 0; I < 3; ++I)
            {
                UStaticMesh* HandStone = AHomesteadCharacter::LoadHandStone(I);
                UStaticMesh* Rock = HandStone ? HandStone : ImportedRock.Get();
                if (Rock && Rock->GetBoundingBox().GetSize().GetMax() > 0)
                {
                    const int32 First = Visual.Components.Num();
                    Authored(Rock, FVector2D(I * 17 - 17, I % 2 * 14), I * 79, true,
                        AHomesteadCharacter::StonePileSize(I, HandStone != nullptr) / Rock->GetBoundingBox().GetSize().GetMax());
                    // Authored hand stones keep their baked granite material.
                    if (HandStone && Visual.Components.Num() > First)
                        if (auto* Placed = Cast<UStaticMeshComponent>(Visual.Components.Last()))
                            for (int32 Slot = 0; Slot < HandStone->GetStaticMaterials().Num(); ++Slot)
                                Placed->SetMaterial(Slot, HandStone->GetMaterial(Slot));
                }
                else
                {
                    bVisualBuildFailed = true;
                    UE_LOG(LogHomesteadWorld, Error, TEXT("Stone resource %d is missing admitted rock geometry."), Node.id);
                }
            }
            // A scatter of pebbles and cobbles round them (the Rocks Agent's GraniteHandPile), so loose
            // stones read at a glance as small enough to pick up. Appended after the three lifted stones,
            // whose component order the kneel gather relies on.
            static const TCHAR* const Clusters[] = {TEXT("SM_GraniteHandPile_A"), TEXT("SM_GraniteHandPile_B"), TEXT("SM_GraniteHandPile_C")};
            const TCHAR* ClusterName = Clusters[Variation % UE_ARRAY_COUNT(Clusters)];
            if (UStaticMesh* Cluster = LoadObject<UStaticMesh>(nullptr,
                *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/GraniteHandPile/%s.%s"), ClusterName, ClusterName),
                nullptr, LOAD_NoWarn | LOAD_Quiet))
            {
                const int32 First = Visual.Components.Num();
                // A touch larger than life (still under 15 cm tall) so the scatter reads from 10-15 m away.
                constexpr float HandPileScale = 1.25f;
                Authored(Cluster, FVector2D::ZeroVector, static_cast<float>(Variation % 360), true, HandPileScale, 0.0f, true);
                if (Visual.Components.Num() > First)
                    if (auto* Placed = Cast<UStaticMeshComponent>(Visual.Components.Last()))
                        for (int32 Slot = 0; Slot < Cluster->GetStaticMaterials().Num(); ++Slot)
                            Placed->SetMaterial(Slot, Cluster->GetMaterial(Slot));
            }
        }
        break;
    case Homestead::ResourceKind::BerryBush:
        if (Node.id >= Homestead::EstatePlacementIdBase && Node.id < Homestead::TransientResourceIdBase)
        {
            // Estate: a wild red-currant bush hung with ripe red strings that go when she picks them.
            auto Load = [&](const TCHAR* Folder, const TCHAR* Name) -> UStaticMesh*
            {
                auto* Mesh = LoadObject<UStaticMesh>(nullptr,
                    *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/%s.%s"), Folder, Name, Name));
                if (!Mesh)
                {
                    bVisualBuildFailed = true;
                    UE_LOG(LogHomesteadWorld, Error, TEXT("Berry bush %d is missing authored mesh %s"), Node.id, Name);
                }
                return Mesh;
            };
            const float Yaw = static_cast<float>(Variation % 360);
            const float BushScale = CurrantBushScale * Random.FRandRange(0.9f, 1.12f);
            Authored(Load(TEXT("WildCurrant"), TEXT("SM_WildCurrant")), FVector2D::ZeroVector, Yaw, false, BushScale, 0.0f, true, true);
            UStaticMesh* Fruit = Load(TEXT("CurrantProduce"), TEXT("SM_CurrantProduce"));
            for (int I = 0; I < CurrantProduceSectors && Fruit; ++I)
                Authored(Fruit, FVector2D::ZeroVector, Yaw + I * CurrantSectorYawStep, true, BushScale, 0.0f, true, true);
            break;
        }
        for (int I = 0; I < 3; ++I)
        {
            Authored(LoadResource(I == 1 ? TEXT("SM_Shrub04_a") : TEXT("SM_Shrub04_c")),
                FVector2D((I - 1) * 10, I % 2 * 10 - 5), I * 113, false);
        }
        if (bProduceOnly)
        {
            for (int I = 0; I < 8; ++I)
            {
                const float Angle = I * 2.399f;
                Part(Sphere, FVector(FMath::Cos(Angle) * 15, FMath::Sin(Angle) * 11, 16 + I % 3 * 4),
                    FVector(3.8f), FLinearColor(0.42f, 0.025f, 0.055f), FRotator::ZeroRotator, true);
            }
        }
        break;
    case Homestead::ResourceKind::Roots:
        for (int I = 0; I < 3; ++I)
        {
            Authored(LoadResource(TEXT("SM_Shrub04_a")), FVector2D(I * 8 - 8, I % 2 * 9), I * 120, false);
        }
        if (bProduceOnly)
        {
            // Two root crowns: the heroine's pouch gather lifts one per pickup.
            for (int I = 0; I < 2; ++I)
                Part(Sphere, FVector(I * 11 - 5, I * 4, 4), FVector(11, 11, 8), FLinearColor(0.65f, 0.43f, 0.19f),
                    FRotator::ZeroRotator, true);
        }
        break;
    case Homestead::ResourceKind::Flowers:
        // Wild marjoram, knee-high and rosy-purple in flower, so a herb patch reads from across the
        // pasture; she cuts the flowering stems and leaves the grass stubble.
        if (UStaticMesh* Marjoram = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Game/SurvivalGame/Environment/Props/WildMarjoram/SM_WildMarjoram.SM_WildMarjoram"), nullptr,
                LOAD_NoWarn | LOAD_Quiet))
        {
            const float Yaw = static_cast<float>(Variation % 360);
            Authored(LoadResource(TEXT("SM_GrassMedium01_tiny_a"), true), FVector2D(6, -4), Yaw, false);
            Authored(Marjoram, FVector2D::ZeroVector, Yaw, true, Random.FRandRange(0.9f, 1.1f));
            break;
        }
        for (int I = 0; I < 2; ++I)
        {
            const FVector2D Offset(I * 18 - 9, I * 6 - 3);
            Authored(LoadResource(TEXT("SM_GrassMedium01_tiny_a"), true), Offset, I * 137, false);
            Authored(LoadResource(I ? TEXT("SM_FlowerEmpodium_b") : TEXT("SM_FlowerEmpodium_a")),
                Offset, I * 137, true);
        }
        break;
    case Homestead::ResourceKind::Reeds:
        {
            const TCHAR* Path = bProduceOnly
                ? TEXT("/Game/SurvivalGame/Environment/Reeds/SM_ReedClump.SM_ReedClump")
                : TEXT("/Game/SurvivalGame/Environment/Reeds/SM_ReedStubble.SM_ReedStubble");
            UStaticMesh* Reeds = LoadObject<UStaticMesh>(nullptr, Path);
            if (!Reeds)
            {
                bVisualBuildFailed = true;
                UE_LOG(LogHomesteadWorld, Error, TEXT("Reed resource %d is missing its original authored mesh: %s"),
                    Node.id, Path);
                break;
            }
            Authored(Reeds, FVector2D::ZeroVector, static_cast<float>(Variation % 360), bProduceOnly);
        }
        break;
    case Homestead::ResourceKind::DeerRemains:
        {
            // The hide-covered remains while there is fur to take, bare bones after; UpdateVisuals
            // hides the bones while the remains are showing (both share one pivot and footprint).
            const TCHAR* Path = bProduceOnly
                ? TEXT("/Game/SurvivalGame/Environment/Props/DeerRemains/SM_DeerRemains.SM_DeerRemains")
                : TEXT("/Game/SurvivalGame/Environment/Props/DeerRemains/SM_DeerBones.SM_DeerBones");
            UStaticMesh* Deer = LoadObject<UStaticMesh>(nullptr, Path);
            if (!Deer)
            {
                bVisualBuildFailed = true;
                UE_LOG(LogHomesteadWorld, Error, TEXT("Deer remains %d are missing their authored mesh: %s"), Node.id, Path);
                break;
            }
            Authored(Deer, FVector2D::ZeroVector, static_cast<float>(Variation % 360), bProduceOnly);
        }
        break;
    case Homestead::ResourceKind::Sapling:
    {
        // Estate saplings are placed, not generated.
        if (Node.id >= Homestead::EstatePlacementIdBase && Node.id < Homestead::TransientResourceIdBase)
        {
            if (!bProduceOnly)
                BuildOvergrowth(Node, Variation, [&](UStaticMesh* Mesh, FVector2D Offset, float Yaw, bool bProduce, float Scale, bool bPivotGround)
                    { Authored(Mesh, Offset, Yaw, bProduce, Scale, 0.0f, bPivotGround); });
            break;
        }
        Homestead::Generation::GeneratedEntity Entity;
        if (Homestead::Generation::FindEntity(Descriptor, Node.key, Entity) != Homestead::Generation::Status::Ok)
        {
            bVisualBuildFailed = true;
            UE_LOG(LogHomesteadWorld, Error, TEXT("Generated sapling key cannot resolve."));
            break;
        }
        Authored(LoadResource(TEXT("SM_GrassMedium01_tiny_a"), true), FVector2D::ZeroVector,
            Entity.yawDegrees, false);
        if (Entity.paletteRole == Homestead::Generation::TreePaletteRole::BroadleafYoung)
            Authored(LoadResource(TEXT("SM_TreeSmall02_Woodland")), FVector2D::ZeroVector,
                Entity.yawDegrees, true, Entity.scalePermille / 1000.0f);
        else if (Entity.paletteRole == Homestead::Generation::TreePaletteRole::ConiferYoung)
        {
            if (Entity.variantIndex == 2)
            {
                auto* Mesh = LoadObject<UStaticMesh>(nullptr,
                    TEXT("/Game/Trials/TreePalette_20260921_01/Meshes/SM_FirPole.SM_FirPole"));
                if (!Mesh)
                {
                    bVisualBuildFailed = true;
                    UE_LOG(LogHomesteadWorld, Error, TEXT("Generated intermediate fir is missing; no substitute."));
                }
                Authored(Mesh, FVector2D::ZeroVector, Entity.yawDegrees, true,
                    Entity.scalePermille / 1000.0f);
            }
            else
                Authored(LoadResource(Entity.variantIndex % 2 ? TEXT("SM_FirSapling_a") : TEXT("SM_FirSapling_c")),
                    FVector2D::ZeroVector, Entity.yawDegrees, true, Entity.scalePermille / 1000.0f);
        }
        else
        {
            bVisualBuildFailed = true;
            UE_LOG(LogHomesteadWorld, Error, TEXT("Generated sapling has illegal palette role %d."),
                static_cast<int32>(Entity.paletteRole));
        }
        break;
    }
    default:
        // Estate overgrowth and flowers (add-overgrown-estate-clearing).
        BuildOvergrowth(Node, Variation, [&](UStaticMesh* Mesh, FVector2D Offset, float Yaw, bool bProduce, float Scale, bool bPivotGround)
            { Authored(Mesh, Offset, Yaw, bProduce, Scale, 0.0f, bPivotGround); });
        break;
    }
}

void AHomesteadWorld::BuildOvergrowth(const Homestead::ResourceNode& Node, uint32 Variation,
    const TFunctionRef<void(UStaticMesh*, FVector2D, float, bool, float, bool)>& Place)
{
    auto Load = [&](const TCHAR* Folder, const TCHAR* Name) -> UStaticMesh*
    {
        const FString Path = FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/%s.%s"), Folder, Name, Name);
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
        if (!Mesh)
        {
            bVisualBuildFailed = true;
            UE_LOG(LogHomesteadWorld, Error, TEXT("Overgrowth %d is missing authored mesh %s"), Node.id, *Path);
        }
        return Mesh;
    };
    // An optional authored mesh from another lane: loaded quietly, so its stand-in shows until it lands.
    auto QuietProp = [](const TCHAR* Folder, const TCHAR* Name) -> UStaticMesh*
    {
        return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/%s.%s"), Folder, Name, Name),
            nullptr, LOAD_NoWarn | LOAD_Quiet);
    };
    FRandomStream Random(static_cast<int32>(Variation * 2654435761u));
    const float Yaw = static_cast<float>(Variation % 360);
    // Overgrowth has no separate produce: the whole clump goes when she clears it.
    auto Whole = [&](UStaticMesh* Mesh, FVector2D Offset, float Turn, float Scale) { Place(Mesh, Offset, Yaw + Turn, false, Scale, false); };
    // Authored part-sunk, pivot on the ground line (the Rocks Agent's granite pick rocks).
    auto Sunk = [&](UStaticMesh* Mesh, float Scale) { Place(Mesh, FVector2D::ZeroVector, Yaw, false, Scale, true); };
    // The ruin's own slate and granite heaps (HomesteadRuinDebris.h) keep the mesh, turn and size the
    // ruin gave them, with their ground-centre pivot on the ground, so clearing them is the only change.
    if (const Homestead::RuinDebris::Spot* Debris = Homestead::RuinDebris::Find(Node.id))
    {
        const FString Folder = UTF8_TO_TCHAR(Debris->mesh);
        Place(Load(*Folder, *(TEXT("SM_") + Folder)), FVector2D::ZeroVector, static_cast<float>(Debris->yaw), false,
            static_cast<float>(Debris->scale), true);
        return;
    }
    switch (Node.kind)
    {
    case Homestead::ResourceKind::BrambleThin:
        if (IsMvpWoodlandId(Node.id))
            Whole(Load(TEXT("BlackberryBramble"), TEXT("SM_BlackberryBramble")), FVector2D::ZeroVector, 0, Random.FRandRange(0.85f, 1.2f));
        else
            Whole(Load(TEXT("BrambleOvergrowth"), TEXT("SM_BrambleThin")), FVector2D::ZeroVector, 0, Random.FRandRange(0.9f, 1.1f));
        break;
    case Homestead::ResourceKind::BrambleThicket:
        if (IsMvpWoodlandId(Node.id))
            Whole(Load(TEXT("BlackberryBramble"), TEXT("SM_BlackberryBrambleLarge")), FVector2D::ZeroVector, 0, Random.FRandRange(0.9f, 1.15f));
        else
            Whole(Load(TEXT("BrambleOvergrowth"), TEXT("SM_BrambleThicket")), FVector2D::ZeroVector, 0, Random.FRandRange(0.95f, 1.1f));
        break;
    case Homestead::ResourceKind::BrambleBank:
        Whole(Load(TEXT("BrambleOvergrowth"), TEXT("SM_BrambleBank")), FVector2D::ZeroVector, 0, Random.FRandRange(1.0f, 1.12f));
        break;
    case Homestead::ResourceKind::TallGrass:
        for (int32 I = 0; I < 3; ++I)
            Whole(Load(TEXT("GrassYarrowTuft"), TEXT("SM_GrassYarrowTuft")),
                FVector2D(FMath::Cos(I * 2.1f) * 16, FMath::Sin(I * 2.1f) * 16), I * 97.0f, Random.FRandRange(1.05f, 1.3f));
        break;
    case Homestead::ResourceKind::Weeds:
    {
        // A pullable weed must read as a weed, never as the pasture round it (Jenny's playtest: "Weeds
        // [E] Pull" with nothing to see). Dock, thistle-and-ragwort, or dandelion-and-plantain clumps
        // (weed_clump.py). Until they're imported, a young nettle patch with a coarse tuft stands in:
        // dark toothed leaves, taller than the grazed ring. Every live node draws one at its point.
        static const TCHAR* const Clumps[] = {TEXT("SM_WeedClump_Dock"), TEXT("SM_WeedClump_Thistle"), TEXT("SM_WeedClump_Dandelion")};
        if (UStaticMesh* Clump = QuietProp(TEXT("WeedClump"), Clumps[Variation % UE_ARRAY_COUNT(Clumps)]))
            // Authored with the rosette's base at z 0 and a few leaf tips dipping below it: the pivot
            // goes on the soil, so the rosette sits on it rather than hovering on its lowest leaf. Its
            // pivot is also where the pull's thinning (ThinResource) shrinks it, so it stays seated.
            Place(Clump, FVector2D::ZeroVector, Yaw, false, Random.FRandRange(0.95f, 1.15f), true);
        else
        {
            Whole(Load(TEXT("Nettle"), TEXT("SM_NettlePatch")), FVector2D::ZeroVector, 0, Random.FRandRange(0.7f, 0.8f));
            Whole(Load(TEXT("GrassYarrowTuft"), TEXT("SM_GrassYarrowTuft")), FVector2D(14, -10), 140, Random.FRandRange(0.9f, 1.0f));
        }
        break;
    }
    case Homestead::ResourceKind::Sapling:
        Whole(Load(TEXT("Hazel"), TEXT("SM_Hazel")), FVector2D::ZeroVector, 0, Random.FRandRange(0.45f, 0.6f));
        break;
    // Pickaxe rocks read as single chunky rocks far too big to lift, never as the pebbles she picks
    // up by hand (Jenny's playtest): the Rocks Agent's GranitePickRocks, with bigger granite stand-ins until
    // they land. Only the boulder blocks her (see Authored).
    case Homestead::ResourceKind::SmallRock:
        if (UStaticMesh* Rock = QuietProp(TEXT("GranitePickRocks"), TEXT("SM_GranitePickRock_Small")))
            Sunk(Rock, Random.FRandRange(0.95f, 1.05f));
        else
            Whole(Load(TEXT("GraniteBoulderLow"), TEXT("SM_GraniteBoulderLow")), FVector2D::ZeroVector, 0, Random.FRandRange(0.42f, 0.5f));
        break;
    case Homestead::ResourceKind::Rubble:
        // Masonry shed from the ruin: a heap of broken granite blocks.
        if (UStaticMesh* Rock = QuietProp(TEXT("GranitePickRubble"), TEXT("SM_GranitePickRubble")))
            Sunk(Rock, Random.FRandRange(0.95f, 1.05f));
        else
            Whole(Load(TEXT("GraniteBlockTalus"), TEXT("SM_GraniteBlockTalus")), FVector2D::ZeroVector, 0, Random.FRandRange(0.7f, 0.82f));
        break;
    case Homestead::ResourceKind::Boulder:
    {
        // Mostly the waist-high split boulder, with the rounded loaf as a second shape; both are far
        // bigger than any small rock or rubble heap.
        UStaticMesh* Rock = Variation % 3 == 0 ? QuietProp(TEXT("GranitePickRocks"), TEXT("SM_GranitePickRock_Medium")) : nullptr;
        if (!Rock) Rock = QuietProp(TEXT("GranitePickRocks"), TEXT("SM_GranitePickRock_Large"));
        if (Rock)
            Sunk(Rock, Random.FRandRange(0.95f, 1.05f));
        else
            Whole(Load(TEXT("GraniteBoulderLoaf"), TEXT("SM_GraniteBoulderLoaf")), FVector2D::ZeroVector, 0, Random.FRandRange(1.15f, 1.3f));
        break;
    }
    case Homestead::ResourceKind::SalvagePile:
        // Rusted iron among the ruin's leavings (the Crops Agent's scrap heap), not a pile of stones she
        // might take for loose ones; fallen masonry until it lands.
        if (UStaticMesh* Scrap = QuietProp(TEXT("EstateRubbish"), TEXT("SM_ScrapHeap")))
            Whole(Scrap, FVector2D::ZeroVector, 0, 1.0f);
        else
            Whole(Load(TEXT("GraniteCobbles"), TEXT("SM_GraniteCobbles")), FVector2D::ZeroVector, 0, 0.7f);
        break;
    case Homestead::ResourceKind::StumpSmall:
        Whole(Load(TEXT("EstateTimber"), TEXT("SM_StumpSmall")), FVector2D::ZeroVector, 0, Random.FRandRange(0.9f, 1.15f));
        break;
    case Homestead::ResourceKind::StumpLarge:
        Whole(Load(TEXT("EstateTimber"), TEXT("SM_StumpLarge")), FVector2D::ZeroVector, 0, Random.FRandRange(0.9f, 1.1f));
        break;
    case Homestead::ResourceKind::StumpAncient:
        Whole(Load(TEXT("EstateTimber"), TEXT("SM_StumpAncient")), FVector2D::ZeroVector, 0, Random.FRandRange(0.95f, 1.05f));
        break;
    case Homestead::ResourceKind::FallenBranch:
        Whole(Load(TEXT("EstateTimber"), TEXT("SM_FallenBough")), FVector2D::ZeroVector, 0, Random.FRandRange(0.85f, 1.1f));
        break;
    case Homestead::ResourceKind::FallenLog:
        Whole(Load(TEXT("EstateTimber"), TEXT("SM_FallenLog")), FVector2D::ZeroVector, 0, Random.FRandRange(0.9f, 1.1f));
        break;
    case Homestead::ResourceKind::GiantLog:
        Whole(Load(TEXT("EstateTimber"), TEXT("SM_GiantLog")), FVector2D::ZeroVector, 0, Random.FRandRange(0.95f, 1.05f));
        break;
    // The manor clear-out (add-coral-island-clearout).
    case Homestead::ResourceKind::Nettles:
        Whole(Load(TEXT("Nettle"), TEXT("SM_NettlePatch")), FVector2D::ZeroVector, 0, Random.FRandRange(0.85f, 1.15f));
        break;
    case Homestead::ResourceKind::StumpMedium:
        Whole(Load(TEXT("EstateTimber"), TEXT("SM_StumpLarge")), FVector2D::ZeroVector, 0, Random.FRandRange(0.58f, 0.7f));
        break;
    case Homestead::ResourceKind::BrokenCrate:
    case Homestead::ResourceKind::BrokenBarrel:
    case Homestead::ResourceKind::RubbishHeap:
    {
        // The Farm Agent's estate debris (EstateDebris, EstateRubbish); the store goods stand in until it lands.
        // The first clear-out rows (570000-570007) are its eight freed spots and keep the full
        // midden; elsewhere a heap is a small midden or a rusty scrap pile.
        auto Quiet = [](const TCHAR* Name, const TCHAR* Folder = TEXT("EstateDebris")) -> UStaticMesh*
        {
            return LoadObject<UStaticMesh>(nullptr,
                *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/%s.%s"), Folder, Name, Name), nullptr,
                LOAD_NoWarn | LOAD_Quiet);
        };
        const bool Crate = Node.kind == Homestead::ResourceKind::BrokenCrate;
        const bool Barrel = Node.kind == Homestead::ResourceKind::BrokenBarrel;
        UStaticMesh* Debris = nullptr;
        float Scale = Random.FRandRange(0.8f, 0.95f);
        if (Crate) Debris = Quiet(TEXT("SM_BrokenCrate"));
        else if (Barrel) Debris = Quiet(TEXT("SM_BrokenBarrel"));
        else if (Node.id >= 570000 && Node.id < 570008) Debris = Quiet(TEXT("SM_RubbishHeap"));
        else
        {
            Debris = Quiet(Variation % 2 ? TEXT("SM_ScrapHeap") : TEXT("SM_RubbishHeapSmall"), TEXT("EstateRubbish"));
            if (!Debris && (Debris = Quiet(TEXT("SM_RubbishHeap")))) Scale = Random.FRandRange(0.45f, 0.55f);
        }
        if (Debris)
            Whole(Debris, FVector2D::ZeroVector, 0, Scale);
        else if (Crate)
            Whole(Load(TEXT("StoreCrate"), TEXT("SM_Store_Crate")), FVector2D::ZeroVector, 0, 0.9f);
        else if (Barrel)
            Whole(Load(TEXT("StoreBarrel"), TEXT("SM_Store_Barrel")), FVector2D::ZeroVector, 0, 0.9f);
        else
        {
            // A midden stand-in: broken stone and a rotten board.
            Whole(Load(TEXT("GraniteCobbles"), TEXT("SM_GraniteCobbles")), FVector2D::ZeroVector, 0, 0.8f);
            Whole(Load(TEXT("EstateTimber"), TEXT("SM_FallenBough")), FVector2D(20, 10), 70, 0.6f);
        }
        break;
    }
    case Homestead::ResourceKind::RottenPlanks:
        if (auto* Planks = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Game/SurvivalGame/Environment/Props/EstateRubbish/SM_RottenPlanks.SM_RottenPlanks"), nullptr, LOAD_NoWarn | LOAD_Quiet))
            Whole(Planks, FVector2D::ZeroVector, 0, Random.FRandRange(0.9f, 1.05f));
        else
            // Until the Farm Agent's plank pile lands: the ruin's fallen roof timbers at plank scale.
            Whole(Load(TEXT("RuinFallenTimbers"), TEXT("SM_RuinFallenTimbers")), FVector2D::ZeroVector, 0, Random.FRandRange(0.34f, 0.4f));
        break;
    default:
        break;
    }
    if (Node.kind == Homestead::ResourceKind::WildGarlic || Node.kind == Homestead::ResourceKind::Bluebells
        || Node.kind == Homestead::ResourceKind::Primroses || Node.kind == Homestead::ResourceKind::WildDaffodils)
    {
        // Authored spring flowers (Scripts/Blender/Recipes wild_garlic.py, bluebell.py, primrose.py and wild_daffodil.py): the
        // whole flowering clump is what she picks, so it's the produce.
        const TCHAR* Path = Node.kind == Homestead::ResourceKind::WildGarlic
            ? TEXT("/Game/SurvivalGame/Environment/Props/WildGarlic/SM_WildGarlic.SM_WildGarlic")
            : Node.kind == Homestead::ResourceKind::Bluebells
            ? TEXT("/Game/SurvivalGame/Environment/Props/Bluebell/SM_BluebellClump.SM_BluebellClump")
            : Node.kind == Homestead::ResourceKind::Primroses
            ? TEXT("/Game/SurvivalGame/Environment/Props/Primrose/SM_PrimroseClump.SM_PrimroseClump")
            : TEXT("/Game/SurvivalGame/Environment/Props/WildDaffodil/SM_WildDaffodilClump.SM_WildDaffodilClump");
        if (auto* Clump = LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet))
        {
            // Primroses hug the ground, so their clumps are drawn a little large to read above the pasture.
            const bool Primrose = Node.kind == Homestead::ResourceKind::Primroses;
            Place(Clump, FVector2D::ZeroVector, Yaw, true, Random.FRandRange(0.9f, 1.15f) * (Primrose ? 1.3f : 1.0f), false);
            return;
        }
    }
    if (Node.kind == Homestead::ResourceKind::Primroses || Node.kind == Homestead::ResourceKind::Bluebells
        || Node.kind == Homestead::ResourceKind::WildDaffodils || Node.kind == Homestead::ResourceKind::WildGarlic)
    {
        // Stand-in: the meadow-herb flowers in a grass tuft, until each spring flower is authored.
        auto* Flower = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FlowerEmpodium_a.SM_FlowerEmpodium_a"));
        for (int32 I = 0; I < 3 && Flower; ++I)
            Place(Flower, FVector2D(FMath::Cos(I * 2.1f) * 12, FMath::Sin(I * 2.1f) * 12), Yaw + I * 120.0f, true, 1.0f, false);
        Whole(Load(TEXT("GrassYarrowTuft"), TEXT("SM_GrassYarrowTuft")), FVector2D::ZeroVector, 0, 0.6f);
    }
}

void AHomesteadWorld::StartClearPop(FHomesteadWorldVisual& Visual, const Homestead::ResourceNode& Node)
{
    FClearPop Pop;
    for (USceneComponent* Part : Visual.Components)
        if (IsValid(Part))
        {
            // A popping obstacle no longer blocks her.
            if (auto* Primitive = Cast<UPrimitiveComponent>(Part)) Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Pop.Parts.Add(Part);
            Pop.Scales.Add(Part->GetRelativeScale3D());
            Pop.Locations.Add(Part->GetRelativeLocation());
        }
    // The popping parts now belong to the pop, not the node's visual.
    Visual.Components.Reset();
    if (!Pop.Parts.IsEmpty()) ClearPops.Add(MoveTemp(Pop));

    // Chips of whatever it was: clippings, splinters or grit.
    using Homestead::ResourceKind;
    const ResourceKind Kind = Node.kind;
    const bool bStone = Kind == ResourceKind::Rubble || Kind == ResourceKind::SmallRock || Kind == ResourceKind::Boulder
        || Kind == ResourceKind::RubbishHeap || Kind == ResourceKind::SalvagePile || Kind == ResourceKind::SlateHeap;
    const bool bGreen = Kind == ResourceKind::TallGrass || Kind == ResourceKind::Weeds || Kind == ResourceKind::Nettles
        || Kind == ResourceKind::BrambleThin || Kind == ResourceKind::BrambleThicket || Kind == ResourceKind::BrambleBank
        || Kind == ResourceKind::Sapling;
    const TCHAR* Path = bStone ? TEXT("/Game/SurvivalGame/Environment/Props/GraniteSpalls/SM_GraniteSpalls.SM_GraniteSpalls")
        : bGreen ? TEXT("/Game/SurvivalGame/Environment/Props/GrassYarrowTuft/SM_GrassYarrowTuft.SM_GrassYarrowTuft")
        : TEXT("/Game/SurvivalGame/Environment/Props/EstateTimber/SM_FallenBough.SM_FallenBough");
    UStaticMesh* ChipMesh = LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (!ChipMesh) return;
    const float Size = Kind == ResourceKind::Boulder || Kind == ResourceKind::StumpLarge || Kind == ResourceKind::StumpAncient
        || Kind == ResourceKind::RubbishHeap || Kind == ResourceKind::BrambleThicket || Kind == ResourceKind::RuinTimbers ? 1.5f : 1.0f;
    FRandomStream Random(Node.id * 7919 + static_cast<int32>(GetWorld() ? GetWorld()->GetTimeSeconds() * 10.0 : 0.0));
    FClearPop Chips;
    Chips.bChips = true;
    Chips.Life = 0.75f;
    const FVector Origin = AtGround(Node.position.x, Node.position.y) + FVector(0, 0, 18.0f * Size);
    const int32 Count = FMath::RoundToInt(7 * Size);
    for (int32 Index = 0; Index < Count; ++Index)
    {
        auto* Chip = NewObject<UStaticMeshComponent>(this);
        Chip->SetupAttachment(GetRootComponent());
        Chip->SetMobility(EComponentMobility::Movable);
        Chip->SetStaticMesh(ChipMesh);
        Chip->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Chip->SetCollisionResponseToAllChannels(ECR_Ignore);
        Chip->SetGenerateOverlapEvents(false);
        Chip->SetCanEverAffectNavigation(false);
        Chip->SetCastShadow(false);
        const float Scale = (bStone ? 0.12f : bGreen ? 0.16f : 0.07f) * Random.FRandRange(0.7f, 1.2f) * FMath::Sqrt(Size);
        const FVector Start = Origin + FVector(Random.FRandRange(-12, 12), Random.FRandRange(-12, 12), Random.FRandRange(0, 10)) * Size;
        const FRotator Turn(Random.FRandRange(-40, 40), Random.FRandRange(0, 360), Random.FRandRange(-40, 40));
        Chip->SetRelativeTransform(FTransform(Turn, Start, FVector(Scale)));
        Chip->RegisterComponent();
        const float Heading = Random.FRandRange(0.0f, 2.0f * PI);
        const float Out = Random.FRandRange(90.0f, 220.0f) * Size;
        Chips.Parts.Add(Chip);
        Chips.Scales.Add(FVector(Scale));
        Chips.Locations.Add(Start);
        Chips.Velocities.Add(FVector(FMath::Cos(Heading) * Out, FMath::Sin(Heading) * Out, Random.FRandRange(260.0f, 420.0f)));
        Chips.Spins.Add(FRotator(Random.FRandRange(-540, 540), Random.FRandRange(-540, 540), Random.FRandRange(-540, 540)));
    }
    ClearPops.Add(MoveTemp(Chips));
}

void AHomesteadWorld::UpdateClearPops(float DeltaSeconds)
{
    for (int32 PopIndex = ClearPops.Num() - 1; PopIndex >= 0; --PopIndex)
    {
        FClearPop& Pop = ClearPops[PopIndex];
        Pop.Age += DeltaSeconds;
        const float T = FMath::Clamp(Pop.Age / Pop.Life, 0.0f, 1.0f);
        for (int32 Index = 0; Index < Pop.Parts.Num(); ++Index)
        {
            USceneComponent* Part = Pop.Parts[Index].Get();
            if (!Part) continue;
            if (Pop.bChips)
            {
                // Thrown out and falling, tumbling, shrinking away over the last third.
                const float Seconds = Pop.Age;
                const FVector& V = Pop.Velocities[Index];
                Part->SetRelativeLocation(Pop.Locations[Index] + FVector(V.X * Seconds, V.Y * Seconds,
                    V.Z * Seconds - 0.5f * 980.0f * Seconds * Seconds));
                Part->SetRelativeRotation(Part->GetRelativeRotation() + Pop.Spins[Index] * DeltaSeconds);
                Part->SetRelativeScale3D(Pop.Scales[Index] * FMath::Clamp((1.0f - T) * 3.0f, 0.0f, 1.0f));
            }
            else
            {
                // A quick swell, then it shrinks into the ground.
                const float Swell = T < 0.2f ? 1.0f + 0.1f * (T / 0.2f)
                    : 1.1f * (1.0f - FMath::SmoothStep(0.0f, 1.0f, (T - 0.2f) / 0.8f));
                Part->SetRelativeScale3D(Pop.Scales[Index] * FMath::Max(Swell, 0.001f));
                Part->SetRelativeLocation(Pop.Locations[Index] - FVector(0, 0, 12.0f * T));
            }
        }
        if (Pop.Age >= Pop.Life)
        {
            for (const TWeakObjectPtr<USceneComponent>& Part : Pop.Parts)
                if (Part.IsValid()) Part->DestroyComponent();
            ClearPops.RemoveAtSwap(PopIndex);
        }
    }
}
