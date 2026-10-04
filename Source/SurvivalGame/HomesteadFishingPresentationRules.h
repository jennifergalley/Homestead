#pragma once

#include "Simulation/HomesteadFishing.h"

namespace HomesteadFishingPresentationRules
{
inline bool NewCast(const Homestead::FishingSession& session, std::uint64_t presentedToken)
{
    return session.phase == Homestead::FishingPhase::Casting && session.token != presentedToken;
}

inline bool FinishedMiss(bool playingMiss, bool hasClip, double clipTime, double authoredEnd, double endMargin)
{
    return playingMiss && (!hasClip || clipTime >= authoredEnd - endMargin);
}
}
