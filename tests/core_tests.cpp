#include "mc/core/fluid_simulator.hpp"
#include "mc/core/java_random.hpp"
#include "mc/core/player_physics.hpp"
#include "mc/core/world.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
int failures = 0;

void check(bool condition, std::string_view name) {
    if (!condition) {
        std::cerr << "FAIL: " << name << '\n';
        ++failures;
    }
}
} // namespace

int main() {
    mc::JavaRandom random(0);
    check(random.nextInt() == -1155484576, "java.util.Random nextInt seed 0");
    check(random.nextInt(100) == 48, "java.util.Random bounded nextInt sequence");

    mc::Chunk chunk({0, 0});
    check(chunk.set(15, 255, 15, {mc::Block::Sand, 2}), "chunk accepts last valid cell");
    check(chunk.get(15, 255, 15) == mc::Cell{mc::Block::Sand, 2}, "chunk stores cell metadata");
    const auto revision = chunk.revision();
    check(chunk.set(15, 255, 15, {mc::Block::Sand, 2}) && chunk.revision() == revision,
          "unchanged block does not invalidate chunk mesh");
    check(chunk.set(15, 255, 15, {mc::Block::Stone, 0}) && chunk.revision() > revision,
          "changed block invalidates chunk mesh");
    check(!chunk.set(16, 0, 0, {mc::Block::Stone, 0}), "chunk rejects out-of-range coordinate");
    check(mc::floorDiv16(-1) == -1 && mc::localCoord16(-1) == 15, "negative block coordinates map correctly");

    mc::World world(1234);
    check(world.get({0, 63, 0}).block == mc::Block::Grass, "test terrain surface");
    check(world.get({-1, 63, 0}).block == mc::Block::Grass, "negative chunk coordinates");
    check(world.get({0, -1, 0}).block == mc::Block::Bedrock, "below-world boundary");
    check(world.get({0, 256, 0}).block == mc::Block::Air, "above-world boundary");
    const auto spawn = world.findSurfaceSpawn(0, 0);
    check(spawn.has_value() && spawn->y == 64, "surface spawn is above grass");
    world.set({0, 64, 0}, {mc::Block::Stone, 0});
    check(world.findSurfaceSpawn(0, 0)->y == 65, "surface spawn moves to new top block");
    check(mc::texturePath(mc::Block::Grass) == "textures/blocks/grass_top.png",
          "vanilla grass texture path");

    mc::FluidSimulator fluids;
    fluids.placeSource(world, {4, 70, 4}, mc::Block::Water);
    fluids.tick(world);
    check(world.get({4, 69, 4}).block == mc::Block::Water, "water flows downward");

    mc::PlayerPhysics physics;
    mc::PlayerState player;
    player.position = {8.5, 70.0, 8.5};
    for (int i = 0; i < 100; ++i) {
        physics.tick(world, player);
    }
    check(player.onGround && std::abs(player.position.y - 64.0) < 0.02, "player lands on terrain");
    physics.jump(player);
    check(player.velocity.y > 0, "grounded player can jump");

    if (failures == 0) {
        std::cout << "All core checks passed.\n";
        return EXIT_SUCCESS;
    }
    std::cerr << failures << " core check(s) failed.\n";
    return EXIT_FAILURE;
}
