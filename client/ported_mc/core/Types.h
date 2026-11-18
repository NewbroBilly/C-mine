#pragma once
// Basic constants and value types shared by all modules.
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <algorithm>

namespace mc {

constexpr int CHUNK_SIZE   = 16;
constexpr int WORLD_MIN_Y  = -64;
constexpr int WORLD_HEIGHT = 384;                       // y in [-64, 320)
constexpr int WORLD_MAX_Y  = WORLD_MIN_Y + WORLD_HEIGHT; // exclusive
constexpr int SECTION_SIZE = 16;
constexpr int SECTION_COUNT = WORLD_HEIGHT / SECTION_SIZE; // 24
constexpr int SEA_LEVEL    = 63;
constexpr int TPS          = 20;                         // game ticks per second

inline int floorDiv(int a, int b) { int q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }
inline int floorMod(int a, int b) { int m = a % b; if (m != 0 && ((m < 0) != (b < 0))) m += b; return m; }
inline int floorInt(double v) { int i = (int)v; return (v < (double)i) ? i - 1 : i; }
template <class T> inline T clampv(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline double lerpd(double a, double b, double t) { return a + (b - a) * t; }
inline double smoothstep(double e0, double e1, double x) {
    double t = clampv((x - e0) / (e1 - e0), 0.0, 1.0); return t * t * (3.0 - 2.0 * t);
}

struct Vec3d {
    double x = 0, y = 0, z = 0;
    Vec3d() = default;
    Vec3d(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    Vec3d operator+(const Vec3d& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3d operator-(const Vec3d& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3d operator*(double s) const { return {x * s, y * s, z * s}; }
    double length() const { return std::sqrt(x * x + y * y + z * z); }
    double lengthSq() const { return x * x + y * y + z * z; }
};

struct BlockPos {
    int x = 0, y = 0, z = 0;
    BlockPos() = default;
    BlockPos(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}
    bool operator==(const BlockPos& o) const { return x == o.x && y == o.y && z == o.z; }
    bool operator!=(const BlockPos& o) const { return !(*this == o); }
    BlockPos operator+(const BlockPos& o) const { return {x + o.x, y + o.y, z + o.z}; }
};

struct ChunkPos {
    int x = 0, z = 0;
    ChunkPos() = default;
    ChunkPos(int x_, int z_) : x(x_), z(z_) {}
    bool operator==(const ChunkPos& o) const { return x == o.x && z == o.z; }
    bool operator!=(const ChunkPos& o) const { return !(*this == o); }
};

inline uint64_t mix64(uint64_t x) {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}
struct ChunkPosHash { size_t operator()(const ChunkPos& c) const { return (size_t)mix64(((uint64_t)(uint32_t)c.x << 32) | (uint32_t)c.z); } };
struct BlockPosHash {
    size_t operator()(const BlockPos& p) const {
        return (size_t)mix64(((uint64_t)(uint32_t)p.x * 0x1F1F1F1FULL) ^ ((uint64_t)(uint32_t)p.z << 21) ^ ((uint64_t)(uint32_t)(p.y + 1024) << 42));
    }
};

// Face / direction order used everywhere: +X, -X, +Y, -Y, +Z, -Z
enum Dir : int { DIR_EAST = 0, DIR_WEST = 1, DIR_UP = 2, DIR_DOWN = 3, DIR_SOUTH = 4, DIR_NORTH = 5 };
constexpr int DIR_DX[6] = {1, -1, 0, 0, 0, 0};
constexpr int DIR_DY[6] = {0, 0, 1, -1, 0, 0};
constexpr int DIR_DZ[6] = {0, 0, 0, 0, 1, -1};
constexpr int DIR_OPP[6] = {1, 0, 3, 2, 5, 4};
constexpr int HORIZ_DIRS[4] = {DIR_EAST, DIR_WEST, DIR_SOUTH, DIR_NORTH};

} // namespace mc
