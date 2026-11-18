#include "TerrainGenerator.h"
#include "../world/Block.h"

namespace mc {

TerrainGenerator::TerrainGenerator(uint64_t seed) : seed_(seed) {
    Blocks::init();
    auto S = [&](int k) { return featureSeed(seed, k, 0, 0x7E44A1); };
    cont_ = OctaveNoise(S(1), 6); eros_ = OctaveNoise(S(2), 5); weird_ = OctaveNoise(S(3), 5);
    temp_ = OctaveNoise(S(4), 5); humid_ = OctaveNoise(S(5), 5); river_ = OctaveNoise(S(6), 4);
    hill_ = OctaveNoise(S(7), 5); mush_ = OctaveNoise(S(8), 4); shape3_ = OctaveNoise(S(9), 4);
    cheese_ = OctaveNoise(S(10), 3); spag1_ = OctaveNoise(S(11), 3); spag2_ = OctaveNoise(S(12), 3);
    noodle1_ = OctaveNoise(S(13), 2); noodle2_ = OctaveNoise(S(14), 2); thick_ = OctaveNoise(S(15), 2);
    entrance_ = OctaveNoise(S(16), 2); aquifer_ = OctaveNoise(S(17), 2); surf_ = OctaveNoise(S(18), 2); patch_ = OctaveNoise(S(19), 3);
}

static double pl(const double (*pts)[2], int n, double x) {
    if (x <= pts[0][0]) return pts[0][1];
    for (int i = 1; i < n; ++i) if (x <= pts[i][0]) { double t = (x - pts[i - 1][0]) / (pts[i][0] - pts[i - 1][0]); return lerpd(pts[i - 1][1], pts[i][1], t); }
    return pts[n - 1][1];
}

ColumnInfo TerrainGenerator::column(int x, int z) const {
    ColumnInfo c{};
    c.cont = cont_.sample2(x / 1500.0, z / 1500.0);
    c.eros = eros_.sample2(x / 1100.0, z / 1100.0);
    c.weird = weird_.sample2(x / 900.0, z / 900.0);
    c.temp = temp_.sample2(x / 1400.0, z / 1400.0);
    c.humid = humid_.sample2(x / 1200.0, z / 1200.0);
    double rv = river_.sample2(x / 800.0, z / 800.0);
    double a = std::fabs(clampv(c.weird, -1.0, 1.0));
    c.pv = -(std::fabs(a - 2.0 / 3.0) - 1.0 / 3.0) * 3.0;

    static const double base[][2] = {{-1.3, 18}, {-0.8, 28}, {-0.45, 40}, {-0.22, 56}, {-0.1, 63.6}, {0.0, 65}, {0.15, 68}, {0.4, 73}, {0.8, 82}, {1.5, 96}};
    double H = pl(base, 10, c.cont);
    double landF = smoothstep(-0.18, 0.05, c.cont);
    c.mountain = clampv((-c.eros - 0.1) / 0.6, 0.0, 1.0) * smoothstep(0.1, 0.4, c.cont);
    double hillAmp = lerpd(1.5, 13.0, clampv((1.0 - c.eros) / 2.0, 0.0, 1.0)) * landF;
    double hills = hill_.sample2(x / 90.0, z / 90.0) * hillAmp;
    double peak = c.mountain * (25.0 + 95.0 * clampv((c.pv + 0.3) / 1.3, 0.0, 1.0));
    H += hills + peak;
    // mushroom islands: rare, deep-ocean only
    double mush = smoothstep(0.55, 0.8, mush_.sample2(x / 600.0, z / 600.0)) * smoothstep(-0.55, -0.85, c.cont);
    if (mush > 0) H = lerpd(H, 66.0 + hills * 0.5, mush);
    c.river = (1.0 - smoothstep(0.015, 0.06, std::fabs(rv))) * landF * (1.0 - c.mountain * 0.75);
    if (c.river > 0 && mush < 0.1) H = lerpd(H, SEA_LEVEL - 4.0, c.river);

    // ---- biome
    double Tt = c.temp - std::max(0.0, (H - 95.0)) * 0.012;
    int ti = Tt < -0.45 ? 0 : Tt < -0.15 ? 1 : Tt < 0.2 ? 2 : Tt < 0.55 ? 3 : 4;
    int hi = c.humid < -0.35 ? 0 : c.humid < -0.1 ? 1 : c.humid < 0.1 ? 2 : c.humid < 0.3 ? 3 : 4;
    Biome b;
    if (mush > 0.5) b = Biome::MushroomFields;
    else if (H < SEA_LEVEL - 1.5 && c.river < 0.5) {
        if (Tt < -0.45) b = Biome::FrozenOcean; else if (Tt > 0.5) b = Biome::WarmOcean; else b = H < 44 ? Biome::DeepOcean : Biome::Ocean;
    } else if (c.river > 0.5) b = Tt < -0.3 ? Biome::FrozenRiver : Biome::River;
    else if (H < 66.5 && c.cont < -0.02 && c.mountain < 0.2) b = Biome::Beach;
    else if (c.mountain > 0.35 && H > 92) b = Tt < -0.15 ? Biome::SnowyPeaks : Biome::Mountains;
    else {
        static const Biome tab[5][5] = {
            {Biome::SnowyPlains, Biome::SnowyPlains, Biome::SnowyPlains, Biome::SnowyTaiga, Biome::SnowyTaiga},
            {Biome::Plains, Biome::Plains, Biome::Forest, Biome::Taiga, Biome::Taiga},
            {Biome::Plains, Biome::Plains, Biome::Forest, Biome::BirchForest, Biome::Forest},
            {Biome::Savanna, Biome::Savanna, Biome::Forest, Biome::Jungle, Biome::Jungle},
            {Biome::Desert, Biome::Desert, Biome::Desert, Biome::Desert, Biome::Desert}};
        b = tab[ti][hi];
        if (ti == 4 && c.weird > 0.35) b = Biome::Badlands;
        if (c.eros > 0.35 && ti >= 1 && ti <= 3 && c.humid > 0.05 && H < 69 && H > 61) b = Biome::Swamp;
    }
    if (b == Biome::Swamp) H = 62.4 + hill_.sample2(x / 50.0, z / 50.0) * 1.8;
    if (b == Biome::Badlands) H += c.mountain * 6;

    c.biome = b; c.height = H;
    double flat = clampv((c.eros + 0.2) / 1.0, 0.0, 1.0);
    c.amp3 = (H < SEA_LEVEL - 2) ? 1.5 : lerpd(7.0, 1.2, flat) + 26.0 * c.mountain;
    if (b == Biome::Swamp || b == Biome::Beach || c.river > 0.3) c.amp3 = 1.0;
    return c;
}

int TerrainGenerator::aquiferLevel(int x, int z) const {
    double a = aquifer_.sample2(x / 220.0, z / 220.0);
    if (a < 0.2) return WORLD_MIN_Y - 1;
    return std::min(40, -40 + 8 * (int)((a - 0.2) * 25.0));
}

// Positive value => carve air. cheese caves, spaghetti tunnels, noodle tunnels.
double TerrainGenerator::caveStrength(int x, int y, int z, const ColumnInfo& ci, double entrance) const {
    if (y < WORLD_MIN_Y + 3) return 0;
    double depth = ci.height - y;                         // blocks below the surface
    double mask = smoothstep(6.0, 16.0, depth);
    if (entrance > 0.45 && ci.height > SEA_LEVEL + 1) mask = std::max(mask, smoothstep(-2.0, 4.0, depth));
    if (ci.height < SEA_LEVEL + 1) mask = std::min(mask, smoothstep(8.0, 20.0, depth));
    if (mask <= 0.0) return 0;
    double s = 0;
    double ch = cheese_.sample(x / 80.0, y / 46.0, z / 80.0);
    s = std::max(s, (ch - 0.62) * 1.6);
    double thick = 0.17 + 0.06 * thick_.sample(x / 120.0, y / 120.0, z / 120.0);
    double n1 = spag1_.sample(x / 70.0, y / 52.0, z / 70.0), n2 = spag2_.sample(x / 70.0, y / 52.0, z / 70.0);
    s = std::max(s, (thick - std::sqrt(n1 * n1 + n2 * n2)) * 6.0);
    double m1 = noodle1_.sample(x / 40.0, y / 36.0, z / 40.0), m2 = noodle2_.sample(x / 40.0, y / 36.0, z / 40.0);
    s = std::max(s, (0.085 - std::sqrt(m1 * m1 + m2 * m2)) * 6.0);
    return s * mask;
}

void TerrainGenerator::generate(Chunk& chunk) const {
    const int bx = chunk.cx * 16, bz = chunk.cz * 16;
    constexpr int NY = WORLD_HEIGHT / 4 + 1;      // 97 corner layers
    ColumnInfo ci[5][5]; double ent[5][5];
    for (int i = 0; i < 5; ++i) for (int j = 0; j < 5; ++j) {
        ci[i][j] = column(bx + i * 4, bz + j * 4);
        ent[i][j] = entrance_.sample2((bx + i * 4) / 60.0, (bz + j * 4) / 60.0);
    }
    static thread_local float dens[5][5][NY];
    for (int i = 0; i < 5; ++i) for (int j = 0; j < 5; ++j) {
        const ColumnInfo& c = ci[i][j];
        double H = c.height, A = c.amp3, top = H + 1.5 * A + 1.0, bot = H - 1.5 * A - 1.0;
        for (int k = 0; k < NY; ++k) {
            int y = WORLD_MIN_Y + k * 4, wx = bx + i * 4, wz = bz + j * 4;
            double d;
            if (y > top) d = H - y;
            else if (y < bot) d = H - y;
            else d = (H - y) + shape3_.sample(wx / 40.0, y / 30.0, wz / 40.0) * A;
            if (d > 4.0) d = 4.0;
            if (d > -2.0 && y <= top + 4) {} // near surface keep shape
            double cs = (y <= H + 2) ? caveStrength(wx, y, wz, c, ent[i][j]) : 0.0;
            if (cs > 0.0) d = -(cs * 10.0 + 0.6);
            dens[i][j][k] = (float)d;
        }
    }
    uint8_t sol[WORLD_HEIGHT];
    for (int z = 0; z < 16; ++z) for (int x = 0; x < 16; ++x) {
        const int wx = bx + x, wz = bz + z;
        const int ci0 = x >> 2, cj0 = z >> 2; const float fx = (x & 3) / 4.0f, fz = (z & 3) / 4.0f;
        int topSolid = WORLD_MIN_Y - 1;
        for (int k = 0; k < NY - 1; ++k) {
            const float a000 = dens[ci0][cj0][k], a100 = dens[ci0 + 1][cj0][k], a001 = dens[ci0][cj0 + 1][k], a101 = dens[ci0 + 1][cj0 + 1][k];
            const float b000 = dens[ci0][cj0][k + 1], b100 = dens[ci0 + 1][cj0][k + 1], b001 = dens[ci0][cj0 + 1][k + 1], b101 = dens[ci0 + 1][cj0 + 1][k + 1];
            const float lo = (a000 * (1 - fx) + a100 * fx) * (1 - fz) + (a001 * (1 - fx) + a101 * fx) * fz;
            const float hi = (b000 * (1 - fx) + b100 * fx) * (1 - fz) + (b001 * (1 - fx) + b101 * fx) * fz;
            for (int dy = 0; dy < 4; ++dy) {
                int idx = k * 4 + dy;
                sol[idx] = (lo + (hi - lo) * (dy / 4.0f)) > 0.0f;
                if (sol[idx]) topSolid = WORLD_MIN_Y + idx;
            }
        }
        const ColumnInfo& cc = ci[ci0][cj0];
        // biome of this exact column (not the corner)
        Biome biome = column(wx, wz).biome;
        const int aq = aquiferLevel(wx, wz);
        // 1. stone, fluids
        for (int idx = 0; idx < WORLD_HEIGHT; ++idx) {
            int y = WORLD_MIN_Y + idx; BlockState s = 0;
            if (sol[idx]) {
                bool deep = y < 0 || (y < 8 && hashUnit(seed_, wx, y, wz) < (8 - y) / 8.0);
                s = makeState(deep ? B::DEEPSLATE : B::STONE);
                if (y < WORLD_MIN_Y + 5) { double p = (y == WORLD_MIN_Y) ? 1.0 : 1.0 - (y - WORLD_MIN_Y) / 5.0; if (hashUnit(seed_ ^ 0xBED, wx, y, wz) < p) s = makeState(B::BEDROCK); }
            } else if (y <= SEA_LEVEL - 1 && y > topSolid) s = makeState(B::WATER);
            else if (y < topSolid) { if (y <= aq) s = makeState(B::WATER); else if (y <= -54) s = makeState(B::LAVA); }
            if (s) chunk.set(x, y, z, s);
        }
        // 2. surface rules
        int ts = topSolid; if (ts < WORLD_MIN_Y) { chunk.biome[z * 16 + x] = (uint8_t)biome; continue; }
        bool under = ts < SEA_LEVEL - 1;           // water above the surface
        double sn = surf_.sample2(wx / 14.0, wz / 14.0), pn = patch_.sample2(wx / 45.0, wz / 45.0);
        int depth = 3 + (int)std::floor((sn + 1.0) * 1.2);
        auto put = [&](int y, BlockId id) { if (y >= WORLD_MIN_Y + 5) chunk.set(x, y, z, makeState(id)); };
        auto column = [&](BlockId topB, BlockId fill, int n) { put(ts, topB); for (int i = 1; i <= n; ++i) if (ts - i >= WORLD_MIN_Y && sol[ts - i - WORLD_MIN_Y]) put(ts - i, fill); };
        bool dry = !under && ts >= SEA_LEVEL - 1;
        switch (biome) {
            case Biome::Desert: case Biome::Beach: column(B::SAND, B::SAND, depth); for (int i = depth + 1; i <= depth + 4; ++i) if (ts - i >= WORLD_MIN_Y) put(ts - i, B::SANDSTONE); break;
            case Biome::Badlands: {
                static const BlockId bands[7] = {B::TERRACOTTA, B::ORANGE_TERRACOTTA, B::YELLOW_TERRACOTTA, B::BROWN_TERRACOTTA, B::RED_TERRACOTTA, B::WHITE_TERRACOTTA, B::LIGHT_GRAY_TERRACOTTA};
                for (int i = 0; i < 28; ++i) { int y = ts - i; if (y < WORLD_MIN_Y + 5 || !sol[y - WORLD_MIN_Y]) break; int yy = y + (int)(sn * 2.0); put(y, bands[(int)(hashUnit(seed_, 0, floorDiv(yy, 2), 7) * 7.0)]); }
                if (dry && sn > 0.1) put(ts, B::RED_SAND);
                break; }
            case Biome::Ocean: case Biome::DeepOcean: case Biome::FrozenOcean: case Biome::WarmOcean: case Biome::River: case Biome::FrozenRiver:
                if (pn > 0.5) column(B::CLAY, B::CLAY, 2); else if (pn < -0.3 || biome == Biome::WarmOcean) column(B::SAND, B::SAND, 3); else if (pn > 0.2) column(B::GRAVEL, B::GRAVEL, 3); else column(B::DIRT, B::DIRT, depth);
                if (biome == Biome::FrozenOcean || biome == Biome::FrozenRiver) chunk.set(x, SEA_LEVEL - 1, z, makeState(B::ICE));
                break;
            case Biome::MushroomFields: column(B::MYCELIUM, B::DIRT, depth); break;
            case Biome::SnowyPeaks:
                if (ts >= 150) column(B::SNOW_BLOCK, B::STONE, 2); else if (ts > 118) { put(ts, B::STONE); if (ts + 1 < WORLD_MAX_Y) chunk.set(x, ts + 1, z, makeState(B::SNOW_LAYER)); } else { column(B::GRASS_BLOCK, B::DIRT, depth); if (ts + 1 < WORLD_MAX_Y) chunk.set(x, ts + 1, z, makeState(B::SNOW_LAYER)); }
                break;
            case Biome::Mountains:
                if (ts > 105 || pn > 0.45) { if (pn > 0.55 && ts < 130) column(B::GRAVEL, B::GRAVEL, 2); } else column(B::GRASS_BLOCK, B::DIRT, depth);
                break;
            case Biome::SnowyPlains: case Biome::SnowyTaiga:
                if (under) column(B::DIRT, B::DIRT, depth); else { column(B::GRASS_BLOCK, B::DIRT, depth); if (ts + 1 < WORLD_MAX_Y && ts + 1 >= SEA_LEVEL) chunk.set(x, ts + 1, z, makeState(B::SNOW_LAYER)); }
                break;
            case Biome::Taiga: if (!under && pn > 0.35) column(B::PODZOL, B::DIRT, depth); else column(under ? B::DIRT : B::GRASS_BLOCK, B::DIRT, depth); break;
            default:
                if (under) { if (pn > 0.4) column(B::GRAVEL, B::GRAVEL, 2); else if (pn < -0.4) column(B::SAND, B::SAND, 3); else column(B::DIRT, B::DIRT, depth); }
                else column(B::GRASS_BLOCK, B::DIRT, depth);
                break;
        }
        (void)cc;
        chunk.biome[z * 16 + x] = (uint8_t)biome;
        chunk.terrainHeight[z * 16 + x] = (int16_t)ts;
        chunk.surfaceState[z * 16 + x] = chunk.get(x, ts, z);
    }
    chunk.status.store(ChunkStatus::Terrain);
}

} // namespace mc
