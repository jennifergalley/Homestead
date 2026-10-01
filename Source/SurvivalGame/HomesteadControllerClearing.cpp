#include "HomesteadController.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWorld.h"
#include "Simulation/HomesteadOvergrowth.h"
#include "Simulation/HomesteadSwingTiming.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

using HomesteadControllerText::Text;

namespace HomesteadClearingMix
{
// The scythe's airy swish (Assets/Audio/Effects/ScytheSwish.wav, loudest 100 ms about -17 dBFS) at
// about -31 dBFS with default Effects 0.8: a few dB over the ambience and music, about 9 dB under an
// axe chop (0.75) and still about 20 dB over her bare steps on grass. At 0.8 it was as loud as a chop.
constexpr float ScytheSwishGain = 0.25f;
}

void AHomesteadController::StartMacheteHack()
{
    if (bHackPending || Focus != EFocus::Underbrush) return;
    if (Sim.Count(Homestead::Item::Machete) == 0)
    {
        Notify(TEXT("Take your machete from storage to hack through undergrowth."), true);
        return;
    }
    // Refuse before the swing rather than after it when she's too tired to clear this plant.
    const auto Rested = Sim.CheckExertion(bFocusBrushWoody
        ? Homestead::Exertion::WoodyUnderbrushEnergy : Homestead::Exertion::SoftUnderbrushEnergy);
    if (!Rested)
    {
        Notify(Rested);
        return;
    }
    const Homestead::Point Target{FocusBrushPosition.X, FocusBrushPosition.Y};
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    HackChunk = FocusBrushChunk;
    HackIndex = FocusBrushIndex;
    HackPosition = FocusBrushPosition;
    bHackWoody = bFocusBrushWoody;
    if (Avatar && Avatar->PlayMacheteHack(Target))
    {
        bHackPending = true;
        HackSince = GetWorld()->GetTimeSeconds();
        return;
    }
    // No hacking clip (legacy heroine): clear at once with the generic swing.
    const auto Result = Sim.ClearUnderbrush({HackChunk.X, HackChunk.Y}, HackIndex, bHackWoody, Target, PlayerPoint());
    NotifyResourceAction(Result, WoodTapB);
    if (Result.ok && Avatar) Avatar->PlayClear(Target);
}

void AHomesteadController::PresentFelling(int32 ResourceId, Homestead::Point Target, bool bTree)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar) return;
    // Called just after the clear committed, before the woodland rebuilds, so the tree is still
    // in the world's batches. A mature tree takes three strokes; a sapling one.
    FVector2D Trunk(Target.x, Target.y);
    float Radius = 5.0f;
    const bool bTreeTrunk = (Landscape && Landscape->TreeChopTarget(ResourceId, Trunk, Radius)) || bTree;
    const auto* Animation = Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance());
    const int32 Strokes = bTreeTrunk ? 3 : 1;
    if (Animation && Avatar->PlayFell({Trunk.X, Trunk.Y}, Strokes, FMath::Max(Radius, 5.0f)))
    {
        if (Landscape) Landscape->BeginFelling(ResourceId);
        FellResource = ResourceId;
        FellStrokes = Strokes;
        FellStrokesHeard = 0;
        FellStartsBefore = Animation->FellStarts();
        bFellSeen = false;
        FellSince = GetWorld()->GetTimeSeconds();
        return;
    }
    Avatar->PlayClear(Target);
}

void AHomesteadController::UpdatePendingFell()
{
    FVector Landing;
    if (Landscape && Landscape->TakeFelledTreeLanding(Landing))
    {
        if (TreeFallThud) PlayEffect(TreeFallThud, 0.7f);
        else PlayEffect(WoodTapA, 0.6f);
    }
    if (FellResource == INDEX_NONE) return;
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    const bool Felling = Animation && Animation->IsFelling() && Animation->FellStarts() != FellStartsBefore;
    bFellSeen |= Felling;
    // Waiting for the clip to start (the request lands on the next animation update).
    if (!bFellSeen && Animation && (Avatar->IsApproachingFell() || GetWorld()->GetTimeSeconds() - FellSince < 0.5))
    {
        if (Avatar->IsApproachingFell()) FellSince = GetWorld()->GetTimeSeconds();
        return;
    }
    const float Phase = Felling ? Animation->FellPhase() : 1e6f;
    while (Felling && FellStrokesHeard < FellStrokes
        && Phase >= AHomesteadCharacter::FellStrikeSeconds(FellStrokesHeard))
    {
        // About 7 dB under the old wood taps; the hammer-cut variant is a touch hotter.
        if (!ChopStrokes.IsEmpty())
        {
            const int32 Pick = FellStrokesHeard % ChopStrokes.Num();
            PlayEffect(ChopStrokes[Pick].Get(), Pick == 2 ? 0.65f : 0.8f);
        }
        else PlayEffect(FellStrokesHeard % 2 ? WoodTapA.Get() : WoodTapB.Get(), 0.55f);
        ++FellStrokesHeard;
    }
    // The last stroke through the notch, or she stopped: the tree goes over.
    if (!Felling || Phase >= AHomesteadCharacter::FellStrikeSeconds(FellStrokes - 1) + 0.2f)
    {
        if (Landscape)
        {
            const FVector From = Avatar ? Avatar->GetActorLocation() : FVector::ZeroVector;
            Landscape->DropFelledTree(FVector2D(From.X, From.Y));
        }
        FellResource = INDEX_NONE;
    }
}

