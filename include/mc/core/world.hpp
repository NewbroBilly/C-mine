#pragma once

#include "mc/core/chunk.hpp"
#include "mc/core/coordinates.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>

namespace mc {

class World {
public:
    explicit World(std::int64_t seed = 0) noexcept : seed_(seed) {}

    [[nodiscard]] std::int64_t seed() const noexcept { return seed_; }
    [[nodiscard]] Cell get(BlockPos position);
    bool set(BlockPos position, Cell cell);
    [[nodiscard]] Chunk& loadChunk(ChunkPos position);
    // Returns the feet position for a two-block-high entity on the column.
    // The block below must be solid and both feet/head cells must be clear.
    [[nodiscard]] std::optional<BlockPos> findSurfaceSpawn(std::int32_t x,
                                                            std::int32_t z);
    [[nodiscard]] std::size_t loadedChunkCount() const noexcept { return chunks_.size(); }

private:
    [[nodiscard]] Chunk& ensureChunk(ChunkPos position);
    void generateTestTerrain(Chunk& chunk) const noexcept;

    std::int64_t seed_;
    std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash> chunks_;
};

} // namespace mc
