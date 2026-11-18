#pragma once
// Improved Perlin noise with per-instance seeded permutation + octave sums.
#include "Random.h"
#include <vector>

namespace mc {

class PerlinNoise {
public:
    explicit PerlinNoise(Rng& rng);
    double noise(double x, double y, double z) const;   // roughly [-1, 1]
private:
    uint8_t p_[512];
    double xo_, yo_, zo_;
};

// Sum of octaves. sample() is normalised so typical values fill about [-1, 1].
class OctaveNoise {
public:
    OctaveNoise() = default;
    OctaveNoise(uint64_t seed, int octaves, double persistence = 0.5, double lacunarity = 2.0);
    double sample(double x, double y, double z) const;
    double sample2(double x, double z) const { return sample(x, 0.0, z); }
private:
    std::vector<PerlinNoise> oct_;
    std::vector<double> amp_, freq_;
    double norm_ = 1.0;
};

} // namespace mc
