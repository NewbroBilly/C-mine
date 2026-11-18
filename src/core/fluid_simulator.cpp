#include "mc/core/fluid_simulator.hpp"

#include "mc/core/block.hpp"

namespace mc {
namespace {
constexpr BlockPos neighbors[] = {
    {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0}
};
constexpr BlockPos horizontal[] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}};

BlockPos add(BlockPos a, BlockPos b) noexcept {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
} // namespace

void FluidSimulator::schedule(BlockPos position, std::uint64_t delay) {
    pending_.push({tick_ + delay, position, order_++});
}

void FluidSimulator::placeSource(World& world, BlockPos position, Block fluid) {
    if (!isFluid(fluid)) {
        return;
    }
    world.set(position, {fluid, 0});
    schedule(position, 1);
}

void FluidSimulator::notifyBlockChanged(BlockPos position) {
    for (BlockPos offset : neighbors) {
        schedule(add(position, offset), 1);
    }
}

void FluidSimulator::tick(World& world, std::size_t updateBudget) {
    ++tick_;
    std::size_t processed = 0;
    while (!pending_.empty() && pending_.top().dueTick <= tick_ && processed < updateBudget) {
        const BlockPos position = pending_.top().position;
        pending_.pop();
        update(world, position);
        ++processed;
    }
}

void FluidSimulator::update(World& world, BlockPos position) {
    const Cell source = world.get(position);
    if (!isFluid(source.block)) {
        return;
    }
    if (source.block == Block::Lava && (tick_ % 4U) != 0U) {
        schedule(position, 1);
        return;
    }

    bool changed = false;
    const BlockPos below = add(position, {0, -1, 0});
    Cell belowCell = world.get(below);
    if (belowCell.block == (source.block == Block::Water ? Block::Lava : Block::Water)) {
        world.set(below, {Block::Cobblestone, 0});
        belowCell = world.get(below);
        changed = true;
    }
    if (belowCell.block == Block::Air) {
        changed |= world.set(below, {source.block, 8});
    } else {
        const std::uint8_t level = source.metadata >= 8 ? 1 : source.metadata;
        if (level < 7) {
            for (BlockPos offset : horizontal) {
                const BlockPos target = add(position, offset);
                Cell targetCell = world.get(target);
                if (targetCell.block == (source.block == Block::Water ? Block::Lava : Block::Water)) {
                    world.set(target, {Block::Cobblestone, 0});
                    changed = true;
                    continue;
                }
                if (targetCell.block == Block::Air) {
                    changed |= world.set(target, {source.block, static_cast<std::uint8_t>(level + 1)});
                }
            }
        }
    }
    if (changed) {
        for (BlockPos offset : neighbors) {
            schedule(add(position, offset), source.block == Block::Lava ? 4 : 1);
        }
    }
}

} // namespace mc
