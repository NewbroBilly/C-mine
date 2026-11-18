#include "mc/core/chunk.hpp"

namespace mc {

Chunk::Chunk(ChunkPos position) noexcept : position_(position) {
    cells_.fill(Cell{Block::Air, 0});
}

Cell Chunk::get(std::int32_t x, std::int32_t y, std::int32_t z) const noexcept {
    if (x < 0 || x >= width || z < 0 || z >= width || y < 0 || y >= height) {
        return {};
    }
    return cells_[index(x, y, z)];
}

bool Chunk::set(std::int32_t x, std::int32_t y, std::int32_t z, Cell cell) noexcept {
    if (x < 0 || x >= width || z < 0 || z >= width || y < 0 || y >= height) {
        return false;
    }
    Cell& target = cells_[index(x, y, z)];
    if (target == cell) {
        return true;
    }
    target = cell;
    ++revision_;
    return true;
}

} // namespace mc
