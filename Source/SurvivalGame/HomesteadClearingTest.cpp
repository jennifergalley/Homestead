#include "HomesteadSmokeTest.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadHatchet.h"
#include "HomesteadWateringTool.h"
#include "HomesteadActionTestState.h"
#include "HomesteadWorld.h"
#include "HomesteadTestPaths.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/FileHelper.h"

namespace
{
struct FClearProbe
{
    Homestead::Simulation Expected;
    double Hour = 0;
    uint32 Starts = 0, GatherStarts = 0, WaterStarts = 0;
    bool Ready = false;
    FVector Actor, Hand, LeftToe, RightToe;
    FVector CameraCenter = FVector::ZeroVector;
    FLinearColor CameraParameter, TargetParameter;
    FRotator View;
    TArray<FString> CameraSnapshots;
    FString DecorationsBeforeHarvest;
};
}

void AHomesteadSmokeTest::PrepareClearingChecks()
{
    auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
    TArray<Homestead::ResourceNode> Saplings;
    TArray<Homestead::ResourceNode> FocusableSaplings;
    TArray<Homestead::ResourceNode> FocusableMatureTrees;
    for (const auto& Node : Controller->State().resources)
    {
        if (Node.kind != Homestead::ResourceKind::Sapling) continue;
        Saplings.Add(Node);
        const Homestead::Point ApproachPoint{Node.position.x, Node.position.y - 110};
        const Homestead::ResourceNode* Winner = nullptr;
        double Best = 280.0;
        for (const auto& Other : Controller->State().resources)
        {
            if (Other.cleared) continue;
            const double Distance = FMath::Sqrt(FMath::Square(Other.position.x - ApproachPoint.x)
                + FMath::Square(Other.position.y - ApproachPoint.y));
            if (Distance < Best) { Best = Distance; Winner = &Other; }
        }
        if (Winner && Winner->id == Node.id) FocusableSaplings.Add(Node);
    }
    for (const auto& Node : Controller->State().resources)
    {
        if (Node.kind != Homestead::ResourceKind::ForestTree) continue;
        const Homestead::Point ApproachPoint{Node.position.x, Node.position.y - 110};
        const Homestead::ResourceNode* Winner = nullptr;
        double Best = 280.0;
        for (const auto& Other : Controller->State().resources)
        {
            if (Other.cleared) continue;
            const double Distance = FMath::Sqrt(FMath::Square(Other.position.x - ApproachPoint.x)
                + FMath::Square(Other.position.y - ApproachPoint.y));
            if (Distance < Best) { Best = Distance; Winner = &Other; }
        }
        FocusableMatureTrees.Add(Node);
    }
    const auto CenterChunk = Controller->State().activeChunk;
    for (int32 Y = CenterChunk.y - 6; Y <= CenterChunk.y + 6
        && (FocusableSaplings.Num() < 3 || FocusableMatureTrees.IsEmpty()); ++Y)
        for (int32 X = CenterChunk.x - 6; X <= CenterChunk.x + 6
            && (FocusableSaplings.Num() < 3 || FocusableMatureTrees.IsEmpty()); ++X)
        {
            Homestead::Generation::ChunkBaseline Baseline;
            if (Homestead::Generation::GenerateChunk(Controller->State().world, {X, Y}, Baseline)
                != Homestead::Generation::Status::Ok) continue;
            for (const auto& Entity : Baseline.entities)
            {
                const bool NeedSapling = FocusableSaplings.Num() < 3
                    && Entity.kind == Homestead::Generation::EntityKind::Sapling;
                const bool NeedMature = FocusableMatureTrees.IsEmpty()
                    && Entity.kind == Homestead::Generation::EntityKind::ForestTree;
                if (!NeedSapling && !NeedMature) continue;
                const bool Known = Saplings.ContainsByPredicate(
                    [&Entity](const Homestead::ResourceNode& Node) { return Node.key == Entity.key; });
                if (Known) continue;
                Homestead::ResourceNode Node{};
                Node.key = Entity.key;
                Node.kind = NeedSapling ? Homestead::ResourceKind::Sapling : Homestead::ResourceKind::ForestTree;
                Node.position = {static_cast<double>(Entity.xCm), static_cast<double>(Entity.yCm)};
                if (NeedSapling) { Saplings.Add(Node); FocusableSaplings.Add(Node); }
                else FocusableMatureTrees.Add(Node);
            }
        }
    if (!Avatar || !Avatar->HasHeroine() || Saplings.Num() < 3
        || FocusableSaplings.Num() < 3 || FocusableMatureTrees.IsEmpty())
    { Finish(false, TEXT("Clearing needs the real heroine, three saplings and a mature tree.")); return; }
    const auto Tree = MakeShared<Homestead::ResourceNode>(FocusableSaplings[0]);
    const auto MatureTree = MakeShared<Homestead::ResourceNode>(FocusableMatureTrees[0]);
    auto Probe = MakeShared<FClearProbe>();
    auto Animation = [Avatar]() { return Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()); };
    auto Hidden = [Avatar, Animation]() { return Animation() && Animation()->ActionWeight() < 0.001f
        && !Avatar->GetHatchet()->IsPresented() && !Avatar->GetWateringTool()->IsPresented(); };
    auto Snapshot = [this, Probe, Animation]()
    {
        Probe->Expected = Controller->Simulation(); Probe->Hour = Controller->State().hour;
        Probe->Starts = Animation()->ClearStarts(); Probe->GatherStarts = Animation()->GatherStarts();
        Probe->WaterStarts = Animation()->WaterStarts();
    };
    auto Matches = [this, Probe]() { return MatchesActionState(*Controller, Probe->Expected, Probe->Hour); };
    auto Approach = [this, Avatar, Hidden](TSharedPtr<Homestead::ResourceNode> Node)
    {
        Add(TEXT("Resolve and approach semantic sapling ") + FString::FromInt(Node->id),
            [this, Avatar, Node]()
            {
                Teleport(Node->position);
                Homestead::ResourceNode Current{};
                if (Controller->Simulation().ResolveGeneratedResource(Node->key, Current)) *Node = Current;
                Homestead::Point ApproachPoint{Node->position.x, Node->position.y - 110};
                bool Found = false;
                for (const double Radius : {110.0, 150.0, 190.0})
                    for (int32 Offset = 0; Offset < 16 && !Found; ++Offset)
                    {
                        const int32 Direction = (12 + Offset) % 16;
                        const double Angle = 2.0 * PI * Direction / 16.0;
                        const Homestead::Point Candidate{Node->position.x + Radius * FMath::Cos(Angle),
                            Node->position.y + Radius * FMath::Sin(Angle)};
                        const Homestead::ResourceNode* Winner = nullptr;
                        double Best = 280.0;
                        for (const auto& Other : Controller->State().resources)
                        {
                            if (Other.cleared) continue;
                            const double Distance = FMath::Sqrt(FMath::Square(Other.position.x - Candidate.x)
                                + FMath::Square(Other.position.y - Candidate.y));
                            if (Distance < Best) { Best = Distance; Winner = &Other; }
                        }
                        if (Winner && Winner->id == Node->id) { ApproachPoint = Candidate; Found = true; }
                    }
                Avatar->CancelAction(); Teleport(ApproachPoint);
            },
            [this, Node, Hidden]() { return Controller->IsResourceFocused(Node->id) && Hidden(); }, 0.7f);
    };
    auto Restore = [this, Hidden, Approach, Tree]()
    {
        Add(TEXT("Reload the real crafted checkpoint, including unchanged sapling IDs"),
            [this]() { Tap(EKeys::F9); }, [this, Hidden]() { return !Controller->ToastIsError() && Hidden(); }, 0.8f);
        Approach(Tree);
    };
    auto Clear = [this, Avatar, Probe, Animation, Snapshot, Matches](TSharedPtr<Homestead::ResourceNode> Node, FKey Key)
    {
        Add(TEXT("One authoritative tree clear with scale-one held hatchet: ") + Key.ToString(),
            [this, Avatar, Probe, Snapshot, Node, Key]()
            {
                Snapshot(); Probe->Ready = Probe->Expected.Clear(Node->id, Controller->PlayerPoint()).ok;
                Probe->Actor = Avatar->GetActorLocation(); Probe->Hand = Avatar->GetMesh()->GetBoneLocation(TEXT("hand_r"));
                Probe->LeftToe = Avatar->GetMesh()->GetBoneLocation(TEXT("ball_l")); Probe->RightToe = Avatar->GetMesh()->GetBoneLocation(TEXT("ball_r"));
                Probe->View = Controller->GetControlRotation(); Tap(Key);
            }, [this, Avatar, Probe, Animation, Matches, Node]()
            {
                const auto* Tool = Avatar->GetHatchet();
                const float ExpectedYaw = FMath::RadiansToDegrees(FMath::Atan2(
                    Node->position.y - Probe->Actor.Y, Node->position.x - Probe->Actor.X));
                return Probe->Ready && !Controller->ToastIsError() && Matches()
                    && Animation()->ClearStarts() == Probe->Starts + 1 && Animation()->GatherStarts() == Probe->GatherStarts
                    && Animation()->WaterStarts() == Probe->WaterStarts && Animation()->ClearWeight() > 0.99f
                    && Tool->IsPresented() && !Avatar->GetWateringTool()->IsPresented()
                    && Tool->GetAttachParent() == Avatar->GetMesh() && Tool->GetAttachSocketName() == TEXT("hand_r")
                    && Tool->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !Tool->GetGenerateOverlapEvents()
                    && !Tool->CanEverAffectNavigation() && Tool->GetNumSections() == 3
                    && Tool->GetComponentScale().Equals(FVector::OneVector, 0.001f)
                    && Tool->Bounds.SphereRadius > 10 && Tool->Bounds.SphereRadius < 35
                    && FVector::Dist(Tool->GripPosition(), Avatar->GetMesh()->GetBoneLocation(TEXT("hand_r"))) < 10
                    && FVector::Dist(Probe->Hand, Avatar->GetMesh()->GetBoneLocation(TEXT("hand_r"))) > 20
                    && FVector::Dist(Probe->Actor, Avatar->GetActorLocation()) < 1
                    && FVector::Dist(Probe->LeftToe, Avatar->GetMesh()->GetBoneLocation(TEXT("ball_l"))) < 2
                    && FVector::Dist(Probe->RightToe, Avatar->GetMesh()->GetBoneLocation(TEXT("ball_r"))) < 2
                    && Probe->View.Equals(Controller->GetControlRotation(), 0.01f)
                    && FMath::Abs(FMath::FindDeltaAngleDegrees(
                        Avatar->ClearTargetYaw(), ExpectedYaw)) < 0.1f
                    && Controller->State().hour - Probe->Hour < 0.02;
            }, 0.45f);
    };
    Add(TEXT("Close notes for actual gather/craft setup"), [this]() { Tap(EKeys::Gamepad_Special_Right); },
        [this, Hidden]() { return !Controller->IsBookOpen() && Hidden(); });
    Approach(Tree);
    Add(TEXT("No-hatchet rejection gives no reward, energy cost, clear or prop"),
        [this, Snapshot]() { Snapshot(); Tap(EKeys::F); },
        [this, Probe, Animation, Hidden, Matches]()
        { return Controller->ToastIsError() && Hidden() && Matches() && Animation()->ClearStarts() == Probe->Starts; });
    QueueGatherTo(Homestead::Item::Branch, 4);
    QueueGatherTo(Homestead::Item::Stone, 3);
    QueueGatherTo(Homestead::Item::Fiber, 2);
    QueueCraft(Homestead::Recipe::Hatchet);
    Add(TEXT("Save actual crafted setup for independent appearance/transaction cases"),
        [this]() { Tap(EKeys::F5); }, [this, Hidden]()
        { return !Controller->ToastIsError() && Controller->Simulation().Count(Homestead::Item::Hatchet) == 1 && Hidden(); });
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadCameraLifecycle")))
    {
        const auto Decorations = [this]()
        {
            auto* Landscape = Controller->Landscape.Get();
            if (!Landscape) return FString(TEXT("missing landscape"));
            TArray<FString> Parts;
            for (const auto& Entry : Landscape->DecorationBatches)
                Parts.Add(FString::Printf(TEXT("%s:%s:%d"), *Entry.Key.ToString(),
                    *GetPathNameSafe(Entry.Value.Get()), Entry.Value->GetInstanceCount()));
            TArray<UStaticMeshComponent*> Meshes;
            Landscape->GetComponents(Meshes);
            for (const auto* Mesh : Meshes)
                if (Mesh->ComponentHasTag(TEXT("AuthoredTreeSmall02Grove")) || Mesh->ComponentHasTag(TEXT("AuthoredFern02"))
                    || Mesh->ComponentHasTag(TEXT("AuthoredFirUnderstory")))
                    Parts.Add(Mesh->GetPathName() + TEXT(":") + Mesh->GetComponentTransform().ToString());
            Parts.Sort();
            return Landscape->ResourceLayoutSignature + FString::Join(Parts, TEXT("\n"));
        };
        Add(TEXT("Woodland reservations distinguish access, canopy and a permanently cleared cell"),
            []() {}, [this]()
            {
                auto State = Controller->State();
                State.resources.clear(); State.structures.clear(); State.plots.clear();
                Homestead::ResourceNode Node{};
                Node.id = 999; Node.position = {-1950, -1950};
                State.resources.push_back(Node);
                const auto Reserved = [&State](float X, float Y, float Footprint, float Canopy = 0)
                    { return AHomesteadWorld::IsDecorationReserved(State, X, Y, Footprint, Canopy); };
                const bool Access = Reserved(-1750, -1950, 90, 330);
                const bool OverlapAllowed = !Reserved(-1700, -1950, 90, 330);
                const bool CornerInitiallyOpen = !Reserved(-1801, -1801, 20);
                State.resources[0].readyAtHour = State.hour + 48;
                const bool HarvestKeepsCorner = !Reserved(-1801, -1801, 20);
                const bool LowCoverNearResource = AHomesteadWorld::IsDecorationReserved(State, -1850, -1950, 20)
                    && !AHomesteadWorld::IsDecorationReserved(State, -1850, -1950, 20, 0, true);
                const bool LowCoverHomeEdge = !AHomesteadWorld::IsDecorationReserved(
                    State, -600, 400, 20, 0, true);
                const bool PathProtected = !AHomesteadWorld::IsDecorationReserved(
                    State, 0, 75, 20, 0, true);
                State.resources[0].cleared = true;
                const bool ClearedCornerReserved = Reserved(-1801, -1801, 20)
                    && AHomesteadWorld::IsDecorationReserved(State, -1801, -1801, 20, 0, true);
                State.resources.clear();
                Homestead::Structure Structure{};
                Structure.cellX = -7; Structure.cellY = -7;
                State.structures.push_back(Structure);
                const bool CanopyProtectsBuilding = Reserved(-1550, -1950, 90, 330)
                    && !Reserved(-1550, -1950, 90);
                const bool LowCoverProtectsBuilding = AHomesteadWorld::IsDecorationReserved(State, -1750, -1950, 20, 0, true);
                State.structures.clear();
                Homestead::Plot Plot{};
                Plot.cellX = -7; Plot.cellY = -7;
                State.plots.push_back(Plot);
                const bool LowCoverProtectsPlot = AHomesteadWorld::IsDecorationReserved(State, -1800, -1950, 20, 0, true);
                Results.Add(FString::Printf(TEXT("RESERVATION access=%d overlap=%d corner=%d harvest=%d cleared=%d canopy_build=%d low_resource=%d low_home=%d path=%d low_build=%d low_plot=%d creek_far=%.6f creek_home=%.6f"),
                    Access, OverlapAllowed, CornerInitiallyOpen, HarvestKeepsCorner,
                    ClearedCornerReserved, CanopyProtectsBuilding, LowCoverNearResource,
                    LowCoverHomeEdge, PathProtected, LowCoverProtectsBuilding,
                    LowCoverProtectsPlot,
                    Homestead::Generation::CreekGroundBlendWeight(
                        Controller->State().world, -2900, -2900),
                    Homestead::Generation::CreekGroundBlendWeight(
                        Controller->State().world, -1000, 0)));
                return Access && OverlapAllowed && CornerInitiallyOpen && HarvestKeepsCorner
                    && ClearedCornerReserved && CanopyProtectsBuilding && LowCoverNearResource
                    && LowCoverHomeEdge && PathProtected && LowCoverProtectsBuilding && LowCoverProtectsPlot
                    && Homestead::Generation::CreekGroundBlendWeight(
                        Controller->State().world, -2900, -2900) >= 0
                    && Homestead::Generation::CreekGroundBlendWeight(
                        Controller->State().world, -2900, -2900) <= 1
                    && Homestead::Generation::CreekGroundBlendWeight(
                        Controller->State().world, -1000, 0) >= 0
                    && Homestead::Generation::CreekGroundBlendWeight(
                        Controller->State().world, -1000, 0) <= 1;
            });
        const auto CameraCanopy = [this, Tree, Probe](bool Ready, bool Cleared)
        {
            auto* Landscape = Controller->Landscape.Get();
            const auto* Produce = Landscape ? Landscape->ResourceProduceVisuals.Find(Tree->id) : nullptr;
            const auto* Base = Landscape ? Landscape->ResourceVisuals.Find(Tree->id) : nullptr;
            if (!Produce || !Base || Produce->Components.Num() != (Ready ? 1 : 0)
                || Base->Components.Num() != (Cleared ? 0 : 1)) return false;
            int32 CameraSafeMeshes = 0;
            for (const auto& Component : Produce->Components)
            {
                const auto* Mesh = Cast<UStaticMeshComponent>(Component);
                const bool ReadyMesh = Mesh && Mesh->IsRegistered() && Mesh->IsVisible()
                    && !Mesh->bHiddenInGame && !Mesh->CanEverAffectNavigation()
                    && !Mesh->GetGenerateOverlapEvents()
                    && Mesh->ComponentHasTag(TEXT("AuthoredResource"));
                const FString MeshPath = Mesh && Mesh->GetStaticMesh()
                    ? Mesh->GetStaticMesh()->GetPathName() : FString();
                const bool Broadleaf = MeshPath.Contains(
                    TEXT("SM_TreeSmall02_Woodland"));
                const bool Intermediate = MeshPath.Contains(TEXT("SM_FirPole."));
                const bool KnownSapling = Broadleaf
                    || MeshPath.Contains(TEXT("SM_FirSapling_"))
                    || MeshPath.Contains(TEXT("SM_FirPole."));
                const bool PolicyReady = Mesh && KnownSapling
                    && Mesh->GetComponentScale().X >= 0.8f
                    && Mesh->GetComponentScale().X <= 1.2f
                    && Mesh->ComponentHasTag(TEXT("CameraSafeFoliage"))
                    && Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
                    && !Mesh->IsQueryCollisionEnabled()
                    && Mesh->GetCollisionResponseToChannel(ECC_Camera) == ECR_Ignore
                    && Mesh->GetNumMaterials() == (Broadleaf || Intermediate ? 3 : 2)
                    && (Broadleaf
                        ? GetPathNameSafe(Mesh->GetMaterial(1)).Contains(
                            TEXT("MI_CameraSafe_TreeSmallLeaves"))
                            && Mesh->GetMaterial(0) == Mesh->GetStaticMesh()->GetMaterial(0)
                            && Mesh->GetMaterial(2) == Mesh->GetStaticMesh()->GetMaterial(2)
                        : Intermediate
                            ? GetPathNameSafe(Mesh->GetMaterial(1)).Contains(
                                TEXT("MI_CameraSafe_FirPoleTwigs"))
                                && Mesh->GetMaterial(0) == Mesh->GetStaticMesh()->GetMaterial(0)
                                && Mesh->GetMaterial(2) == Mesh->GetStaticMesh()->GetMaterial(2)
                            : GetPathNameSafe(Mesh->GetMaterial(0)).Contains(
                            TEXT("MI_CameraSafe_FirSaplingBranches"))
                            && GetPathNameSafe(Mesh->GetMaterial(1)).Contains(
                                TEXT("MI_CameraSafe_FirSaplingTwigs")));
                Probe->CameraSnapshots.Add(FString::Printf(
                    TEXT("component=%s ready_mesh=%d policy=%d collision=%d query=%d camera=%d tags=%s material0=%s material1=%s"),
                    *GetPathNameSafe(Mesh), ReadyMesh, PolicyReady,
                    Mesh ? static_cast<int32>(Mesh->GetCollisionEnabled()) : -1,
                    Mesh && Mesh->IsQueryCollisionEnabled(),
                    Mesh ? static_cast<int32>(Mesh->GetCollisionResponseToChannel(ECC_Camera)) : -1,
                    Mesh ? *FString::JoinBy(Mesh->ComponentTags, TEXT("|"),
                        [](FName Name) { return Name.ToString(); }) : TEXT(""),
                    Mesh ? *GetPathNameSafe(Mesh->GetMaterial(0)) : TEXT(""),
                    Mesh ? *GetPathNameSafe(Mesh->GetMaterial(1)) : TEXT("")));
                if (!ReadyMesh || !PolicyReady)
                {
                    FFileHelper::SaveStringArrayToFile(Probe->CameraSnapshots,
                        *FPaths::Combine(HomesteadTestOutputDirectory(),
                            TEXT("camera-lifecycle.txt")));
                    return false;
                }
                Probe->CameraCenter = Mesh->Bounds.Origin;
                ++CameraSafeMeshes;
            }
            const FVector Center = Probe->CameraCenter;
            if (Center.ContainsNaN() || Center.Z <= Controller->GroundHeight(Tree->position.x, Tree->position.y) + 20)
                return false;
            FHitResult Hit;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(SaplingCameraLifecycle), false, Controller->GetPawn());
            const bool Blocked = GetWorld()->SweepSingleByChannel(Hit, Center - FVector(150, 0, 0),
                Center + FVector(150, 0, 0), FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(12), Query);
            bool OwnProduceHit = false;
            for (const auto& Component : Produce->Components) OwnProduceHit |= Hit.GetComponent() == Component;
            Probe->CameraSnapshots.Add(FString::Printf(TEXT("node=%d ready=%d cleared=%d camera_safe=%d sweep=%d own_produce=%d"),
                Tree->id, Ready, Cleared, CameraSafeMeshes, Blocked, OwnProduceHit));
            const bool Persisted = FFileHelper::SaveStringArrayToFile(Probe->CameraSnapshots,
                *FPaths::Combine(HomesteadTestOutputDirectory(), TEXT("camera-lifecycle.txt")));
            return Persisted && CameraSafeMeshes == (Ready ? 1 : 0) && !Blocked && !OwnProduceHit;
        };
        Approach(Tree);
        Add(TEXT("Visible sapling canopy ignores the real camera query"),
            []() {}, [CameraCanopy]() { return CameraCanopy(true, false); });
        Add(TEXT("Post-camera collection matches the actual gameplay camera and heroine target"),
            []() {}, [this, Probe]()
            {
                auto* Collection = LoadObject<UMaterialParameterCollection>(nullptr,
                    TEXT("/Game/SurvivalGame/Environment/CameraSafeFoliage/MPC_CameraSafeFoliage.MPC_CameraSafeFoliage"));
                auto* Parameters = Collection
                    ? GetWorld()->GetParameterCollectionInstance(Collection) : nullptr;
                FLinearColor CameraValue = FLinearColor::Transparent;
                FLinearColor TargetValue = FLinearColor::Transparent;
                FVector CameraPosition;
                FRotator CameraRotation;
                Controller->GetPlayerViewPoint(CameraPosition, CameraRotation);
                const FVector HeroTarget = Controller->GetPawn()->GetActorLocation()
                    + FVector(0, 0, 65);
                const bool Matches = Parameters
                    && Parameters->GetVectorParameterValue(TEXT("CameraPosition"), CameraValue)
                    && Parameters->GetVectorParameterValue(TEXT("HeroTargetPosition"), TargetValue)
                    && FVector(CameraValue.R, CameraValue.G, CameraValue.B).Equals(
                        CameraPosition, 2.0f)
                    && FVector(TargetValue.R, TargetValue.G, TargetValue.B).Equals(
                        HeroTarget, 2.0f);
                if (Matches)
                {
                    Probe->CameraParameter = CameraValue;
                    Probe->TargetParameter = TargetValue;
                }
                return Matches;
            });
        Add(TEXT("Appearance portrait cannot overwrite gameplay foliage parameters"),
            [this]() { Controller->OpenBook(6); },
            [this, Probe]()
            {
                auto* Collection = LoadObject<UMaterialParameterCollection>(nullptr,
                    TEXT("/Game/SurvivalGame/Environment/CameraSafeFoliage/MPC_CameraSafeFoliage.MPC_CameraSafeFoliage"));
                auto* Parameters = Collection
                    ? GetWorld()->GetParameterCollectionInstance(Collection) : nullptr;
                FLinearColor CameraValue = FLinearColor::Transparent;
                FLinearColor TargetValue = FLinearColor::Transparent;
                return Controller->BookPage() == 6 && Parameters
                    && Parameters->GetVectorParameterValue(TEXT("CameraPosition"), CameraValue)
                    && Parameters->GetVectorParameterValue(TEXT("HeroTargetPosition"), TargetValue)
                    && CameraValue.Equals(Probe->CameraParameter, 0.01f)
                    && TargetValue.Equals(Probe->TargetParameter, 0.01f);
            }, 0.5f);
        Add(TEXT("Return from portrait and reacquire the same sapling"),
            [this]() { Controller->CloseBook(); },
            [this]() { return !Controller->IsBookOpen(); });
        Approach(Tree);
        Add(TEXT("Capture the camera-safe sapling before harvesting"),
            [this]() { Screenshot(TEXT("camera-safe-sapling")); },
            [CameraCanopy]() { return CameraCanopy(true, false); }, 0.8f);
        Add(TEXT("Mapped sapling harvest removes camera-safe produce without gameplay changes"),
            [this, Probe, Decorations]() { Probe->DecorationsBeforeHarvest = Decorations(); Tap(EKeys::E); },
            [this, Tree, CameraCanopy, Probe, Decorations]() { return !Controller->ToastIsError()
                && !Controller->Simulation().CanHarvest(Tree->id) && CameraCanopy(false, false)
                && Decorations() == Probe->DecorationsBeforeHarvest; });
        Restore();
        Add(TEXT("Checkpoint reload restores the visible camera-safe canopy"),
            []() {}, [CameraCanopy]() { return CameraCanopy(true, false); });
        Clear(Tree, EKeys::F);
        Add(TEXT("Permanent clear removes the camera-safe canopy"),
            [this]() { Screenshot(TEXT("clearing-swing")); },
            [CameraCanopy]() { return CameraCanopy(false, true); }, 0.12f);
        Add(TEXT("Clearing recovery leaves no invisible blocker"),
            []() {}, [Hidden, CameraCanopy]() { return Hidden() && CameraCanopy(false, true); }, 2.0f);
        Add(TEXT("Capture recovered cleared site"),
            [this]() { Screenshot(TEXT("clearing-recovered")); }, Hidden);
        Add(TEXT("Save the permanent clear"), [this]() { Tap(EKeys::F5); },
            [this, CameraCanopy]() { return !Controller->ToastIsError() && CameraCanopy(false, true); });
        Add(TEXT("Reload preserves the cleared ID and absence of camera blockers"),
            [this]() { Tap(EKeys::F9); },
            [this, Hidden, Matches, CameraCanopy]() { return !Controller->ToastIsError()
                && Hidden() && Matches() && CameraCanopy(false, true); }, 0.8f);
        const int32 ClearedCellX = FMath::FloorToInt(Tree->position.x / Homestead::CellSize);
        const int32 ClearedCellY = FMath::FloorToInt(Tree->position.y / Homestead::CellSize);
        Add(TEXT("Cleared build-cell corners remain reserved after the actual saved reload"),
            []() {}, [this, ClearedCellX, ClearedCellY]()
            {
                const auto Center = Homestead::CellCenter(ClearedCellX, ClearedCellY);
                for (const int32 X : {-1, 1})
                    for (const int32 Y : {-1, 1})
                    {
                        const float PX = Center.x + X * 100, PY = Center.y + Y * 100;
                        if (!AHomesteadWorld::IsDecorationReserved(Controller->State(), PX, PY, 20))
                            return false;
                    }
                return true;
            });
        return;
    }
    Homestead::ResourceNode OtherTree{};
    Homestead::ResourceNode Branch{};
    double BestPairDistance = TNumericLimits<double>::Max();
    for (const auto& Candidate : Saplings)
    {
        if (Candidate.id == Tree->id) continue;
        for (const auto& Node : Controller->State().resources)
        {
            if (Node.kind != Homestead::ResourceKind::Branches || Node.cleared) continue;
            const double Distance = FMath::Sqrt(FMath::Square(Node.position.x - Candidate.position.x)
                + FMath::Square(Node.position.y - Candidate.position.y));
            if (Distance < BestPairDistance)
            { BestPairDistance = Distance; OtherTree = Candidate; Branch = Node; }
        }
    }
    if (!OtherTree.id || !Branch.id || BestPairDistance > 300.0)
    { Finish(false, TEXT("Default adjacent branch/sapling fixture is missing.")); return; }
    Add(TEXT("Functional approach between adjacent branch and sapling for actual rapid input"),
        [this, Branch, OtherTree]() { Teleport({Branch.position.x * 0.6 + OtherTree.position.x * 0.4,
            Branch.position.y * 0.6 + OtherTree.position.y * 0.4}); },
        [this, Branch, Hidden]() { return Controller->IsResourceFocused(Branch.id) && Hidden(); }, 0.7f);
    Add(TEXT("Rapid mapped E/F/F preserves branch gather, branch clear and sapling yield once, with one pose"),
        [this, Probe, Snapshot, Branch, OtherTree]()
        {
            Snapshot();
            Probe->Ready = Probe->Expected.Harvest(Branch.id, Controller->PlayerPoint()).ok
                && Probe->Expected.Clear(Branch.id, Controller->PlayerPoint()).ok
                && Probe->Expected.Clear(OtherTree.id, Controller->PlayerPoint()).ok;
            Tap(EKeys::E); Tap(EKeys::F); Tap(EKeys::F);
        }, [this, Avatar, Probe, Animation, Matches]()
        {
            return Probe->Ready && !Controller->ToastIsError() && Matches()
                && Animation()->ClearStarts() == Probe->Starts + 1 && Animation()->GatherStarts() == Probe->GatherStarts
                && Animation()->WaterStarts() == Probe->WaterStarts && Avatar->GetHatchet()->IsPresented()
                && !Avatar->GetWateringTool()->IsPresented();
        }, 0.55f);
    Restore();
    Add(TEXT("Sapling harvest A is not permanent clearing and does not swing the hatchet"),
        [this, Probe, Snapshot, Tree]()
        { Snapshot(); Probe->Ready = Probe->Expected.Harvest(Tree->id, Controller->PlayerPoint()).ok; Tap(EKeys::E); },
        [Probe, Animation, Hidden, Matches]()
        { return Probe->Ready && Matches() && Hidden() && Animation()->ClearStarts() == Probe->Starts; });
    Clear(Tree, EKeys::Gamepad_FaceButton_Left);
    Add(TEXT("Capture restrained clearing swing"), [this]() { Screenshot(TEXT("clearing-swing")); },
        [Avatar, Animation]() { return Animation()->ClearWeight() > 0.99f && Avatar->GetHatchet()->IsPresented(); }, 0.12f);
    Homestead::Point EmptyContext{};
    bool HasEmptyContext = false;
    for (int32 Radius = 200; Radius <= 300 && !HasEmptyContext; Radius += 50)
        for (int32 Direction = 0; Direction < 16 && !HasEmptyContext; ++Direction)
        {
            const double Angle = 2.0 * PI * Direction / 16.0;
            const Homestead::Point Candidate{Tree->position.x + Radius * FMath::Cos(Angle),
                Tree->position.y + Radius * FMath::Sin(Angle)};
            bool NearResource = false;
            for (const auto& Node : Controller->State().resources)
            {
                if (Node.cleared || Node.id == Tree->id) continue;
                const double Distance = FMath::Sqrt(FMath::Square(Node.position.x - Candidate.x)
                    + FMath::Square(Node.position.y - Candidate.y));
                if (Distance < 300.0) { NearResource = true; break; }
            }
            if (!NearResource) { EmptyContext = Candidate; HasEmptyContext = true; }
        }
    if (!HasEmptyContext)
    { Finish(false, TEXT("Clearing needs a nearby empty context for repeated-input rejection.")); return; }
    Add(TEXT("Rapid X/F after permanent removal follows the new empty context, no second clear"),
        [this, Probe, Snapshot, Tree, EmptyContext]()
        {
            Teleport(EmptyContext);
            Snapshot(); Probe->Ready = !Probe->Expected.Clear(Tree->id, Controller->PlayerPoint()).ok;
            Tap(EKeys::F); Tap(EKeys::Gamepad_FaceButton_Left);
        }, [this, Probe, Animation, Matches]()
        { return Probe->Ready && Controller->ToastIsError() && Matches() && Animation()->ClearStarts() == Probe->Starts; }, 0.12f);
    Add(TEXT("Other pending hand requests cannot stack or grant transactions"),
        [Animation]() { Animation()->RequestWater(); Animation()->RequestGather(); },
        [Avatar, Probe, Animation, Matches]() { return Matches() && Animation()->ClearStarts() == Probe->Starts
            && Animation()->WaterStarts() == Probe->WaterStarts && Animation()->GatherStarts() == Probe->GatherStarts
            && !Avatar->GetWateringTool()->IsPresented(); }, 0.12f);
    Add(TEXT("Natural recovery preserves the depleted zero-yield clear and hides the tool"),
        []() {}, [Hidden, Matches]() { return Hidden() && Matches(); }, 2.0f);
    Add(TEXT("Capture recovered clearing"), [this]() { Screenshot(TEXT("clearing-recovered")); }, Hidden);
    Restore(); Approach(MatureTree); Clear(MatureTree, EKeys::F);
    Add(TEXT("Mature-tree reward and permanent generated key persist through recovery"),
        []() {}, [this, MatureTree, Hidden, Matches]()
        {
            Homestead::ResourceNode Current{};
            return Hidden() && Matches()
                && Controller->Simulation().ResolveGeneratedResource(MatureTree->key, Current)
                && Current.cleared && !Controller->Simulation().CanHarvest(Current.id);
        }, 2.0f);
    Restore(); Clear(Tree, EKeys::F);
    Add(TEXT("Movement remains available and cancels both contextual props"),
        [this, Avatar, Probe]() { Probe->Actor = Avatar->GetActorLocation(); Axis(EKeys::Gamepad_LeftY, 0.8f); },
        [Avatar, Probe, Hidden]() { return Hidden() && FVector::Dist2D(Probe->Actor, Avatar->GetActorLocation()) > 8; });
    Add(TEXT("Stopping cannot replay the swing"), [this]() { Axis(EKeys::Gamepad_LeftY, 0); }, Hidden, 0.5f);
    Restore(); Clear(Tree, EKeys::F);
    Add(TEXT("Book pauses state and cancels clearing"), [this, Probe]() { Tap(EKeys::I); Probe->Hour = Controller->State().hour; },
        [this, Probe, Hidden]() { return Controller->IsBookOpen() && Controller->State().hour == Probe->Hour && Hidden(); });
    Add(TEXT("Look preview hides hatchet"), [this]() { Tap(EKeys::Gamepad_LeftShoulder); },
        [this, Hidden]() { return Controller->BookPage() == 6 && Hidden(); });
    Add(TEXT("Close Look"), [this]() { Tap(EKeys::Gamepad_FaceButton_Right); }, Hidden);
    Restore(); Clear(Tree, EKeys::F);
    Add(TEXT("Planning cancels clearing"), [this]() { Tap(EKeys::B); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Hidden]() { return Controller->IsPlanning() && Hidden(); });
    Add(TEXT("Cancel planning"), [this]() { Tap(EKeys::Gamepad_FaceButton_Right); }, Hidden);
    const bool bShippingQA = FParse::Param(FCommandLine::Get(), TEXT("HomesteadShippingQA"));
    for (int32 Body = 0; Body < 3; ++Body)
        for (int32 Hair = 0; Hair < 3; ++Hair)
            for (int32 Outfit = 0; Outfit < 2; ++Outfit)
            {
                if (bShippingQA && !((Body == 0 && Hair == 0 && Outfit == 0)
                    || (Body == 1 && Hair == 1 && Outfit == 1)
                    || (Body == 2 && Hair == 2 && Outfit == 0))) continue;
                Restore();
                Add(FString::Printf(TEXT("Select hatchet appearance %d/%d/%d"), Body, Hair, Outfit),
                    [this, Avatar, Probe, Body, Hair, Outfit]()
                    {
                        auto Look = Controller->GetAppearance(); Look.BodyPreset = Body; Look.HairStyle = Hair; Look.Outfit = Outfit;
                        FString Error;
                        Probe->Ready = Avatar->PrepareEquipment(Controller->State(), Look, Error)
                            && Avatar->ApplyPreparedEquipment(Error);
                    }, [Probe, Hidden]() { return Probe->Ready && Hidden(); });
                Clear(Tree, EKeys::Gamepad_FaceButton_Left);
                Add(TEXT("Color-only change cancels hatchet without recoloring wood/stone"),
                    [this, Avatar, Probe, Body, Hair, Outfit]()
                    {
                        auto Look = Controller->GetAppearance(); Look.BodyPreset = Body; Look.HairStyle = Hair; Look.Outfit = Outfit;
                        Look.HairColor = 2; Look.TunicColor = 1;
                        FString Error;
                        Probe->Ready = Avatar->PrepareEquipment(Controller->State(), Look, Error)
                            && Avatar->ApplyPreparedEquipment(Error);
                    }, [Avatar, Probe, Hidden, Matches]()
                    {
                        FLinearColor Wood, Stone;
                        const auto* W = Cast<UMaterialInstanceDynamic>(Avatar->GetHatchet()->GetMaterial(0));
                        const auto* S = Cast<UMaterialInstanceDynamic>(Avatar->GetHatchet()->GetMaterial(1));
                        return Probe->Ready && Hidden() && Matches() && W && S
                            && W->GetVectorParameterValue(TEXT("Tint"), Wood) && Wood.Equals(FLinearColor(0.27f, 0.145f, 0.065f))
                            && S->GetVectorParameterValue(TEXT("Tint"), Stone) && Stone.Equals(FLinearColor(0.25f, 0.28f, 0.24f));
                    });
            }
    Restore(); Clear(Tree, EKeys::F);
    Add(TEXT("Save permanent cleared ID and its single yield during the gesture"),
        [this]() { Tap(EKeys::F5); }, [this, Matches]() { return !Controller->ToastIsError() && Matches(); }, 0.12f);
    Add(TEXT("Load keeps exact cleared IDs/yield but neither held tool nor half swing"),
        [this]() { Tap(EKeys::F9); }, [this, Hidden, Matches]()
        { return !Controller->ToastIsError() && Hidden() && Matches(); }, 0.8f);
    Add(TEXT("Current empty-context input is not sapling clearing"),
        [this, EmptyContext]() { Teleport(EmptyContext); }, Hidden, 0.7f);
    Add(TEXT("Empty-context secondary rejection preserves state and clearing starts"),
        [this, Snapshot]() { Snapshot(); Tap(EKeys::F); },
        [this, Probe, Animation, Hidden, Matches]()
        { return Controller->ToastIsError() && Hidden() && Matches() && Animation()->ClearStarts() == Probe->Starts; });

    if (bShippingQA) return;
    const auto Depleted = MakeShared<Homestead::ResourceNode>(FocusableSaplings[1]);
    const auto Reserved = MakeShared<Homestead::ResourceNode>(FocusableSaplings[2]);
    Approach(Depleted);
    Add(TEXT("Deplete another sapling through its actual harvest, before filling pack"),
        [this, Probe, Snapshot, Depleted]()
        { Snapshot(); Probe->Ready = Probe->Expected.Harvest(Depleted->id, Controller->PlayerPoint()).ok; Tap(EKeys::E); },
        [Probe, Hidden, Matches]() { return Probe->Ready && Hidden() && Matches(); });
    for (const auto& Node : Controller->State().resources)
    {
        if (Node.kind == Homestead::ResourceKind::Sapling) continue;
        auto Skip = [this, Node]() { return !Controller->Simulation().CanHarvest(Node.id)
            || Controller->Simulation().UsedCapacity() > Homestead::InventoryCapacity - 10; };
        Add(TEXT("Approach supply for real capacity setup ") + FString::FromInt(Node.id),
            [this, Node]() { Teleport(Node.position); },
            [this, Node]() { return Controller->IsResourceFocused(Node.id); }, 0.65f);
        Steps.Last().Skip = Skip;
        Add(TEXT("Fill pack through a real mapped harvest"), [this]() { Tap(EKeys::E); },
            [this]() { return !Controller->ToastIsError(); });
        Steps.Last().Skip = Skip;
    }
    Approach(Reserved);
    Add(TEXT("Ready sapling capacity rejection has no clear, yield, energy debit or swing"),
        [this, Probe, Snapshot, Reserved]()
        { Snapshot(); Probe->Ready = !Probe->Expected.Clear(Reserved->id, Controller->PlayerPoint()).ok; Tap(EKeys::F); },
        [this, Probe, Animation, Hidden, Matches]()
        { return Probe->Ready && Controller->ToastIsError() && Hidden() && Matches() && Animation()->ClearStarts() == Probe->Starts; });
    Approach(Depleted);
    Clear(Depleted, EKeys::F);
    Add(TEXT("Depleted sapling may still clear with a nearly full pack and zero yield"),
        []() {}, [Hidden, Matches]() { return Hidden() && Matches(); }, 2.2f);
}
