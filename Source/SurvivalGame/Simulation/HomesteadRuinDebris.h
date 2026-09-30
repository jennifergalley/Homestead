#pragma once

#include "HomesteadSimulation.h"

// The ruin's loose debris she can clear (Jenny's playtest: slate and rubble heaps in the manor that
// looked clearable but weren't). These heaps left AHomesteadManorRuin's static plan and became
// estate placements (ids 582000-582099, registry: docs/handoff/round-2.md), drawn with the same
// meshes at the same spots. The fallen masonry under the front door and the collapsed south-west
// corner and the big fallen blocks stay part of the ruin; the fallen roof timbers joined the list later.
namespace Homestead
{
namespace RuinDebris
{
struct Spot
{
    int id;
    ResourceKind kind;
    // HomesteadManorRuin's frame (cm): u east from the footprint's west end, v north from its south front.
    double u;
    double v;
    double yaw;   // Degrees, as the ruin placed it.
    double scale;
    const char* mesh; // /Game/SurvivalGame/Environment/Props/<mesh>/SM_<mesh>
};

constexpr int FirstId = 582000;
constexpr int LastId = 582099;

// Slate first (hand-cleared), then the loose granite (worn pickaxe), then the roof timbers (worn axe).
// Append only; ids never reused.
constexpr Spot Spots[] = {
    {582000, ResourceKind::SlateHeap, 1500, 195, 0, 1.0, "RuinSlateScatter"},
    {582001, ResourceKind::SlateHeap, 2150, 1600, 180, 1.0, "RuinSlateScatter"},
    {582002, ResourceKind::SlateHeap, 200, 1200, 90, 1.0, "RuinSlateScatter"},
    {582003, ResourceKind::SlateHeap, 700, -145, 180, 1.0, "RuinSlateScatter"},
    {582004, ResourceKind::SlateHeap, -150, 1150, 270, 1.0, "RuinSlateScatter"},
    {582005, ResourceKind::SlateHeap, 2350, 1945, 0, 1.0, "RuinSlateScatter"},
    {582006, ResourceKind::Rubble, 1300, 900, 45, 1.3, "GraniteCobbles"},
    {582007, ResourceKind::Rubble, 400, 600, 300, 1.1, "GraniteCobbles"},
    {582008, ResourceKind::Rubble, 2150, 1100, 250, 1.2, "GraniteSpalls"},
    {582009, ResourceKind::Rubble, 2700, 900, 10, 1.0, "GraniteSpalls"},
    {582010, ResourceKind::Rubble, 700, 1000, 160, 1.0, "GraniteRubble"},
    {582011, ResourceKind::Rubble, 2500, 1650, 310, 0.9, "GraniteRubble"},
    // Fallen roof timbers in the hall and the west rooms (worn axe), placed after the forage sections.
    {582012, ResourceKind::RuinTimbers, 2450, 1300, 35, 1.0, "RuinFallenTimbers"},
    {582013, ResourceKind::RuinTimbers, 900, 1350, 110, 1.0, "RuinFallenTimbers"},
};
constexpr int SpotCount = static_cast<int>(sizeof(Spots) / sizeof(Spots[0]));

// The spot a placement id names, or null.
inline const Spot* Find(int id)
{
    for (const Spot& spot : Spots)
        if (spot.id == id) return &spot;
    return nullptr;
}
// Whether the ruin's static plan entry (mesh, u, v) is one of these, so the ruin actor leaves it out.
inline bool Replaces(const char* mesh, double u, double v)
{
    for (const Spot& spot : Spots)
    {
        const char* a = mesh;
        const char* b = spot.mesh;
        while (*a && *a == *b) { ++a; ++b; }
        if (*a == *b && spot.u == u && spot.v == v) return true;
    }
    return false;
}
}
}
