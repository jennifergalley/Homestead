#include "HomesteadFishing.h"
#include "HomesteadSimulation.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Homestead
{
namespace Fishing
{
std::uint64_t Mix(std::uint64_t value)
{
    value += 0x9E3779B97F4A7C15ull;
    value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ull;
    value = (value ^ (value >> 27)) * 0x94D049BB133111EBull;
    return value ^ (value >> 31);
}
const char* WaterName(FishingWater water)
{
    switch (water)
    {
    case FishingWater::River: return "River";
    case FishingWater::Lake: return "Lake";
    case FishingWater::Ocean: return "Sea";
    default: return "Water";
    }
}
Item CatchFor(FishingWater water, std::uint64_t seed)
{
    const bool second = (Mix(seed) & 1u) != 0;
    switch (water)
    {
    case FishingWater::River: return second ? Item::RiverSalmon : Item::RiverTrout;
    case FishingWater::Lake: return second ? Item::LakeCarp : Item::LakePerch;
    case FishingWater::Ocean: return second ? Item::SeaBass : Item::SeaMackerel;
    default: return Item::Count;
    }
}
double Marker(const FishingSession& session)
{
    if (session.phase == FishingPhase::Landing)
        return std::clamp((session.elapsed - session.strikeAfter) / StrikeWindowSeconds, 0.0, 1.0);
    if (session.phase == FishingPhase::Bite)
        return std::clamp((session.elapsed - session.biteAfter) / HookWindowSeconds, 0.0, 1.0);
    return 0.0;
}
bool StrikeReady(const FishingSession& session)
{
    return session.phase == FishingPhase::Landing && session.elapsed >= session.strikeAfter
        && session.elapsed <= session.strikeAfter + StrikeWindowSeconds;
}
double StrikeAfter(const FishingSession& session)
{
    const auto seed = Mix(session.timingSeed ^ static_cast<std::uint64_t>(session.landedBeats + 1));
    return MinStrikeSeconds + StrikeVariationSeconds * static_cast<double>(seed >> 11) / 9007199254740992.0;
}
}

FishingWater Simulation::FishingWaterAt(Point player) const
{
    if (!std::isfinite(player.x) || !std::isfinite(player.y)
        || std::abs(player.x) > MaxWorldCoordinate || std::abs(player.y) > MaxWorldCoordinate)
        return FishingWater::None;
    const auto water = fishingWaterProbe_ ? fishingWaterProbe_(player)
        : !state_.fixedEstate && NearWater(player) ? FishingWater::River : FishingWater::None;
    return water > FishingWater::None && water < FishingWater::Count ? water : FishingWater::None;
}

Result Simulation::CheckFishing(Point player) const
{
    if (state_.failed) return {false, "You need to recover first.", ResultCode::Unavailable, revision_};
    if (fishing_.phase != FishingPhase::Idle)
        return {false, "Finish or cancel your current cast first.", ResultCode::Unavailable, revision_};
    if (Count(Item::FishingPole) == 0)
        return {false, "Take a fishing pole in your pack; the General Store sells one for 1500 coins.", ResultCode::Unavailable, revision_};
    if (FishingWaterAt(player) == FishingWater::None)
        return {false, "Stand beside a river, lake or the sea to cast.", ResultCode::Unavailable, revision_};
    if (UsedCapacity() >= PackCapacity())
        return {false, "Make room in your pack for a fish first.", ResultCode::Capacity, revision_};
    return CheckExertion(Fishing::CastEnergy);
}

Result Simulation::BeginFishing(Point player)
{
    const auto ready = CheckFishing(player);
    if (!ready) return ready;
    if (nextFishingToken_ == std::numeric_limits<std::uint64_t>::max())
        return {false, "Fishing cast identities are exhausted.", ResultCode::Unavailable, revision_};
    const auto water = FishingWaterAt(player);
    if (water == FishingWater::None)
        return {false, "There is no fishable water within reach.", ResultCode::Unavailable, revision_};
    const auto seed = Fishing::Mix(state_.world.seed ^ Fishing::Mix(nextFishingToken_) ^ Fishing::Mix(revision_)
        ^ static_cast<std::uint64_t>(std::llround(state_.hour * 3600.0))
        ^ static_cast<std::uint64_t>(std::llround(player.x)) * 0xD1B54A32D192ED03ull
        ^ static_cast<std::uint64_t>(std::llround(player.y)) * 0x94D049BB133111EBull);
    fishing_ = {};
    fishing_.token = nextFishingToken_++;
    fishing_.timingSeed = seed;
    fishing_.water = water;
    fishing_.catchItem = Fishing::CatchFor(fishing_.water, seed);
    fishing_.originX = player.x;
    fishing_.originY = player.y;
    fishing_.biteAfter = Fishing::MinBiteSeconds + Fishing::BiteVariationSeconds
        * static_cast<double>(seed >> 11) / 9007199254740992.0;
    fishing_.phase = FishingPhase::Casting;
    return Exert(Fishing::CastEnergy, {true, "", ResultCode::None, ++revision_});
}

Result Simulation::CancelFishing()
{
    if (fishing_.phase == FishingPhase::Idle)
        return {false, "There is no fishing line to reel in.", ResultCode::Unavailable, revision_};
    fishing_ = {};
    return {true, "Line reeled in. No fish caught.", ResultCode::None, revision_};
}

Result Simulation::FishingAnimationInterrupted(std::uint64_t token)
{
    if (token == 0 || token != fishing_.token || fishing_.phase == FishingPhase::Idle)
        return {false, "That fishing cast is no longer active.", ResultCode::Unavailable, revision_};
    return CancelFishing();
}

Result Simulation::AdvanceFishing(double seconds, Point player)
{
    if (!std::isfinite(seconds) || seconds < 0.0 || seconds > 60.0
        || !std::isfinite(player.x) || !std::isfinite(player.y)
        || std::abs(player.x) > MaxWorldCoordinate || std::abs(player.y) > MaxWorldCoordinate)
        return {false, "Fishing needs a valid elapsed time and position.", ResultCode::Invalid, revision_};
    if (fishing_.phase == FishingPhase::Idle)
        return {false, "Cast your line first.", ResultCode::Unavailable, revision_};
    if (state_.failed || Count(Item::FishingPole) == 0 || FishingWaterAt(player) != fishing_.water
        || std::hypot(player.x - fishing_.originX, player.y - fishing_.originY) > Fishing::WalkAwayCm)
        return CancelFishing();
    fishing_.elapsed += seconds;
    const double deadline = fishing_.phase == FishingPhase::Casting ? Fishing::CastContactTimeoutSeconds
        : fishing_.phase == FishingPhase::Catching ? Fishing::CatchContactTimeoutSeconds
        : fishing_.phase == FishingPhase::Landing ? fishing_.strikeAfter + Fishing::StrikeWindowSeconds
        : fishing_.biteAfter + Fishing::HookWindowSeconds;
    if (fishing_.elapsed > deadline)
    {
        fishing_ = {};
        return {true, "No catch this time. Cast again when you're ready.", ResultCode::None, revision_};
    }
    if (fishing_.phase == FishingPhase::Waiting && fishing_.elapsed >= fishing_.biteAfter)
        fishing_.phase = FishingPhase::Bite;
    return {true, "", ResultCode::None, revision_};
}

Result Simulation::FishingPress(Point player)
{
    if (fishing_.phase == FishingPhase::Idle) return BeginFishing(player);
    const auto advanced = AdvanceFishing(0.0, player);
    if (!advanced || fishing_.phase == FishingPhase::Idle) return advanced;
    if (fishing_.phase == FishingPhase::Bite)
    {
        fishing_.phase = FishingPhase::Landing;
        fishing_.elapsed = 0.0;
        fishing_.strikeAfter = Fishing::StrikeAfter(fishing_);
        return {true, "", ResultCode::None, revision_};
    }
    if (Fishing::StrikeReady(fishing_))
    {
        if (fishing_.landedBeats + 1 < Fishing::LandingBeats)
        {
            ++fishing_.landedBeats;
            fishing_.elapsed = 0.0;
            fishing_.strikeAfter = Fishing::StrikeAfter(fishing_);
            return {true, "", ResultCode::None, revision_};
        }
        ++fishing_.landedBeats;
        fishing_.phase = FishingPhase::Catching;
        fishing_.elapsed = 0.0;
        return {true, "", ResultCode::None, revision_};
    }
    if (fishing_.phase == FishingPhase::Casting || fishing_.phase == FishingPhase::Catching)
        return {false, "Let the fishing animation finish before pressing again.", ResultCode::Unavailable, revision_};
    fishing_ = {};
    return {true, "The fish escaped the hook. Cast again and follow the timing cue.", ResultCode::None, revision_};
}

Result Simulation::FishingAnimationContact(FishingContact contact, std::uint64_t token, Point player)
{
    const bool splash = contact == FishingContact::CastSplash;
    if ((contact != FishingContact::CastSplash && contact != FishingContact::CatchLift) || token == 0
        || token != fishing_.token || fishing_.phase != (splash ? FishingPhase::Casting : FishingPhase::Catching))
        return {false, "That fishing animation contact no longer belongs to this cast.", ResultCode::Invalid, revision_};
    const double beat = splash ? Fishing::CastSplashSeconds : Fishing::CatchLiftSeconds;
    const double deadline = splash ? Fishing::CastContactTimeoutSeconds : Fishing::CatchContactTimeoutSeconds;
    if (fishing_.elapsed < beat || fishing_.elapsed > deadline)
        return {false, "That fishing animation contact is outside its authored beat.", ResultCode::Invalid, revision_};
    const auto advanced = AdvanceFishing(0.0, player);
    if (!advanced || fishing_.phase == FishingPhase::Idle) return advanced;
    if (splash)
    {
        fishing_.phase = FishingPhase::Waiting;
        fishing_.elapsed = 0.0;
        return {true, "", ResultCode::None, revision_};
    }
    const Item caught = fishing_.catchItem;
    Inventory yield{};
    yield[static_cast<int>(caught)] = 1;
    if (!TryAdjust(yield))
    {
        fishing_ = {};
        return {false, "The catch could not fit in your pack; the fish was returned to the water.", ResultCode::Capacity, revision_};
    }
    fishing_ = {};
    return {true, "Caught " + CountedName(caught, 1) + ".", ResultCode::None, revision_};
}
}
