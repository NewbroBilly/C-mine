#pragma once
// Phase-2 generation: ores, trees, plants, dungeons.
// Chunk X is decorated by replaying the feature streams of its 3x3 neighbourhood and keeping only the
// blocks that fall inside X. Each feature has its own RNG derived from (seed, chunk, feature, index), and
// only immutable terrain data of neighbours is read, so the result never depends on generation order.
#include "TerrainGenerator.h"

namespace mc {

class Decorator {
public:
    Decorator(uint64_t seed, const TerrainGenerator& tg) : seed_(seed), tg_(tg) {}
    // nb[(dz+1)*3 + (dx+1)] are the 3x3 terrain-complete chunks; nb[4] == &target.
    void decorate(Chunk& target, Chunk* const nb[9]) const;
private:
    uint64_t seed_;
    const TerrainGenerator& tg_;
};

} // namespace mc
