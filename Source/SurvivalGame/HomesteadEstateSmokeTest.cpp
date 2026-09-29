// The packaged Estate smoke route (-HomesteadSmokeTest -HomesteadEstateSmoke, run by
// Scripts\Test-Game.ps1 -EstateSmoke). Every other packaged suite plays the legacy woodland map, so
// this one covers what Jenny plays: a new estate game started the way a player starts one (the Names
// step skipped with -HomesteadSkipNewGameSetup), her ground settle at each part of the estate, a few
// ordinary actions, and frame timing at the manor and in the woods. The runner also fails the run on
// material-usage, Default Material, ground-hold and Error lines in the log.
#include "HomesteadSmokeTest.h"

#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "Simulation/HomesteadEstate.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"

namespace EstateSmokeRoute
{
// She arrives through the controller's ground settle, which holds her up to 180 s while World
// Partition streams the collision in, so an arrival may take that long in a cold package.
constexpr float ArriveSeconds = 190.0f;
// Frame timing: let streaming and shader work settle after she arrives, then sample.
constexpr double TimingSettleSeconds = 6.0;
constexpr double TimingSampleSeconds = 12.0;
// Where she stands to act on a resource (the controller's focus reaches 280 cm).
constexpr double ApproachCm = 110.0;
// A resource is used for an action only when nothing else she could focus is this close.
constexpr double IsolationCm = 420.0;
// Visits beside a tree or a heap stand this far off, clear of its trunk or collision.
constexpr double StandOffCm = 260.0;
// How far from the requested spot the settle may leave her and still count as arrived.
constexpr double ArrivedWithinCm = 300.0;
// Hand actions commit at the clip's contact beat, a few seconds after the press.
constexpr float ActionSeconds = 12.0f;
// Night for the lamp: after dark on the estate (AHomesteadWorld::UpdateLighting).
constexpr float NightHour = 22.0f;

double Distance2D(Homestead::Point A, Homestead::Point B)
{
    return FMath::Sqrt(FMath::Square(A.x - B.x) + FMath::Square(A.y - B.y));
}

// The first uncleared resource of `Kind` with no other focusable thing within IsolationCm, so the
// controller's nearest-thing focus lands on it; any uncleared one of that kind otherwise.
const Homestead::ResourceNode* FindActionTarget(const Homestead::State& State, Homestead::ResourceKind Kind)
{
    const Homestead::ResourceNode* Fallback = nullptr;
    for (const auto& Node : State.resources)
    {
        if (Node.kind != Kind || Node.cleared || Node.readyAtHour > State.hour) continue;
        if (!Fallback) Fallback = &Node;
        bool bCrowded = false;
        for (const auto& Other : State.resources)
            if (Other.id != Node.id && !Other.cleared && Distance2D(Other.position, Node.position) < IsolationCm)
            { bCrowded = true; break; }
        for (const auto& Piece : State.structures)
            if (!bCrowded && Distance2D(Homestead::StructureCenter(State, Piece), Node.position) < IsolationCm) bCrowded = true;
        if (!bCrowded) return &Node;
    }
    return Fallback;
}

// The first uncleared resource whose placement id lies in [First, Last).
const Homestead::ResourceNode* FindInRange(const Homestead::State& State, int32 First, int32 Last)
{
    for (const auto& Node : State.resources)
        if (Node.id >= First && Node.id < Last && !Node.cleared) return &Node;
    return nullptr;
}

const Homestead::ResourceNode* FindNode(const Homestead::State& State, int32 Id)
{
    for (const auto& Node : State.resources)
        if (Node.id == Id) return &Node;
    return nullptr;
}
}