void AHomesteadController::UpdatePendingHack()
{
    if (!bHackPending) return;
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    if (Animation && Animation->IsHacking() && Animation->MachetePhase() >= AHomesteadCharacter::MacheteClearSeconds)
    {
        bHackPending = false;
        const auto Result = Sim.ClearUnderbrush({HackChunk.X, HackChunk.Y}, HackIndex, bHackWoody,
            {HackPosition.X, HackPosition.Y}, PlayerPoint());
        NotifyResourceAction(Result, WoodTapB);
        return;
    }
    // Interrupted (she moved, opened the book) before the cut landed: nothing is cleared.
    if (!Animation || (!Animation->IsHacking() && GetWorld()->GetTimeSeconds() - HackSince > 0.4))
        bHackPending = false;
}

void AHomesteadController::ResetOvergrowthSwing()
{
    SwingNode = INDEX_NONE;
    SwingsLanded = 0;
    bSwingPending = false;
    ScytheTargets.Reset();
}

void AHomesteadController::SwingAtOvergrowth(Homestead::Item Tool)
{
    if (bSwingPending) return;
    const auto Position = PlayerPoint();
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const FVector Forward = GetPawn() ? GetPawn()->GetActorForwardVector() : FVector::ForwardVector;
    const Homestead::Point Facing{Forward.X, Forward.Y};
    // Walking off resets a half-cleared target: its earlier blows never persist.
    if (SwingNode != INDEX_NONE && FVector2D::Distance(SwingFrom, FVector2D(Position.x, Position.y)) > 150.0)
        ResetOvergrowthSwing();

    int32 Target = INDEX_NONE;
    if (Tool == Homestead::Item::Scythe)
    {
        const auto Arc = Sim.ScytheArcTargets(Position, Facing);
        ScytheTargets.Reset();
        for (const int32 Id : Arc) if (Sim.CheckOvergrowth(Id, Tool, Position)) ScytheTargets.Add(Id);
        if (!ScytheTargets.IsEmpty()) Target = ScytheTargets[0];
    }
    else
    {
        // Only what she's aimed at (the prompt names the same target, HeldToolFocus): never overgrowth
        // behind her or off to the side, even if it's nearer and this tool clears it.
        Target = Sim.FindAimedOvergrowth(Position, Facing, Tool);
    }
    if (Target == INDEX_NONE)
    {
        // Aimed at overgrowth another tool clears: say which.
        if (Focus == EFocus::Resource)
            for (const auto& Node : State().resources)
                if (Node.id == FocusId && Homestead::IsOvergrowth(Node.kind))
                {
                    const auto Check = Sim.CheckOvergrowth(FocusId, Tool, Position);
                    // This tool handles it, but it isn't ahead of her (or is past the scythe's short arc).
                    if (Check) Notify(Tool == Homestead::Item::Scythe ? TEXT("Step closer and face it to mow.")
                        : TEXT("Turn to face it."), true);
                    else Notify(Check);
                    return;
                }
        Notify(Tool == Homestead::Item::Scythe ? TEXT("Face tall grass, weeds or nettles to mow.")
            : Tool == Homestead::Item::Billhook ? TEXT("Aim at bramble or a sapling.")
            : Tool == Homestead::Item::Pickaxe ? TEXT("Aim at rubble or a rock.")
            : TEXT("Aim at a tree, stump or fallen timber."), true);
        return;
    }
    const auto Ready = Sim.CheckOvergrowth(Target, Tool, Position);
    if (!Ready)
    {
        // Out of tier (or otherwise refused): she doesn't swing at all, and nothing changes.
        Notify(Ready);
        return;
    }
    if (Target != SwingNode)
    {
        SwingNode = Target;
        SwingsLanded = 0;
    }
    SwingFrom = FVector2D(Position.x, Position.y);
    SwingTool = Tool;
    Homestead::Point Aim = Position;
    auto Kind = Homestead::ResourceKind::Count;
    for (const auto& Node : State().resources) if (Node.id == Target) { Aim = Node.position; Kind = Node.kind; }
    bool bAnimated = false;
    bSwingFellTimed = false;
    SwingStrokes = 1;
    SwingStrokesLanded = 0;
    if (Avatar)
    {
        // Rough footprint radius (cm) of what she strikes, so the point or bit lands on its near side.
        const float Radius = Kind == Homestead::ResourceKind::StumpSmall ? 16.0f
            : Kind == Homestead::ResourceKind::StumpMedium ? 30.0f
            : Kind == Homestead::ResourceKind::StumpLarge ? 44.0f
            : Kind == Homestead::ResourceKind::StumpAncient ? 45.0f
            : Kind == Homestead::ResourceKind::FallenLog ? 20.0f
            : Kind == Homestead::ResourceKind::GiantLog ? 38.0f
            : Kind == Homestead::ResourceKind::Rubble ? 42.0f
            : Kind == Homestead::ResourceKind::Boulder ? 70.0f
            : Kind == Homestead::ResourceKind::SmallRock ? 30.0f
            // The ruin's fallen roof timbers (about 430 x 230 cm): she strikes the near beam from outside the pile.
            : Kind == Homestead::ResourceKind::RuinTimbers ? 120.0f
            // Brambles for the billhook's cut to meet on their near side (the sapling's stem is the default).
            : Kind == Homestead::ResourceKind::BrambleThin ? 35.0f
            : Kind == Homestead::ResourceKind::BrambleThicket ? 55.0f
            : Kind == Homestead::ResourceKind::BrambleBank ? 80.0f : 8.0f;
        if (Tool == Homestead::Item::Scythe)
        {
            // Mowing turns about her: she keeps facing the swath rather than the first tuft.
            const Homestead::Point Ahead{Position.x + Forward.X * 100.0, Position.y + Forward.Y * 100.0};
            bSwingFellTimed = Avatar->PlayStrike(Ahead, Tool, 1, -1.0f);
        }
        else if (Tool == Homestead::Item::Hatchet || Tool == Homestead::Item::Pickaxe)
        {
            // Jenny (09-29): one press on a rock plays every blow it still needs (the strike clip's two
            // authored contacts, looped for more), each landing at its own contact, one reward at the end.
            if (Tool == Homestead::Item::Pickaxe)
                SwingStrokes = FMath::Clamp(Sim.OvergrowthSwings(Target) - SwingsLanded, 1, MaxPickStrokesPerPress);
            bSwingFellTimed = Avatar->PlayStrike(Aim, Tool, SwingStrokes, Radius);
            // Without the strike clip the axe falls back to its felling chop.
            if (!bSwingFellTimed && Tool == Homestead::Item::Hatchet) bSwingFellTimed = Avatar->PlayFell(Aim, 1, 12.0f);
        }
        bAnimated = bSwingFellTimed;
        // The billhook reuses the machete hack; a scythe without its mowing clip borrows it too.
        if (!bAnimated && Tool != Homestead::Item::Pickaxe && Tool != Homestead::Item::Hatchet)
            bAnimated = Avatar->PlayMacheteHack(Aim, Tool, Tool == Homestead::Item::Billhook ? Radius : -1.0f);
    }
    if (!bAnimated)
    {
        if (Avatar) Avatar->PlayClear(Aim);
        LandOvergrowthSwing();
        return;
    }
    bSwingPending = true;
    SwingSince = GetWorld()->GetTimeSeconds();
    if (const auto* Animation = Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()))
    {
        SwingFellStartsBefore = Animation->FellStarts();
        SwingHackStartsBefore = Animation->MacheteStarts();
    }
}

