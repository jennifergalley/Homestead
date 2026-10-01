#include "HomesteadSmokeTest.h"
#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWateringTool.h"
#include "HomesteadHatchet.h"
#include "HomesteadDiggingStick.h"
#include "HomesteadActionTestState.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
struct FWaterProbe
{
    Homestead::Simulation Expected;
    double Hour = 0;
    uint32 Starts = 0;
    uint32 GatherStarts = 0;
    uint32 ClearStarts = 0;
    uint32 TillStarts = 0;
    bool Ready = false;
    FVector Actor, Hand, Toe;
    FRotator View;
};
}

void AHomesteadSmokeTest::PrepareWateringChecks()
{
    auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
    if (!Avatar || !Avatar->HasHeroine())
    {
        Finish(false, TEXT("Watering requires the real heroine and shared action instance."));
        return;
    }
    auto Animation = [Avatar]() { return Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()); };
    auto Hidden = [Avatar, Animation]() { return Animation() && Animation()->WaterWeight() < 0.001f
        && Animation()->TillWeight() < 0.001f && !Avatar->GetWateringTool()->IsPresented()
        && !Avatar->GetHatchet()->IsPresented() && !Avatar->GetDiggingStick()->IsPresented(); };
    const Homestead::Point Garden = Homestead::GardenCellCenter(-13, 1);
    const auto TillApproach = MakeShared<Homestead::Point>(Homestead::Point{-1160, 150});
    const auto Stream = MakeShared<Homestead::Point>(
        Homestead::Point{Homestead::StreamX(2700) - 120, 2700});
    auto Probe = MakeShared<FWaterProbe>();
    auto Matches = [this, Probe]()
    {
        return MatchesActionState(*Controller, Probe->Expected, Probe->Hour);
    };
    auto Approach = [this, Garden, Hidden]()
    {
        Add(TEXT("Return beside the real planted fixture plot"), [this, Garden]() { Teleport(Garden); },
            [this, Hidden]() { return Controller->FocusActions().Contains(TEXT("Water")) && Hidden(); }, 0.7f);
    };
    auto Refill = [this, Stream, Hidden, Approach]()
    {
        Add(TEXT("Approach stream for ordinary mapped refill"), [this, Stream]()
        {
            const double OriginY = Stream->y;
            for (int32 Index = 0; Index < 32; ++Index)
            {
                const double Y = OriginY + (Index % 2 ? -1.0 : 1.0) * ((Index + 1) / 2) * 180.0;
                for (const double Side : {-120.0, 120.0})
                {
                    const Homestead::Point Candidate{Homestead::StreamX(Y) + Side, Y};
                    Teleport(Candidate);
                    bool Occupied = false;
                    for (const auto& Node : Controller->State().resources)
                        if (!Node.cleared && Controller->Simulation().CanHarvest(Node.id)
                            && FMath::Square(Node.position.x - Candidate.x)
                                + FMath::Square(Node.position.y - Candidate.y) < FMath::Square(280.0))
                        { Occupied = true; break; }
                    if (Occupied) continue;
                    *Stream = Candidate;
                    Controller->UpdateFocus();
                    if (Controller->FocusTitle() == TEXT("Fresh stream water")) return;
                }
            }
        },
            [this, Hidden]() { return Controller->FocusTitle() == TEXT("Fresh stream water") && Hidden(); }, 0.7f);
        Add(TEXT("Refill changes water stock but does not start a watering pose"),
            [this]() { if (!Controller->ChooseOnHotbar(Homestead::Item::WateringCan)) { Finish(false, TEXT("The pail could not be chosen on the hotbar.")); return; } Tap(EKeys::Gamepad_RightTrigger); },
            [this, Hidden]() { return Controller->Simulation().Count(Homestead::Item::Water) == 6 && Hidden(); });
        Approach();
    };
    auto Water = [this, Avatar, Probe, Animation, Matches](FKey Key, bool DoubleTap = false)
    {
        Add(TEXT("One correct watering transaction with attached tool: ") + Key.ToString(),
            [this, Avatar, Probe, Animation, Key, DoubleTap]()
            {
                Probe->Expected = Controller->Simulation();
                Probe->Hour = Controller->State().hour;
                Probe->Ready = Probe->Expected.Water(GardenPlotId, Controller->PlayerPoint()).ok;
                Probe->Starts = Animation()->WaterStarts();
                Probe->GatherStarts = Animation()->GatherStarts();
                Probe->ClearStarts = Animation()->ClearStarts();
                Probe->Actor = Avatar->GetActorLocation();
                Probe->Hand = Avatar->GetMesh()->GetBoneLocation(TEXT("hand_r"));
                Probe->Toe = Avatar->GetMesh()->GetBoneLocation(TEXT("ball_r"));
                Probe->View = Controller->GetControlRotation();
                // Watering is the pail on the tool button (LMB / RT); E / A never waters (Jenny 2026-09-30).
                if (!Controller->ChooseOnHotbar(Homestead::Item::WateringCan))
                { Finish(false, TEXT("The pail could not be chosen on the hotbar.")); return; }
                const FKey Use = Key == EKeys::E ? EKeys::LeftMouseButton : Key == EKeys::Gamepad_FaceButton_Bottom ? EKeys::Gamepad_RightTrigger : Key;
                Tap(Use);
                if (DoubleTap) Tap(Use);
            },
            [this, Avatar, Probe, Animation, Matches, DoubleTap]()
            {
                const auto* Tool = Avatar->GetWateringTool();
                const auto Target = Homestead::GardenCellCenter(-13, 1);
                const float ExpectedYaw = FMath::RadiansToDegrees(FMath::Atan2(
                    Target.y - Probe->Actor.Y, Target.x - Probe->Actor.X));
                return Probe->Ready && Matches() && Controller->ToastIsError() == DoubleTap
                    && Animation()->WaterStarts() == Probe->Starts + 1 && Animation()->GatherStarts() == Probe->GatherStarts
                    && Animation()->ClearStarts() == Probe->ClearStarts && !Avatar->GetHatchet()->IsPresented()
                    && Animation()->WaterWeight() > 0.99f && Animation()->GatherWeight() < 0.001f && Tool->IsPresented()
                    && Tool->GetAttachParent() == Avatar->GetMesh() && Tool->GetAttachSocketName() == TEXT("hand_r")
                    && Tool->GetCollisionEnabled() == ECollisionEnabled::NoCollision && Tool->GetNumSections() == 2
                    && Tool->GetComponentScale().Equals(FVector::OneVector, 0.001f)
                    && Tool->Bounds.SphereRadius > 10 && Tool->Bounds.SphereRadius < 60
                    && FVector::Dist(Tool->GripPosition(), Avatar->GetMesh()->GetBoneLocation(TEXT("hand_r"))) < 10
                    && FVector::Dist(Probe->Hand, Avatar->GetMesh()->GetBoneLocation(TEXT("hand_r"))) > 20
                    && FVector::Dist(Probe->Actor, Avatar->GetActorLocation()) < 1
                    && FVector::Dist(Probe->Toe, Avatar->GetMesh()->GetBoneLocation(TEXT("ball_r"))) < 2
                    && Probe->View.Equals(Controller->GetControlRotation(), 0.01f)
                    && FMath::Abs(FMath::FindDeltaAngleDegrees(
                        Avatar->WaterTargetYaw(), ExpectedYaw)) < 0.1f
                    && Controller->State().hour - Probe->Hour < 0.02;
            }, 0.55f);
    };
    // A refused watering on the tool button: with the pail chosen, the simulation's own refusal (worked
    // out on a copy); with no pail, an empty hotbar cell chosen, so the press has no tool at all.
    auto Rejected = [this, Probe, Animation, Hidden, Matches](const FString& Reason, bool bHasPail)
    {
        const auto ExpectedToast = MakeShared<FString>();
        Add(Reason, [this, Probe, Animation, bHasPail, ExpectedToast]()
            {
                Probe->Expected = Controller->Simulation();
                Probe->Hour = Controller->State().hour;
                Probe->Starts = Animation()->WaterStarts();
                if (bHasPail)
                {
                    if (!Controller->ChooseOnHotbar(Homestead::Item::WateringCan))
                    { Finish(false, TEXT("The pail could not be chosen on the hotbar.")); return; }
                    Homestead::Simulation Copy = Controller->Simulation();
                    *ExpectedToast = UTF8_TO_TCHAR(Copy.Water(GardenPlotId, Controller->PlayerPoint()).message.c_str());
                }
                else
                {
                    const int32 Empty = Controller->FirstEmptyHotbarCell();
                    if (Controller->Simulation().Count(Homestead::Item::WateringCan) > 0 || Empty == INDEX_NONE)
                    { Finish(false, TEXT("The no-pail fixture has a pail or no empty hotbar cell.")); return; }
                    Controller->SelectHotbarSlot(Empty);
                    *ExpectedToast = TEXT("Choose a carried tool first.");
                }
                Tap(EKeys::LeftMouseButton);
            }, [this, Probe, Animation, Hidden, Matches, ExpectedToast]()
            {
                return Controller->ToastIsError() && Controller->Toast() == *ExpectedToast && !ExpectedToast->IsEmpty()
                    && Matches() && Hidden() && Animation()->WaterStarts() == Probe->Starts;
            });
    };

    Add(TEXT("Close notes; isolated setup uses real gather/craft/till/plant transactions"),
        [this]() { Tap(EKeys::Gamepad_Special_Right); }, [this, Hidden]() { return !Controller->IsBookOpen() && Hidden(); });
    QueueGatherTo(Homestead::Item::Branch, 10);
    QueueGatherTo(Homestead::Item::Stone, 4);
    QueueGatherTo(Homestead::Item::Seeds, 1);
    QueueGrant(Homestead::Item::RustedAxeHead, 1);
    QueueCraft(Homestead::Recipe::HaftAxe);
    QueueClearCell(-5, 0);
    Add(TEXT("Stand east of the cleared garden without a digging stick"),
        [this, Garden, TillApproach]()
        {
            bool Found = false;
            for (const double Radius : {150.0, 190.0, 230.0})
                for (int32 Direction = 0; Direction < 16 && !Found; ++Direction)
                {
                    const double Angle = 2.0 * PI * Direction / 16.0;
                    const Homestead::Point Candidate{Garden.x + Radius * FMath::Cos(Angle),
                        Garden.y + Radius * FMath::Sin(Angle)};
                    const bool Occupied = std::any_of(Controller->State().resources.begin(),
                        Controller->State().resources.end(), [&Candidate](const Homestead::ResourceNode& Node)
                        {
                            return !Node.cleared && FMath::Square(Node.position.x - Candidate.x)
                                + FMath::Square(Node.position.y - Candidate.y) < FMath::Square(280.0);
                        });
                    if (!Occupied) { *TillApproach = Candidate; Found = true; }
                }
            Teleport(*TillApproach);
            const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(
                Garden.y - TillApproach->y, Garden.x - TillApproach->x));
            Controller->GetPawn()->SetActorRotation(FRotator(0, Yaw, 0));
            Controller->SetControlRotation(FRotator(-20, Yaw, 0));
        }, [this]() { return Controller->FocusTitle() == TEXT("Woodland"); }, 0.7f);
    Add(TEXT("X / F never tills: no change and no presentation without the hoe on the tool button"),
        [this, Probe, Animation]() { Probe->Expected = Controller->Simulation(); Probe->Hour = Controller->State().hour;
            Probe->TillStarts = Animation()->TillStarts(); Tap(EKeys::Gamepad_FaceButton_Left); },
        [this, Probe, Animation, Hidden, Matches]() { return Matches() && Hidden()
            && Animation()->TillStarts() == Probe->TillStarts; });
    QueueGrant(Homestead::Item::RustedHoeBlade, 1);
    QueueCraft(Homestead::Recipe::HaftHoe);
    Add(TEXT("Stand east of the cleared garden"),
        [this, Garden, TillApproach]()
        {
            Teleport(*TillApproach);
            const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(
                Garden.y - TillApproach->y, Garden.x - TillApproach->x));
            Controller->GetPawn()->SetActorRotation(FRotator(0, Yaw, 0));
            Controller->SetControlRotation(FRotator(-20, Yaw, 0));
        }, [this]() { return Controller->FocusTitle() == TEXT("Woodland"); }, 0.7f);
    Add(TEXT("Till with one target-directed planted-foot digging-stick action"),
        [this, Avatar, Probe, Animation]()
        {
            Probe->Expected = Controller->Simulation(); Probe->Hour = Controller->State().hour;
            Probe->Ready = Probe->Expected.Till(-13, 1, Controller->PlayerPoint()).ok;
            Probe->TillStarts = Animation()->TillStarts();
            Probe->GatherStarts = Animation()->GatherStarts();
            Probe->ClearStarts = Animation()->ClearStarts();
            Probe->Starts = Animation()->WaterStarts();
            Probe->Actor = Avatar->GetActorLocation();
            Probe->Toe = Avatar->GetMesh()->GetBoneLocation(TEXT("ball_r"));
            Probe->View = Controller->GetControlRotation();
            Controller->ChooseOnHotbar(Homestead::Item::DiggingStick);
            Tap(EKeys::Gamepad_RightTrigger);
        },
        [this, Avatar, Probe, Animation, Matches]()
        {
            const auto* Tool = Avatar->GetDiggingStick();
            for (const auto& Plot : Controller->State().plots)
                if (Plot.cellX == -13 && Plot.cellY == 1)
                {
                    GardenPlotId = Plot.id;
                    const auto Target = Homestead::GardenCellCenter(-13, 1);
                    const float ExpectedYaw = FMath::RadiansToDegrees(FMath::Atan2(
                        Target.y - Probe->Actor.Y, Target.x - Probe->Actor.X));
                    return Probe->Ready && !Controller->ToastIsError() && Matches()
                        && Animation()->TillStarts() == Probe->TillStarts + 1
                        && Animation()->GatherStarts() == Probe->GatherStarts
                        && Animation()->ClearStarts() == Probe->ClearStarts
                        && Animation()->WaterStarts() == Probe->Starts
                        && Animation()->TillWeight() > 0.99f && Tool->IsPresented()
                        && Tool->GetAttachParent() == Avatar->GetMesh()
                        && Tool->GetAttachSocketName() == TEXT("hand_r")
                        && Tool->GetCollisionEnabled() == ECollisionEnabled::NoCollision
                        && !Tool->GetGenerateOverlapEvents() && !Tool->CanEverAffectNavigation()
                        && Tool->GetNumSections() == 1
                        && Tool->GetComponentScale().Equals(FVector::OneVector, 0.001f)
                        && Tool->Bounds.SphereRadius > 35 && Tool->Bounds.SphereRadius < 60
                        && FVector::Dist(Probe->Actor, Avatar->GetActorLocation()) < 1
                        && FVector::Dist(Probe->Toe, Avatar->GetMesh()->GetBoneLocation(TEXT("ball_r"))) < 2
                        && Probe->View.Equals(Controller->GetControlRotation(), 0.01f)
                        && FMath::Abs(FMath::FindDeltaAngleDegrees(Avatar->TillTargetYaw(), ExpectedYaw)) < 0.1f;
                }
            return false;
        }, 0.45f);
    Add(TEXT("Rapid competing hand requests cannot stack onto tilling"),
        [Animation]() { Animation()->RequestGather(); Animation()->RequestWater(); Animation()->RequestClear(); },
        [Probe, Animation, Matches]() { return Matches()
            && Animation()->TillStarts() == Probe->TillStarts + 1
            && Animation()->GatherStarts() == Probe->GatherStarts
            && Animation()->ClearStarts() == Probe->ClearStarts
            && Animation()->WaterStarts() == Probe->Starts; }, 0.12f);
    Add(TEXT("Tilling recovers with no orphaned prop"), []() {}, Hidden, 1.9f);
    Add(TEXT("Occupied plot rejects a second mapped Till without presentation"),
        [this, Probe, Animation]() { Probe->Expected = Controller->Simulation(); Probe->Hour = Controller->State().hour;
            Probe->TillStarts = Animation()->TillStarts(); Controller->ChooseOnHotbar(Homestead::Item::DiggingStick); Tap(EKeys::Gamepad_RightTrigger); },
        [this, Probe, Animation, Hidden, Matches]() { return Controller->ToastIsError() && Matches() && Hidden()
            && Animation()->TillStarts() == Probe->TillStarts; });
    Add(TEXT("Movement cancels a till presentation without replay"),
        [this, Avatar, Garden]() { Avatar->PlayTill(Garden); Axis(EKeys::Gamepad_LeftY, 0.8f); },
        [Avatar, Hidden]() { return Hidden() && Avatar->GetVelocity().Size2D() > 1; }, 0.35f);
    Add(TEXT("Stop after till cancellation"), [this]() { Axis(EKeys::Gamepad_LeftY, 0); }, Hidden, 0.5f);
    Add(TEXT("Menu cancels a till presentation and leaves no prop"),
        [this, Avatar, Garden]() { Avatar->PlayTill(Garden); Tap(EKeys::I); },
        [this, Hidden]() { return Controller->IsBookOpen() && Hidden(); }, 0.35f);
    Add(TEXT("Close menu after till cancellation"), [this]() { Tap(EKeys::Gamepad_FaceButton_Right); }, Hidden);
    Add(TEXT("Save the committed plot before load cancellation"), [this]() { Tap(EKeys::F5); },
        [this]() { return !Controller->ToastIsError(); });
    Add(TEXT("Load cancels a till presentation without another plot"),
        [this, Avatar, Garden]() { Avatar->PlayTill(Garden); Tap(EKeys::F9); },
        [this, Hidden]() { return !Controller->ToastIsError() && Hidden(); }, 0.8f);
    Add(TEXT("Approach and plant actual wild-root seeds"), [this, Garden]() { Teleport(Garden); },
        [this]() { return Controller->FocusTitle() == TEXT("Tilled soil"); }, 0.7f);
    Add(TEXT("Plant with gamepad A (Seeds chosen on the hotbar); no watering prop"),
        [this]() { Controller->ChooseOnHotbar(Homestead::Item::Seeds); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Hidden]() { return !Controller->ToastIsError() && Hidden() && Controller->FocusTitle() != TEXT("Tilled soil"); });
    Rejected(TEXT("No pail: the tool button on an empty cell neither debits water nor presents a free tool"), false);
    QueueGrant(Homestead::Item::WateringCan, 1);
    Rejected(TEXT("Empty-pail rejection has no pose, prop or moisture reward"), true);
    Refill();
    Water(EKeys::Gamepad_FaceButton_Bottom, true);
    Add(TEXT("Capture watering with the real hand-held tool"), [this]() { Screenshot(TEXT("watering-pour")); },
        [Avatar, Animation]() { return Animation()->WaterWeight() > 0.99f && Avatar->GetWateringTool()->IsPresented(); }, 0.12f);
    Add(TEXT("Alternating presentation requests cannot compete or grant rewards"),
        [Animation]() { Animation()->RequestGather(); Animation()->RequestWater(); Animation()->RequestClear(); },
        [Probe, Animation, Matches]() { return Matches() && Animation()->WaterStarts() == Probe->Starts + 1
            && Animation()->GatherStarts() == Probe->GatherStarts && Animation()->GatherWeight() < 0.001f
            && Animation()->ClearStarts() == Probe->ClearStarts; }, 0.12f);
    Add(TEXT("Natural recovery hides the tool without a second debit"), []() {},
        [Hidden, Matches]() { return Hidden() && Matches(); }, 2.0f);
    Add(TEXT("Capture watering recovery"), [this]() { Screenshot(TEXT("watering-recovered")); }, Hidden);

    Water(EKeys::E);
    Add(TEXT("Movement remains available and immediately cancels tool presentation"),
        [this, Avatar, Probe]() { Probe->Actor = Avatar->GetActorLocation(); Axis(EKeys::Gamepad_LeftY, 0.8f); },
        [Avatar, Probe, Hidden]() { return Hidden() && FVector::Dist2D(Probe->Actor, Avatar->GetActorLocation()) > 8; }, 0.35f);
    Add(TEXT("Stopping cannot resume a cancelled watering action"), [this]() { Axis(EKeys::Gamepad_LeftY, 0); }, Hidden, 0.5f);
    Approach();
    Water(EKeys::Gamepad_FaceButton_Bottom);
    Add(TEXT("Menu pauses simulation and hides the active tool"),
        [this, Probe]() { Tap(EKeys::I); Probe->Hour = Controller->State().hour; },
        [this, Probe, Hidden]() { return Controller->IsBookOpen() && Controller->State().hour == Probe->Hour && Hidden(); });
    Add(TEXT("Look preview has no orphan watering tool"), [this]() { Tap(EKeys::Gamepad_LeftShoulder); },
        [this, Hidden]() { return Controller->BookPage() == 6 && Hidden(); });
    Add(TEXT("Return from Look"), [this]() { Tap(EKeys::Gamepad_FaceButton_Right); }, Hidden);
    Water(EKeys::E);
    Add(TEXT("Planning cancels active watering"),
        [this]() { Tap(EKeys::B); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Hidden]() { return Controller->IsPlanning() && Hidden(); });
    Add(TEXT("Cancel planning"), [this]() { Tap(EKeys::Gamepad_FaceButton_Right); }, Hidden);
    Refill();
    Water(EKeys::E);
    Add(TEXT("Save during watering keeps a single debit"),
        [this]() { Tap(EKeys::F5); }, [this, Matches]() { return !Controller->ToastIsError() && Matches(); }, 0.12f);
    Add(TEXT("Loading cannot restore a held contextual prop"), [this]() { Tap(EKeys::F9); },
        [this, Hidden, Matches]() { return !Controller->ToastIsError() && Hidden() && Matches(); }, 0.7f);

    for (int32 Body = 0; Body < 3; ++Body)
        for (int32 Hair = 0; Hair < 3; ++Hair)
            for (int32 Outfit = 0; Outfit < 2; ++Outfit)
            {
                Add(FString::Printf(TEXT("Select tool-compatible appearance %d/%d/%d"), Body, Hair, Outfit),
                    [this, Avatar, Probe, Body, Hair, Outfit]()
                    {
                        auto Look = Controller->GetAppearance();
                        Look.BodyPreset = Body; Look.HairStyle = Hair; Look.Outfit = Outfit;
                        Look.HairColor = 2; Look.TunicColor = 1;
                        FString Error;
                        Probe->Ready = Avatar->PrepareEquipment(Controller->State(), Look, Error)
                            && Avatar->ApplyPreparedEquipment(Error);
                    }, [Probe, Hidden]() { return Probe->Ready && Hidden(); });
                Refill();
                Water(EKeys::Gamepad_FaceButton_Bottom);
                Add(TEXT("Color-only appearance application hides prop and leaves its wood tint stable"),
                    [this, Avatar, Probe, Body, Hair, Outfit]()
                    {
                        auto Look = Controller->GetAppearance();
                        Look.BodyPreset = Body; Look.HairStyle = Hair; Look.Outfit = Outfit;
                        Look.HairColor = 3; Look.TunicColor = 2;
                        FString Error;
                        Probe->Ready = Avatar->PrepareEquipment(Controller->State(), Look, Error)
                            && Avatar->ApplyPreparedEquipment(Error);
                    }, [Avatar, Probe, Hidden]()
                    {
                        const auto* Material = Cast<UMaterialInstanceDynamic>(Avatar->GetWateringTool()->GetMaterial(0));
                        FLinearColor Tint;
                        return Probe->Ready && Hidden() && Material && Material->GetVectorParameterValue(TEXT("Tint"), Tint)
                            && Tint.Equals(FLinearColor(0.27f, 0.145f, 0.065f));
                    });
            }
    Add(TEXT("Restore saved appearance"), [this, Avatar]()
        {
            FString Error;
            Avatar->PrepareEquipment(Controller->State(), Controller->GetAppearance(), Error);
            Avatar->ApplyPreparedEquipment(Error);
        }, Hidden);
    Refill();
    for (int32 Portion = 0; Portion < 6; ++Portion)
        Add(TEXT("Each allowed repeat consumes exactly one real water portion"),
            [this, Probe]()
            {
                Probe->Expected = Controller->Simulation();
                Probe->Hour = Controller->State().hour;
                Probe->Ready = Probe->Expected.Water(GardenPlotId, Controller->PlayerPoint()).ok;
                Controller->ChooseOnHotbar(Homestead::Item::WateringCan);
                Tap(EKeys::LeftMouseButton);
            }, [this, Probe, Matches]() { return Probe->Ready && !Controller->ToastIsError() && Matches(); }, 0.12f);
    Add(TEXT("Wait for repeated watering to recover"), []() {}, Hidden, 2.3f);
    Rejected(TEXT("Exhausted water follows the existing refill rejection without a pose"), true);
    Add(TEXT("Move to a valid empty context beyond all plot/resource focus"),
        [this, Garden]()
        {
            for (int32 Radius = 500; Radius <= 2000; Radius += 250)
                for (int32 Direction = 0; Direction < 16; ++Direction)
                {
                    const double Angle = 2.0 * PI * Direction / 16.0;
                    const Homestead::Point Candidate{Garden.x + Radius * FMath::Cos(Angle),
                        Garden.y + Radius * FMath::Sin(Angle)};
                    bool Occupied = false;
                    for (const auto& Node : Controller->State().resources)
                        if (!Node.cleared && FMath::Square(Node.position.x - Candidate.x)
                            + FMath::Square(Node.position.y - Candidate.y) < FMath::Square(300.0))
                        { Occupied = true; break; }
                    for (const auto& Plot : Controller->State().plots)
                    {
                        const auto Center = Homestead::PlotCenter(Plot);
                        if (FMath::Square(Center.x - Candidate.x) + FMath::Square(Center.y - Candidate.y)
                            < FMath::Square(300.0)) Occupied = true;
                    }
                    if (!Occupied) { Teleport(Candidate); return; }
                }
        },
        [this, Hidden]() { return Controller->FocusTitle() == TEXT("Woodland") && Hidden(); }, 0.7f);
    Add(TEXT("E with nothing in front of her does nothing: no water debit or presentation"),
        [this, Probe]() { Probe->Expected = Controller->Simulation(); Probe->Hour = Controller->State().hour; Tap(EKeys::E); },
        [Hidden, Matches]() { return Hidden() && Matches(); });
}
