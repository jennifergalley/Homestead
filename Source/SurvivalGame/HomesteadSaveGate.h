#pragma once

// A save written before a new game has settled is restored silently on the next launch: the pawn is still
// at the world origin (or her old spot) and the names are the defaults, so she skips the name form and
// wakes far from the manor. Saves wait until she has been placed and the setup steps are done.
enum class EHomesteadSaveHold
{
    None,
    AwaitingSpawn,
    AwaitingNewGameSetup,
};

struct FHomesteadSaveReadiness
{
    bool bSpawnPending = false;
    bool bNewGameSetup = false;
};

constexpr EHomesteadSaveHold HomesteadSaveHold(FHomesteadSaveReadiness Now)
{
    if (Now.bSpawnPending) return EHomesteadSaveHold::AwaitingSpawn;
    if (Now.bNewGameSetup) return EHomesteadSaveHold::AwaitingNewGameSetup;
    return EHomesteadSaveHold::None;
}
