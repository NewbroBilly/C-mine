#pragma once
enum BlockID {
    AIR, STONE, DIRT, GRASS_BLOCK, SAND, LOG, LEAVES, GLOWSTONE, WATER,
    GLASS, GRASS, TORCH, PLANKS, BEDROCK,
#define BASE_BLOCK(...)
#define EXTRA_BLOCK(id, ...) id,
#include <extra_blocks.def>
#undef BASE_BLOCK
#undef EXTRA_BLOCK
    BLOCK_COUNT
};
