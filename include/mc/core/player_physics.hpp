#pragma once

#include "mc/core/world.hpp"

namespace mc {

enum class GameMode { Survival, Creative, Adventure, Spectator };

struct Vec3 {
    double x{};
    double y{};
    double z{};
};

struct PlayerState {
    Vec3 position{}; // Feet position, matching the legacy player bounding box convention.
    Vec3 velocity{};
    GameMode mode{GameMode::Survival};
    bool onGround{false};
};

class PlayerPhysics {
public:
    static constexpr double width = 0.6;
    static constexpr double height = 1.8;

    void tick(World& world, PlayerState& player) const;
    void jump(PlayerState& player) const noexcept;

private:
    [[nodiscard]] bool collides(World& world, const PlayerState& player) const;
};

} // namespace mc
