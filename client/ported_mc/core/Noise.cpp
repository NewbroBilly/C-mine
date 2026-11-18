#include "Noise.h"
#include <numeric>

namespace mc {

static inline double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }
static inline double grad(int h, double x, double y, double z) {
    switch (h & 15) {
        case 0: return x + y;  case 1: return -x + y; case 2: return x - y;  case 3: return -x - y;
        case 4: return x + z;  case 5: return -x + z; case 6: return x - z;  case 7: return -x - z;
        case 8: return y + z;  case 9: return -y + z; case 10: return y - z; case 11: return -y - z;
        case 12: return y + x; case 13: return -y + z; case 14: return y - x; default: return -y - z;
    }
}

PerlinNoise::PerlinNoise(Rng& rng) {
    xo_ = rng.nextDouble() * 256.0; yo_ = rng.nextDouble() * 256.0; zo_ = rng.nextDouble() * 256.0;
    uint8_t perm[256];
    for (int i = 0; i < 256; ++i) perm[i] = (uint8_t)i;
    for (int i = 255; i > 0; --i) std::swap(perm[i], perm[rng.nextInt(i + 1)]);
    for (int i = 0; i < 512; ++i) p_[i] = perm[i & 255];
}

double PerlinNoise::noise(double x, double y, double z) const {
    x += xo_; y += yo_; z += zo_;
    double fx = std::floor(x), fy = std::floor(y), fz = std::floor(z);
    int X = (int)((int64_t)fx & 255), Y = (int)((int64_t)fy & 255), Z = (int)((int64_t)fz & 255);
    x -= fx; y -= fy; z -= fz;
    double u = fade(x), v = fade(y), w = fade(z);
    int A = p_[X] + Y, AA = p_[A] + Z, AB = p_[A + 1] + Z;
    int B = p_[X + 1] + Y, BA = p_[B] + Z, BB = p_[B + 1] + Z;
    auto L = [](double t, double a, double b) { return a + t * (b - a); };
    return L(w,
        L(v, L(u, grad(p_[AA], x, y, z), grad(p_[BA], x - 1, y, z)),
             L(u, grad(p_[AB], x, y - 1, z), grad(p_[BB], x - 1, y - 1, z))),
        L(v, L(u, grad(p_[AA + 1], x, y, z - 1), grad(p_[BA + 1], x - 1, y, z - 1)),
             L(u, grad(p_[AB + 1], x, y - 1, z - 1), grad(p_[BB + 1], x - 1, y - 1, z - 1))));
}

OctaveNoise::OctaveNoise(uint64_t seed, int octaves, double persistence, double lacunarity) {
    Rng rng(seed);
    double a = 1.0, f = 1.0, sum = 0.0, sq = 0.0;
    for (int i = 0; i < octaves; ++i) {
        oct_.emplace_back(rng); amp_.push_back(a); freq_.push_back(f);
        sum += a; sq += a * a; a *= persistence; f *= lacunarity;
    }
    // Perlin std-dev ~0.29; scale so that ~2 sigma maps to 1.
    double stddev = 0.29 * std::sqrt(sq);
    norm_ = 0.5 / stddev; (void)sum;
}

double OctaveNoise::sample(double x, double y, double z) const {
    double s = 0.0;
    for (size_t i = 0; i < oct_.size(); ++i) s += amp_[i] * oct_[i].noise(x * freq_[i], y * freq_[i], z * freq_[i]);
    return clampv(s * norm_, -1.5, 1.5);
}

} // namespace mc