void AHomesteadController::UpdatePendingSwing()
{
    if (!bSwingPending) return;
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    const double Age = GetWorld()->GetTimeSeconds() - SwingSince;
    if (bSwingFellTimed)
    {
        const bool Felling = Animation && Animation->IsFelling() && Animation->FellStarts() != SwingFellStartsBefore;
        // Each blow of the press lands at its own contact; the last one ends the swing.
        while (Felling && bSwingPending && SwingStrokesLanded < SwingStrokes
            && Animation->FellPhase() >= AHomesteadCharacter::FellStrikeSeconds(SwingStrokesLanded))
        {
            ++SwingStrokesLanded;
            const bool bMore = SwingStrokesLanded < SwingStrokes;
            if (!bMore) bSwingPending = false;
            LandOvergrowthSwing(bMore);
        }
        if (Felling) return;
        // Still stepping into the stance, or the clip hasn't started yet.
        if (!Felling && Animation && (Avatar->IsApproachingFell() || Age < 0.5))
        {
            if (Avatar->IsApproachingFell()) SwingSince = GetWorld()->GetTimeSeconds();
            return;
        }
        if (!Felling) bSwingPending = false;
        return;
    }
    if (!Animation)
    {
        bSwingPending = false;
        return;
    }
    // Only this press's own hack lands, at its contact: a click during the last hack's follow-through
    // mustn't count that old clip against the new target (Homestead::SwingTiming::Advance). Walking up
    // to the stance keeps it waiting; a hack that never starts, or stops early, drops it.
    const bool bApproaching = Avatar && Avatar->IsApproachingFell();
    if (bApproaching) SwingSince = GetWorld()->GetTimeSeconds();
    switch (Homestead::SwingTiming::Advance(SwingHackStartsBefore, Animation->MacheteStarts(), Animation->IsHacking(),
        Animation->MachetePhase(), AHomesteadCharacter::MacheteClearSeconds, bApproaching,
        GetWorld()->GetTimeSeconds() - SwingSince, 0.4))
    {
    case Homestead::SwingTiming::Step::Land:
        bSwingPending = false;
        LandOvergrowthSwing();
        break;
    case Homestead::SwingTiming::Step::Drop:
        bSwingPending = false;
        break;
    default:
        break;
    }
}

