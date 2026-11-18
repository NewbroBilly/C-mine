#include "mc/core/player_physics.hpp"
#include "mc/core/world.hpp"

#include <charconv>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

int main(int argc, char** argv) {
    std::int64_t seed = 0;
    if (argc == 3 && std::string_view(argv[1]) == "--seed") {
        const std::string_view value(argv[2]);
        const auto result = std::from_chars(value.data(), value.data() + value.size(), seed);
        if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) {
            std::cerr << "Invalid seed. Usage: minecraft_1122 [--seed INTEGER]\n";
            return 2;
        }
    } else if (argc != 1) {
        std::cerr << "Usage: minecraft_1122 [--seed INTEGER]\n";
        return 2;
    }

    mc::World world(seed);
    const mc::Cell surface = world.get({0, 63, 0});
    const auto spawn = world.findSurfaceSpawn(0, 0);
    mc::PlayerPhysics physics;
    mc::PlayerState player;
    player.position = {8.5, 70.0, 8.5};
    for (int tick = 0; tick < 100; ++tick) {
        physics.tick(world, player);
    }

    std::cout << "Minecraft 1.12.2 C++ Edition - headless core prototype\n"
              << "Seed: " << world.seed() << "\n"
              << "Loaded chunks: " << world.loadedChunkCount() << "\n"
              << "Surface block at (0,63,0): " << mc::blockName(surface.block) << "\n"
              << "Surface texture: " << mc::texturePath(surface.block) << "\n"
              << "Spawn: " << (spawn ? std::to_string(spawn->x) + "," +
                                      std::to_string(spawn->y) + "," + std::to_string(spawn->z)
                                  : "none") << "\n"
              << "Player feet after 100 ticks: y=" << player.position.y
              << (player.onGround ? " (grounded)\n" : " (falling)\n")
              << "Terrain and physics are prototype approximations, not 1.12.2 parity.\n";
    return 0;
}
