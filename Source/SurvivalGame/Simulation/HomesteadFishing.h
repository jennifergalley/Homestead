#pragma once

#include "HomesteadItems.h"
#include <cstdint>

namespace Homestead
{
enum class FishingWater : int { None, River, Lake, Ocean, Count };
enum class FishingPhase : int { Idle, Waiting, Bite, Landing };

struct FishingSession
{
    FishingPhase phase = FishingPhase::Idle;
    FishingWater water = FishingWater::None;
    Item catchItem = Item::Count;
    double originX = 0.0, originY = 0.0;
    double elapsed = 0.0, biteAfter = 0.0;
    int landedBeats = 0;
};

namespace Fishing
{
constexpr std::int64_t PolePriceCoins = 1500;
constexpr double CastEnergy = 1.5;
constexpr double BankReachCm = 240.0;
constexpr double WalkAwayCm = 100.0;
// Real seconds: a short wait and forgiving half-second landing bands for the first fishing slice.
constexpr double MinBiteSeconds = 2.0, BiteVariationSeconds = 2.0, HookWindowSeconds = 0.9;
constexpr double LandingSeconds = 1.8, LandingBandStart = 0.55, LandingBandEnd = 0.85;
constexpr int LandingBeats = 2;
const char* WaterName(FishingWater water);
Item CatchFor(FishingWater water, std::uint64_t seed);
double Marker(const FishingSession& session);
}
}
