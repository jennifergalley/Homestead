#pragma once

#include <string_view>

namespace HomesteadActionHints
{
inline constexpr const char* PlantSeeds = "Plant Seeds";
inline constexpr const char* Harvest = "Harvest";
// Gathered every day, so these never retire: the prompt must always show at a gatherable rock.
inline constexpr const char* GatherMussels = "Gather mussels";
inline constexpr const char* GatherLaver = "Gather laver";

inline bool ShouldRetire(std::string_view verb, int uses, int retireUses)
{
    const bool harvest = verb == Harvest || (verb.size() > 8 && verb.substr(0, 8) == "Harvest ");
    const bool daily = verb == GatherMussels || verb == GatherLaver;
    return verb != PlantSeeds && !harvest && !daily && uses >= retireUses;
}
}
