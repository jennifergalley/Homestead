// Prints the estate placement table as built before the compact map's retire pass (one "id x y" line per
// placement, centimetres), for Scripts/Terrain/compact_placements.py: it sees every row, including the ones
// HomesteadEstate.cpp places in code rather than from generated tables.
//   cmake --build Build\Native --config Release --target HomesteadPlacementDump
//   Build\Native\Release\HomesteadPlacementDump.exe
#include "HomesteadEstate.h"

#include <cstdio>

int main()
{
    for (const Homestead::EstatePlacement& p : Homestead::EstatePlacementsBeforeCompactMap().placements)
        std::printf("%d %.1f %.1f\n", p.id, p.position.x, p.position.y);
    return 0;
}
