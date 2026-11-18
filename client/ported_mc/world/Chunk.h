#pragma once
// A 16x384x16 column of blocks split into 16-high sections, plus light and per-column data.
#include "Block.h"
#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace mc {

enum class ChunkStatus : uint8_t { Empty = 0, Terrain = 1, Decorated = 2, Lit = 3 };

class Chunk {
public:
    const int cx, cz;
    Chunk(int cx_, int cz_) : cx(cx_), cz(cz_) {
        for (int i = 0; i < 256; ++i) { terrainHeight[i] = WORLD_MIN_Y; surfaceState[i] = 0; biome[i] = 0; lightTop[i] = WORLD_MIN_Y - 1; }
    }

    static int secIndex(int y) { return (y - WORLD_MIN_Y) >> 4; }
    static int cellIndex(int x, int y, int z) { return (((y - WORLD_MIN_Y) & 15) * 16 + z) * 16 + x; }

    BlockState get(int x, int y, int z) const {
        if (y < WORLD_MIN_Y || y >= WORLD_MAX_Y) return 0;
        const auto& s = sections_[secIndex(y)];
        return s ? s[cellIndex(x, y, z)] : 0;
    }
    void set(int x, int y, int z, BlockState st) {            // raw write: keeps counters + light height in sync
        if (y < WORLD_MIN_Y || y >= WORLD_MAX_Y) return;
        int si = secIndex(y);
        auto& s = sections_[si];
        if (!s) { if (st == 0) return; s.reset(new BlockState[4096]()); }
        BlockState& cell = s[cellIndex(x, y, z)];
        if (cell == st) return;
        if (cell == 0) ++count_[si]; else if (st == 0) --count_[si];
        cell = st;
        int col = z * 16 + x;
        bool op = Blocks::get(stateId(st)).lightOpacity > 0;
        if (op) { if (y > lightTop[col]) lightTop[col] = (int16_t)y; }
        else if (y == lightTop[col]) { int t = y - 1; while (t >= WORLD_MIN_Y && Blocks::get(stateId(get(x, t, z))).lightOpacity == 0) --t; lightTop[col] = (int16_t)t; }
    }
    bool sectionEmpty(int si) const { return count_[si] == 0; }
    const BlockState* sectionData(int si) const { return sections_[si].get(); }
    int highestNonEmptySection() const { for (int i = SECTION_COUNT - 1; i >= 0; --i) if (count_[i]) return i; return -1; }
    int lowestNonEmptySection() const { for (int i = 0; i < SECTION_COUNT; ++i) if (count_[i]) return i; return SECTION_COUNT; }

    // light: sky<<4 | block
    bool hasLight() const { return !light_.empty(); }
    void allocLight() { light_.assign((size_t)16 * 16 * WORLD_HEIGHT, 0); }
    uint8_t lightRaw(int x, int y, int z) const {
        if (light_.empty()) return 0;
        return light_[(size_t)((y - WORLD_MIN_Y) * 16 + z) * 16 + x];
    }
    void setLightRaw(int x, int y, int z, uint8_t v) { light_[(size_t)((y - WORLD_MIN_Y) * 16 + z) * 16 + x] = v; }
    int skyLight(int x, int y, int z) const { return lightRaw(x, y, z) >> 4; }
    int blockLight(int x, int y, int z) const { return lightRaw(x, y, z) & 15; }

    // Column data produced by terrain generation and read (immutable) by neighbours' decoration.
    uint8_t biome[256];
    int16_t terrainHeight[256];     // topmost solid terrain block (before features)
    BlockState surfaceState[256];   // block at terrainHeight
    int16_t lightTop[256];          // topmost block with lightOpacity>0 (kept in sync by set())

    std::atomic<ChunkStatus> status{ChunkStatus::Empty};
    bool modified = false;          // differs from generated terrain -> must be saved
    bool fromSave = false;
    bool meshDirty = true;
    uint32_t meshVersion = 0;

private:
    std::array<std::unique_ptr<BlockState[]>, SECTION_COUNT> sections_;
    std::array<int, SECTION_COUNT> count_{};
    std::vector<uint8_t> light_;
};

} // namespace mc
