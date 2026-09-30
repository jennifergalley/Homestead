#pragma once

#include "HomesteadSimulation.h"

// How the heroine gathers each resource by hand (Jenny's playtest, 2026-09-29: the generic
// slight-knee-bend gather looked wrong). Solid things are lifted with the stone kneel, wood is
// broken into sticks and cradled, and anything that grows is picked or pulled into the hip pouch.
// Reeds and deer remains keep their knife work; everything else needs a tool (None). There is no
// generic pose: AHomesteadController::Interact plays exactly this one.
namespace Homestead
{
enum class GatherPose : int { None, Sticks, Stones, Pouch, Reeds, KnifeCut };

constexpr GatherPose HandGatherPose(ResourceKind kind)
{
    switch (kind)
    {
    case ResourceKind::Branches:
    case ResourceKind::FallenBranch:
    case ResourceKind::BrokenCrate:
    case ResourceKind::BrokenBarrel:
    case ResourceKind::RottenPlanks:
        return GatherPose::Sticks;
    case ResourceKind::Stones:
    case ResourceKind::SalvagePile:
    case ResourceKind::RubbishHeap:
    case ResourceKind::SlateHeap:
        return GatherPose::Stones;
    case ResourceKind::Roots:
    case ResourceKind::BerryBush:
    case ResourceKind::Flowers:
    case ResourceKind::Primroses:
    case ResourceKind::Bluebells:
    case ResourceKind::WildDaffodils:
    case ResourceKind::WildGarlic:
    case ResourceKind::Weeds:
    case ResourceKind::Nettles:
        return GatherPose::Pouch;
    case ResourceKind::Reeds:
        return GatherPose::Reeds;
    case ResourceKind::DeerRemains:
        return GatherPose::KnifeCut;
    default:
        return GatherPose::None;
    }
}
}
