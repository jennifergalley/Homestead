#pragma once

// The weed pull's pending state machine (AHomesteadController::UpdatePendingWeedPull), kept free of
// Unreal types so its timing rules are pinned by native tests. A pull commits once, when its own clip
// passes the second root; an earlier pull's tail (still playing when she pressed again) never counts,
// and a pull that stops before its commit changes nothing.
namespace Homestead
{
namespace WeedPull
{
enum class Step : int { Wait, Thin, Commit, Drop };

struct Pending
{
    // AHomesteadCharacter::PullWeedsStarts() when she pressed: this pull's clip is the next one to begin.
    unsigned startsBefore = 0;
    double since = 0.0; // When she pressed (seconds).
    bool started = false; // Its own clip has been seen playing.
};

// One tick. startsNow is PullWeedsStarts(); phase is PullWeedsPhase() (-1 when no pull clip plays).
inline Step Advance(Pending& pending, unsigned startsNow, float phase, double now, float firstPull, float commit,
    double startTimeout)
{
    const bool ours = startsNow != pending.startsBefore;
    if (!ours || phase < 0.0f)
    {
        // Not this pull's clip: it hasn't begun (an earlier pull may still be finishing), or it began and
        // stopped before the second root came out.
        if (pending.started || now - pending.since > startTimeout) return Step::Drop;
        return Step::Wait;
    }
    pending.started = true;
    if (phase >= commit) return Step::Commit;
    return phase >= firstPull ? Step::Thin : Step::Wait;
}
}
}
