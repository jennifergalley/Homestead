#pragma once

// When a pending tool swing lands (AHomesteadController::UpdatePendingSwing, the billhook's hack), kept
// free of Unreal types so the rule is pinned by native tests. A blow counts only at the contact of the
// clip this press started: an earlier hack still finishing its follow-through, already past its contact,
// must never land on the target just chosen (code review 0930, MEDIUM).
namespace Homestead
{
namespace SwingTiming
{
enum class Step : int { Wait, Land, Drop };

// startsBefore: the clip's start counter when she pressed; startsNow, playing and phase: the clip now.
// approaching: she is still walking up to the stance, so the clip hasn't been asked for yet. age: seconds
// since the press (or since the approach ended); past startGrace with no clip of hers playing, the swing
// was interrupted and is dropped.
inline Step Advance(unsigned startsBefore, unsigned startsNow, bool playing, float phase, float contact,
    bool approaching, double age, double startGrace)
{
    const bool own = playing && startsNow != startsBefore;
    if (own && phase >= contact) return Step::Land;
    if (approaching || playing) return Step::Wait;
    return age > startGrace ? Step::Drop : Step::Wait;
}
}
}
