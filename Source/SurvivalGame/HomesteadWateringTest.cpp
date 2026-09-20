#include "HomesteadSmokeTest.h"
#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWateringTool.h"
#include "HomesteadHatchet.h"
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
        && !Avatar->GetWateringTool()->IsPresented() && !Avatar->GetHatchet()->IsPresented(); };
    const Homestead::Point Garden = Homestead::CellCenter(-5, 0);
    const Homestead::Point Stream{Homestead::StreamX(2700) - 40, 2700};
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
        Add(TEXT("Approach stream for ordinary mapped refill"), [this, Stream]() { Teleport(Stream); },
            [this, Hidden]() { return Controller->FocusTitle() == TEXT("Fresh stream water") && Hidden(); }, 0.7f);
        Add(TEXT("Refill changes water stock but does not start a watering pose"),
            [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
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
                Tap(Key);
                if (DoubleTap) Tap(Key);
            },
            [this, Avatar, Probe, Animation, Matches, DoubleTap]()
            {
                const auto* Tool = Avatar->GetWateringTool();
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
                    && Controller->State().hour - Probe->Hour < 0.02;
            }, 0.55f);
    };
    auto Rejected = [this, Probe, Animation, Hidden, Matches](const FString& Reason)
    {
        Add(Reason, [this, Probe, Animation]()
            {
                Probe->Expected = Controller->Simulation();
                Probe->Hour = Controller->State().hour;
                Probe->Starts = Animation()->WaterStarts();
                Tap(EKeys::E);
            }, [this, Probe, Animation, Hidden, Matches]()
            {
                return Controller->ToastIsError() && Matches() && Hidden() && Animation()->WaterStarts() == Probe->Starts;
            });
    };

    Add(TEXT("Close notes; isolated setup uses real gather/craft/till/plant transactions"),
        [this]() { Tap(EKeys::Gamepad_Special_Right); }, [this, Hidden]() { return !Controller->IsBookOpen() && Hidden(); });
    QueueGatherTo(Homestead::Item::Branch, 10);
    QueueGatherTo(Homestead::Item::Stone, 4);
    QueueGatherTo(Homestead::Item::Fiber, 4);
    QueueGatherTo(Homestead::Item::Seeds, 1);
    QueueCraft(Homestead::Recipe::Hatchet);
    QueueCraft(Homestead::Recipe::DiggingStick);
    QueueClearCell(-5, 0);
    Add(TEXT("Stand east of the cleared garden"),
        [this]()
        {
            Teleport({-1160, 150});
            Controller->GetPawn()->SetActorRotation(FRotator(0, 180, 0));
            Controller->SetControlRotation(FRotator(-20, 180, 0));
        }, [this]() { return Controller->FocusTitle() == TEXT("The clearing"); }, 0.7f);
    Add(TEXT("Till with the real crafted digging stick"), [this]() { Tap(EKeys::Gamepad_FaceButton_Left); },
        [this]()
        {
            for (const auto& Plot : Controller->State().plots)
                if (Plot.cellX == -5 && Plot.cellY == 0) { GardenPlotId = Plot.id; return !Controller->ToastIsError(); }
            return false;
        });
    Add(TEXT("Approach and plant actual wild-root seeds"), [this, Garden]() { Teleport(Garden); },
        [this]() { return Controller->FocusTitle() == TEXT("A little patch of earth"); }, 0.7f);
    Add(TEXT("Plant with gamepad A; no watering prop"), [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Hidden]() { return !Controller->ToastIsError() && Hidden() && Controller->FocusActions().Contains(TEXT("Water")); });
    Rejected(TEXT("No-can rejection neither debits water nor presents a free tool"));
    QueueCraft(Homestead::Recipe::WateringCan);
    Rejected(TEXT("Empty-can rejection has no pose, prop or moisture reward"));
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
                        Probe->Ready = Avatar->ApplyAppearance(Look);
                    }, [Probe, Hidden]() { return Probe->Ready && Hidden(); });
                Refill();
                Water(EKeys::Gamepad_FaceButton_Bottom);
                Add(TEXT("Color-only appearance application hides prop and leaves its wood tint stable"),
                    [this, Avatar, Probe, Body, Hair, Outfit]()
                    {
                        auto Look = Controller->GetAppearance();
                        Look.BodyPreset = Body; Look.HairStyle = Hair; Look.Outfit = Outfit;
                        Look.HairColor = 3; Look.TunicColor = 2;
                        Probe->Ready = Avatar->ApplyAppearance(Look);
                    }, [Avatar, Probe, Hidden]()
                    {
                        const auto* Material = Cast<UMaterialInstanceDynamic>(Avatar->GetWateringTool()->GetMaterial(0));
                        FLinearColor Tint;
                        return Probe->Ready && Hidden() && Material && Material->GetVectorParameterValue(TEXT("Tint"), Tint)
                            && Tint.Equals(FLinearColor(0.27f, 0.145f, 0.065f));
                    });
            }
    Add(TEXT("Restore saved appearance"), [this, Avatar]() { Avatar->ApplyAppearance(Controller->GetAppearance()); }, Hidden);
    Refill();
    for (int32 Portion = 0; Portion < 6; ++Portion)
        Add(TEXT("Each allowed repeat consumes exactly one real water portion"),
            [this, Probe]()
            {
                Probe->Expected = Controller->Simulation();
                Probe->Hour = Controller->State().hour;
                Probe->Ready = Probe->Expected.Water(GardenPlotId, Controller->PlayerPoint()).ok;
                Tap(EKeys::E);
            }, [this, Probe, Matches]() { return Probe->Ready && !Controller->ToastIsError() && Matches(); }, 0.12f);
    Add(TEXT("Wait for repeated watering to recover"), []() {}, Hidden, 2.3f);
    Rejected(TEXT("Exhausted water follows the existing refill rejection without a pose"));
    Add(TEXT("Move beyond all plots"), [this]() { Teleport({3500, -3400}); }, Hidden, 0.7f);
    Add(TEXT("Out-of-range interaction has no water debit or presentation"),
        [this, Probe]() { Probe->Expected = Controller->Simulation(); Probe->Hour = Controller->State().hour; Tap(EKeys::E); },
        [this, Hidden, Matches]() { return Controller->Toast().Contains(TEXT("Walk closer")) && Hidden() && Matches(); });
}
