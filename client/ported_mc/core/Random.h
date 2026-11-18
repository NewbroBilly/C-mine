#pragma once
// Deterministic PRNG (xoroshiro128++) + helpers to derive per-feature seeds.
#include "Types.h"

namespace mc {

class Rng {
public:
    explicit Rng(uint64_t seed = 0) { setSeed(seed); }
    void setSeed(uint64_t seed) {
        uint64_t s = seed;
        auto sm = [&s]() { s += 0x9E3779B97F4A7C15ULL; uint64_t z = s; z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL; z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL; return z ^ (z >> 31); };
        s0 = sm(); s1 = sm(); if ((s0 | s1) == 0) s1 = 1;
    }
    uint64_t next() {
        const uint64_t a = s0; uint64_t b = s1;
        const uint64_t r = rotl(a + b, 17) + a;
        b ^= a; s0 = rotl(a, 49) ^ b ^ (b << 21); s1 = rotl(b, 28);
        return r;
    }
    uint32_t nextU32() { return (uint32_t)(next() >> 32); }
    int nextInt(int bound) { return bound <= 1 ? 0 : (int)(((next() >> 11) * (uint64_t)bound) >> 53); }   // [0,bound)
    int range(int lo, int hi) { return lo + nextInt(hi - lo + 1); }                                    // inclusive
    double nextDouble() { return (double)(next() >> 11) * (1.0 / 9007199254740992.0); }                 // [0,1)
    float nextFloat() { return (float)nextDouble(); }
    bool chance(double p) { return nextDouble() < p; }
    bool nextBool() { return (next() >> 63) != 0; }
private:
    static uint64_t rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
    uint64_t s0, s1;
};

// Seed for a position-dependent feature: same (seed, a, b, salt) -> same stream, in any order.
inline uint64_t featureSeed(uint64_t seed, int64_t a, int64_t b, int64_t salt) {
    return mix64(seed ^ mix64((uint64_t)a * 0x9E3779B185EBCA87ULL + mix64((uint64_t)b * 0xC2B2AE3D27D4EB4FULL + (uint64_t)salt * 0x165667B19E3779F9ULL)));
}
// Stateless random in [0,1) from coordinates.
inline double hashUnit(uint64_t seed, int x, int y, int z) {
    uint64_t h = mix64(seed ^ mix64(((uint64_t)(uint32_t)x << 32) ^ (uint32_t)z ^ ((uint64_t)(uint32_t)(y + 4096) * 0x9E3779B97F4A7C15ULL)));
    return (double)(h >> 11) * (1.0 / 9007199254740992.0);
}

} // namespace mc
