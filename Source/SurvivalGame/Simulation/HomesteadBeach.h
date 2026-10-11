#pragma once

#include "HomesteadSimulation.h"

// The beach (add-beach-gathering; Jenny's card jenny-mv36t3cy-p7dm5i, Balance 2026-10-10). Shells,
// driftwood, pebbles, mussels and laver lie along the beach and come back every morning at the 06:00
// day rollover, however late in the day she picked them. The beach campfire is a fixed fire pit she
// lights with driftwood: it burns for CampfireBurnHours, lights the sand round it and cooks like a
// cookfire, then goes out until she lights it again. Its ResourceNode::readyAtHour is the hour it burns
// out, so the lit state is saved with the ordinary resource edits (no save-format change).
namespace Homestead
{
namespace Beach
{
// Driftwood it takes to light the campfire, and how long one lighting burns (4 game hours, about ten
// real minutes).
constexpr int CampfireDriftwood = 3;
constexpr double CampfireBurnHours = 4.0;
// The beach campfire lights the sand like the oil lamp: the same reach (HomesteadLampLight.h, 0.2 lux
// at about 13 m), warmer and flickering.
constexpr double CampfireLightRadiusCm = 2000.0;

// The beach finds that come back each morning.
constexpr bool IsDailyForage(ResourceKind kind)
{
    return kind == ResourceKind::Seashells || kind == ResourceKind::Driftwood || kind == ResourceKind::BeachPebbles
        || kind == ResourceKind::MusselBed || kind == ResourceKind::LaverRocks;
}
// Whether `kind` is anything on the beach this change adds.
constexpr bool IsBeachKind(ResourceKind kind)
{
    return IsDailyForage(kind) || kind == ResourceKind::BeachCampfire;
}
// The running game hour of the next 06:00 day rollover strictly after `hour`.
double NextMorning(double hour);
// What one gathering of a daily beach find gives; empty for anything else.
Inventory DailyYield(ResourceKind kind);
// Whether this campfire is burning at `hour`.
inline bool CampfireLit(const ResourceNode& node, double hour)
{
    return node.kind == ResourceKind::BeachCampfire && !node.cleared && node.readyAtHour > hour;
}
}
}