void AHomesteadController::LandOvergrowthSwing(bool bMoreComing)
{
    const auto Position = PlayerPoint();
    if (SwingTool == Homestead::Item::Scythe)
    {
        // One sweep mows everything in the arc, each tuft its own transaction, with one summary.
        const int32 HayBefore = Sim.Count(Homestead::Item::Hay), WeedsBefore = Sim.Count(Homestead::Item::Weeds);
        const int32 SeedsBefore = Sim.Count(Homestead::Item::Seeds);
        const auto Sweep = Sim.MowSweep(std::vector<int>(ScytheTargets.GetData(),
            ScytheTargets.GetData() + ScytheTargets.Num()), Position);
        const int32 Mown = Sweep.mown;
        const FString Problem = UTF8_TO_TCHAR(Sweep.problem.c_str());
        ResetOvergrowthSwing();
        if (Mown == 0)
        {
            Notify(Problem.IsEmpty() ? TEXT("Nothing left in reach to mow.") : Problem, true);
            return;
        }
        FString Summary = FString::Printf(TEXT("Mowed %d %s"), Mown, Mown == 1 ? TEXT("tuft") : TEXT("tufts"));
        const int32 Hay = Sim.Count(Homestead::Item::Hay) - HayBefore, Weeds = Sim.Count(Homestead::Item::Weeds) - WeedsBefore;
        if (Hay > 0 || Weeds > 0) Summary += TEXT(":");
        if (Hay > 0) Summary += FString::Printf(TEXT(" +%d Hay"), Hay);
        if (Weeds > 0) Summary += FString::Printf(TEXT("%s +%d Weeds"), Hay > 0 ? TEXT(",") : TEXT(""), Weeds);
        if (const int32 Seeds = Sim.Count(Homestead::Item::Seeds) - SeedsBefore; Seeds > 0)
            Summary += FString::Printf(TEXT(", +%d Seeds"), Seeds);
        Notify(Summary + TEXT("."));
        // One airy swish at blade contact for the whole sweep; nothing on a miss or a cancel. With the
        // cue missing she mows in silence (InitializeAudio logged it) rather than with a footstep.
        if (ScytheSwish) PlayEffect(ScytheSwish, HomesteadClearingMix::ScytheSwishGain);
        return;
    }
    if (SwingNode == INDEX_NONE) return;
    ++SwingsLanded;
    // Blows count from where she actually stood when they landed (a billhook swing may walk her up first).
    SwingFrom = FVector2D(Position.x, Position.y);
    const int32 Needed = Sim.OvergrowthSwings(SwingNode);
    if (SwingsLanded < Needed)
    {
        // A hit that doesn't break it yet: a chop or a crack, and how much is left.
        if (!ChopStrokes.IsEmpty()) PlayEffect(ChopStrokes[SwingsLanded % ChopStrokes.Num()].Get(), 0.75f);
        else PlayEffect(WoodTapA, 0.6f);
        if (bMoreComing) return;
        const int32 Left = Needed - SwingsLanded;
        Notify(FString::Printf(TEXT("%d more %s."), Left, Left == 1 ? TEXT("swing") : TEXT("swings")));
        return;
    }
    const auto Result = Sim.ClearOvergrowth(SwingNode, SwingTool, Position);
    ResetOvergrowthSwing();
    Notify(Result, SwingTool == Homestead::Item::Pickaxe ? CraftStrikeA.Get() : WoodTapB.Get());
}
