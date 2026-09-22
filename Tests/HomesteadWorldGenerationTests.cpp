#include "HomesteadWorldGeneration.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
#include <algorithm>
#include <vector>

using namespace Homestead::Generation;

static_assert(static_cast<int>(TreePaletteRole::None) == 0);
static_assert(static_cast<int>(TreePaletteRole::BroadleafMature) == 1);
static_assert(static_cast<int>(TreePaletteRole::BroadleafYoung) == 2);
static_assert(static_cast<int>(TreePaletteRole::ConiferMature) == 3);
static_assert(static_cast<int>(TreePaletteRole::ConiferYoung) == 4);
static_assert(static_cast<int>(TreePaletteRole::WoodlandAccent) == 5);

namespace
{
int checks = 0;
void Check(bool condition, const char* expression, int line)
{
    ++checks;
    if (!condition)
    {
        std::cerr << "FAIL line " << line << ": " << expression << '\n';
        std::exit(1);
    }
}
#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)

bool Same(const TerrainSample& a, const TerrainSample& b)
{
    return a.heightCm == b.heightCm && a.normalX == b.normalX && a.normalY == b.normalY &&
        a.normalZ == b.normalZ && a.woodland == b.woodland &&
        a.streamCenterXCm == b.streamCenterXCm && a.waterHeightCm == b.waterHeightCm;
}
bool Same(const GeneratedEntity& a, const GeneratedEntity& b)
{
    return a.key == b.key && a.kind == b.kind && a.xCm == b.xCm && a.yCm == b.yCm &&
        a.heightCm == b.heightCm && a.paletteRole == b.paletteRole &&
        a.variantIndex == b.variantIndex && a.yawDegrees == b.yawDegrees &&
        a.scalePermille == b.scalePermille;
}
bool Same(const ChunkBaseline& a, const ChunkBaseline& b)
{
    if (a.chunk != b.chunk || a.entities.size() != b.entities.size()) return false;
    for (int i = 0; i < TerrainVertexCount; ++i)
        if (!Same(a.terrain[i], b.terrain[i])) return false;
    for (std::size_t i = 0; i < a.entities.size(); ++i)
        if (!Same(a.entities[i], b.entities[i])) return false;
    return true;
}
std::uint32_t Key(EntityKind kind, int slot)
{
    return (static_cast<std::uint32_t>(kind) << 24) | static_cast<std::uint32_t>(slot + 1);
}

void CoordinatesAndFailures()
{
    ChunkCoord coord{17, 23};
    CHECK(ChunkAt(-1, -2401, coord) == Status::Ok);
    CHECK((coord == ChunkCoord{-1, -2}));
    CHECK(ChunkAt(-2400, 2399, coord) == Status::Ok);
    CHECK((coord == ChunkCoord{-1, 0}));
    CHECK(ChunkAt(2400, 0, coord) == Status::Ok);
    CHECK((coord == ChunkCoord{1, 0}));
    CHECK(ChunkAt(MinWorldCm, MaxWorldCmExclusive - 1, coord) == Status::Ok);
    CHECK((coord == ChunkCoord{MinChunkCoordinate, MaxChunkCoordinate}));
    const auto before = coord;
    CHECK(ChunkAt(MinWorldCm - 1, 0, coord) == Status::OutOfRange);
    CHECK(coord == before);
    CHECK(ChunkAt(0, MaxWorldCmExclusive, coord) == Status::OutOfRange);
    CHECK(coord == before);
    CHECK(ChunkAt(std::numeric_limits<std::int64_t>::min(), 0, coord) == Status::OutOfRange);
    CHECK(ChunkAt(0, std::numeric_limits<std::int64_t>::max(), coord) == Status::OutOfRange);

    TerrainSample terrain;
    terrain.heightCm = 987.0;
    for (std::uint32_t version : {1U, 2U})
    {
        CHECK(SampleTerrain({0, version}, 0, 0, terrain) == Status::UnsupportedVersion);
        CHECK(terrain.heightCm == 987.0);
    }
    CHECK(SampleTerrain({}, MinWorldCm - SamplingHaloCm - 1, 0, terrain) == Status::OutOfRange);
    CHECK(SampleTerrain({}, 0, MaxWorldCmExclusive + SamplingHaloCm + 1, terrain) == Status::OutOfRange);
    CHECK(terrain.heightCm == 987.0);
    CHECK(SampleTerrain({}, MinWorldCm - SamplingHaloCm, MaxWorldCmExclusive + SamplingHaloCm,
        terrain) == Status::Ok);

    ChunkBaseline baseline;
    baseline.chunk = {123, 456};
    CHECK(GenerateChunk({0, 2}, {}, baseline) == Status::UnsupportedVersion);
    CHECK((baseline.chunk == ChunkCoord{123, 456}));
    GeneratedEntity entity;
    entity.xCm = 987;
    CHECK(FindEntity({0, 2}, {}, entity) == Status::UnsupportedVersion);
    for (const auto id : {0U, 0xff000001U, Key(EntityKind::ForestTree, 36),
        Key(EntityKind::Branches, 1), Key(EntityKind::Reeds, 4), 0x01000000U})
    {
        CHECK(FindEntity({}, {{0, 0}, id}, entity) == Status::InvalidKey);
        CHECK(entity.xCm == 987);
    }
    CHECK(FindEntity({}, {{-1, 0}, Key(EntityKind::Reeds, 0)}, entity) == Status::NotFound);
    CHECK(entity.xCm == 987);
}

void SeamsAndOrder()
{
    const WorldDescriptor world{UINT64_C(0xfedcba9876543210), WorldGenerationVersion};
    for (const ChunkCoord origin : {ChunkCoord{-2, -2}, ChunkCoord{-1, -1},
        ChunkCoord{0, 0}, ChunkCoord{417, -419},
        ChunkCoord{MinChunkCoordinate, MinChunkCoordinate},
        ChunkCoord{MaxChunkCoordinate - 1, MaxChunkCoordinate - 1}})
    {
        ChunkBaseline first, right, top, again;
        CHECK(GenerateChunk(world, origin, first) == Status::Ok);
        CHECK(GenerateChunk(world, {origin.x, origin.y + 1}, top) == Status::Ok);
        CHECK(GenerateChunk(world, {origin.x + 1, origin.y}, right) == Status::Ok);
        CHECK(GenerateChunk(world, origin, again) == Status::Ok);
        CHECK(Same(first, again));
        for (int i = 0; i < TerrainVerticesPerSide; ++i)
        {
            CHECK(Same(first.terrain[i * TerrainVerticesPerSide + TerrainCellsPerChunk],
                right.terrain[i * TerrainVerticesPerSide]));
            CHECK(Same(first.terrain[TerrainCellsPerChunk * TerrainVerticesPerSide + i],
                top.terrain[i]));
        }
        for (const auto& sample : first.terrain)
        {
            const double norm = sample.normalX * sample.normalX +
                sample.normalY * sample.normalY + sample.normalZ * sample.normalZ;
            CHECK(std::abs(norm - 1.0) < 1e-12);
            CHECK(std::isfinite(sample.heightCm));
            CHECK(sample.normalZ > 0.75);
        }
    }
}

void IdentitiesAndDistribution()
{
    for (std::uint64_t seed : {UINT64_C(0), UINT64_C(1), UINT64_C(817391),
        UINT64_C(817392), UINT64_C(0x100000000), UINT64_C(0xffffffffffffffff)})
    {
        const WorldDescriptor world{seed, WorldGenerationVersion};
        std::set<GeneratedEntityKey> keys;
        int trees = 0;
        int nearSpawnTrees = 0;
        std::array<bool, 9> bootstrap{};
        for (int cy = -1; cy <= 1; ++cy)
            for (int cx = -2; cx <= 0; ++cx)
            {
                ChunkBaseline chunk;
                CHECK(GenerateChunk(world, {cx, cy}, chunk) == Status::Ok);
                CHECK(chunk.entities.size() <= MaxEntitiesPerChunk);
                std::array<int, 9> counts{};
                for (const auto& entity : chunk.entities)
                {
                    CHECK(keys.insert(entity.key).second);
                    CHECK(entity.key.localId >> 24 == static_cast<std::uint32_t>(entity.kind));
                    GeneratedEntity lookup;
                    CHECK(FindEntity(world, entity.key, lookup) == Status::Ok);
                    CHECK(Same(entity, lookup));
                    ChunkCoord owner;
                    CHECK(ChunkAt(entity.xCm, entity.yCm, owner) == Status::Ok);
                    CHECK(owner == chunk.chunk);
                    TerrainSample terrain;
                    CHECK(SampleTerrain(world, entity.xCm, entity.yCm, terrain) == Status::Ok);
                    CHECK(terrain.heightCm == entity.heightCm);
                    CHECK(entity.yawDegrees < 360);
                    CHECK(entity.scalePermille >= 900 && entity.scalePermille <= 1100);
                    CHECK(entity.variantIndex <= 3);
                    ++counts[static_cast<int>(entity.kind)];
                    const double dx = static_cast<double>(entity.xCm) + 1000.0;
                    const double dy = static_cast<double>(entity.yCm);
                    if (dx * dx + dy * dy <= 4000.0 * 4000.0)
                        bootstrap[static_cast<int>(entity.kind)] = true;
                    if (entity.kind == EntityKind::ForestTree)
                    {
                        CHECK(entity.paletteRole == TreePaletteRole::BroadleafMature ||
                            entity.paletteRole == TreePaletteRole::ConiferMature ||
                            entity.paletteRole == TreePaletteRole::WoodlandAccent);
                        if (entity.paletteRole == TreePaletteRole::BroadleafMature)
                            CHECK(entity.scalePermille >= 900 && entity.scalePermille <= 1040);
                        else if (entity.paletteRole == TreePaletteRole::ConiferMature)
                            CHECK(entity.scalePermille >= 900 && entity.scalePermille <= 1050);
                        else
                            CHECK(entity.scalePermille >= 900 && entity.scalePermille <= 1060);
                        ++trees;
                        if (dx * dx + dy * dy < 700.0 * 700.0) ++nearSpawnTrees;
                        CHECK(std::abs(static_cast<double>(entity.xCm) - terrain.streamCenterXCm) >= 250.0);
                        for (const auto& other : chunk.entities)
                            if (other.kind == EntityKind::ForestTree && other.key != entity.key)
                            {
                                const auto tx = other.xCm - entity.xCm;
                                const auto ty = other.yCm - entity.yCm;
                                CHECK(tx * tx + ty * ty >= 100 * 100);
                            }
                    }
                    else if (entity.kind == EntityKind::Sapling)
                    {
                        CHECK(entity.paletteRole == TreePaletteRole::BroadleafYoung ||
                            entity.paletteRole == TreePaletteRole::ConiferYoung);
                        CHECK(entity.scalePermille >= (entity.paletteRole ==
                            TreePaletteRole::BroadleafYoung ? 900 : 920));
                        CHECK(entity.scalePermille <= 1080);
                    }
                    else
                    {
                        CHECK(entity.paletteRole == TreePaletteRole::None);
                        CHECK(entity.variantIndex == 0);
                    }
                    if (entity.kind == EntityKind::Reeds)
                        CHECK(std::abs(std::abs(static_cast<double>(entity.xCm) -
                            terrain.streamCenterXCm) - 120.0) <= 0.500001);
                }
                CHECK(counts[static_cast<int>(EntityKind::Branches)] == 1);
                for (int kind = 3; kind <= 8; ++kind) CHECK(counts[kind] <= ForageCandidatesPerKind);
            }
        CHECK(trees >= 250);
        CHECK(nearSpawnTrees >= 5);
        CHECK(bootstrap[static_cast<int>(EntityKind::Branches)]);
        CHECK(bootstrap[static_cast<int>(EntityKind::Stones)]);
        CHECK(bootstrap[static_cast<int>(EntityKind::Reeds)]);
    }
    ChunkBaseline a, b, highSeed;
    CHECK(GenerateChunk({817391, WorldGenerationVersion}, {-1, 0}, a) == Status::Ok);
    CHECK(GenerateChunk({817392, WorldGenerationVersion}, {-1, 0}, b) == Status::Ok);
    CHECK(GenerateChunk({UINT64_C(817391) + (UINT64_C(1) << 32), WorldGenerationVersion},
        {-1, 0}, highSeed) == Status::Ok);
    CHECK(!Same(a, b));
    CHECK(!Same(a, highSeed));
    CHECK(a.terrain[0].heightCm != b.terrain[0].heightCm);
}

void StreamAndNormals()
{
    const WorldDescriptor world{817391, WorldGenerationVersion};
    for (std::int64_t y : {-1000000, -2401, -1, 0, 1, 2400, 1000000})
    {
        CHECK(StreamCenterCm(static_cast<double>(y)) ==
            1500.0 + 180.0 * std::sin(static_cast<double>(y) / 800.0));
        const auto x = static_cast<std::int64_t>(std::llround(StreamCenterCm(static_cast<double>(y))));
        TerrainSample center, edge, dxp, dxm, dyp, dym;
        CHECK(SampleTerrain(world, x, y, center) == Status::Ok);
        CHECK(std::abs(center.waterHeightCm - center.heightCm - 19.0) < 1e-10);
        CHECK(SampleTerrain(world, -1000, y, edge) == Status::Ok);
        CHECK(SampleTerrain(world, -950, y, dxp) == Status::Ok);
        CHECK(SampleTerrain(world, -1050, y, dxm) == Status::Ok);
        CHECK(SampleTerrain(world, -1000, y + 50, dyp) == Status::Ok);
        CHECK(SampleTerrain(world, -1000, y - 50, dym) == Status::Ok);
        CHECK(std::abs(edge.normalX / edge.normalZ + (dxp.heightCm - dxm.heightCm) / 100.0) < 1e-12);
        CHECK(std::abs(edge.normalY / edge.normalZ + (dyp.heightCm - dym.heightCm) / 100.0) < 1e-12);
    }
}

void ClusteredLayout()
{
    const WorldDescriptor world{817391, WorldGenerationVersion};
    int totalTrees = 0;
    int chunksWithMicroOpenings = 0;
    std::set<int> xOffsets;
    std::set<int> yOffsets;
    std::set<TreePaletteRole> roles;
    std::set<std::uint8_t> variants;
    for (int cy = -2; cy <= 2; ++cy)
        for (int cx = -8; cx <= -4; ++cx)
        {
            ChunkBaseline chunk;
            CHECK(GenerateChunk(world, {cx, cy}, chunk) == Status::Ok);
            int trees = 0;
            for (const auto& entity : chunk.entities)
                if (entity.kind == EntityKind::ForestTree)
                {
                    ++trees;
                    const auto localX = entity.xCm - static_cast<std::int64_t>(cx) * ChunkSizeCm;
                    const auto localY = entity.yCm - static_cast<std::int64_t>(cy) * ChunkSizeCm;
                    CHECK(localX % 400 >= 50 && localX % 400 <= 350);
                    CHECK(localY % 400 >= 50 && localY % 400 <= 350);
                    xOffsets.insert(static_cast<int>(localX % 400));
                    yOffsets.insert(static_cast<int>(localY % 400));
                    roles.insert(entity.paletteRole);
                    variants.insert(entity.variantIndex);
                }
            CHECK(trees >= 30);
            totalTrees += trees;
            if (trees <= 34) ++chunksWithMicroOpenings;
        }
    CHECK(totalTrees >= 800);
    CHECK(chunksWithMicroOpenings >= 1);
    CHECK(xOffsets.size() >= 40);
    CHECK(yOffsets.size() >= 40);
    CHECK(roles.size() == 3);
    CHECK(variants.size() == 4);
}

void TerrainReliefAndBuildPockets()
{
    const WorldDescriptor world{817391, WorldGenerationVersion};
    std::vector<double> slopes;
    double minimum = std::numeric_limits<double>::max();
    double maximum = std::numeric_limits<double>::lowest();
    for (std::int64_t y = -6000; y <= 6000; y += 100)
        for (std::int64_t x = -6000; x <= 6000; x += 100)
        {
            TerrainSample sample;
            CHECK(SampleTerrain(world, x, y, sample) == Status::Ok);
            if (std::abs(static_cast<double>(x) - sample.streamCenterXCm) <= 300.0) continue;
            minimum = std::min(minimum, sample.heightCm);
            maximum = std::max(maximum, sample.heightCm);
            slopes.push_back(std::sqrt(sample.normalX * sample.normalX +
                sample.normalY * sample.normalY) / sample.normalZ);
        }
    std::sort(slopes.begin(), slopes.end());
    CHECK(maximum - minimum >= 500.0);
    CHECK(slopes.back() <= 0.20);
    CHECK(slopes[slopes.size() * 95 / 100] <= 0.14);

    minimum = std::numeric_limits<double>::max();
    maximum = std::numeric_limits<double>::lowest();
    for (std::int64_t y = -4800; y <= 4800; y += 2400)
        for (std::int64_t x = -4800; x <= 4800; x += 2400)
        {
            TerrainSample sample;
            CHECK(SampleTerrain(world, x, y, sample) == Status::Ok);
            minimum = std::min(minimum, sample.heightCm);
            maximum = std::max(maximum, sample.heightCm);
        }
    CHECK(maximum - minimum >= 350.0);

    // Discoverable build pockets use a deterministic 3m center lattice and
    // center/cardinal/diagonal samples over a 6m radius.
    int pockets = 0;
    constexpr std::int64_t diagonal = 424;
    constexpr std::int64_t offsets[][2] = {
        {0, 0}, {600, 0}, {-600, 0}, {0, 600}, {0, -600},
        {diagonal, diagonal}, {diagonal, -diagonal},
        {-diagonal, diagonal}, {-diagonal, -diagonal}};
    for (std::int64_t y = -5400; y <= 5400 && pockets < 3; y += 300)
        for (std::int64_t x = -5400; x <= 5400 && pockets < 3; x += 300)
        {
            double low = std::numeric_limits<double>::max();
            double high = std::numeric_limits<double>::lowest();
            for (const auto& offset : offsets)
            {
                TerrainSample sample;
                CHECK(SampleTerrain(world, x + offset[0], y + offset[1], sample) == Status::Ok);
                low = std::min(low, sample.heightCm);
                high = std::max(high, sample.heightCm);
            }
            if (high - low <= 60.0) ++pockets;
        }
    CHECK(pockets >= 3);

    minimum = std::numeric_limits<double>::max();
    maximum = std::numeric_limits<double>::lowest();
    for (int step = 0; step <= 10; ++step)
    {
        TerrainSample sample;
        CHECK(SampleTerrain(world, -1300 + step * 30, -80 + step * 8, sample) == Status::Ok);
        minimum = std::min(minimum, sample.heightCm);
        maximum = std::max(maximum, sample.heightCm);
    }
    CHECK(maximum - minimum <= 45.0);
}

void VersionFixture()
{
    ChunkBaseline chunk;
    CHECK(GenerateChunk({817391, WorldGenerationVersion}, {-1, 0}, chunk) == Status::Ok);
    std::uint64_t fingerprint = UINT64_C(14695981039346656037);
    const auto append = [&](std::uint64_t value) {
        for (int byte = 0; byte < 8; ++byte)
        {
            fingerprint ^= (value >> (byte * 8)) & 255U;
            fingerprint *= UINT64_C(1099511628211);
        }
    };
    for (const auto& sample : chunk.terrain)
    {
        // Quantized heights and integer metadata pin the supported Windows v3 baseline.
        append(static_cast<std::uint64_t>(std::llround(sample.heightCm * 256.0)));
        append(sample.woodland);
    }
    for (const auto& entity : chunk.entities)
    {
        append(entity.key.localId);
        append(static_cast<std::uint64_t>(entity.xCm));
        append(static_cast<std::uint64_t>(entity.yCm));
        append(static_cast<std::uint64_t>(entity.paletteRole));
        append(entity.variantIndex);
        append(entity.yawDegrees);
        append(entity.scalePermille);
    }
    CHECK(fingerprint == UINT64_C(10706599526293181624));
    std::cout << "Generation v3 fixture fingerprint: " << fingerprint << '\n';
}
}

int main()
{
    CoordinatesAndFailures();
    SeamsAndOrder();
    IdentitiesAndDistribution();
    StreamAndNormals();
    ClusteredLayout();
    TerrainReliefAndBuildPockets();
    VersionFixture();
    std::cout << "World generation: " << checks << " checks passed.\n";
    return 0;
}
