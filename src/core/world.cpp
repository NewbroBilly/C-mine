#include "mc/core/world.hpp"

namespace mc {

Chunk& World::ensureChunk(ChunkPos position) {
    auto found = chunks_.find(position);
    if (found != chunks_.end()) {
        return *found->second;
    }
    auto chunk = std::make_unique<Chunk>(position);
    generateTestTerrain(*chunk);
    Chunk& result = *chunk;
    chunks_.emplace(position, std::move(chunk));
    return result;
}

Chunk& World::loadChunk(ChunkPos position) {
    return ensureChunk(position);
}

Cell World::get(BlockPos position) {
    if (position.y < 0) {
        return {Block::Bedrock, 0};
    }
    if (position.y >= Chunk::height) {
        return {Block::Air, 0};
    }
    const ChunkPos chunkPosition{floorDiv16(position.x), floorDiv16(position.z)};
    return ensureChunk(chunkPosition).get(localCoord16(position.x), position.y,
                                           localCoord16(position.z));
}

std::optional<BlockPos> World::findSurfaceSpawn(std::int32_t x, std::int32_t z) {
    const ChunkPos chunkPosition{floorDiv16(x), floorDiv16(z)};
    Chunk& chunk = ensureChunk(chunkPosition);
    const auto localX = localCoord16(x);
    const auto localZ = localCoord16(z);

    // Scan the already-loaded chunk directly. This avoids calling get() for
    // every y level and prevents a surface query from creating extra chunks.
    for (std::int32_t y = Chunk::height - 2; y >= 0; --y) {
        if (!isSolid(chunk.get(localX, y, localZ).block)) {
            continue;
        }
        if (chunk.get(localX, y + 1, localZ).block != Block::Air ||
            chunk.get(localX, y + 2, localZ).block != Block::Air) {
            continue;
        }
        return BlockPos{x, y + 1, z};
    }
    return std::nullopt;
}

bool World::set(BlockPos position, Cell cell) {
    if (position.y < 0 || position.y >= Chunk::height) {
        return false;
    }
    const ChunkPos chunkPosition{floorDiv16(position.x), floorDiv16(position.z)};
    return ensureChunk(chunkPosition).set(localCoord16(position.x), position.y,
                                           localCoord16(position.z), cell);
}

void World::generateTestTerrain(Chunk& chunk) const noexcept {
    // A flat harness makes chunk layout and collision behavior testable before
    // the version-matched 1.12.2 noise and biome pipeline exists.
    for (std::int32_t z = 0; z < Chunk::width; ++z) {
        for (std::int32_t x = 0; x < Chunk::width; ++x) {
            auto& cells = chunk.cells();
            cells[(static_cast<std::size_t>(0) * Chunk::width + static_cast<std::size_t>(z)) * Chunk::width + static_cast<std::size_t>(x)] = {Block::Bedrock, 0};
            for (std::int32_t y = 1; y < 60; ++y) {
                cells[(static_cast<std::size_t>(y) * Chunk::width + static_cast<std::size_t>(z)) * Chunk::width + static_cast<std::size_t>(x)] = {Block::Stone, 0};
            }
            for (std::int32_t y = 60; y < 63; ++y) {
                cells[(static_cast<std::size_t>(y) * Chunk::width + static_cast<std::size_t>(z)) * Chunk::width + static_cast<std::size_t>(x)] = {Block::Dirt, 0};
            }
            cells[(static_cast<std::size_t>(63) * Chunk::width + static_cast<std::size_t>(z)) * Chunk::width + static_cast<std::size_t>(x)] = {Block::Grass, 0};
        }
    }
}

} // namespace mc
