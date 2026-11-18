#pragma once

#include "mc/core/coordinates.hpp"
#include "mc/core/world.hpp"

#include <cstddef>
#include <cstdint>
#include <queue>
#include <vector>

namespace mc {

class FluidSimulator {
public:
    void placeSource(World& world, BlockPos position, Block fluid);
    void notifyBlockChanged(BlockPos position);
    void tick(World& world, std::size_t updateBudget = 128);
    [[nodiscard]] std::uint64_t currentTick() const noexcept { return tick_; }

private:
    struct ScheduledUpdate {
        std::uint64_t dueTick{};
        BlockPos position{};
        std::uint64_t order{};
    };
    struct Later {
        [[nodiscard]] bool operator()(const ScheduledUpdate& a, const ScheduledUpdate& b) const noexcept {
            return a.dueTick != b.dueTick ? a.dueTick > b.dueTick : a.order > b.order;
        }
    };

    void schedule(BlockPos position, std::uint64_t delay);
    void update(World& world, BlockPos position);

    std::priority_queue<ScheduledUpdate, std::vector<ScheduledUpdate>, Later> pending_;
    std::uint64_t tick_{};
    std::uint64_t order_{};
};

} // namespace mc
