#include "HomesteadSmokeTest.h"
#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

namespace
{
struct FGatherProbe
{
    Homestead::State Expected;
    int32 Id = -1;
    uint32 Starts = 0;
    bool Ready = false;
    double Hour = 0;
    float Phase = 0;
    FVector Actor, Hand, LeftToe, RightToe;
    FRotator View;
};

bool SameGatherDelta(const Homestead::State& Actual, const Homestead::State& Expected)
{
    if (Actual.inventory != Expected.inventory || Actual.resources.size() != Expected.resources.size()) return false;
    for (size_t Index = 0; Index < Actual.resources.size(); ++Index)
    {
        const auto& A = Actual.resources[Index];
        const auto& B = Expected.resources[Index];
        if (A.id != B.id || A.kind != B.kind || A.position.x != B.position.x || A.position.y != B.position.y
            || A.readyAtHour != B.readyAtHour || A.cleared != B.cleared) return false;
    }
    return true;
}
}

void AHomesteadSmokeTest::PrepareGatheringChecks()
{
    auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
    TArray<Homestead::ResourceNode> Berries;
    for (const auto& Node : Controller->State().resources)
        if (Node.kind == Homestead::ResourceKind::BerryBush) Berries.Add(Node);
    if (!Avatar || !Avatar->HasHeroine() || Berries.Num() < 7)
    {
        Finish(false, TEXT("Gathering fixture requires the real heroine and seven default berry patches."));
        return;
    }
    auto Probe = MakeShared<FGatherProbe>();
    auto Animation = [Avatar]() { return Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()); };
    auto Approach = [this, Avatar, Probe, Animation](Homestead::ResourceNode Node)
    {
        Add(TEXT("Approach fresh gathering fixture ") + FString::FromInt(Node.id),
            [this, Avatar, Probe, Node]()
            {
                Avatar->CancelAction();
                Probe->Id = Node.id;
                Teleport(Node.position);
                Homestead::ResourceNode Current;
                const auto Resolved = Controller->Simulation().ResolveGeneratedResource(Node.key, Current);
                if (!Resolved || Current.id <= 0 || Current.kind != Homestead::ResourceKind::BerryBush)
                {
                    Finish(false, TEXT("Berry patch key did not resolve after active-window or save reload."));
                    return;
                }
                Probe->Id = Current.id;
            },
            [this, Probe, Animation]()
            {
                const bool Ready = Controller->IsResourceFocused(Probe->Id)
                    && Controller->Simulation().CanHarvest(Probe->Id)
                    && Animation() && Animation()->GatherWeight() < 0.001f;
                if (!Ready)
                    Results.Add(FString::Printf(TEXT("GATHER_APPROACH id=%d focus=%d can_harvest=%d animation=%d weight=%.4f"),
                        Probe->Id, Controller->FocusId, Controller->Simulation().CanHarvest(Probe->Id),
                        Animation() != nullptr, Animation() ? Animation()->GatherWeight() : -1.0f));
                return Ready;
            }, 0.65f);
    };
    auto Gather = [this, Avatar, Probe, Animation](FKey Key)
    {
        Add(TEXT("Exactly one successful wild gather starts a visible action: ") + Key.ToString(),
            [this, Avatar, Probe, Animation, Key]()
            {
                auto Expected = Controller->Simulation();
                Probe->Ready = Expected.Harvest(Probe->Id, Controller->PlayerPoint()).ok;
                Probe->Expected = Expected.GetState();
                Probe->Starts = Animation()->GatherStarts();
                Probe->Hour = Controller->State().hour;
                Probe->Actor = Avatar->GetActorLocation();
                Probe->View = Controller->GetControlRotation();
                Probe->Hand = Avatar->GetMesh()->GetBoneLocation(TEXT("hand_r"));
                Probe->LeftToe = Avatar->GetMesh()->GetBoneLocation(TEXT("ball_l"));
                Probe->RightToe = Avatar->GetMesh()->GetBoneLocation(TEXT("ball_r"));
                Tap(Key);
            },
            [this, Avatar, Probe, Animation]()
            {
                return Probe->Ready && SameGatherDelta(Controller->State(), Probe->Expected)
                    && Controller->Toast().IsEmpty()
                    && Animation()->GatherStarts() == Probe->Starts + 1 && Animation()->GatherWeight() > 0.5f
                    && FVector::Dist(Probe->Hand, Avatar->GetMesh()->GetBoneLocation(TEXT("hand_r"))) > 8
                    && FVector::Dist(Probe->LeftToe, Avatar->GetMesh()->GetBoneLocation(TEXT("ball_l"))) < 2
                    && FVector::Dist(Probe->RightToe, Avatar->GetMesh()->GetBoneLocation(TEXT("ball_r"))) < 2
                    && FVector::Dist(Probe->Actor, Avatar->GetActorLocation()) < 1
                    && Probe->View.Equals(Controller->GetControlRotation(), 0.01f)
                    && Controller->State().hour - Probe->Hour < 0.02;
            }, 0.3f);
    };

    Add(TEXT("Close initial notes with gamepad Menu"), [this]() { Tap(EKeys::Gamepad_Special_Right); },
        [this, Animation]() { return !Controller->IsBookOpen() && Animation() && Animation()->GatherWeight() < 0.001f; });
    Approach(Berries[0]);
    Gather(EKeys::Gamepad_FaceButton_Bottom);
    Add(TEXT("Rapid A/E on a depleted patch grants nothing and does not restart"),
        [this, Probe, Animation]()
        {
            Probe->Starts = Animation()->GatherStarts();
            Probe->Phase = Animation()->GatherPhase();
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Tap(EKeys::E);
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, Probe, Animation]()
        {
            return Controller->ToastIsError() && SameGatherDelta(Controller->State(), Probe->Expected)
                && Animation()->GatherStarts() == Probe->Starts && Animation()->GatherPhase() > Probe->Phase;
        }, 0.12f);
    Add(TEXT("Capture the real picking phase"), [this]() { Screenshot(TEXT("gather-reach")); },
        [Animation]() { return Animation()->GatherWeight() > 0.5f; }, 0.15f);
    Add(TEXT("Picking recovers without another reward"),
        []() {}, [this, Probe, Animation]()
        {
            return Animation()->GatherWeight() < 0.001f && Animation()->GatherPhase() >= 1.59f
                && SameGatherDelta(Controller->State(), Probe->Expected);
        }, 1.8f);
    Add(TEXT("Capture recovered idle"), [this]() { Screenshot(TEXT("gather-recovered")); },
        [Animation]() { return Animation()->GatherWeight() < 0.001f; });

    Approach(Berries[1]);
    Gather(EKeys::E);
    Add(TEXT("Left stick immediately interrupts picking and still moves"),
        [this, Avatar, Probe]() { Probe->Actor = Avatar->GetActorLocation(); Axis(EKeys::Gamepad_LeftY, 0.8f); },
        [Avatar, Probe, Animation]()
        {
            return Animation()->GatherWeight() < 0.001f && Avatar->GetVelocity().Size2D() > 20
                && FVector::Dist2D(Probe->Actor, Avatar->GetActorLocation()) > 8;
        }, 0.35f);
    Add(TEXT("Releasing the stick does not resume interrupted picking"),
        [this]() { Axis(EKeys::Gamepad_LeftY, 0); },
        [Avatar, Animation]() { return Avatar->GetVelocity().Size2D() < 1 && Animation()->GatherWeight() < 0.001f; }, 0.5f);

    Approach(Berries[2]);
    Gather(EKeys::Gamepad_FaceButton_Bottom);
    Add(TEXT("Book cancels picking while preserving paused simulation"),
        [this, Probe]() { Tap(EKeys::I); Probe->Hour = Controller->State().hour; },
        [this, Probe, Animation]()
        {
            return Controller->IsBookOpen() && Controller->State().hour == Probe->Hour
                && Animation()->GatherWeight() < 0.001f;
        }, 0.5f);
    Add(TEXT("Look preview remains upright"),
        [this]() { Tap(EKeys::Gamepad_LeftShoulder); },
        [this, Animation]() { return Controller->BookPage() == 6 && Animation()->GatherWeight() < 0.001f; });
    Add(TEXT("Leave Look without restarting the action"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this, Animation]() { return !Controller->IsBookOpen() && Animation()->GatherWeight() < 0.001f; });
    Add(TEXT("Build planning stays free of the interrupted action"),
        [this]() { Tap(EKeys::B); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Animation]() { return Controller->IsPlanning() && Animation()->GatherWeight() < 0.001f; });
    Add(TEXT("Leave planning"), [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsPlanning(); });

    Approach(Berries[3]);
    Gather(EKeys::E);
    Add(TEXT("Saving during picking does not grant again or cancel the gesture"),
        [this, Probe, Animation]() { Probe->Phase = Animation()->GatherPhase(); Tap(EKeys::F5); },
        [this, Probe, Animation]()
        {
            return !Controller->ToastIsError() && SameGatherDelta(Controller->State(), Probe->Expected)
                && Animation()->GatherPhase() > Probe->Phase && Animation()->GatherWeight() > 0.5f;
        }, 0.12f);
    Add(TEXT("Loading restores inventory once without restoring a half-finished pose"),
        [this]() { Tap(EKeys::F9); },
        [this, Probe, Animation]()
        {
            return !Controller->ToastIsError() && SameGatherDelta(Controller->State(), Probe->Expected)
                && Animation()->GatherWeight() < 0.001f;
        }, 0.65f);

    Approach(Berries[4]);
    Gather(EKeys::Gamepad_FaceButton_Bottom);
    Add(TEXT("Color-only appearance changes cancel picking cleanly"),
        [this, Avatar, Probe]()
        {
            auto Look = Controller->GetAppearance();
            Look.HairColor = (Look.HairColor + 1) % 4;
            FString Error;
            Probe->Ready = Avatar->PrepareEquipment(Controller->State(), Look, Error)
                && Avatar->ApplyPreparedEquipment(Error);
        },
        [Probe, Animation]() { return Probe->Ready && Animation()->GatherWeight() < 0.001f; }, 0.3f);
    Approach(Berries[5]);
    Gather(EKeys::E);
    Add(TEXT("Body and hair replacement with owned clothing cannot keep a bent pose"),
        [this, Avatar, Probe]()
        {
            auto Look = Controller->GetAppearance();
            Look.BodyPreset = 1;
            Look.HairStyle = 2;
            FString Error;
            Probe->Ready = Avatar->PrepareEquipment(Controller->State(), Look, Error)
                && Avatar->ApplyPreparedEquipment(Error);
        },
        [Avatar, Probe, Animation]()
        {
            const auto* Presentation = Avatar->GetEquipmentPresentation();
            return Probe->Ready && Presentation && Presentation->Garments.Num() == 2
                && Avatar->GetMesh()->GetSkeletalMeshAsset()->GetName() == TEXT("SK_Modular_Willow_Base_Ponytail")
                && Animation() && Animation()->GatherWeight() < 0.001f && Animation()->WalkWeight() < 0.001f;
        });
    Add(TEXT("Restore saved appearance after the mesh-swap fixture"),
        [this, Avatar, Probe]()
        {
            FString Error;
            Probe->Ready = Avatar->PrepareEquipment(Controller->State(), Controller->GetAppearance(), Error)
                && Avatar->ApplyPreparedEquipment(Error);
        },
        [Probe, Animation]() { return Probe->Ready && Animation() && Animation()->GatherWeight() < 0.001f; });

    Add(TEXT("Invalid distant world use has no gathering presentation or inventory mutation"),
        [this, Probe, Animation]()
        {
            Teleport({3500, -3400});
            Probe->Expected = Controller->State();
            Probe->Starts = Animation()->GatherStarts();
            Tap(EKeys::E);
        },
        [this, Probe, Animation]()
        {
            return SameGatherDelta(Controller->State(), Probe->Expected)
                && Controller->ToastIsError()
                && Animation()->GatherStarts() == Probe->Starts && Animation()->GatherWeight() < 0.001f;
        });

    const auto Reserved = Berries.Last();
    for (const auto& Node : Controller->State().resources)
    {
        if (Node.kind == Homestead::ResourceKind::Sapling || Node.id == Reserved.id) continue;
        auto Skip = [this, Id = Node.id]()
        {
            return !Controller->Simulation().CanHarvest(Id)
                || Controller->Simulation().UsedCapacity() > Homestead::InventoryCapacity - 5;
        };
        Add(TEXT("Approach resource to fill the isolated test pack ") + FString::FromInt(Node.id),
            [this, Node]() { Teleport(Node.position); },
            [this, Node]() { return Controller->IsResourceFocused(Node.id); }, 0.65f);
        Steps.Last().Skip = Skip;
        Add(TEXT("Fill the test pack through mapped gathering ") + FString::FromInt(Node.id),
            [this]() { Tap(EKeys::E); }, [this]() { return !Controller->ToastIsError(); });
        Steps.Last().Skip = Skip;
    }
    Approach(Reserved);
    Add(TEXT("CONTROLLED top-up after mapped gathers prepares an exact full-pack rejection"),
        [this]()
        {
            const int32 Remaining = Homestead::InventoryCapacity - Controller->Simulation().UsedCapacity();
            if (Remaining <= 0) return;
            Homestead::Simulation Candidate = Controller->Simulation();
            auto& State = const_cast<Homestead::State&>(Candidate.GetState());
            State.inventory[static_cast<int32>(Homestead::Item::Branch)] += Remaining;
            State.inventoryLayout.push_back({State.nextGroupId++, Homestead::Item::Branch, Remaining, 0});
            const auto Result = Controller->Sim.Deserialize(Candidate.Serialize());
            if (!Result) Finish(false, UTF8_TO_TCHAR(Result.message.c_str()));
        },
        [this]() { return Controller->Simulation().UsedCapacity() == Homestead::InventoryCapacity; });
    Add(TEXT("A full-pack rejection has no action, inventory delta or regrowth mutation"),
        [this, Probe, Animation]()
        {
            Probe->Expected = Controller->State();
            Probe->Starts = Animation()->GatherStarts();
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, Probe, Animation]()
        {
            return Controller->Simulation().UsedCapacity() > Homestead::InventoryCapacity - 5
                && Controller->ToastIsError() && Controller->Simulation().CanHarvest(Probe->Id)
                && SameGatherDelta(Controller->State(), Probe->Expected)
                && Animation()->GatherStarts() == Probe->Starts && Animation()->GatherWeight() < 0.001f;
        });
}
