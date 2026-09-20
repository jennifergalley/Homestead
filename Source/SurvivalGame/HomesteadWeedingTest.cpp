#include "HomesteadSmokeTest.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWateringTool.h"
#include "HomesteadActionTestState.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
struct FWeedProbe
{
    Homestead::Simulation Expected;
    double Hour = 0;
    uint32 Starts = 0, WaterStarts = 0;
    bool Ready = false;
    FVector Actor, Hand, Toe;
    FRotator View;
};
}

void AHomesteadSmokeTest::PrepareWeedingChecks()
{
    auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
    Homestead::Point Garden{};
    for (const auto& Plot : Controller->State().plots)
        if (Plot.planted && Plot.weeds >= 0.125 && Plot.growth < 1)
        {
            GardenPlotId = Plot.id;
            Garden = Homestead::CellCenter(Plot.cellX, Plot.cellY);
            break;
        }
    if (!Avatar || !Avatar->HasHeroine() || GardenPlotId < 0)
    {
        Finish(false, TEXT("Copied fixture needs an actual immature planted crop with visible weeds."));
        return;
    }
    auto Probe = MakeShared<FWeedProbe>();
    auto Animation = [Avatar]() { return Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()); };
    auto Hidden = [Avatar, Animation]() { return Animation() && Animation()->ActionWeight() < 0.001f && !Avatar->GetWateringTool()->IsPresented(); };
    auto Matches = [this, Probe]() { return MatchesActionState(*Controller, Probe->Expected, Probe->Hour); };
    auto Snapshot = [this, Probe, Animation]()
    {
        Probe->Expected = Controller->Simulation();
        Probe->Hour = Controller->State().hour;
        Probe->Starts = Animation()->GatherStarts();
        Probe->WaterStarts = Animation()->WaterStarts();
    };
    auto Approach = [this, Garden, Hidden]()
    {
        Add(TEXT("Functional fixture teleport beside the planted plot; not ordinary footage"),
            [this, Garden]() { Teleport(Garden); },
            [this, Hidden]() { return Controller->FocusActions().Contains(TEXT("Weed")) && Hidden(); }, 0.7f);
    };
    auto Weed = [this, Avatar, Probe, Animation, Matches, Snapshot](FKey Key, bool DoubleTap = false)
    {
        Add(TEXT("Exact weed-only delta and existing pick gesture: ") + Key.ToString(),
            [this, Avatar, Probe, Snapshot, Key, DoubleTap]()
            {
                Snapshot();
                Probe->Ready = Probe->Expected.Weed(GardenPlotId, Controller->PlayerPoint()).ok;
                Probe->Actor = Avatar->GetActorLocation();
                Probe->Hand = Avatar->GetMesh()->GetBoneLocation(TEXT("hand_r"));
                Probe->Toe = Avatar->GetMesh()->GetBoneLocation(TEXT("ball_r"));
                Probe->View = Controller->GetControlRotation();
                Tap(Key);
                if (DoubleTap) Tap(Key);
            }, [this, Avatar, Probe, Animation, Matches, DoubleTap]()
            {
                return Probe->Ready && Matches() && Controller->ToastIsError() == DoubleTap
                    && Animation()->GatherStarts() == Probe->Starts + 1 && Animation()->WaterStarts() == Probe->WaterStarts
                    && Animation()->GatherWeight() > 0.5f && !Avatar->GetWateringTool()->IsPresented()
                    && FVector::Dist(Probe->Hand, Avatar->GetMesh()->GetBoneLocation(TEXT("hand_r"))) > 8
                    && FVector::Dist(Probe->Toe, Avatar->GetMesh()->GetBoneLocation(TEXT("ball_r"))) < 2
                    && FVector::Dist(Probe->Actor, Avatar->GetActorLocation()) < 1
                    && Probe->View.Equals(Controller->GetControlRotation(), 0.01f)
                    && Controller->State().hour - Probe->Hour < 0.02;
            }, 0.3f);
    };
    Approach();
    Weed(EKeys::Gamepad_FaceButton_Left, true);
    Add(TEXT("Capture pull; same-frame already-clean rejection did not restart it"),
        [this]() { Screenshot(TEXT("weeding-pull")); },
        [Animation]() { return Animation()->GatherWeight() > 0.5f; }, 0.12f);
    Add(TEXT("Weeding returns to idle without inventory, water, moisture or resource mutations"),
        []() {}, [Hidden, Matches]() { return Hidden() && Matches(); }, 1.9f);
    Add(TEXT("Capture recovered weeding"), [this]() { Screenshot(TEXT("weeding-recovered")); }, Hidden);

    Weed(EKeys::F);
    Add(TEXT("Movement interrupts weeding and remains responsive"),
        [this, Avatar, Probe]() { Probe->Actor = Avatar->GetActorLocation(); Axis(EKeys::Gamepad_LeftY, 0.8f); },
        [Avatar, Probe, Hidden]() { return Hidden() && FVector::Dist2D(Probe->Actor, Avatar->GetActorLocation()) > 8; });
    Add(TEXT("Stopping does not replay weeding"), [this]() { Axis(EKeys::Gamepad_LeftY, 0); }, Hidden, 0.5f);
    Approach();
    Weed(EKeys::F);
    Add(TEXT("Real water then weed while picking: both valid deltas, one pose, no can"),
        [this, Probe, Snapshot]()
        {
            Snapshot();
            Probe->Ready = Probe->Expected.Water(GardenPlotId, Controller->PlayerPoint()).ok
                && Probe->Expected.Weed(GardenPlotId, Controller->PlayerPoint()).ok;
            Tap(EKeys::E); Tap(EKeys::F);
        }, [this, Avatar, Probe, Animation, Matches]()
        {
            return Probe->Ready && !Controller->ToastIsError() && Matches()
                && Animation()->GatherStarts() == Probe->Starts && Animation()->WaterStarts() == Probe->WaterStarts
                && Animation()->GatherWeight() > 0.5f && !Avatar->GetWateringTool()->IsPresented();
        }, 0.12f);
    Add(TEXT("Alternation recovers without delayed queued actions"), []() {},
        [Hidden, Matches]() { return Hidden() && Matches(); }, 2.0f);
    Add(TEXT("Start real watering before reverse alternation"),
        [this, Probe, Snapshot]()
        {
            Snapshot();
            Probe->Ready = Probe->Expected.Water(GardenPlotId, Controller->PlayerPoint()).ok;
            Tap(EKeys::E);
        }, [Avatar, Probe, Animation, Matches]()
        {
            return Probe->Ready && Matches() && Animation()->WaterStarts() == Probe->WaterStarts + 1
                && Animation()->GatherStarts() == Probe->Starts && Avatar->GetWateringTool()->IsPresented();
        }, 0.55f);
    Add(TEXT("Weed during watering does not double debit, stack a pose or steal the can"),
        [this, Probe, Snapshot]()
        {
            Snapshot();
            Probe->Ready = Probe->Expected.Weed(GardenPlotId, Controller->PlayerPoint()).ok;
            Tap(EKeys::Gamepad_FaceButton_Left);
        }, [this, Probe, Animation, Matches]()
        {
            return Probe->Ready && !Controller->ToastIsError() && Matches()
                && Animation()->GatherStarts() == Probe->Starts && Animation()->WaterStarts() == Probe->WaterStarts
                && Animation()->GatherWeight() == 0 && Animation()->WaterWeight() > 0.5f;
        }, 0.12f);
    Add(TEXT("Reverse alternation naturally hides the can; no queued pull"), []() {},
        [Hidden, Matches]() { return Hidden() && Matches(); }, 2.3f);

    Weed(EKeys::F);
    Add(TEXT("Book cancels weeding and preserves paused game time"),
        [this, Probe]() { Tap(EKeys::I); Probe->Hour = Controller->State().hour; },
        [this, Probe, Hidden]() { return Controller->IsBookOpen() && Controller->State().hour == Probe->Hour && Hidden(); });
    Add(TEXT("Look remains free of weeding pose and prop"), [this]() { Tap(EKeys::Gamepad_LeftShoulder); },
        [this, Hidden]() { return Controller->BookPage() == 6 && Hidden(); });
    Add(TEXT("Return from Look"), [this]() { Tap(EKeys::Gamepad_FaceButton_Right); }, Hidden);
    Weed(EKeys::F);
    Add(TEXT("Planning cancels weeding; X rotates rather than weeds"),
        [this]() { Tap(EKeys::B); Tap(EKeys::Gamepad_FaceButton_Bottom); Tap(EKeys::Gamepad_FaceButton_Left); },
        [this, Hidden]() { return Controller->IsPlanning() && Hidden(); });
    Add(TEXT("Leave planning without a queued pose"), [this]() { Tap(EKeys::Gamepad_FaceButton_Right); }, Hidden);
    Weed(EKeys::F);
    Add(TEXT("Loading original fixture during weeding restores its weeds but not its pose"),
        [this]() { Tap(EKeys::F9); }, [this, Hidden]()
        {
            for (const auto& Plot : Controller->State().plots)
                if (Plot.id == GardenPlotId) return Plot.weeds >= 0.125 && Hidden() && !Controller->ToastIsError();
            return false;
        }, 0.8f);
    Approach();
    Weed(EKeys::F);
    Add(TEXT("Reapplying saved appearance cancels weeding without changing inventory"),
        [this, Avatar, Probe]() { Probe->Ready = Avatar->ApplyAppearance(Controller->GetAppearance()); },
        [Probe, Hidden, Matches]() { return Probe->Ready && Hidden() && Matches(); });
    Weed(EKeys::F);
    Add(TEXT("Save the weeded state during its gesture"), [this]() { Tap(EKeys::F5); },
        [this, Matches]() { return !Controller->ToastIsError() && Matches(); }, 0.12f);
    Add(TEXT("Load preserves removal and colors without replaying weeding"),
        [this]() { Tap(EKeys::F9); }, [this, Hidden, Matches]()
        { return !Controller->ToastIsError() && Hidden() && Matches() && VerifyPresentationMaterials(); }, 0.8f);

    Homestead::ResourceNode Forage{};
    bool FoundForage = false;
    for (const auto& Node : Controller->State().resources)
        if (Node.kind == Homestead::ResourceKind::BerryBush && Controller->Simulation().CanHarvest(Node.id))
        { Forage = Node; FoundForage = true; break; }
    if (!FoundForage) { Finish(false, TEXT("Weeding fixture needs an available berry patch for real action alternation.")); return; }
    Add(TEXT("Approach real forage after weeding"), [this, Forage]() { Teleport(Forage.position); },
        [this, Forage, Hidden]() { return Controller->IsResourceFocused(Forage.id) && Hidden(); }, 0.7f);
    Add(TEXT("Wild gathering still grants exactly its own reward after weeding"),
        [this, Probe, Snapshot, Forage]()
        {
            Snapshot(); Probe->Ready = Probe->Expected.Harvest(Forage.id, Controller->PlayerPoint()).ok;
            Tap(EKeys::E);
        }, [Probe, Animation, Matches]()
        { return Probe->Ready && Matches() && Animation()->GatherStarts() == Probe->Starts + 1; }, 0.3f);
    Add(TEXT("Resource clearing on X remains clearing, not another weed/pick request"),
        [this, Probe, Snapshot, Forage]()
        {
            Snapshot(); Probe->Ready = Probe->Expected.Clear(Forage.id, Controller->PlayerPoint()).ok;
            Tap(EKeys::Gamepad_FaceButton_Left);
        }, [this, Probe, Animation, Matches]()
        { return Probe->Ready && !Controller->ToastIsError() && Matches() && Animation()->GatherStarts() == Probe->Starts; }, 0.12f);
    Add(TEXT("Cleared patch leaves no delayed pose"), []() {}, Hidden, 2.0f);
    Add(TEXT("Stand outside the garden and map to reject secondary tilling"),
        [this]() { Teleport({3900, 3900}); }, Hidden, 0.7f);
    Add(TEXT("No-plot/out-of-bounds secondary failure has no weeding delta or animation"),
        [this, Snapshot]() { Snapshot(); Tap(EKeys::F); },
        [this, Probe, Animation, Hidden, Matches]()
        { return Controller->ToastIsError() && Hidden() && Matches() && Animation()->GatherStarts() == Probe->Starts; });
}
