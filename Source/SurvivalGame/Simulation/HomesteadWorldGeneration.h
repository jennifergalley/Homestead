#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <vector>

namespace Homestead::Generation
{
constexpr std::uint32_t WorldGenerationVersion = 5;
constexpr std::int64_t ChunkSizeCm = 2400;
constexpr int TerrainCellsPerChunk = 24;
constexpr int TerrainVerticesPerSide = TerrainCellsPerChunk + 1;
constexpr int TerrainVertexCount = TerrainVerticesPerSide * TerrainVerticesPerSide;
constexpr std::int64_t TerrainSpacingCm = ChunkSizeCm / TerrainCellsPerChunk;
constexpr int TreeCandidateCount = 36;
constexpr int ForageCandidatesPerKind = 4;
constexpr int MaxEntitiesPerChunk = TreeCandidateCount + 28;
constexpr std::int32_t MinChunkCoordinate = std::numeric_limits<std::int32_t>::min();
constexpr std::int32_t MaxChunkCoordinate = std::numeric_limits<std::int32_t>::max();
constexpr std::int64_t MinWorldCm = static_cast<std::int64_t>(MinChunkCoordinate) * ChunkSizeCm;
constexpr std::int64_t MaxWorldCmExclusive =
    (static_cast<std::int64_t>(MaxChunkCoordinate) + 1) * ChunkSizeCm;
constexpr std::int64_t SamplingHaloCm = 100;
constexpr double StreamWaterHalfWidthCm = 52.0;
constexpr double CreekWaterMinimumHalfWidthCm = 38.0;
constexpr double CreekWaterMaximumHalfWidthCm = 66.0;
constexpr double StreamBankOuterCm = 175.0;
constexpr double WaterReachCm = 180.0;
constexpr std::int64_t GraniteKnobRadiusCm = 540;

struct WorldDescriptor
{
    std::uint64_t seed = 0;
    std::uint32_t generationVersion = WorldGenerationVersion;
};

struct ChunkCoord { std::int32_t x = 0; std::int32_t y = 0; };
bool operator==(ChunkCoord a, ChunkCoord b);
bool operator!=(ChunkCoord a, ChunkCoord b);
bool operator<(ChunkCoord a, ChunkCoord b);

// Keys are scoped by WorldDescriptor. High 8 bits are EntityKind; low 24 are slot + 1.
struct GeneratedEntityKey
{
    ChunkCoord chunk;
    std::uint32_t localId = 0;
};
bool operator==(GeneratedEntityKey a, GeneratedEntityKey b);
bool operator!=(GeneratedEntityKey a, GeneratedEntityKey b);
bool operator<(GeneratedEntityKey a, GeneratedEntityKey b);

// These are generation tags, deliberately distinct from Simulation's ResourceKind ordinals.
enum class EntityKind : std::uint8_t
{
    ForestTree = 1, Branches = 2, Stones = 3, BerryBush = 4,
    Roots = 5, Flowers = 6, Reeds = 7, Sapling = 8
};

// Stable silhouette/age strata, not botanical species or asset identities.
enum class TreePaletteRole : std::uint8_t
{
    None = 0, BroadleafMature = 1, BroadleafYoung = 2,
    ConiferMature = 3, ConiferYoung = 4, WoodlandAccent = 5
};

enum class Status { Ok, UnsupportedVersion, OutOfRange, InvalidKey, NotFound };
const char* StatusMessage(Status status);

struct TerrainSample
{
    double heightCm = 0.0;
    double normalX = 0.0;
    double normalY = 0.0;
    double normalZ = 1.0;
    std::uint16_t woodland = 0;
    double streamCenterXCm = 0.0;
    double waterHeightCm = 0.0;
};

struct GeneratedEntity
{
    GeneratedEntityKey key;
    EntityKind kind = EntityKind::ForestTree;
    std::int64_t xCm = 0;
    std::int64_t yCm = 0;
    double heightCm = 0.0;
    std::uint16_t yawDegrees = 0;
    std::uint16_t scalePermille = 1000;
    TreePaletteRole paletteRole = TreePaletteRole::None;
    std::uint8_t variantIndex = 0;
};

struct ChunkBaseline
{
    ChunkCoord chunk;
    // Global-coordinate samples, indexed y * TerrainVerticesPerSide + x.
    std::array<TerrainSample, TerrainVertexCount> terrain{};
    std::vector<GeneratedEntity> entities;
};

// All status-returning calls leave output unchanged on failure. No spawn/save/occupancy filtering.
Status ChunkAt(std::int64_t xCm, std::int64_t yCm, ChunkCoord& output);
Status SampleTerrain(WorldDescriptor world, std::int64_t xCm, std::int64_t yCm,
    TerrainSample& output);
Status GenerateChunk(WorldDescriptor world, ChunkCoord chunk, ChunkBaseline& output);
Status FindEntity(WorldDescriptor world, GeneratedEntityKey key, GeneratedEntity& output);
// Some chunks hold a house-sized granite knob; trees and forage keep GraniteKnobRadiusCm clear of it.
bool GraniteKnob(WorldDescriptor world, ChunkCoord chunk, std::int64_t& xCm, std::int64_t& yCm);

// Shared legacy adaptation, not regional hydrology. Input is finite global centimeters.
// Integer noise/hash/keys are exact; libm stream geometry/normals are compared with tolerance.
double StreamCenterCm(double yCm);
double CreekWaterHalfWidthCm(WorldDescriptor world, double yCm, bool rightBank);
double CreekGroundBlendWeight(WorldDescriptor world, double xCm, double yCm);
}
