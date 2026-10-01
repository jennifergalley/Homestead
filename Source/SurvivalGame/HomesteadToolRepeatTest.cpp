#include "HomesteadSmokeTest.h"

#include "HomesteadAnimInstance.h"
#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "Simulation/HomesteadOvergrowth.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"

// Hold-to-repeat tool strikes on the Estate map (-HomesteadToolRepeatTest; Scripts\Test-Game.ps1
// -ToolRepeat). Holding the left mouse button keeps striking the aimed target until it clears and never
// moves on; a click is one blow; letting go finishes the blow under way; "Too tired." stops it. Each
// tool's blows and their spacing are reported as TOOL_REPEAT lines.
namespace ToolRepeatRoute
{
// Estate ground snap gives World Partition up to 90 s for collision (as the Estate smoke route).
constexpr float ArriveSeconds = 190.0f;
// Where she stands before pressing; a strike or hack walks her into its own stance from here.
constexpr double ApproachCm = 110.0;
// Targets with other overgrowth the same tool clears this close are skipped, so "it never moved on" is unambiguous.
constexpr double IsolationCm = 300.0;
// Long enough for a worn axe's held blows (36 frames apart) plus the walk-up and recovery.
constexpr float HoldSeconds = 20.0f;
// How long she keeps holding after a clear, to show nothing else is struck.
constexpr float AfterClearSeconds = 2.5f;

double Distance2D(Homestead::Point A, Homestead::Point B)
{
    return FMath::Sqrt(FMath::Square(A.x - B.x) + FMath::Square(A.y - B.y));
}

// The uncleared node of `Kind` nearest `From` that a tool of tier Tier may clear (its placement asks
// for no more), preferring one with no other overgrowth that tool clears within IsolationCm.
const Homestead::ResourceNode* FindTarget(const Homestead::State& State, Homestead::ResourceKind Kind,
    Homestead::ToolTier Tier, Homestead::Point From)
{
    const Homestead::ResourceNode* Best = nullptr;
    const Homestead::ResourceNode* Crowded = nullptr;
    for (const auto& Node : State.resources)
    {
        if (Node.kind != Kind || Node.cleared || Node.minTier > Tier) continue;
        // Only neighbours the same tool clears could be struck instead; anything else can stay close.
        const auto* Info = Homestead::FindOvergrowth(Kind);
        bool bCrowded = false;
        for (const auto& Other : State.resources)
        {
            const auto* OtherInfo = Homestead::FindOvergrowth(Other.kind);
            if (Other.id != Node.id && !Other.cleared && Info && OtherInfo && OtherInfo->tool == Info->tool
                && Distance2D(Other.position, Node.position) < IsolationCm)
            { bCrowded = true; break; }
        }
        auto*& Pick = bCrowded ? Crowded : Best;
        if (!Pick || Distance2D(Node.position, From) < Distance2D(Pick->position, From)) Pick = &Node;
    }
    return Best ? Best : Crowded;
}

bool Cleared(const Homestead::State& State, int32 Id)
{
    for (const auto& Node : State.resources) if (Node.id == Id) return Node.cleared;
    return false;
}

int32 ClearedCount(const Homestead::State& State)
{
    int32 Count = 0;
    for (const auto& Node : State.resources) Count += Node.cleared;
    return Count;
}

// One held run's evidence: blows landed (with their times), strike and hack clip starts.
struct FRun
{
    uint32 BlowsBefore = 0;
    uint32 FellStartsBefore = 0;
    uint32 HackStartsBefore = 0;
    uint32 LastBlows = 0;
    int32 ClearedBefore = 0;
    TArray<double> BlowTimes;
};
}