void AHomesteadSmokeTest::PrepareEstateSmokeChecks()
{
    using Homestead::Point;
    const Homestead::State& Start = Controller->State();
    if (!Controller->IsEstateMap() || !Start.fixedEstate)
    {
        Finish(false, TEXT("The Estate smoke route needs the Estate map (/Game/SurvivalGame/Maps/Estate) and a new estate game."));
        return;
    }
    const Homestead::EstateLayout& Layout = Homestead::ProvisionalEstateLayout();

    const auto OnGround = [this]()
    {
        const auto* Avatar = Cast<ACharacter>(Controller->GetPawn());
        return Avatar && Avatar->GetCharacterMovement() && Avatar->GetCharacterMovement()->IsMovingOnGround();
    };
    // Visits go through HomesteadTeleport, the same ground settle a player's teleport or spawn uses.
    const auto Visit = [this, OnGround](const FString& Label, Point Target, const FString& Capture)
    {
        FStep& Arrive = Steps.AddDefaulted_GetRef();
        Arrive.Name = TEXT("Arrive on the ground at ") + Label;
        Arrive.Action = [this, Target]() { Controller->HomesteadTeleport(Target.x, Target.y); };
        Arrive.Check = [this, OnGround, Target]()
        {
            return !Controller->bPendingGroundSnap && !Controller->bPendingSpawn && OnGround()
                && EstateSmokeRoute::Distance2D(Controller->PlayerPoint(), Target) < EstateSmokeRoute::ArrivedWithinCm;
        };
        Arrive.Wait = EstateSmokeRoute::ArriveSeconds;
        Arrive.bCompleteWhenReady = true;
        if (!Capture.IsEmpty())
            Add(TEXT("Capture ") + Label, [this, Capture]() { Screenshot(Capture); }, [OnGround]() { return OnGround(); }, 3.0f);
    };
    // Frame timing over TimingSampleSeconds after TimingSettleSeconds, reported as PERFORMANCE_AT.
    const auto Measure = [this](const FString& Place)
    {
        TSharedRef<double> Began = MakeShared<double>(0.0);
        TSharedRef<double> Last = MakeShared<double>(0.0);
        TSharedRef<TArray<double>> Frames = MakeShared<TArray<double>>();
        FStep& Step = Steps.AddDefaulted_GetRef();
        Step.Name = TEXT("Frame timing at ") + Place;
        Step.Action = [Began, Last, Frames]() { *Began = *Last = FPlatformTime::Seconds(); Frames->Reset(); };
        Step.Repeat = [Began, Last, Frames]()
        {
            const double Now = FPlatformTime::Seconds();
            if (Now - *Began >= EstateSmokeRoute::TimingSettleSeconds && Now > *Last) Frames->Add((Now - *Last) * 1000.0);
            *Last = Now;
        };
        Step.Check = [this, Place, Frames]()
        {
            if (Frames->Num() < 30) return false;
            TArray<double> Sorted = *Frames;
            Sorted.Sort();
            double Total = 0;
            for (const double Value : Sorted) Total += Value;
            const int32 LastIndex = Sorted.Num() - 1;
            Results.Add(FString::Printf(TEXT("PERFORMANCE_AT %s mean_fps=%.2f p95_frame_ms=%.2f p99_frame_ms=%.2f samples=%d"),
                *Place, 1000.0 / (Total / Sorted.Num()), Sorted[FMath::FloorToInt(LastIndex * 0.95)],
                Sorted[FMath::FloorToInt(LastIndex * 0.99)], Sorted.Num()));
            return true;
        };
        Step.Wait = static_cast<float>(EstateSmokeRoute::TimingSettleSeconds + EstateSmokeRoute::TimingSampleSeconds);
    };

    // 1. The new game: BeginPlay started an estate game and skipped the setup (default names).
    {
        FStep& Step = Steps.AddDefaulted_GetRef();
        Step.Name = TEXT("New estate game: she wakes on the ground in the standing room");
        Step.Action = []() {};
        Step.Check = [this, OnGround]()
        {
            return Controller->State().fixedEstate && !Controller->bPendingSpawn && OnGround()
                && !Controller->IsBookOpen() && !Controller->IsNewGameSetup() && !Controller->IsNamingSetup()
                && !Controller->State().heroineName.empty() && Controller->HasHeroine();
        };
        Step.Wait = EstateSmokeRoute::ArriveSeconds;
        Step.bCompleteWhenReady = true;
    }
    Add(TEXT("Hands free for the first actions"), [this]() { Controller->SelectHotbarSlot(0); },
        [this]() { return Controller->SelectedCarriedTool() == Homestead::Item::Count; });
    Add(TEXT("Capture the standing room"), [this]() { Screenshot(TEXT("estate-manor")); }, []() { return true; }, 3.0f);
    Measure(TEXT("manor"));

    // 2. Gather a loose stone by hand.
    if (const auto* Stone = EstateSmokeRoute::FindActionTarget(Start, Homestead::ResourceKind::Stones))
    {
        const int32 Id = Stone->id;
        Visit(TEXT("a loose stone"), {Stone->position.x + EstateSmokeRoute::ApproachCm, Stone->position.y}, FString());
        Add(TEXT("The stone is in focus"), []() {}, [this, Id]() { return Controller->IsResourceFocused(Id); }, 1.0f);
        TSharedRef<int32> Before = MakeShared<int32>(0);
        FStep& Gather = Steps.AddDefaulted_GetRef();
        Gather.Name = TEXT("Gather the stone with gamepad A");
        Gather.Action = [this, Before]() { *Before = Controller->Simulation().Count(Homestead::Item::Stone); Tap(EKeys::Gamepad_FaceButton_Bottom); };
        Gather.Check = [this, Before]() { return Controller->Simulation().Count(Homestead::Item::Stone) > *Before; };
        Gather.Wait = EstateSmokeRoute::ActionSeconds;
        Gather.bCompleteWhenReady = true;
    }
    else { Finish(false, TEXT("The estate has no loose stones to gather.")); return; }

    // 3. Pull a weed by hand.
    if (const auto* Weed = EstateSmokeRoute::FindActionTarget(Start, Homestead::ResourceKind::Weeds))
    {
        const int32 Id = Weed->id;
        Visit(TEXT("a weed"), {Weed->position.x + EstateSmokeRoute::ApproachCm, Weed->position.y}, FString());
        Add(TEXT("The weed is in focus"), []() {}, [this, Id]() { return Controller->IsResourceFocused(Id); }, 1.0f);
        TSharedRef<int32> WeedsBefore = MakeShared<int32>(0);
        FStep& Pull = Steps.AddDefaulted_GetRef();
        Pull.Name = TEXT("Pull the weed with gamepad A");
        Pull.Action = [this, WeedsBefore]() { *WeedsBefore = Controller->Simulation().Count(Homestead::Item::Weeds); Tap(EKeys::Gamepad_FaceButton_Bottom); };
        Pull.Check = [this, Id, WeedsBefore]()
        {
            const auto* Node = EstateSmokeRoute::FindNode(Controller->State(), Id);
            return Node && Node->cleared && Controller->Simulation().Count(Homestead::Item::Weeds) > *WeedsBefore;
        };
        Pull.Wait = EstateSmokeRoute::ActionSeconds;
        Pull.bCompleteWhenReady = true;
    }
    else { Finish(false, TEXT("The estate has no weeds to pull.")); return; }

    // 4. Each part of the estate: she must settle on the ground, and the log must stay clean.
    if (const auto* Clearout = EstateSmokeRoute::FindInRange(Start, 570000, 580000))
        Visit(TEXT("the manor clear-out"), {Clearout->position.x + EstateSmokeRoute::StandOffCm, Clearout->position.y}, TEXT("estate-clearout"));
    else { Finish(false, TEXT("The manor clear-out placements (570000+) are missing.")); return; }
    const Homestead::DerelictFarmPlan Farm = Homestead::EstateDerelictFarm(Layout);
    if (Farm.valid)
        Visit(TEXT("the derelict farm"), Farm.World(Farm.lengthU * 0.5, Farm.lengthV * 0.5), TEXT("estate-farm"));
    else { Finish(false, TEXT("The derelict farm's field is missing from the estate layout.")); return; }
    if (const auto* Woods = EstateSmokeRoute::FindInRange(Start, 560000, 570000))
    {
        Visit(TEXT("the MVP woodland"), {Woods->position.x + EstateSmokeRoute::StandOffCm, Woods->position.y}, TEXT("estate-woods"));
        Measure(TEXT("woods"));
    }
    else { Finish(false, TEXT("The MVP woodland placements (560000+) are missing.")); return; }
    if (const auto* Gateway = Layout.FindLandmark(Homestead::Anchor::EstateGateway))
        Visit(TEXT("the drive at the estate gateway"), Gateway->position, TEXT("estate-drive"));
    else { Finish(false, TEXT("The estate gateway anchor is missing.")); return; }
    if (const auto* Store = Layout.FindLandmark(Homestead::Anchor::GeneralStoreDoor))
        Visit(TEXT("the general store door"), Store->position, TEXT("estate-store"));
    else { Finish(false, TEXT("The general store door anchor is missing.")); return; }

    // 5. The lamp at night: select it on the hotbar (key 8) and it lights in her hand.
    Add(TEXT("Night falls"), [this]() { Controller->HomesteadEnergy(100.0f); Controller->HomesteadMorning(EstateSmokeRoute::NightHour); },
        [this]() { return FMath::Fmod(Controller->State().hour, 24.0) >= EstateSmokeRoute::NightHour - 0.01; }, 1.0f);
    Add(TEXT("Select the oil lamp with key 8"), [this]() { Tap(EKeys::Eight); },
        [this]()
        {
            return Controller->SelectedCarriedTool() == Homestead::Item::OilLamp
                && Controller->Simulation().IsLampInHand() && Controller->Simulation().IsLampLit();
        }, 2.0f);
    Add(TEXT("Capture the lamp at night"), [this]() { Screenshot(TEXT("estate-lamp-night")); }, []() { return true; }, 3.0f);
}
