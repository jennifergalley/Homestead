#pragma once

#include <string_view>

namespace HomesteadActionHints
{
inline constexpr const char* PlantSeeds = "Plant Seeds";
inline constexpr const char* Harvest = "Harvest";

inline bool ShouldRetire(std::string_view verb, int uses, int retireUses)
{
    const bool harvest = verb == Harvest || (verb.size() > 8 && verb.substr(0, 8) == "Harvest ");
    return verb != PlantSeeds && !harvest && uses >= retireUses;
}
}
