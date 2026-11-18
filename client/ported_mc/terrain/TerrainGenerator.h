#pragma once
// Phase-1 world generation: climate noise -> biomes -> height -> 3D density -> caves -> fluids -> surface rules.
// Everything is a pure function of (seed, x, y, z): the same seed always gives the same terrain.
#include "Biome.h"
#include "../core/Noise.h"
#include "../world/Chunk.h"

namespace mc {

struct ColumnInfo {
    double cont, eros, weird, pv, temp, humid, river, mountain;
    double height;      // target surface height in blocks
    double amp3;        // amplitude of 3D shape noise (overhangs)
    Biome biome;
};

class TerrainGenerator {
public:
    explicit TerrainGenerator(uint64_t seed);
    ColumnInfo column(int x, int z) const;
    void generate(Chunk& chunk) const;            // fills blocks + biome/terrainHeight/surfaceState
    uint64_t seed() const { return seed_; }
private:
    double caveStrength(int x, int y, int z, const ColumnInfo& ci, double entrance) const;
    int aquiferLevel(int x, int z) const;
    uint64_t seed_;
    OctaveNoise cont_, eros_, weird_, temp_, humid_, river_, hill_, mush_, shape3_;
    OctaveNoise cheese_, spag1_, spag2_, noodle1_, noodle2_, thick_, entrance_, aquifer_, surf_, patch_;
};

} // namespace mc
