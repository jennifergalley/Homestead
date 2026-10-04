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
constexpr double MinBiteSeconds = 2.0, BiteVariationSeconds = 2.0, HookWindowSeconds = 0.9;
constexpr double MinStrikeSeconds = 0.65, StrikeVariationSeconds = 0.6, StrikeWindowSeconds = 0.7;
constexpr int LandingBeats = 2;
const char* WaterName(FishingWater water);
Item CatchFor(FishingWater water, std::uint64_t seed);
double Marker(const FishingSession& session);
bool StrikeReady(const FishingSession& session);
}
}
