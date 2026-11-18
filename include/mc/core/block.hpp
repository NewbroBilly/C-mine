#pragma once

#include <cstdint>
#include <string_view>

namespace mc {

enum class Block : std::uint8_t {
    Air = 0,
    Stone,
    Dirt,
    Grass,
    Bedrock,
    Sand,
    Water,
    Lava,
    Cobblestone,
    Obsidian,
};

struct Cell {
    Block block{Block::Air};
    std::uint8_t metadata{0};

    friend constexpr bool operator==(Cell, Cell) = default;
};

[[nodiscard]] constexpr bool isFluid(Block block) noexcept {
    return block == Block::Water || block == Block::Lava;
}

[[nodiscard]] constexpr bool isSolid(Block block) noexcept {
    return block != Block::Air && !isFluid(block);
}

[[nodiscard]] constexpr std::string_view blockName(Block block) noexcept {
    switch (block) {
    case Block::Air: return "air";
    case Block::Stone: return "stone";
    case Block::Dirt: return "dirt";
    case Block::Grass: return "grass";
    case Block::Bedrock: return "bedrock";
    case Block::Sand: return "sand";
    case Block::Water: return "water";
    case Block::Lava: return "lava";
    case Block::Cobblestone: return "cobblestone";
    case Block::Obsidian: return "obsidian";
    }
    return "unknown";
}

// Paths used by the vanilla 1.12.2 resource pack. The files themselves are
// intentionally not redistributed by this project; a resource pack can be
// mounted by the renderer later.
[[nodiscard]] constexpr std::string_view texturePath(Block block) noexcept {
    switch (block) {
    case Block::Stone: return "textures/blocks/stone.png";
    case Block::Dirt: return "textures/blocks/dirt.png";
    case Block::Grass: return "textures/blocks/grass_top.png";
    case Block::Bedrock: return "textures/blocks/bedrock.png";
    case Block::Sand: return "textures/blocks/sand.png";
    case Block::Water: return "textures/blocks/water_still.png";
    case Block::Lava: return "textures/blocks/lava_still.png";
    case Block::Cobblestone: return "textures/blocks/cobblestone.png";
    case Block::Obsidian: return "textures/blocks/obsidian.png";
    case Block::Air: return {};
    }
    return {};
}

} // namespace mc
