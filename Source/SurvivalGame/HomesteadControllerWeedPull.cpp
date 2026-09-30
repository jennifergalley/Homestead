// Weeding by hand (Jenny's playtest): she kneels on both knees, pulls two fistfuls and tosses them back
// over each shoulder (homestead_agent.kneel_pull_weeds). The pull is one transaction, a weed node's
// Harvest or a garden square's Weed, and it lands when the second root comes out
// (AHomesteadCharacter::PullWeedsCommit). Interrupted before then, nothing changes: no weeds in the
// pack, no Energy spent, the weed still standing. Without the clip the caller uses the pouch kneel.
#include "HomesteadController.h"

#include "HomesteadCharacter.h"
#include "HomesteadWorld.h"
#include "Simulation/HomesteadWeedPull.h"

#include "Engine/World.h"

namespace WeedPullTuning
{
// Turning to the clump, the step and the drop to both knees come before the clip reports itself as
// playing; past this, a pull that never started is dropped.
constexpr double StartTimeoutSeconds = 4.5;
// With the first fistful out the clump she pulls from shows at this share of its size (presentation
// only; the simulation changes at the second root).
constexpr float ThinnedFraction = 0.55f;
}

bool AHomesteadController::StartWeedPull(int32 NodeId, int32 PlotId, Homestead::Point Target)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar || !Avatar->CanPullWeeds()) return false;
    // One pull at a time: a press while she's still at it is ignored, never queued for later.
    if (PendingWeedNode != INDEX_NONE || PendingWeedPlot != INDEX_NONE) return true;
    // Refuse before she kneels, with the reason the commit itself would give.
    Homestead::Simulation Probe = Sim;
    const auto Position = PlayerPoint();
    const auto Ready = NodeId != INDEX_NONE ? Probe.Harvest(NodeId, Position) : Probe.Weed(PlotId, Position);
    if (!Ready)
    {
        Notify(Ready);
        return true;
    }
    // Her fistfuls look like the clump she pulls; the garden square's weeds are nettle tufts.
    UStaticMesh* Handful = NodeId != INDEX_NONE && Landscape ? Landscape->ResourceVisualMesh(NodeId) : nullptr;
    // Taken before the kneel is queued: if she's still finishing an earlier pull, its tail isn't this one's.
    const uint32 StartsBefore = Avatar->PullWeedsStarts();
    if (!Avatar->PlayPullWeeds(Target, Handful)) return false;
    PendingWeedStartsBefore = StartsBefore;
    PendingWeedNode = NodeId;
    PendingWeedPlot = PlotId;
    PendingWeedSince = GetWorld()->GetTimeSeconds();
    bPendingWeedStarted = false;
    return true;
}

void AHomesteadController::UpdatePendingWeedPull()
{
    if (PendingWeedNode == INDEX_NONE && PendingWeedPlot == INDEX_NONE) return;
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const float Phase = Avatar ? Avatar->PullWeedsPhase() : -1.0f;
    const auto Drop = [this]()
    {
        // A pull that never got to the second root puts the clump back as it was.
        if (Landscape) Landscape->RestoreThinnedResource();
        PendingWeedNode = INDEX_NONE;
        PendingWeedPlot = INDEX_NONE;
        bPendingWeedStarted = false;
    };
    if (!Avatar) { Drop(); return; }
    // Only this pull's own clip counts (Homestead::WeedPull::Advance): an earlier pull's tail never
    // commits it, and a clip stopped before the second root (walking off, the book, another action)
    // changes nothing.
    Homestead::WeedPull::Pending Pending{PendingWeedStartsBefore, PendingWeedSince, bPendingWeedStarted};
    const auto Step = Homestead::WeedPull::Advance(Pending, Avatar->PullWeedsStarts(), Phase, GetWorld()->GetTimeSeconds(),
        AHomesteadCharacter::PullWeedsFirstPull, AHomesteadCharacter::PullWeedsCommit, WeedPullTuning::StartTimeoutSeconds);
    bPendingWeedStarted = Pending.started;
    if (Step == Homestead::WeedPull::Step::Drop) { Drop(); return; }
    if (Step == Homestead::WeedPull::Step::Wait) return;
    if (Step == Homestead::WeedPull::Step::Thin)
    {
        if (PendingWeedNode != INDEX_NONE && Landscape) Landscape->ThinResource(PendingWeedNode, WeedPullTuning::ThinnedFraction);
        return;
    }
    const int32 Node = PendingWeedNode, Plot = PendingWeedPlot;
    const auto Position = PlayerPoint();
    const auto Result = Node != INDEX_NONE ? Sim.Harvest(Node, Position) : Sim.Weed(Plot, Position);
    // Pulled: the refresh removes the clump, so leave it thinned rather than pop it back for a frame.
    if (Result.ok && Landscape) Landscape->ForgetThinnedResource();
    Drop();
    Notify(Result, Node != INDEX_NONE ? WoodTapA.Get() : GrassStepA.Get());
}
