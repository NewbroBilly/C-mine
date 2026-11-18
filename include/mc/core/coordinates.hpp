#pragma once

#include <cstdint>
#include <functional>

namespace mc {

struct BlockPos {
    std::int32_t x{};
    std::int32_t y{};
    std::int32_t z{};
    friend constexpr bool operator==(BlockPos, BlockPos) = default;
};

struct ChunkPos {
    std::int32_t x{};
    std::int32_t z{};
    friend constexpr bool operator==(ChunkPos, ChunkPos) = default;
};

struct BlockPosHash {
    [[nodiscard]] std::size_t operator()(BlockPos p) const noexcept {
        std::size_t h = std::hash<std::int32_t>{}(p.x);
        h ^= std::hash<std::int32_t>{}(p.y) + 0x9e3779b9U + (h << 6U) + (h >> 2U);
        h ^= std::hash<std::int32_t>{}(p.z) + 0x9e3779b9U + (h << 6U) + (h >> 2U);
        return h;
    }
};

struct ChunkPosHash {
    [[nodiscard]] std::size_t operator()(ChunkPos p) const noexcept {
        std::size_t h = std::hash<std::int32_t>{}(p.x);
        h ^= std::hash<std::int32_t>{}(p.z) + 0x9e3779b9U + (h << 6U) + (h >> 2U);
        return h;
    }
};

[[nodiscard]] constexpr std::int32_t floorDiv16(std::int32_t value) noexcept {
    const std::int32_t quotient = value / 16;
    const std::int32_t remainder = value % 16;
    return remainder < 0 ? quotient - 1 : quotient;
}

[[nodiscard]] constexpr std::int32_t localCoord16(std::int32_t value) noexcept {
    const std::int32_t remainder = value % 16;
    return remainder < 0 ? remainder + 16 : remainder;
}

} // namespace mc