void AHomesteadSmokeTest::PrepareToolRepeatChecks()
{
    using Homestead::Item;
    using Homestead::ResourceKind;
    using namespace ToolRepeatRoute;
    const Homestead::State& Start = Controller->State();
    if (!Controller->IsEstateMap() || !Start.fixedEstate)
    {
        Finish(false, TEXT("The tool-repeat route needs the Estate map and a new estate game."));
        return;
    }
    const Homestead::Point Spawn = Controller->PlayerPoint();
    using Homestead::ToolTier;
    const auto* Stump = FindTarget(Start, ResourceKind::StumpMedium, ToolTier::Worn, Spawn);
    const auto* Rubble = FindTarget(Start, ResourceKind::Rubble, ToolTier::Worn, Spawn);
    const auto* Thicket = FindTarget(Start, ResourceKind::BrambleThicket, ToolTier::Iron, Spawn);
    const auto* Grass = FindTarget(Start, ResourceKind::TallGrass, ToolTier::Worn, Spawn);
    if (!Stump || !Rubble || !Thicket)
    {
        Finish(false, TEXT("The estate has no isolated medium stump, rubble heap or bramble thicket."));
        return;
    }

    const auto OnGround = [this]()
    {
        const auto* Avatar = Cast<ACharacter>(Controller->GetPawn());
        return Avatar && Avatar->GetCharacterMovement() && Avatar->GetCharacterMovement()->IsMovingOnGround();
    };
    const auto Anim = [this]() -> const UHomesteadAnimInstance*
    {
        const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
        return Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    };
    const auto Press = [this]() { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton, IE_Pressed, 1)); };
    const auto Release = [this]() { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton, IE_Released, 0)); };
    // Every blow's time, sampled each tick while a run is open.
    const auto Track = [this](const TSharedRef<FRun>& Run)
    {
        return [this, Run]()
        {
            while (Run->LastBlows < Controller->OvergrowthBlowsLanded)
            {
                ++Run->LastBlows;
                Run->BlowTimes.Add(GetWorld()->GetTimeSeconds());
            }
        };
    };
    const auto Begin = [this, Anim](const TSharedRef<FRun>& Run)
    {
        const auto* Animation = Anim();
        Run->BlowsBefore = Run->LastBlows = Controller->OvergrowthBlowsLanded;
        Run->FellStartsBefore = Animation ? Animation->FellStarts() : 0;
        Run->HackStartsBefore = Animation ? Animation->MacheteStarts() : 0;
        Run->ClearedBefore = ClearedCount(Controller->State());
        Run->BlowTimes.Reset();
    };
    const auto Report = [this, Anim](const FString& Label, const TSharedRef<FRun>& Run)
    {
        const auto* Animation = Anim();
        FString Gaps;
        for (int32 Index = 1; Index < Run->BlowTimes.Num(); ++Index)
            Gaps += FString::Printf(TEXT("%s%.2f"), Index > 1 ? TEXT(",") : TEXT(""), Run->BlowTimes[Index] - Run->BlowTimes[Index - 1]);
        Results.Add(FString::Printf(TEXT("TOOL_REPEAT %s blows=%u strike_starts=%u hack_starts=%u gaps_s=[%s] toast=\"%s\""),
            *Label, Controller->OvergrowthBlowsLanded - Run->BlowsBefore,
            Animation ? Animation->FellStarts() - Run->FellStartsBefore : 0,
            Animation ? Animation->MacheteStarts() - Run->HackStartsBefore : 0, *Gaps, *Controller->Toast()));
    };
    // Stand ApproachCm off the target, facing it, with Tool selected from its first-row cell.
    const auto Approach = [this, OnGround](const FString& Label, Homestead::Point Target, Item Tool)
    {
        const Homestead::Point Stand{Target.x + ApproachCm, Target.y};
        FStep& Arrive = Steps.AddDefaulted_GetRef();
        Arrive.Name = TEXT("Arrive beside ") + Label;
        Arrive.Action = [this, Stand]() { Controller->HomesteadTeleport(Stand.x, Stand.y); };
        Arrive.Check = [this, OnGround, Stand]()
        {
            return !Controller->bPendingGroundSnap && !Controller->GroundSnapStreamingActor && !Controller->bPendingSpawn
                && OnGround() && Distance2D(Controller->PlayerPoint(), Stand) < 150.0;
        };
        Arrive.Wait = ArriveSeconds;
        Arrive.bCompleteWhenReady = true;
        Add(TEXT("Face ") + Label + TEXT(" with the tool in hand"), [this, Target, Tool]()
            {
                const FRotator Facing(0, FMath::RadiansToDegrees(FMath::Atan2(Target.y - Controller->PlayerPoint().y,
                    Target.x - Controller->PlayerPoint().x)), 0);
                if (APawn* Pawn = Controller->GetPawn()) Pawn->SetActorRotation(Facing);
                Controller->SetControlRotation(Facing);
                Controller->HomesteadEnergy(100.0f);
                const int32 Cell = Controller->HotbarCellOf(Tool);
                if (Cell == INDEX_NONE) { Finish(false, TEXT("A granted tool is not in the first row of the pack.")); return; }
                Controller->SelectHotbarSlot(Cell);
            },
            [this, Tool]() { return Controller->SelectedCarriedTool() == Tool && !Controller->IsBookOpen(); }, 1.0f);
    };
    // Hold until the target clears, keep holding a little longer, then let go: exactly Expected blows,
    // StrikeStarts strike clips / HackStarts hack clips, and nothing else cleared.
    const auto HoldToClear = [this, Press, Release, Track, Begin, Report, Anim](const FString& Label, int32 Id,
        int32 Expected, int32 StrikeStarts, int32 HackStarts)
    {
        const auto Run = MakeShared<FRun>();
        const auto ClearedAt = MakeShared<double>(-1.0);
        FStep& Hold = Steps.AddDefaulted_GetRef();
        Hold.Name = TEXT("Hold LMB: ") + Label + TEXT(" until it clears, then nothing more");
        Hold.Action = [Begin, Run, Press, ClearedAt]() { Begin(Run); *ClearedAt = -1.0; Press(); };
        Hold.Repeat = [this, Track, Run, Id, ClearedAt]()
        {
            Track(Run)();
            if (*ClearedAt < 0 && Cleared(Controller->State(), Id)) *ClearedAt = GetWorld()->GetTimeSeconds();
        };
        Hold.Check = [this, Run, Id, ClearedAt, Expected, StrikeStarts, HackStarts, Anim]()
        {
            const auto* Animation = Anim();
            return *ClearedAt >= 0 && GetWorld()->GetTimeSeconds() - *ClearedAt >= AfterClearSeconds && Animation
                && Controller->OvergrowthBlowsLanded - Run->BlowsBefore == static_cast<uint32>(Expected)
                && Animation->FellStarts() - Run->FellStartsBefore == static_cast<uint32>(StrikeStarts)
                && Animation->MacheteStarts() - Run->HackStartsBefore == static_cast<uint32>(HackStarts)
                && ClearedCount(Controller->State()) == Run->ClearedBefore + 1;
        };
        Hold.Wait = HoldSeconds;
        Hold.bCompleteWhenReady = true;
        Add(TEXT("Let go after ") + Label, [Release]() { Release(); },
            [Report, Label, Run]() { Report(Label, Run); return true; }, 0.2f);
    };

    Add(TEXT("New estate game: she is on the ground with the book closed"), []() {},
        [this, OnGround]() { return !Controller->bPendingSpawn && OnGround() && !Controller->IsBookOpen(); }, ArriveSeconds);
    Steps.Last().bCompleteWhenReady = true;
    for (const Item Tool : {Item::Hatchet, Item::Pickaxe, Item::Billhook, Item::Scythe})
        if (Controller->Simulation().Count(Tool) == 0) QueueGrant(Tool, 1);
    Add(TEXT("An iron billhook for the thicket; the axe and pickaxe stay worn"), [this]()
        {
            Controller->Sim.SetToolTier(Homestead::ToolKind::Billhook, Homestead::ToolTier::Iron);
            Controller->Sim.SetToolTier(Homestead::ToolKind::Axe, Homestead::ToolTier::Worn);
            Controller->Sim.SetToolTier(Homestead::ToolKind::Pickaxe, Homestead::ToolTier::Worn);
        },
        [this]() { return Controller->Simulation().GetToolTier(Homestead::ToolKind::Billhook) == Homestead::ToolTier::Iron; });

    // 1. A worn axe on a medium stump (five blows): a click, a let-go, "Too tired.", then held to the end.
    const int32 StumpId = Stump->id;
    const Homestead::Point StumpAt = Stump->position;
    const int32 StumpBlows = Controller->Simulation().OvergrowthSwings(StumpId);
    if (StumpBlows < 4) { Finish(false, TEXT("The medium stump needs fewer worn-axe blows than this route assumes.")); return; }
    Approach(TEXT("a medium stump"), StumpAt, Item::Hatchet);
    {
        const auto Run = MakeShared<FRun>();
        FStep& Click = Steps.AddDefaulted_GetRef();
        Click.Name = TEXT("A click is exactly one blow");
        Click.Action = [this, Begin, Run]() { Begin(Run); Tap(EKeys::LeftMouseButton); };
        Click.Repeat = Track(Run);
        Click.Check = [this, Run, Anim, StumpId]()
        {
            const auto* Animation = Anim();
            return Controller->OvergrowthBlowsLanded - Run->BlowsBefore == 1 && Animation && !Animation->IsHandActionBusy()
                && !Cleared(Controller->State(), StumpId) && Controller->Toast().Contains(TEXT("more swings"));
        };
        Click.Wait = HoldSeconds;
        Click.bCompleteWhenReady = true;
        Add(TEXT("Report the click"), []() {}, [Report, Run]() { Report(TEXT("axe-click"), Run); return true; }, 0.1f);
    }
    {
        const auto Run = MakeShared<FRun>();
        FStep& LetGo = Steps.AddDefaulted_GetRef();
        LetGo.Name = TEXT("Letting go as the first held blow lands finishes that blow only");
        LetGo.Action = [Begin, Run, Press]() { Begin(Run); Press(); };
        LetGo.Repeat = [this, Track, Run, Release]()
        {
            Track(Run)();
            if (Run->BlowTimes.Num() == 1 && Controller->IsInputKeyDown(EKeys::LeftMouseButton)) Release();
        };
        LetGo.Check = [this, Run, Anim, StumpId]()
        {
            const auto* Animation = Anim();
            return Controller->OvergrowthBlowsLanded - Run->BlowsBefore == 1 && Animation && !Animation->IsHandActionBusy()
                && Animation->FellStarts() - Run->FellStartsBefore == 1 && !Cleared(Controller->State(), StumpId);
        };
        LetGo.Wait = HoldSeconds;
        LetGo.bCompleteWhenReady = true;
        Add(TEXT("Report the let-go"), []() {}, [Report, Run]() { Report(TEXT("axe-letgo"), Run); return true; }, 0.1f);
    }
    {
        const auto Run = MakeShared<FRun>();
        FStep& Tired = Steps.AddDefaulted_GetRef();
        Tired.Name = TEXT("Held, she stops with \"Too tired.\" once energy falls below the floor");
        Tired.Action = [Begin, Run, Press]() { Begin(Run); Press(); };
        Tired.Repeat = [this, Track, Run]()
        {
            const int32 Before = Run->BlowTimes.Num();
            Track(Run)();
            // Spent between blows: the next decision sees her below 10% energy.
            if (Before == 0 && Run->BlowTimes.Num() >= 1) Controller->HomesteadEnergy(9.5f);
        };
        Tired.Check = [this, Run, Anim, StumpId]()
        {
            const auto* Animation = Anim();
            return Animation && !Animation->IsHandActionBusy() && !Cleared(Controller->State(), StumpId)
                && Controller->OvergrowthBlowsLanded - Run->BlowsBefore <= 2
                && Controller->ToastIsError() && Controller->Toast().Contains(TEXT("Too tired"));
        };
        Tired.Wait = HoldSeconds;
        Tired.bCompleteWhenReady = true;
        Add(TEXT("Let go and rest"), [this, Release]() { Release(); Controller->HomesteadEnergy(100.0f); },
            [Report, Run]() { Report(TEXT("axe-tired"), Run); return true; }, 0.3f);
    }
    // What's left of the five, all in one strike clip that loops its stroke cycle.
    {
        const auto Left = MakeShared<int32>(0);
        Add(TEXT("Count the stump's remaining blows"), [this, Left, StumpId]()
            { *Left = Controller->Simulation().OvergrowthSwings(StumpId) - Controller->SwingsLanded; },
            [Left]() { return *Left >= 1; }, 0.1f);
        const auto Run = MakeShared<FRun>();
        const auto ClearedAt = MakeShared<double>(-1.0);
        FStep& Hold = Steps.AddDefaulted_GetRef();
        Hold.Name = TEXT("Hold LMB on the stump: every remaining blow in one strike, then nothing more");
        Hold.Action = [Begin, Run, Press, ClearedAt]() { Begin(Run); *ClearedAt = -1.0; Press(); };
        Hold.Repeat = [this, Track, Run, StumpId, ClearedAt]()
        {
            Track(Run)();
            if (*ClearedAt < 0 && Cleared(Controller->State(), StumpId)) *ClearedAt = GetWorld()->GetTimeSeconds();
        };
        Hold.Check = [this, Run, Left, ClearedAt, Anim]()
        {
            const auto* Animation = Anim();
            return *ClearedAt >= 0 && GetWorld()->GetTimeSeconds() - *ClearedAt >= AfterClearSeconds && Animation
                && Controller->OvergrowthBlowsLanded - Run->BlowsBefore == static_cast<uint32>(*Left)
                && Animation->FellStarts() - Run->FellStartsBefore == 1
                && ClearedCount(Controller->State()) == Run->ClearedBefore + 1;
        };
        Hold.Wait = HoldSeconds;
        Hold.bCompleteWhenReady = true;
        Add(TEXT("Let go after the stump"), [Release]() { Release(); },
            [Report, Run]() { Report(TEXT("axe-hold"), Run); return true; }, 0.2f);
    }

    // 2. A worn pickaxe on rubble: both blows in one strike clip.
    Approach(TEXT("a rubble heap"), Rubble->position, Item::Pickaxe);
    HoldToClear(TEXT("pickaxe on rubble"), Rubble->id, Controller->Simulation().OvergrowthSwings(Rubble->id), 1, 0);

    // 3. An iron billhook on a thicket: a fresh hack per blow (the hack clip has no stroke loop).
    Approach(TEXT("a bramble thicket"), Thicket->position, Item::Billhook);
    {
        const int32 Hacks = Homestead::FindOvergrowth(ResourceKind::BrambleThicket)
            ->swings[static_cast<int32>(Homestead::ToolTier::Iron)];
        HoldToClear(TEXT("billhook on a thicket"), Thicket->id, Hacks, 0, Hacks);
    }

    // 4. The scythe: one sweep mows the arc; held on, she doesn't carry on to other grass.
    if (Grass)
    {
        Approach(TEXT("tall grass"), Grass->position, Item::Scythe);
        HoldToClear(TEXT("scythe on tall grass"), Grass->id, 1, 1, 0);
    }
    else Results.Add(TEXT("TOOL_REPEAT scythe skipped: no isolated tall grass on the estate"));
}
