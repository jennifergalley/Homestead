#pragma once

#include "HomesteadRegionalGeneration.h"
#include "HomesteadWorldGeneration.h"

namespace Homestead::Generation
{
struct RegionalTerrainInfluence
{
    double heightOffsetCm = 0.0;
    std::uint16_t mountain = 0;
    std::uint16_t ridge = 0;
    std::uint16_t valley = 0;
    std::uint16_t wetness = 0;
};

RegionalGeneration::Status SampleRegionalTerrainInfluence(
    WorldDescriptor world, std::int64_t xCm, std::int64_t yCm,
    RegionalTerrainInfluence& output);
}
