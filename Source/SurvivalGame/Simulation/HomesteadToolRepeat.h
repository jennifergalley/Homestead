#pragma once

#include "HomesteadSimulation.h"

// Hold-to-repeat tool strikes (Jenny 09-30 22:26): holding the tool button (left mouse or the
// gamepad's right trigger, never E or interact) keeps landing blows on the target she aimed at until
// it clears, then stops. Releasing finishes the blow already under way. A click is exactly one blow.
// Kept free of Unreal types so AHomesteadController's decision at each landed blow is pinned by
// native tests.
namespace Homestead
{
namespace ToolRepeat
{
// The overgrowth tools whose blows repeat. The hoe turns one square per press, and the watering can,
// lamp, seeds and food never repeat.
inline bool Repeats(Item tool)
{
    return tool == Item::Hatchet || tool == Item::Billhook || tool == Item::Pickaxe || tool == Item::Scythe;
}

// What the controller sees at the moment a blow lands.
struct Held
{
    bool held = false;      // the left mouse button or right trigger is still down
    bool sameTool = false;  // that tool is still selected on the hotbar and in her pack
    bool menuOpen = false;  // the field book, a shop or a dialog is open
    int aimed = -1;         // the overgrowth she's aimed at now (-1 for none)
};

struct Decision
{
    bool more = false;
    // Set when the simulation refused another blow ("Too tired." below 10% energy, out of reach): the
    // controller shows it once. ok means she simply stopped (released, cleared, looked away, menu).
    Result refusal{true, "", ResultCode::None, 0};
};

inline bool Standing(const Simulation& sim, int target)
{
    for (const auto& node : sim.GetState().resources)
        if (node.id == target) return !node.cleared;
    return false;
}

// Decide, as a blow lands on target (after the simulation counted it), whether another follows.
// It never moves on to anything else: once target clears, the loop ends even while held.
inline Decision AfterBlow(const Simulation& sim, int target, Item tool, Point player, const Held& now)
{
    Decision decision;
    if (!now.held || !Repeats(tool) || target < 0 || !Standing(sim, target)) return decision;
    if (now.menuOpen || !now.sameTool || now.aimed != target) return decision;
    const Result ready = sim.CheckOvergrowth(target, tool, player);
    if (!ready)
    {
        decision.refusal = ready;
        return decision;
    }
    decision.more = true;
    return decision;
}

// The strike clips (axe_fell.FRAMES, shared by the axe, the pickaxe's ground strike and the scythe's
// mow) repeat one stroke cycle, loopStart to loopStart + loop, once per extra stroke. A held blow joins
// the running clip only while it's still inside its last cycle, before the recovery (10 frames after
// that cycle's contact); later, the clip time would jump back into the loop and the pose would pop.
// margin covers the frame between the controller's check and the next animation update.
inline float JoinDeadline(int strokes, float loopStart, float loop, float margin)
{
    return loopStart + static_cast<float>(strokes - 1) * loop - margin;
}

inline bool CanAddStroke(float playTime, int strokes, float loopStart, float loop, float margin)
{
    return strokes >= 1 && playTime < JoinDeadline(strokes, loopStart, loop, margin);
}
}
}
