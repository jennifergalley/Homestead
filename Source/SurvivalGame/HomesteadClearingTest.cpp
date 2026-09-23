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
    FRotator View;
    TArray<FString> CameraSnapshots;
    FString DecorationsBeforeHarvest;
};
}

void AHomesteadSmokeTest::PrepareClearingChecks()
{
    auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
    TArray<Homestead::ResourceNode> Saplings;
    for (const auto& Node : Controller->State().resources)
        if (Node.kind == Homestead::ResourceKind::Sapling) Saplings.Add(Node);
    if (!Avatar || !Avatar->HasHeroine() || Saplings.Num() < 3)
    { Finish(false, TEXT("Clearing needs the real heroine and three default saplings.")); return; }
    const auto Tree = Saplings[0];
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
    auto Approach = [this, Avatar, Hidden](Homestead::ResourceNode Node)
    {
        Add(TEXT("Functional-only approach south of sapling ") + FString::FromInt(Node.id),
            [this, Avatar, Node]() { Avatar->CancelAction(); Teleport({Node.position.x, Node.position.y - 110}); },
            [this, Node, Hidden]() { return Controller->IsResourceFocused(Node.id) && Hidden(); }, 0.7f);
    };
    auto Restore = [this, Hidden, Approach, Tree]()
    {
        Add(TEXT("Reload the real crafted checkpoint, including unchanged sapling IDs"),
            [this]() { Tap(EKeys::F9); }, [this, Hidden]() { return !Controller->ToastIsError() && Hidden(); }, 0.8f);
        Approach(Tree);
    };
    auto Clear = [this, Avatar, Probe, Animation, Snapshot, Matches](Homestead::ResourceNode Node, FKey Key)
    {
        Add(TEXT("One authoritative sapling clear with scale-one held hatchet: ") + Key.ToString(),
            [this, Avatar, Probe, Snapshot, Node, Key]()
            {
                Snapshot(); Probe->Ready = Probe->Expected.Clear(Node.id, Controller->PlayerPoint()).ok;
                Probe->Actor = Avatar->GetActorLocation(); Probe->Hand = Avatar->GetMesh()->GetBoneLocation(TEXT("hand_r"));
                Probe->LeftToe = Avatar->GetMesh()->GetBoneLocation(TEXT("ball_l")); Probe->RightToe = Avatar->GetMesh()->GetBoneLocation(TEXT("ball_r"));
                Probe->View = Controller->GetControlRotation(); Tap(Key);
            }, [this, Avatar, Probe, Animation, Matches, Node]()
            {
                const auto* Tool = Avatar->GetHatchet();
                const float ExpectedYaw = FMath::RadiansToDegrees(FMath::Atan2(
                    Node.position.y - Probe->Actor.Y, Node.position.x - Probe->Actor.X));
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
            }, 0.55f);
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
            const auto* Landscape = Controller->Landscape.Get();
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
                const bool LowCoverHomeEdge = Reserved(-600, 400, 20)
                    && !AHomesteadWorld::IsDecorationReserved(State, -600, 400, 20, 0, true);
                const bool PathProtected = AHomesteadWorld::IsDecorationReserved(State, 0, 75, 20, 0, true);
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
                return Access && OverlapAllowed && CornerInitiallyOpen && HarvestKeepsCorner
                    && ClearedCornerReserved && CanopyProtectsBuilding && LowCoverNearResource
                    && LowCoverHomeEdge && PathProtected && LowCoverProtectsBuilding && LowCoverProtectsPlot
                    && AHomesteadWorld::GrassGroundWeight(-2900, -2900) <= 0.1f
                    && AHomesteadWorld::GrassGroundWeight(-1000, 0) == 0;
            });
        const auto CameraCanopy = [this, Tree, Probe](bool Ready, bool Cleared)
        {
            const auto* Landscape = Controller->Landscape.Get();
            const auto* Produce = Landscape ? Landscape->ResourceProduceVisuals.Find(Tree.id) : nullptr;
            const auto* Base = Landscape ? Landscape->ResourceVisuals.Find(Tree.id) : nullptr;
            if (!Produce || !Base || Produce->Components.Num() != (Ready ? 1 : 0)
                || Base->Components.Num() != (Cleared ? 0 : 1)) return false;
            int32 CameraBlockers = 0;
            for (const auto& Component : Produce->Components)
            {
                const auto* Mesh = Cast<UStaticMeshComponent>(Component);
                if (!Mesh || !Mesh->IsRegistered() || !Mesh->IsVisible() || Mesh->bHiddenInGame
                    || Mesh->CanEverAffectNavigation() || Mesh->GetGenerateOverlapEvents()
                    || !Mesh->ComponentHasTag(TEXT("AuthoredResource")) || !Mesh->GetStaticMesh()
                    || Mesh->GetStaticMesh()->GetPathName() != FString(TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/"))
                        + (Tree.id % 2 ? TEXT("SM_FirSapling_a.SM_FirSapling_a") : TEXT("SM_FirSapling_c.SM_FirSapling_c"))
                    || !Mesh->GetComponentScale().Equals(FVector::OneVector, 0.001f)) return false;
                if (!Mesh->IsQueryCollisionEnabled()) continue;
                if (Mesh->GetCollisionEnabled() != ECollisionEnabled::QueryOnly) return false;
                FCollisionResponseContainer ExpectedResponses(ECR_Ignore);
                ExpectedResponses.SetResponse(ECC_Camera, ECR_Block);
                if (Mesh->GetCollisionResponseToChannels() != ExpectedResponses) return false;
                Probe->CameraCenter = Mesh->Bounds.Origin;
                ++CameraBlockers;
            }
            const FVector Center = Probe->CameraCenter;
            if (Center.ContainsNaN() || Center.Z <= Controller->GroundHeight(Tree.position.x, Tree.position.y) + 20)
                return false;
            FHitResult Hit;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(SaplingCameraLifecycle), false, Controller->GetPawn());
            const bool Blocked = GetWorld()->SweepSingleByChannel(Hit, Center - FVector(150, 0, 0),
                Center + FVector(150, 0, 0), FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(12), Query);
            bool OwnProduceHit = false;
            for (const auto& Component : Produce->Components) OwnProduceHit |= Hit.GetComponent() == Component;
            Probe->CameraSnapshots.Add(FString::Printf(TEXT("node=%d ready=%d cleared=%d blockers=%d sweep=%d own_produce=%d"),
                Tree.id, Ready, Cleared, CameraBlockers, Blocked, OwnProduceHit));
            const bool Persisted = FFileHelper::SaveStringArrayToFile(Probe->CameraSnapshots,
                *FPaths::Combine(HomesteadTestOutputDirectory(), TEXT("camera-lifecycle.txt")));
            return Persisted && CameraBlockers == (Ready ? 1 : 0) && Blocked == Ready && OwnProduceHit == Ready;
        };
        Approach(Tree);
        Add(TEXT("Visible sapling canopy blocks the real camera query only"),
            []() {}, [CameraCanopy]() { return CameraCanopy(true, false); });
        Add(TEXT("Mapped sapling harvest removes produce and camera blockers together"),
            [this, Probe, Decorations]() { Probe->DecorationsBeforeHarvest = Decorations(); Tap(EKeys::E); },
            [this, Tree, CameraCanopy, Probe, Decorations]() { return !Controller->ToastIsError()
                && !Controller->Simulation().CanHarvest(Tree.id) && CameraCanopy(false, false)
                && Decorations() == Probe->DecorationsBeforeHarvest; });
        Restore();
        Add(TEXT("Checkpoint reload restores the visible canopy and its camera query"),
            []() {}, [CameraCanopy]() { return CameraCanopy(true, false); });
        Clear(Tree, EKeys::F);
        Add(TEXT("Permanent clear removes the real camera sweep obstruction"),
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
        Add(TEXT("Cleared build-cell corners remain accessible after the actual saved reload"),
            []() {}, [this, Tree]()
            {
                const auto Center = Homestead::CellCenter(
                    FMath::FloorToInt(Tree.position.x / Homestead::CellSize),
                    FMath::FloorToInt(Tree.position.y / Homestead::CellSize));
                for (const int32 X : {-1, 1})
                    for (const int32 Y : {-1, 1})
                    {
                        const float PX = Center.x + X * 100, PY = Center.y + Y * 100;
                        if (!AHomesteadWorld::IsDecorationReserved(Controller->State(), PX, PY, 20))
                            return false;
                        FCollisionQueryParams Query(SCENE_QUERY_STAT(WoodlandClearedCell), false, Controller->GetPawn());
                        if (GetWorld()->OverlapBlockingTestByChannel(
                            FVector(PX, PY, Controller->GroundHeight(PX, PY) + 100), FQuat::Identity,
                            ECC_Pawn, FCollisionShape::MakeSphere(40), Query)) return false;
                    }
                return true;
            });
        return;
    }
    const auto OtherTree = Saplings[1];
    Homestead::ResourceNode Branch{};
    for (const auto& Node : Controller->State().resources)
        if (Node.kind == Homestead::ResourceKind::Branches && Node.id == OtherTree.id - 6) { Branch = Node; break; }
    if (!Branch.id) { Finish(false, TEXT("Default adjacent branch/sapling fixture is missing.")); return; }
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
        { Snapshot(); Probe->Ready = Probe->Expected.Harvest(Tree.id, Controller->PlayerPoint()).ok; Tap(EKeys::E); },
        [Probe, Animation, Hidden, Matches]()
        { return Probe->Ready && Matches() && Hidden() && Animation()->ClearStarts() == Probe->Starts; });
    Clear(Tree, EKeys::Gamepad_FaceButton_Left);
    Add(TEXT("Capture restrained clearing swing"), [this]() { Screenshot(TEXT("clearing-swing")); },
        [Avatar, Animation]() { return Animation()->ClearWeight() > 0.99f && Avatar->GetHatchet()->IsPresented(); }, 0.12f);
    Add(TEXT("Rapid X/F after permanent removal follows the new empty context, no second clear"),
        [this, Probe, Snapshot, Tree]()
        {
            Snapshot(); Probe->Ready = !Probe->Expected.Clear(Tree.id, Controller->PlayerPoint()).ok;
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
    for (int32 Body = 0; Body < 3; ++Body)
        for (int32 Hair = 0; Hair < 3; ++Hair)
            for (int32 Outfit = 0; Outfit < 2; ++Outfit)
            {
                Restore();
                Add(FString::Printf(TEXT("Select hatchet appearance %d/%d/%d"), Body, Hair, Outfit),
                    [this, Avatar, Probe, Body, Hair, Outfit]()
                    {
                        auto Look = Controller->GetAppearance(); Look.BodyPreset = Body; Look.HairStyle = Hair; Look.Outfit = Outfit;
                        Probe->Ready = Avatar->ApplyAppearance(Look);
                    }, [Probe, Hidden]() { return Probe->Ready && Hidden(); });
                Clear(Tree, EKeys::Gamepad_FaceButton_Left);
                Add(TEXT("Color-only change cancels hatchet without recoloring wood/stone"),
                    [this, Avatar, Probe, Body, Hair, Outfit]()
                    {
                        auto Look = Controller->GetAppearance(); Look.BodyPreset = Body; Look.HairStyle = Hair; Look.Outfit = Outfit;
                        Look.HairColor = 2; Look.TunicColor = 1; Probe->Ready = Avatar->ApplyAppearance(Look);
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
    Add(TEXT("No-target/out-of-range input is not sapling clearing"),
        [this]() { Teleport({3900, 3900}); }, Hidden, 0.7f);
    Add(TEXT("Out-of-map secondary rejection preserves state and clearing starts"),
        [this, Snapshot]() { Snapshot(); Tap(EKeys::F); },
        [this, Probe, Animation, Hidden, Matches]()
        { return Controller->ToastIsError() && Hidden() && Matches() && Animation()->ClearStarts() == Probe->Starts; });

    const auto Depleted = Saplings[1], Reserved = Saplings[2];
    Approach(Depleted);
    Add(TEXT("Deplete another sapling through its actual harvest, before filling pack"),
        [this, Probe, Snapshot, Depleted]()
        { Snapshot(); Probe->Ready = Probe->Expected.Harvest(Depleted.id, Controller->PlayerPoint()).ok; Tap(EKeys::E); },
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
        { Snapshot(); Probe->Ready = !Probe->Expected.Clear(Reserved.id, Controller->PlayerPoint()).ok; Tap(EKeys::F); },
        [this, Probe, Animation, Hidden, Matches]()
        { return Probe->Ready && Controller->ToastIsError() && Hidden() && Matches() && Animation()->ClearStarts() == Probe->Starts; });
    Approach(Depleted);
    Clear(Depleted, EKeys::F);
    Add(TEXT("Depleted sapling may still clear with a nearly full pack and zero yield"),
        []() {}, [Hidden, Matches]() { return Hidden() && Matches(); }, 2.2f);
}
