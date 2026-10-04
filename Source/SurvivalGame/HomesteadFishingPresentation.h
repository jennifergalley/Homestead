#pragma once

#include "CoreMinimal.h"

// What the heroine's fishing presentation shows. The controller maps the simulation's fishing phase
// to one of these every tick (UHomesteadAnimInstance::SetFishingPose); the art never decides a catch.
enum class EHomesteadFishingPose : uint8
{
    None,
    Cast,  // swing the line out; FishCastSplashes() ticks when the float lands
    Wait,  // rod held over the water
    Bite,  // the float bobs; the rod tip nods
    Fight, // rod raised and pulling against a hooked fish
    Catch, // lift the fish clear and take it in hand; FishCatchLifts() ticks when it leaves the water
    Miss,  // slack line, rod lowered
};

// Segments of AN_HeroineMH_Fishing, authored by Content/Python/homestead_agent/fish_cast.py (SEGMENTS
// and EVENTS there must match). Seconds on the clip's own timeline.
namespace HomesteadFishingTiming
{
// The cast plays once, then settles into the wait hold.
constexpr float CastStart = 0.0f;
constexpr float CastEnd = 1.6f;
// The rod tip passes forward and the line leaves it.
constexpr float CastReleaseSeconds = 0.62f;
// The float touches the water (from the cast start): the contact beat that commits the cast.
constexpr float CastSplashSeconds = 1.1f;
// Loops: each begins and ends on the same pose.
constexpr float WaitStart = 1.6f;
constexpr float WaitEnd = 3.6f;
constexpr float BiteStart = 3.6f;
constexpr float BiteEnd = 4.4f;
constexpr float FightStart = 4.4f;
constexpr float FightEnd = 5.6f;
// A quick strike (hook set) that starts from the hold or the fight and ends on the fight pose.
constexpr float StrikeStart = 5.6f;
constexpr float StrikeEnd = 6.0f;
// Lift the fish out and take it in the left hand; holds its last pose.
constexpr float CatchStart = 6.0f;
constexpr float CatchEnd = 7.4f;
// The fish clears the water (from the catch start): the contact beat that grants the reward.
constexpr float CatchLiftSeconds = 0.4f;
constexpr float MissStart = 7.4f;
constexpr float MissEnd = 8.0f;
}
