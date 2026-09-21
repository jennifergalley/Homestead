#include "HomesteadWorldGeneration.h"

#include <cmath>
#include <utility>

namespace Homestead::Generation
{
namespace
{
constexpr std::int64_t FixedOne = 65536;
constexpr std::uint32_t SlotMask = 0x00ffffffU;

std::int64_t FloorDivide(std::int64_t value, std::int64_t divisor)
{
    return value / divisor - (value % divisor < 0 ? 1 : 0);
}

std::uint64_t Mix(std::uint64_t value)
{
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}

std::uint64_t Hash(WorldDescriptor world, std::int64_t x, std::int64_t y,
    std::uint64_t channel)
{
    const auto hy = Mix(static_cast<std::uint64_t>(y) + UINT64_C(0x632be59bd9b4e019));
    return Mix(Mix(world.seed ^ channel) ^
        Mix(static_cast<std::uint64_t>(x) + UINT64_C(0x9e3779b97f4a7c15)) ^
        ((hy << 32) | (hy >> 32)));
}

std::int64_t SmoothFixed(std::int64_t value)
{
    return value * value * (3 * FixedOne - 2 * value) / (FixedOne * FixedOne);
}

std::int64_t LerpFixed(std::int64_t a, std::int64_t b, std::int64_t weight)
{
    return a + (b - a) * weight / FixedOne;
}

std::int64_t Noise(WorldDescriptor world, std::int64_t x, std::int64_t y,
    std::int64_t period, std::uint64_t channel)
{
    const auto cellX = FloorDivide(x, period);
    const auto cellY = FloorDivide(y, period);
    const auto tx = SmoothFixed((x - cellX * period) * FixedOne / period);
    const auto ty = SmoothFixed((y - cellY * period) * FixedOne / period);
    const auto value = [&](std::int64_t cx, std::int64_t cy) {
        return static_cast<std::int64_t>(Hash(world, cx, cy, channel) & 65535U) - 32768;
    };
    return LerpFixed(LerpFixed(value(cellX, cellY), value(cellX + 1, cellY), tx),
        LerpFixed(value(cellX, cellY + 1), value(cellX + 1, cellY + 1), tx), ty);
}

double LandHeight(WorldDescriptor world, std::int64_t x, std::int64_t y)
{
    const auto height = Noise(world, x, y, 19200, 101) * (480 * 256) / 32768 +
        Noise(world, x, y, 7200, 102) * (180 * 256) / 32768 +
        Noise(world, x, y, 2400, 103) * (40 * 256) / 32768;
    return static_cast<double>(height) / 256.0;
}

std::uint16_t Woodland(WorldDescriptor world, std::int64_t x, std::int64_t y)
{
    return static_cast<std::uint16_t>(
        (3 * Noise(world, x, y, 9600, 201) + Noise(world, x, y, 2400, 202)) / 4 + 32768);
}

double Smooth(double low, double high, double value)
{
    if (value <= low) return 0.0;
    if (value >= high) return 1.0;
    const double t = (value - low) / (high - low);
    return t * t * (3.0 - 2.0 * t);
}

double Height(WorldDescriptor world, std::int64_t x, std::int64_t y)
{
    const double distance = std::abs(static_cast<double>(x) - StreamCenterCm(static_cast<double>(y)));
    const double bank = (distance - 190.0) / 85.0;
    return LandHeight(world, x, y) - 38.0 * (1.0 - Smooth(45.0, StreamBankOuterCm, distance))
        + 7.0 * std::exp(-bank * bank);
}

bool InSamplingRange(std::int64_t value)
{
    return value >= MinWorldCm - SamplingHaloCm &&
        value <= MaxWorldCmExclusive + SamplingHaloCm;
}

std::uint32_t LocalId(EntityKind kind, int slot)
{
    return (static_cast<std::uint32_t>(kind) << 24) | static_cast<std::uint32_t>(slot + 1);
}

std::uint64_t EntityHash(WorldDescriptor world, ChunkCoord chunk, EntityKind kind, int slot)
{
    return Hash(world, chunk.x, chunk.y,
        UINT64_C(0xd1b54a32d192ed03) ^ LocalId(kind, slot));
}

Status Candidate(WorldDescriptor world, ChunkCoord chunk, EntityKind kind, int slot,
    GeneratedEntity& output)
{
    const auto hash = EntityHash(world, chunk, kind, slot);
    const auto originX = static_cast<std::int64_t>(chunk.x) * ChunkSizeCm;
    const auto originY = static_cast<std::int64_t>(chunk.y) * ChunkSizeCm;
    GeneratedEntity entity;
    entity.key = {chunk, LocalId(kind, slot)};
    entity.kind = kind;
    if (kind == EntityKind::ForestTree)
    {
        entity.xCm = originX + (slot % 6) * 400 + 80 + static_cast<std::int64_t>(hash % 241);
        entity.yCm = originY + (slot / 6) * 400 + 80 + static_cast<std::int64_t>((hash >> 16) % 241);
        const auto woodland = Woodland(world, entity.xCm, entity.yCm);
        if ((Mix(hash ^ 301) & 65535U) >= 59000U + woodland / 12U ||
            std::abs(static_cast<double>(entity.xCm) -
                StreamCenterCm(static_cast<double>(entity.yCm))) < 250.0)
            return Status::NotFound;
    }
    else if (kind == EntityKind::Reeds)
    {
        entity.yCm = originY + 300 + slot * 600 + static_cast<std::int64_t>(hash % 161) - 80;
        const auto center = static_cast<std::int64_t>(std::llround(StreamCenterCm(static_cast<double>(entity.yCm))));
        entity.xCm = center + ((hash & 256U) ? 120 : -120);
        if (entity.xCm < originX || entity.xCm >= originX + ChunkSizeCm)
            return Status::NotFound;
    }
    else
    {
        // Each source kind has its own small patch; optional members keep their original slot IDs.
        const auto anchor = EntityHash(world, chunk, kind, 0);
        entity.xCm = originX + 400 + static_cast<std::int64_t>(anchor % 1601);
        entity.yCm = originY + 400 + static_cast<std::int64_t>((anchor >> 16) % 1601);
        if (slot > 0)
        {
            const auto woodland = Woodland(world, entity.xCm, entity.yCm);
            const std::uint64_t threshold = kind == EntityKind::Flowers
                ? 48000U - woodland / 3U : 22000U + woodland / 3U;
            if ((hash & 65535U) >= threshold) return Status::NotFound;
            entity.xCm += static_cast<std::int64_t>((hash >> 16) % 241) - 120;
            entity.yCm += static_cast<std::int64_t>((hash >> 32) % 241) - 120;
        }
        const double center = StreamCenterCm(static_cast<double>(entity.yCm));
        if (std::abs(static_cast<double>(entity.xCm) - center) < 230.0)
            entity.xCm = static_cast<std::int64_t>(std::llround(center)) +
                (static_cast<double>(entity.xCm) < center ? -250 : 250);
    }
    entity.heightCm = Height(world, entity.xCm, entity.yCm);
    entity.yawDegrees = static_cast<std::uint16_t>(Mix(hash ^ 401) % 360);
    entity.scalePermille = static_cast<std::uint16_t>(900 + Mix(hash ^ 402) % 201);
    output = entity;
    return Status::Ok;
}
}

bool operator==(ChunkCoord a, ChunkCoord b) { return a.x == b.x && a.y == b.y; }
bool operator!=(ChunkCoord a, ChunkCoord b) { return !(a == b); }
bool operator<(ChunkCoord a, ChunkCoord b) { return a.x < b.x || (a.x == b.x && a.y < b.y); }
bool operator==(GeneratedEntityKey a, GeneratedEntityKey b)
{
    return a.chunk == b.chunk && a.localId == b.localId;
}
bool operator!=(GeneratedEntityKey a, GeneratedEntityKey b) { return !(a == b); }
bool operator<(GeneratedEntityKey a, GeneratedEntityKey b)
{
    return a.chunk < b.chunk || (a.chunk == b.chunk && a.localId < b.localId);
}

const char* StatusMessage(Status status)
{
    switch (status)
    {
    case Status::Ok: return "Generated baseline is available.";
    case Status::UnsupportedVersion: return "World generation version is unsupported.";
    case Status::OutOfRange: return "World coordinates exceed the generation sampling range.";
    case Status::InvalidKey: return "Generated resource key has an unsupported kind or candidate slot.";
    case Status::NotFound: return "This candidate is not present in the generated baseline.";
    }
    return "Unknown generation status.";
}

double StreamCenterCm(double yCm) { return 1500.0 + 180.0 * std::sin(yCm / 800.0); }

Status ChunkAt(std::int64_t xCm, std::int64_t yCm, ChunkCoord& output)
{
    if (xCm < MinWorldCm || xCm >= MaxWorldCmExclusive ||
        yCm < MinWorldCm || yCm >= MaxWorldCmExclusive)
        return Status::OutOfRange;
    output = {static_cast<std::int32_t>(FloorDivide(xCm, ChunkSizeCm)),
        static_cast<std::int32_t>(FloorDivide(yCm, ChunkSizeCm))};
    return Status::Ok;
}

Status SampleTerrain(WorldDescriptor world, std::int64_t xCm, std::int64_t yCm,
    TerrainSample& output)
{
    if (world.generationVersion != WorldGenerationVersion) return Status::UnsupportedVersion;
    if (!InSamplingRange(xCm) || !InSamplingRange(yCm)) return Status::OutOfRange;
    TerrainSample sample;
    sample.heightCm = Height(world, xCm, yCm);
    const double dx = (Height(world, xCm + 50, yCm) - Height(world, xCm - 50, yCm)) / 100.0;
    const double dy = (Height(world, xCm, yCm + 50) - Height(world, xCm, yCm - 50)) / 100.0;
    const double length = std::sqrt(dx * dx + dy * dy + 1.0);
    sample.normalX = -dx / length;
    sample.normalY = -dy / length;
    sample.normalZ = 1.0 / length;
    sample.woodland = Woodland(world, xCm, yCm);
    sample.streamCenterXCm = StreamCenterCm(static_cast<double>(yCm));
    const auto streamX = static_cast<std::int64_t>(std::llround(sample.streamCenterXCm));
    sample.waterHeightCm = Height(world, streamX, yCm) + 19.0;
    output = sample;
    return Status::Ok;
}

Status FindEntity(WorldDescriptor world, GeneratedEntityKey key, GeneratedEntity& output)
{
    if (world.generationVersion != WorldGenerationVersion) return Status::UnsupportedVersion;
    const auto tag = key.localId >> 24;
    const auto index = key.localId & SlotMask;
    if (tag < static_cast<std::uint32_t>(EntityKind::ForestTree) ||
        tag > static_cast<std::uint32_t>(EntityKind::Sapling) || index == 0)
        return Status::InvalidKey;
    const auto kind = static_cast<EntityKind>(tag);
    const auto count = kind == EntityKind::ForestTree ? TreeCandidateCount :
        (kind == EntityKind::Branches ? 1 : ForageCandidatesPerKind);
    if (index > static_cast<std::uint32_t>(count)) return Status::InvalidKey;
    return Candidate(world, key.chunk, kind, static_cast<int>(index) - 1, output);
}

Status GenerateChunk(WorldDescriptor world, ChunkCoord chunk, ChunkBaseline& output)
{
    if (world.generationVersion != WorldGenerationVersion) return Status::UnsupportedVersion;
    ChunkBaseline baseline;
    baseline.chunk = chunk;
    const auto x = static_cast<std::int64_t>(chunk.x) * ChunkSizeCm;
    const auto y = static_cast<std::int64_t>(chunk.y) * ChunkSizeCm;
    for (int row = 0; row < TerrainVerticesPerSide; ++row)
        for (int column = 0; column < TerrainVerticesPerSide; ++column)
        {
            const auto status = SampleTerrain(world, x + column * TerrainSpacingCm,
                y + row * TerrainSpacingCm, baseline.terrain[row * TerrainVerticesPerSide + column]);
            if (status != Status::Ok) return status;
        }
    baseline.entities.reserve(MaxEntitiesPerChunk);
    for (std::uint32_t tag = 1; tag <= static_cast<std::uint32_t>(EntityKind::Sapling); ++tag)
    {
        const auto kind = static_cast<EntityKind>(tag);
        const int count = kind == EntityKind::ForestTree ? TreeCandidateCount :
            (kind == EntityKind::Branches ? 1 : ForageCandidatesPerKind);
        for (int slot = 0; slot < count; ++slot)
        {
            GeneratedEntity entity;
            const auto status = Candidate(world, chunk, kind, slot, entity);
            if (status == Status::Ok) baseline.entities.push_back(entity);
            else if (status != Status::NotFound) return status;
        }
    }
    output = std::move(baseline);
    return Status::Ok;
}
}
