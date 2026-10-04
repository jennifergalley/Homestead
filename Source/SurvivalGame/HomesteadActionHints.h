#pragma once

#include <string_view>

namespace HomesteadActionHints
{
inline constexpr const char* PlantSeeds = "Plant Seeds";

inline bool ShouldRetire(std::string_view verb, int uses, int retireUses)
{
    return verb != PlantSeeds && uses >= retireUses;
}
}
