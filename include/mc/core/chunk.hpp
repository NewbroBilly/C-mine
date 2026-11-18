#pragma once

#include "mc/core/block.hpp"
#include "mc/core/coordinates.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace mc {

class Chunk {
public:
    static constexpr std::int32_t width = 16;
    static constexpr std::int32_t height = 256;
    static constexpr std::size_t cellCount = static_cast<std::size_t>(width) * width * height;

    explicit Chunk(ChunkPos position) noexcept;

    [[nodiscard]] ChunkPos position() const noexcept { return position_; }
    [[nodiscard]] Cell get(std::int32_t x, std::int32_t y, std::int32_t z) const noexcept;
    bool set(std::int32_t x, std::int32_t y, std::int32_t z, Cell cell) noexcept;
    [[nodiscard]] const std::array<Cell, cellCount>& cells() const noexcept { return cells_; }
    [[nodiscard]] std::array<Cell, cellCount>& cells() noexcept { return cells_; }
    [[nodiscard]] std::uint64_t revision() const noexcept { return revision_; }

private:
    [[nodiscard]] static constexpr std::size_t index(std::int32_t x, std::int32_t y,
                                                      std::int32_t z) noexcept {
        return (static_cast<std::size_t>(y) * width + static_cast<std::size_t>(z)) * width
             + static_cast<std::size_t>(x);
    }

    ChunkPos position_;
    std::array<Cell, cellCount> cells_{};
    std::uint64_t revision_{0};
};

} // namespace mc
