#include "mc/core/player_physics.hpp"

#include <algorithm>
#include <cmath>

namespace mc {
namespace {
int floorToInt(double value) {
    return static_cast<int>(std::floor(value));
}

bool solidAt(World& world, int x, int y, int z) {
    return isSolid(world.get({x, y, z}).block);
}
} // namespace

bool PlayerPhysics::collides(World& world, const PlayerState& player) const {
    constexpr double epsilon = 1.0e-7;
    const double half = width / 2.0;
    const int minX = floorToInt(player.position.x - half + epsilon);
    const int maxX = floorToInt(player.position.x + half - epsilon);
    const int minY = floorToInt(player.position.y + epsilon);
    const int maxY = floorToInt(player.position.y + height - epsilon);
    const int minZ = floorToInt(player.position.z - half + epsilon);
    const int maxZ = floorToInt(player.position.z + half - epsilon);
    for (int y = minY; y <= maxY; ++y) {
        for (int z = minZ; z <= maxZ; ++z) {
            for (int x = minX; x <= maxX; ++x) {
                if (solidAt(world, x, y, z)) {
                    return true;
                }
            }
        }
    }
    return false;
}

void PlayerPhysics::tick(World& world, PlayerState& player) const {
    if (player.mode != GameMode::Creative && player.mode != GameMode::Spectator) {
        player.velocity.y -= 0.08;
    }
    player.position.x += player.velocity.x;
    if (collides(world, player)) {
        player.position.x -= player.velocity.x;
        player.velocity.x = 0;
    }
    player.position.z += player.velocity.z;
    if (collides(world, player)) {
        player.position.z -= player.velocity.z;
        player.velocity.z = 0;
    }
    const double previousY = player.position.y;
    player.position.y += player.velocity.y;
    player.onGround = false;
    if (collides(world, player)) {
        constexpr double epsilon = 1.0e-7;
        const double half = width / 2.0;
        const int minX = floorToInt(player.position.x - half + epsilon);
        const int maxX = floorToInt(player.position.x + half - epsilon);
        const int minZ = floorToInt(player.position.z - half + epsilon);
        const int maxZ = floorToInt(player.position.z + half - epsilon);
        if (player.velocity.y < 0) {
            double supportY = -1.0e30;
            const int firstY = floorToInt(player.position.y + epsilon);
            const int lastY = floorToInt(previousY + epsilon);
            for (int z = minZ; z <= maxZ; ++z) {
                for (int x = minX; x <= maxX; ++x) {
                    for (int y = firstY; y <= lastY; ++y) {
                        if (solidAt(world, x, y, z)) {
                            const double top = static_cast<double>(y + 1);
                            if (top <= previousY + epsilon && top >= player.position.y - epsilon) {
                                supportY = std::max(supportY, top);
                            }
                        }
                    }
                }
            }
            if (supportY > -1.0e29) {
                player.position.y = supportY;
            } else {
                player.position.y = previousY;
            }
            player.onGround = true;
        } else {
            player.position.y = previousY;
        }
        player.velocity.y = 0;
    }
    if (player.mode != GameMode::Creative && player.mode != GameMode::Spectator) {
        player.velocity.x *= player.onGround ? 0.6 : 0.91;
        player.velocity.z *= player.onGround ? 0.6 : 0.91;
        player.velocity.y *= 0.98;
    }
}

void PlayerPhysics::jump(PlayerState& player) const noexcept {
    if (player.onGround && player.mode != GameMode::Spectator) {
        player.velocity.y = 0.42;
        player.onGround = false;
    }
}

} // namespace mc
