#include "Decorator.h"

namespace mc {
namespace {

enum class Mode { OnlyAir, Leaf, Log, Force, Stone };

struct Ctx {
    Chunk& t; int bx, bz;
    explicit Ctx(Chunk& c) : t(c), bx(c.cx * 16), bz(c.cz * 16) {}
    bool in(int x, int z) const { return x >= bx && x < bx + 16 && z >= bz && z < bz + 16; }
    BlockState get(int x, int y, int z) const { return in(x, z) ? t.get(x - bx, y, z - bz) : 0; }
    bool put(int x, int y, int z, BlockState s, Mode m) {
        if (!in(x, z) || y < WORLD_MIN_Y || y >= WORLD_MAX_Y) return false;
        BlockState cur = t.get(x - bx, y, z - bz);
        const BlockInfo& b = blockOf(cur);
        bool ok = false;
        switch (m) {
            case Mode::OnlyAir: ok = cur == 0; break;
            case Mode::Leaf: ok = cur == 0 || (!b.fluid && (b.plant || b.replaceable || b.id == B::SNOW_LAYER)); break;
            case Mode::Log: ok = cur == 0 || (!b.fluid && (b.plant || b.replaceable || b.layer == RenderLayer::Cutout || b.id == B::SNOW_LAYER)); break;
            case Mode::Force: ok = true; break;
            case Mode::Stone: { BlockId id = b.id; ok = id == B::STONE || id == B::DEEPSLATE || id == B::GRANITE || id == B::DIORITE || id == B::ANDESITE || id == B::TUFF; break; }
        }
        if (ok) t.set(x - bx, y, z - bz, s);
        return ok;
    }
};

struct Feature { const Chunk& m; int mx, mz; };   // source chunk of the stream being replayed

// ---------------- ores ----------------
struct OreCfg { BlockId ore, deep; int count, size, minY, maxY; bool tri; };

void placeBlob(Ctx& c, Rng& r, int x, int y, int z, int size, BlockId ore, BlockId deep, bool anyStone) {
    if (!(x + size + 4 >= c.bx && x - size - 4 < c.bx + 16 && z + size + 4 >= c.bz && z - size - 4 < c.bz + 16)) return;
    double angle = r.nextDouble() * 3.14159265358979;
    double sp = size / 8.0;
    double x1 = x + std::sin(angle) * sp, x2 = x - std::sin(angle) * sp, z1 = z + std::cos(angle) * sp, z2 = z - std::cos(angle) * sp;
    double y1 = y + r.nextInt(3) - 2, y2 = y + r.nextInt(3) - 2;
    for (int i = 0; i < size; ++i) {
        double t = (double)i / size;
        double cx = lerpd(x1, x2, t), cy = lerpd(y1, y2, t), cz = lerpd(z1, z2, t);
        double rad = ((std::sin(3.14159265358979 * t) + 1.0) * r.nextDouble() * size / 16.0 + 1.0) / 2.0;
        for (int ix = (int)std::floor(cx - rad); ix <= (int)std::floor(cx + rad); ++ix)
            for (int iz = (int)std::floor(cz - rad); iz <= (int)std::floor(cz + rad); ++iz)
                for (int iy = (int)std::floor(cy - rad); iy <= (int)std::floor(cy + rad); ++iy) {
                    double dx = (ix + 0.5 - cx) / rad, dy = (iy + 0.5 - cy) / rad, dz = (iz + 0.5 - cz) / rad;
                    if (dx * dx + dy * dy + dz * dz >= 1.0 || !c.in(ix, iz)) continue;
                    BlockState cur = c.get(ix, iy, iz); BlockId id = stateId(cur);
                    bool host = id == B::STONE || id == B::DEEPSLATE || id == B::GRANITE || id == B::DIORITE || id == B::ANDESITE || id == B::TUFF;
                    if (!host && !(anyStone && (id == B::DIRT))) continue;
                    if (!anyStone && !host) continue;
                    BlockId place = ore;
                    if (id == B::DEEPSLATE && deep) place = deep;
                    c.put(ix, iy, iz, makeState(place), Mode::Force);
                }
    }
}

void runOres(Ctx& c, const Feature& f, uint64_t seed) {
    static std::vector<OreCfg> cfg;
    if (cfg.empty()) {
        cfg = {
            {B::DIRT, 0, 7, 33, 0, 160, false}, {B::GRAVEL, 0, 14, 33, -64, 319, false}, {B::GRANITE, 0, 10, 33, 0, 128, false},
            {B::DIORITE, 0, 10, 33, 0, 128, false}, {B::ANDESITE, 0, 10, 33, 0, 128, false}, {B::TUFF, 0, 2, 33, -64, 0, false},
            {B::COAL_ORE, B::DEEPSLATE_COAL_ORE, 20, 17, 0, 192, true}, {B::COAL_ORE, B::DEEPSLATE_COAL_ORE, 30, 17, 136, 319, false},
            {B::IRON_ORE, B::DEEPSLATE_IRON_ORE, 10, 9, -24, 56, true}, {B::IRON_ORE, B::DEEPSLATE_IRON_ORE, 10, 9, 80, 319, true},
            {B::IRON_ORE, B::DEEPSLATE_IRON_ORE, 10, 4, -64, 72, false},
            {B::COPPER_ORE, B::DEEPSLATE_COPPER_ORE, 16, 10, -16, 112, true},
            {B::GOLD_ORE, B::DEEPSLATE_GOLD_ORE, 4, 9, -64, 32, true}, {B::GOLD_ORE, B::DEEPSLATE_GOLD_ORE, 1, 9, -64, -48, false},
            {B::REDSTONE_ORE, B::DEEPSLATE_REDSTONE_ORE, 4, 8, -64, 15, false}, {B::REDSTONE_ORE, B::DEEPSLATE_REDSTONE_ORE, 8, 8, -64, 0, true},
            {B::LAPIS_ORE, B::DEEPSLATE_LAPIS_ORE, 2, 7, -32, 32, true}, {B::LAPIS_ORE, B::DEEPSLATE_LAPIS_ORE, 4, 7, -64, 64, false},
            {B::DIAMOND_ORE, B::DEEPSLATE_DIAMOND_ORE, 7, 4, -64, 16, true}, {B::DIAMOND_ORE, B::DEEPSLATE_DIAMOND_ORE, 4, 8, -64, -4, false},
        };
    }
    for (size_t i = 0; i < cfg.size(); ++i) {
        const OreCfg& o = cfg[i];
        bool soft = o.ore == B::DIRT || o.ore == B::GRAVEL || o.ore == B::GRANITE || o.ore == B::DIORITE || o.ore == B::ANDESITE || o.ore == B::TUFF;
        for (int a = 0; a < o.count; ++a) {
            Rng r(featureSeed(seed, f.m.cx, f.m.cz, 1000 + (int64_t)i * 64 + a));
            int x = f.mx + r.nextInt(16), z = f.mz + r.nextInt(16);
            int y = o.tri ? (o.minY + o.maxY) / 2 + (int)((r.nextDouble() - r.nextDouble()) * (o.maxY - o.minY) / 2.0) : r.range(o.minY, o.maxY);
            placeBlob(c, r, x, y, z, o.size, o.ore, o.deep, soft);
        }
    }
    // emeralds: mountain biomes only, single blocks
    Biome cb = (Biome)f.m.biome[8 * 16 + 8];
    if (cb == Biome::Mountains || cb == Biome::SnowyPeaks) {
        for (int a = 0; a < 6; ++a) {
            Rng r(featureSeed(seed, f.m.cx, f.m.cz, 5000 + a));
            int x = f.mx + r.nextInt(16), z = f.mz + r.nextInt(16), y = r.range(-16, 100);
            BlockId id = stateId(c.get(x, y, z));
            if (c.in(x, z) && (id == B::STONE || id == B::DEEPSLATE)) c.put(x, y, z, makeState(id == B::DEEPSLATE ? B::DEEPSLATE_EMERALD_ORE : B::EMERALD_ORE), Mode::Force);
        }
    }
}

// ---------------- trees ----------------
void leafDisc(Ctx& c, Rng& r, int cx, int y, int cz, int rad, BlockId leaf, bool trimCorners) {
    for (int dx = -rad; dx <= rad; ++dx) for (int dz = -rad; dz <= rad; ++dz) {
        if (trimCorners && std::abs(dx) == rad && std::abs(dz) == rad && (rad == 1 || r.nextBool())) continue;
        c.put(cx + dx, y, cz + dz, makeState(leaf), Mode::Leaf);
    }
}
void treeOak(Ctx& c, Rng& r, int x, int y0, int z, BlockId log, BlockId leaf, int h) {
    int top = y0 + h;
    leafDisc(c, r, x, top - 2, z, 2, leaf, true); leafDisc(c, r, x, top - 1, z, 2, leaf, true);
    leafDisc(c, r, x, top, z, 1, leaf, true);
    for (int d = 0; d < 5; ++d) { static const int px[5] = {0, 1, -1, 0, 0}, pz[5] = {0, 0, 0, 1, -1}; c.put(x + px[d], top + 1, z + pz[d], makeState(leaf), Mode::Leaf); }
    for (int y = y0 + 1; y <= top; ++y) c.put(x, y, z, makeState(log), Mode::Log);
}
void treeSpruce(Ctx& c, Rng& r, int x, int y0, int z, int h) {
    int top = y0 + h;
    for (int y = top + 1; y >= y0 + 3; --y) {
        int dy = top + 1 - y; int rad = (dy + 1) / 2; if (dy % 2 == 0) rad = std::max(0, rad - 1); rad = std::min(rad, 3);
        for (int dx = -rad; dx <= rad; ++dx) for (int dz = -rad; dz <= rad; ++dz) {
            if (rad >= 2 && std::abs(dx) == rad && std::abs(dz) == rad) continue;
            c.put(x + dx, y, z + dz, makeState(B::SPRUCE_LEAVES), Mode::Leaf);
        }
    }
    (void)r;
    for (int y = y0 + 1; y <= top; ++y) c.put(x, y, z, makeState(B::SPRUCE_LOG), Mode::Log);
}
void treeJungle(Ctx& c, Rng& r, int x, int y0, int z, int h) {
    int top = y0 + h;
    for (int dy = -2; dy <= 1; ++dy) leafDisc(c, r, x, top + dy, z, dy <= -1 ? 3 : (dy == 0 ? 2 : 1), B::JUNGLE_LEAVES, true);
    for (int y = y0 + 1; y <= top; ++y) c.put(x, y, z, makeState(B::JUNGLE_LOG), Mode::Log);
}
void bush(Ctx& c, Rng& r, int x, int y0, int z) {
    c.put(x, y0 + 1, z, makeState(B::JUNGLE_LOG), Mode::Log);
    leafDisc(c, r, x, y0 + 1, z, 2, B::OAK_LEAVES, true); leafDisc(c, r, x, y0 + 2, z, 1, B::OAK_LEAVES, false);
}
void treeAcacia(Ctx& c, Rng& r, int x, int y0, int z) {
    int h = 5 + r.nextInt(2); int dxs[4] = {1, -1, 0, 0}, dzs[4] = {0, 0, 1, -1}; int d = r.nextInt(4);
    int tx = x, tz = z, y = y0 + 1;
    for (int i = 0; i < h; ++i, ++y) { if (i >= 3) { tx += dxs[d]; tz += dzs[d]; } c.put(tx, y, tz, makeState(B::ACACIA_LOG), Mode::Log); }
    leafDisc(c, r, tx, y - 1, tz, 2, B::ACACIA_LEAVES, true); leafDisc(c, r, tx, y, tz, 1, B::ACACIA_LEAVES, false);
}
void giantMushroom(Ctx& c, Rng& r, int x, int y0, int z) {
    bool red = r.nextBool(); int h = 4 + r.nextInt(3); BlockId cap = red ? B::RED_MUSHROOM_BLOCK : B::BROWN_MUSHROOM_BLOCK;
    for (int y = y0 + 1; y <= y0 + h; ++y) c.put(x, y, z, makeState(B::MUSHROOM_STEM), Mode::Log);
    int top = y0 + h;
    if (red) { for (int dx = -2; dx <= 2; ++dx) for (int dz = -2; dz <= 2; ++dz) { if (std::abs(dx) == 2 && std::abs(dz) == 2) continue; c.put(x + dx, top - 1 + (std::abs(dx) == 2 || std::abs(dz) == 2 ? 0 : 1), z + dz, makeState(cap), Mode::Log); } }
    else for (int dx = -3; dx <= 3; ++dx) for (int dz = -3; dz <= 3; ++dz) { if (std::abs(dx) == 3 && std::abs(dz) == 3) continue; c.put(x + dx, top + 1, z + dz, makeState(cap), Mode::Log); }
}

bool isSoil(BlockId id) { return id == B::GRASS_BLOCK || id == B::DIRT || id == B::PODZOL || id == B::MYCELIUM; }

void runSurface(Ctx& c, const Feature& f, uint64_t seed) {
    const Chunk& m = f.m;
    Biome cb = (Biome)m.biome[8 * 16 + 8];
    int trees = 0;
    Rng cr(featureSeed(seed, m.cx, m.cz, 77));
    switch (cb) {
        case Biome::Forest: trees = 6 + cr.nextInt(4); break;
        case Biome::BirchForest: trees = 5 + cr.nextInt(4); break;
        case Biome::Taiga: trees = 6 + cr.nextInt(5); break;
        case Biome::SnowyTaiga: trees = 5 + cr.nextInt(4); break;
        case Biome::Jungle: trees = 12 + cr.nextInt(6); break;
        case Biome::Savanna: trees = cr.nextInt(3); break;
        case Biome::Swamp: trees = 2 + cr.nextInt(3); break;
        case Biome::Plains: trees = cr.chance(0.08) ? 1 : 0; break;
        case Biome::Mountains: trees = cr.chance(0.5) ? 1 + cr.nextInt(2) : 0; break;
        case Biome::SnowyPlains: trees = cr.chance(0.06) ? 1 : 0; break;
        case Biome::MushroomFields: trees = cr.chance(0.5) ? 1 : 0; break;
        default: break;
    }
    for (int i = 0; i < trees + 4; ++i) {        // extra attempts are filtered by biome of the actual column
        Rng r(featureSeed(seed, m.cx, m.cz, 100 + i));
        int lx = r.nextInt(16), lz = r.nextInt(16), col = lz * 16 + lx;
        if (i >= trees && !(cb == Biome::Jungle)) continue;
        int x = f.mx + lx, z = f.mz + lz, y0 = m.terrainHeight[col];
        BlockId soil = stateId(m.surfaceState[col]); Biome b = (Biome)m.biome[col];
        if (y0 < SEA_LEVEL - 1 || !isSoil(soil)) continue;
        if (!(x >= c.bx - 5 && x < c.bx + 21 && z >= c.bz - 5 && z < c.bz + 21)) continue;
        double u = r.nextDouble();
        switch (b) {
            case Biome::Forest: if (u < 0.2) treeOak(c, r, x, y0, z, B::BIRCH_LOG, B::BIRCH_LEAVES, 5 + r.nextInt(3)); else treeOak(c, r, x, y0, z, B::OAK_LOG, B::OAK_LEAVES, 4 + r.nextInt(3)); break;
            case Biome::BirchForest: treeOak(c, r, x, y0, z, B::BIRCH_LOG, B::BIRCH_LEAVES, 5 + r.nextInt(3)); break;
            case Biome::Taiga: case Biome::SnowyTaiga: treeSpruce(c, r, x, y0, z, 6 + r.nextInt(5)); break;
            case Biome::Jungle: if (u < 0.55) treeJungle(c, r, x, y0, z, 8 + r.nextInt(6)); else if (u < 0.8) bush(c, r, x, y0, z); else treeOak(c, r, x, y0, z, B::OAK_LOG, B::OAK_LEAVES, 4 + r.nextInt(3)); break;
            case Biome::Savanna: if (u < 0.7) treeAcacia(c, r, x, y0, z); else treeOak(c, r, x, y0, z, B::OAK_LOG, B::OAK_LEAVES, 4 + r.nextInt(2)); break;
            case Biome::Swamp: treeOak(c, r, x, y0, z, B::OAK_LOG, B::OAK_LEAVES, 5 + r.nextInt(3)); break;
            case Biome::Mountains: treeSpruce(c, r, x, y0, z, 6 + r.nextInt(4)); break;
            case Biome::SnowyPlains: treeSpruce(c, r, x, y0, z, 5 + r.nextInt(3)); break;
            case Biome::Plains: treeOak(c, r, x, y0, z, B::OAK_LOG, B::OAK_LEAVES, 4 + r.nextInt(3)); break;
            case Biome::MushroomFields: giantMushroom(c, r, x, y0, z); break;
            default: break;
        }
    }
    // ---- ground cover, flowers, cacti, sugar cane, lily pads: one attempt stream per chunk
    for (int i = 0; i < 40; ++i) {
        Rng r(featureSeed(seed, m.cx, m.cz, 300 + i));
        int lx = r.nextInt(16), lz = r.nextInt(16), col = lz * 16 + lx;
        int x = f.mx + lx, z = f.mz + lz;
        if (!c.in(x, z)) continue;
        int ts = m.terrainHeight[col]; Biome b = (Biome)m.biome[col]; BlockId soil = stateId(m.surfaceState[col]);
        double u = r.nextDouble(), v = r.nextDouble();
        if (ts < SEA_LEVEL - 1) {                                      // under water
            if (b == Biome::Swamp && ts < SEA_LEVEL - 2 && u < 0.35) c.put(x, SEA_LEVEL, z, makeState(B::LILY_PAD), Mode::OnlyAir);
            continue;
        }
        int y = ts + 1;
        if (soil == B::GRASS_BLOCK || soil == B::PODZOL) {
            BlockId p = 0;
            switch (b) {
                case Biome::Plains: if (i < 24) p = u < 0.65 ? B::GRASS : 0; else if (i < 28) { static const BlockId fl[] = {B::DANDELION, B::POPPY, B::OXEYE_DAISY, B::AZURE_BLUET, B::RED_TULIP, B::ORANGE_TULIP, B::WHITE_TULIP, B::PINK_TULIP, B::CORNFLOWER}; p = fl[(int)(v * 9)]; } else if (i == 28 && u < 0.08) p = B::PUMPKIN; break;
                case Biome::Forest: case Biome::BirchForest: if (i < 20) p = u < 0.45 ? B::GRASS : (u < 0.55 ? B::FERN : 0); else if (i < 23 && u < 0.4) p = v < 0.5 ? B::POPPY : B::DANDELION; break;
                case Biome::Taiga: case Biome::SnowyTaiga: if (i < 24) p = u < 0.4 ? B::FERN : (u < 0.8 ? B::GRASS : 0); break;
                case Biome::Jungle: if (i < 34) p = u < 0.5 ? B::FERN : B::GRASS; else if (u < 0.1) p = B::MELON; break;
                case Biome::Savanna: if (i < 30) p = u < 0.75 ? B::GRASS : 0; break;
                case Biome::Swamp: if (i < 20) p = u < 0.5 ? B::GRASS : 0; else if (i < 24 && u < 0.5) p = B::BLUE_ORCHID; break;
                case Biome::Mountains: if (i < 10 && u < 0.4) p = B::GRASS; break;
                case Biome::MushroomFields: break;
                default: break;
            }
            if (p) c.put(x, y, z, makeState(p), Mode::OnlyAir);
        } else if (soil == B::MYCELIUM) { if (u < 0.08) c.put(x, y, z, makeState(v < 0.5 ? B::RED_MUSHROOM : B::BROWN_MUSHROOM), Mode::OnlyAir); }
        else if (soil == B::SAND || soil == B::RED_SAND) {
            if (b == Biome::Desert || b == Biome::Badlands) {
                if (i < 6 && u < (b == Biome::Desert ? 0.5 : 0.25)) { int hgt = 1 + r.nextInt(3); for (int k = 0; k < hgt; ++k) c.put(x, y + k, z, makeState(B::CACTUS), Mode::OnlyAir); }
                else if (i < 16 && u < 0.5) c.put(x, y, z, makeState(B::DEAD_BUSH), Mode::OnlyAir);
            }
        }
        // sugar cane beside water (checked against terrain heights of the same chunk only)
        if (ts <= SEA_LEVEL && (soil == B::SAND || soil == B::GRASS_BLOCK || soil == B::DIRT) && i < 12 && u < 0.5 && lx > 0 && lx < 15 && lz > 0 && lz < 15) {
            bool water = false; for (int d : HORIZ_DIRS) { int nc = (lz + DIR_DZ[d]) * 16 + lx + DIR_DX[d]; if (ts <= SEA_LEVEL - 1 && m.terrainHeight[nc] < ts) water = true; }
            if (water) { int hgt = 1 + r.nextInt(3); for (int k = 0; k < hgt; ++k) c.put(x, y + k, z, makeState(B::SUGAR_CANE), Mode::OnlyAir); }
        }
    }
}

// ---------------- dungeons ----------------
void runDungeons(Ctx& c, const Feature& f, uint64_t seed) {
    const Chunk& m = f.m;
    for (int a = 0; a < 4; ++a) {
        Rng r(featureSeed(seed, m.cx, m.cz, 9000 + a));
        if (!r.chance(0.4)) continue;
        int x = f.mx + r.nextInt(16), z = f.mz + r.nextInt(16), y = r.range(-48, 56);
        int xr = r.range(2, 3), zr = r.range(2, 3);
        if (y + 8 > m.terrainHeight[(z - f.mz) * 16 + (x - f.mx)]) continue;
        if (!(x + 5 >= c.bx && x - 5 < c.bx + 16 && z + 5 >= c.bz && z - 5 < c.bz + 16)) continue;
        for (int dx = -xr - 1; dx <= xr + 1; ++dx) for (int dz = -zr - 1; dz <= zr + 1; ++dz) for (int dy = -1; dy <= 4; ++dy) {
            bool wall = dx == -xr - 1 || dx == xr + 1 || dz == -zr - 1 || dz == zr + 1 || dy == -1 || dy == 4;
            if (wall) c.put(x + dx, y + dy, z + dz, makeState(dy == -1 && r.nextInt(4) != 0 ? B::MOSSY_COBBLESTONE : B::COBBLESTONE), Mode::Force);
            else c.put(x + dx, y + dy, z + dz, 0, Mode::Force);
        }
        c.put(x, y, z, makeState(B::SPAWNER), Mode::Force);
        for (int k = 0; k < 2; ++k) {
            int cx = k == 0 ? x - xr : x + xr, cz = k == 0 ? z : z + (zr > 2 ? 1 : 0);
            c.put(cx, y, cz, makeState(B::CHEST, k == 0 ? 3 : 2), Mode::Force);
        }
    }
}

} // namespace

void Decorator::decorate(Chunk& target, Chunk* const nb[9]) const {
    Ctx ctx(target);
    for (int i = 0; i < 9; ++i) {
        const Chunk* m = nb[i];
        Feature f{*m, m->cx * 16, m->cz * 16};
        runOres(ctx, f, seed_);
        runDungeons(ctx, f, seed_);
        runSurface(ctx, f, seed_);
    }
    target.status.store(ChunkStatus::Decorated);
}

} // namespace mc
