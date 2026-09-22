#include "HomesteadRegionalTerrainAdapter.h"

#include <algorithm>

namespace Homestead::Generation
{
namespace
{
constexpr std::int64_t RegionalFixedOne = 65536;
constexpr double RegionalHeightDivisor = 50.0;
constexpr double MaximumRegionalOffsetCm = 800.0;

std::int64_t RegionalFloorDivide(std::int64_t value, std::int64_t divisor)
{
    return value / divisor - (value % divisor < 0 ? 1 : 0);
}

std::int64_t RegionalSmoothFixed(std::int64_t value)
{
    return value * value * (3 * RegionalFixedOne - 2 * value)
        / (RegionalFixedOne * RegionalFixedOne);
}

std::int64_t RegionalLerpFixed(std::int64_t a, std::int64_t b, std::int64_t weight)
{
    return a + (b - a) * weight / RegionalFixedOne;
}

struct InterpolatedRelief
{
    std::int64_t elevationMm = 0;
    std::int64_t mountain = 0;
    std::int64_t ridge = 0;
    std::int64_t valley = 0;
    std::int64_t wetness = 0;
};

RegionalGeneration::Status Interpolate(
    RegionalGeneration::RegionalDescriptor descriptor,
    std::int64_t xCm, std::int64_t yCm, InterpolatedRelief& output)
{
    using namespace RegionalGeneration;
    const auto nodeX = RegionalFloorDivide(xCm, DrainageSpacingCm);
    const auto nodeY = RegionalFloorDivide(yCm, DrainageSpacingCm);
    const auto tx = RegionalSmoothFixed(
        (xCm - nodeX * DrainageSpacingCm) * RegionalFixedOne / DrainageSpacingCm);
    const auto ty = RegionalSmoothFixed(
        (yCm - nodeY * DrainageSpacingCm) * RegionalFixedOne / DrainageSpacingCm);
    ReliefSample samples[2][2];
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x)
        {
            const auto status = SampleRelief(descriptor, {nodeX + x, nodeY + y}, samples[y][x]);
            if (status != RegionalGeneration::Status::Ok) return status;
        }
    const auto blend = [&](auto member) {
        const auto low = RegionalLerpFixed(samples[0][0].*member, samples[0][1].*member, tx);
        const auto high = RegionalLerpFixed(samples[1][0].*member, samples[1][1].*member, tx);
        return RegionalLerpFixed(low, high, ty);
    };
    output.elevationMm = blend(&ReliefSample::elevationMm);
    output.mountain = blend(&ReliefSample::mountain);
    output.ridge = blend(&ReliefSample::ridge);
    output.valley = blend(&ReliefSample::valley);
    output.wetness = blend(&ReliefSample::wetness);
    return RegionalGeneration::Status::Ok;
}
}

RegionalGeneration::Status SampleRegionalTerrainInfluence(
    WorldDescriptor world, std::int64_t xCm, std::int64_t yCm,
    RegionalTerrainInfluence& output)
{
    using namespace RegionalGeneration;
    RegionalTerrainInfluence result;
    if (world.generationVersion != WorldGenerationVersion)
        return RegionalGeneration::Status::UnsupportedVersion;
    const RegionalDescriptor descriptor{world.seed, RegionalGenerationVersion};
    InterpolatedRelief sample, spawn;
    auto status = Interpolate(descriptor, xCm, yCm, sample);
    if (status != RegionalGeneration::Status::Ok) return status;
    status = Interpolate(descriptor, -1000, 0, spawn);
    if (status != RegionalGeneration::Status::Ok) return status;
    result.heightOffsetCm = std::clamp(
        static_cast<double>(sample.elevationMm - spawn.elevationMm) / RegionalHeightDivisor,
        -MaximumRegionalOffsetCm, MaximumRegionalOffsetCm);
    result.mountain = static_cast<std::uint16_t>(sample.mountain);
    result.ridge = static_cast<std::uint16_t>(sample.ridge);
    result.valley = static_cast<std::uint16_t>(sample.valley);
    result.wetness = static_cast<std::uint16_t>(sample.wetness);
    output = result;
    return RegionalGeneration::Status::Ok;
}
}
