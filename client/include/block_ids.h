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
static_assert(BLOCK_COUNT<=128,"BlockState's signed byte cannot store more block IDs");

// Row 10, columns 0..9 are reserved by ItemModel::cracks and the importers.
#define BASE_BLOCK(id,label,sx,sy,sname,tx,ty,tname,bx,by,bname,emission) \
    static_assert(!(sy==10 && sx<10) && !(ty==10 && tx<10) && !(by==10 && bx<10),"Block atlas overlaps mining crack sprites");
#define EXTRA_BLOCK BASE_BLOCK
#include <extra_blocks.def>
#undef BASE_BLOCK
#undef EXTRA_BLOCK
