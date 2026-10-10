#pragma once

#include "HomesteadItems.h"
#include <cstdint>

namespace Homestead
{
enum class FishingWater : int { None, River, Lake, Ocean, Count };
enum class FishingPhase : int { Idle, Waiting, Bite, Landing, Casting, Catching };
enum class FishingContact : int { CastSplash, CatchLift };

struct FishingSession
{
    FishingPhase phase = FishingPhase::Idle;
    FishingWater water = FishingWater::None;
    Item catchItem = Item::Count;
    double originX = 0.0, originY = 0.0;
    double elapsed = 0.0, biteAfter = 0.0;
    double strikeAfter = 0.0;
    std::uint64_t token = 0, timingSeed = 0;
    int landedBeats = 0;
    // Strikes this fish needs, and the strike at which it throws the hook (-1: it stays on).
    int beats = 0, escapeBeat = -1;
};

namespace Fishing
{
constexpr std::int64_t PolePriceCoins = 1500;
constexpr double CastEnergy = 1.5;
constexpr double BankReachCm = 240.0;
constexpr double WalkAwayCm = 100.0;
// Real seconds; contact beats match Art's HomesteadFishingPresentation.h contract.
constexpr double CastSplashSeconds = 1.1, CatchLiftSeconds = 0.4;
constexpr double CastContactTimeoutSeconds = 2.5;
// The authored 0.8-second strike/lift also needs room for hitches and game-thread counter latency.
constexpr double CatchContactTimeoutSeconds = 2.0;
// Jenny's harder fishing (harder-bob-fishing; numbers per balance.md section 8): about 60% of casts
// fail for an attentive player, half by a hooked fish throwing the hook whatever the timing and the
// rest by mistiming. Every wait is random so the rhythm can't be learned. Seconds of real time.
constexpr double EscapeChance = 0.5;
constexpr double MinBiteSeconds = 4.0, BiteVariationSeconds = 8.0, HookWindowSeconds = 0.8;
constexpr double MinStrikeSeconds = 1.0, StrikeVariationSeconds = 2.5, StrikeWindowSeconds = 0.7;
constexpr int MinLandingBeats = 2, MaxLandingBeats = 4;
// False nibbles while waiting for the bite, and false tugs while waiting for each strike: the float
// shivers for NibbleSeconds but stays up. None start within NibbleQuietSeconds of a segment's ends.
constexpr int MaxBiteNibbles = 3, MaxStrikeTugs = 1;
constexpr double NibbleSeconds = 0.3, NibbleQuietSeconds = 0.8;
const char* WaterName(FishingWater water);
Item CatchFor(FishingWater water, std::uint64_t seed);
double Marker(const FishingSession& session);
bool StrikeReady(const FishingSession& session);
// The float is pulled under: the only time a click helps.
bool FloatUnder(const FishingSession& session);
// How hard a false nibble is shaking the float right now (0 when none, peaking at 1).
double Nibble(const FishingSession& session);
}
}
