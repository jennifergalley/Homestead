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
double Unit(std::uint64_t value)
{
    return static_cast<double>(Mix(value) >> 11) / 9007199254740992.0;
}
// Salts keep each seeded choice of a cast independent of the others.
constexpr std::uint64_t BeatsSalt = 0xB3A75ull, EscapeSalt = 0xE5CA9Eull, EscapeBeatSalt = 0xE5CB3ull;
constexpr std::uint64_t NibbleCountSalt = 0x41BB1Eull, NibbleTimeSalt = 0x41BB2Eull;
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
bool FloatUnder(const FishingSession& session)
{
    return session.phase == FishingPhase::Bite || StrikeReady(session);
}
double Nibble(const FishingSession& session)
{
    const bool waiting = session.phase == FishingPhase::Waiting;
    if (!waiting && session.phase != FishingPhase::Landing) return 0.0;
    const double segment = waiting ? session.biteAfter : session.strikeAfter;
    const double span = segment - 2.0 * NibbleQuietSeconds - NibbleSeconds;
    if (span <= 0.0 || session.elapsed >= segment) return 0.0;
    const std::uint64_t key = session.timingSeed ^ (waiting ? 0ull
        : 0x100ull * static_cast<std::uint64_t>(session.landedBeats + 1));
    const int count = static_cast<int>(Mix(key ^ NibbleCountSalt) % static_cast<std::uint64_t>(
        (waiting ? MaxBiteNibbles : MaxStrikeTugs) + 1));
    double strongest = 0.0;
    for (int index = 0; index < count; ++index)
    {
        const double start = NibbleQuietSeconds + span * Unit(key ^ NibbleTimeSalt ^ static_cast<std::uint64_t>(index + 1));
        const double into = session.elapsed - start;
        if (into >= 0.0 && into < NibbleSeconds)
            strongest = std::max(strongest, std::sin(3.14159265358979 * into / NibbleSeconds));
    }
    return strongest;
}
double StrikeAfter(const FishingSession& session)
{
    const auto seed = Mix(session.timingSeed ^ static_cast<std::uint64_t>(session.landedBeats + 1));
    return MinStrikeSeconds + StrikeVariationSeconds * static_cast<double>(seed >> 11) / 9007199254740992.0;
}
}

void Simulation::SetFishingEscapeChance(double chance)
{
    fishingEscapeChance_ = std::isfinite(chance) ? std::clamp(chance, 0.0, 1.0) : Fishing::EscapeChance;
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
        return {false, "Needs a fishing pole.", ResultCode::Unavailable, revision_};
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
    fishing_.beats = Fishing::MinLandingBeats + static_cast<int>(Fishing::Mix(seed ^ Fishing::BeatsSalt)
        % static_cast<std::uint64_t>(Fishing::MaxLandingBeats - Fishing::MinLandingBeats + 1));
    if (Fishing::Unit(seed ^ Fishing::EscapeSalt) < fishingEscapeChance_)
        // Only at the first or second strike, never the last, so a long fight isn't wasted.
        fishing_.escapeBeat = static_cast<int>(Fishing::Mix(seed ^ Fishing::EscapeBeatSalt)
            % static_cast<std::uint64_t>(std::min(2, fishing_.beats - 1)));
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
    if (fishing_.phase == FishingPhase::Landing && fishing_.landedBeats == fishing_.escapeBeat
        && fishing_.elapsed >= fishing_.strikeAfter)
    {
        fishing_ = {};
        return {true, "It got away.", ResultCode::None, revision_};
    }
    const bool reacting = fishing_.phase == FishingPhase::Bite || fishing_.phase == FishingPhase::Landing
        || fishing_.phase == FishingPhase::Waiting;
    const double deadline = fishing_.phase == FishingPhase::Casting ? Fishing::CastContactTimeoutSeconds
        : fishing_.phase == FishingPhase::Catching ? Fishing::CatchContactTimeoutSeconds
        : fishing_.phase == FishingPhase::Landing ? fishing_.strikeAfter + Fishing::StrikeWindowSeconds
        : fishing_.biteAfter + Fishing::HookWindowSeconds;
    if (fishing_.elapsed > deadline)
    {
        fishing_ = {};
        return {true, reacting ? "The fish slipped the hook." : "No catch this time. Cast again when you're ready.",
            ResultCode::None, revision_};
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
        if (fishing_.landedBeats + 1 < fishing_.beats)
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
    return {true, "Too soon \xE2\x80\x93 it shied away.", ResultCode::None, revision_};
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
    // The pack's "+1 <fish>" pickup notice says it (Jenny, 2026-10-09: one catch message, not two).
    return {true, "", ResultCode::None, revision_};
}
}
