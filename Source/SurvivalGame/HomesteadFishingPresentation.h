#pragma once

#include <cstdint>

// What the heroine's fishing presentation shows. The controller maps the simulation's fishing phase
// to one of these on each phase or cast-token change (UHomesteadAnimInstance::SetFishingPose; every
// Cast call restarts the swing); the art never decides a catch.
enum class EHomesteadFishingPose : std::uint8_t
{
    None,
    Cast,  // swing the line out; FishCastSplashes() ticks when the float lands
    Wait,  // rod held over the water
    Bite,  // the float bobs; the rod tip nods
    Fight, // rod raised and pulling against a hooked fish
    Catch, // lift the fish clear and take it in hand; FishCatchLifts() ticks when it leaves the water
    Miss,  // slack line, rod lowered
};

// What the float on the water shows this frame (harder-bob-fishing), copied from the simulation by
// the controller: a false nibble's shiver, and how far through the click window it is while the
// fish has pulled it under (Homestead::Fishing::FloatUnder / Marker).
struct FHomesteadFishingCue
{
    float Nibble = 0.0f;  // 0 still, 1 at a nibble's peak
    float Window = -1.0f; // 0..1 through the click window while the float is under; -1 when it's up
    float WaitSeconds = -1.0f; // seconds since the float landed, while waiting for a bite; -1 otherwise
    float BiteSeconds = 0.0f;  // when the bite comes, on the same clock (for pacing the fish's approach)
    bool bHooked = false;      // the fish has taken the hook (bite or fight)
    bool Under() const { return Window >= 0.0f; }
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
// One frame of the 30 fps clip (fish_cast.py FPS). Where two segments meet, the earlier one's closing
// key sits a frame early: loops wrap and one-shots hold at End - ClipFrameSeconds.
constexpr float ClipFrameSeconds = 1.0f / 30.0f;
}
