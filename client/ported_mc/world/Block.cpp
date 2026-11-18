#include "Block.h"
#include <cstdio>
#include <unordered_map>

namespace mc {

std::vector<BlockInfo> Blocks::table_;
std::vector<std::string> Blocks::texNames_;
static std::unordered_map<std::string, int> g_texIndex;
static std::unordered_map<std::string, BlockId> g_byName;

int Blocks::textureIndex(const std::string& name) {
    auto it = g_texIndex.find(name);
    if (it != g_texIndex.end()) return it->second;
    int idx = (int)texNames_.size();
    texNames_.push_back(name);
    g_texIndex[name] = idx;
    return idx;
}

BlockId Blocks::byName(const std::string& name) {
    auto it = g_byName.find(name);
    return it == g_byName.end() ? 0 : it->second;
}

bool Blocks::getBox(BlockState s, float out[6]) {
    const BlockInfo& b = get(stateId(s));
    if (b.shape != Shape::Box) return false;
    for (int i = 0; i < 6; ++i) out[i] = b.box[i];
    int m = stateMeta(s);
    if (b.id == B::SNOW_LAYER) out[4] = (float)(m + 1) / 8.0f;
    else if (b.name.size() > 5 && b.name.compare(b.name.size() - 5, 5, "_slab") == 0) { if (m & 1) { out[1] = 0.5f; out[4] = 1.0f; } else { out[1] = 0.0f; out[4] = 0.5f; } }
    return true;
}

bool Blocks::getCollisionBox(BlockState s, float out[6]) {
    const BlockInfo& b = get(stateId(s));
    if (!b.solid) return false;
    if (b.shape == Shape::Cube) { out[0] = out[1] = out[2] = 0; out[3] = out[4] = out[5] = 1; return true; }
    if (b.shape != Shape::Box) return false;
    getBox(s, out);
    if (b.id == B::SNOW_LAYER) { int m = stateMeta(s); if (m == 0) return false; out[4] = (float)m / 8.0f; }
    if (b.id == B::LILY_PAD) return false;
    return true;
}

namespace {

struct D {
    BlockInfo& b;
    static int T(const char* n) { return Blocks::textureIndex(std::string("block/") + n); }
    D& all(const char* n) { int t = T(n); for (auto& x : b.tex) x = (uint16_t)t; return *this; }
    D& tsb(const char* top, const char* side, const char* bottom) {
        b.tex[DIR_UP] = (uint16_t)T(top); b.tex[DIR_DOWN] = (uint16_t)T(bottom);
        int s = T(side); for (int d : HORIZ_DIRS) b.tex[d] = (uint16_t)s; return *this;
    }
    D& ends(const char* top, const char* side) { return tsb(top, side, top); }
    D& front(const char* n) { b.frontTex = (uint16_t)T(n); b.hasFacing = true; return *this; }
    D& hard(float h) { b.hardness = h; return *this; }
    D& pick(int tier = 0) { b.tool = ToolType::Pickaxe; b.toolTier = tier; b.requiresTool = true; return *this; }
    D& axe() { b.tool = ToolType::Axe; return *this; }
    D& shovel() { b.tool = ToolType::Shovel; return *this; }
    D& mat(Material m) { b.material = m; return *this; }
    D& snd(SoundType s) { b.sound = s; return *this; }
    D& layer(RenderLayer l) { b.layer = l; return *this; }
    D& clear() { b.opaque = false; b.lightOpacity = 0; return *this; }
    D& opac(int o) { b.opaque = false; b.lightOpacity = (uint8_t)o; return *this; }
    D& light(int e) { b.emission = (uint8_t)e; return *this; }
    D& drop(const char* item, int mn = 1, int mx = 1) { b.dropItem = item; b.dropMin = mn; b.dropMax = mx; return *this; }
    D& tint(Tint t) { b.tint = t; return *this; }
    D& fixed(uint32_t c) { b.fixedTint = c; return *this; }
    D& replaceable() { b.replaceable = true; return *this; }
    D& cullSame() { b.cullSame = true; return *this; }
    D& gravity() { b.gravity = true; return *this; }
    D& boxed(float x0, float y0, float z0, float x1, float y1, float z1) { b.shape = Shape::Box; b.opaque = false; b.box[0] = x0; b.box[1] = y0; b.box[2] = z0; b.box[3] = x1; b.box[4] = y1; b.box[5] = z1; return *this; }
};

} // namespace

void Blocks::init() {
    if (!table_.empty()) return;
    table_.reserve(1024);
    auto add = [&](const char* name) -> D {
        table_.emplace_back();
        BlockInfo& b = table_.back();
        b.id = (BlockId)(table_.size() - 1); b.name = name; g_byName[name] = b.id;
        return D{b};
    };
    auto rock = [&](const char* n, const char* t, float h, int tier = 0) { return add(n).all(t).hard(h).pick(tier).mat(Material::Stone).snd(SoundType::Stone); };
    auto earth = [&](const char* n, const char* t, float h) { return add(n).all(t).hard(h).shovel().mat(Material::Earth).snd(SoundType::Gravel); };
    auto planks = [&](const char* n, const char* t) { return add(n).all(t).hard(2.0f).axe().mat(Material::Wood).snd(SoundType::Wood); };
    auto log = [&](const char* n, const char* top, const char* side) { return add(n).ends(top, side).hard(2.0f).axe().mat(Material::Wood).snd(SoundType::Wood); };
    auto leaves = [&](const char* n, const char* t) { return add(n).all(t).hard(0.2f).layer(RenderLayer::Cutout).opac(1).cullSame().mat(Material::Leaves).snd(SoundType::Grass).tint(Tint::Foliage).drop("-"); };
    auto plant = [&](const char* n, const char* t) {
        D d = add(n).all(t).hard(0.0f).mat(Material::Plant).snd(SoundType::Grass).layer(RenderLayer::Cutout).clear().drop("-");
        d.b.shape = Shape::Cross; d.b.solid = false; d.b.plant = true; return d;
    };
    auto flower = [&](const char* n, const char* t) { return plant(n, t).drop(""); };
    auto ore = [&](const char* n, float h, int tier, const char* dropItem, int mn = 1, int mx = 1) { return rock(n, n, h, tier).drop(dropItem, mn, mx); };
    auto wool = [&](const char* n) { return add(n).all(n).hard(0.8f).mat(Material::Cloth).snd(SoundType::Cloth); };
    auto slab = [&](const char* n, const char* t, bool wooden) {
        D d = add(n).all(t).hard(wooden ? 2.0f : 2.0f).mat(wooden ? Material::Wood : Material::Stone).snd(wooden ? SoundType::Wood : SoundType::Stone).boxed(0, 0, 0, 1, 0.5f, 1).opac(15);
        if (wooden) d.axe(); else d.pick();
        return d;
    };

    // ---- 0: air
    { D d = add("air"); d.b.shape = Shape::None; d.b.solid = false; d.b.opaque = false; d.b.lightOpacity = 0; d.b.replaceable = true; d.b.targetable = false; d.b.hardness = 0; d.b.sound = SoundType::Silent; d.b.material = Material::Air; d.b.dropItem = "-"; }

    // ---- stone family
    rock("stone", "stone", 1.5f).drop("cobblestone");
    rock("granite", "granite", 1.5f); rock("diorite", "diorite", 1.5f); rock("andesite", "andesite", 1.5f);
    rock("cobblestone", "cobblestone", 2.0f); rock("mossy_cobblestone", "mossy_cobblestone", 2.0f);
    rock("deepslate", "deepslate", 3.0f).drop("cobbled_deepslate"); rock("cobbled_deepslate", "cobbled_deepslate", 3.5f);
    rock("tuff", "tuff", 1.5f);
    rock("bedrock", "bedrock", -1.0f);
    rock("stone_bricks", "stone_bricks", 1.5f); rock("mossy_stone_bricks", "mossy_stone_bricks", 1.5f); rock("cracked_stone_bricks", "cracked_stone_bricks", 1.5f);
    rock("bricks", "bricks", 2.0f); rock("obsidian", "obsidian", 50.0f, 3); rock("crying_obsidian", "crying_obsidian", 50.0f, 3).light(10);
    rock("netherrack", "netherrack", 0.4f); rock("nether_bricks", "nether_bricks", 2.0f); rock("end_stone", "end_stone", 3.0f);
    rock("glowstone", "glowstone", 0.3f).light(15).drop("glowstone_dust", 2, 4).snd(SoundType::Glass);
    rock("magma_block", "magma", 0.5f).light(3);
    rock("prismarine", "prismarine", 1.5f); rock("dark_prismarine", "dark_prismarine", 1.5f); rock("sea_lantern", "sea_lantern", 0.3f).light(15).snd(SoundType::Glass).drop("prismarine_crystals", 2, 3);
    rock("sandstone", "sandstone", 0.8f).tsb("sandstone_top", "sandstone", "sandstone_bottom");
    rock("red_sandstone", "red_sandstone", 0.8f).tsb("red_sandstone_top", "red_sandstone", "red_sandstone_bottom");
    rock("terracotta", "terracotta", 1.25f);
    rock("orange_terracotta", "orange_terracotta", 1.25f); rock("yellow_terracotta", "yellow_terracotta", 1.25f); rock("brown_terracotta", "brown_terracotta", 1.25f);
    rock("red_terracotta", "red_terracotta", 1.25f); rock("white_terracotta", "white_terracotta", 1.25f); rock("light_gray_terracotta", "light_gray_terracotta", 1.25f);
    // ---- earth
    earth("dirt", "dirt", 0.5f);
    { D d = earth("grass_block", "dirt", 0.6f).drop("dirt"); d.b.tex[DIR_UP] = (uint16_t)D::T("grass_block_top"); for (int s : HORIZ_DIRS) d.b.tex[s] = (uint16_t)D::T("grass_block_side"); d.b.tint = Tint::Grass; d.b.tintSideMask = true; d.b.sound = SoundType::Grass; }
    { D d = earth("podzol", "dirt", 0.5f).drop("dirt"); d.b.tex[DIR_UP] = (uint16_t)D::T("podzol_top"); for (int s : HORIZ_DIRS) d.b.tex[s] = (uint16_t)D::T("podzol_side"); }
    { D d = earth("mycelium", "dirt", 0.6f).drop("dirt"); d.b.tex[DIR_UP] = (uint16_t)D::T("mycelium_top"); for (int s : HORIZ_DIRS) d.b.tex[s] = (uint16_t)D::T("mycelium_side"); }
    earth("sand", "sand", 0.5f).gravity().mat(Material::Sand).snd(SoundType::Sand);
    earth("red_sand", "red_sand", 0.5f).gravity().mat(Material::Sand).snd(SoundType::Sand);
    earth("gravel", "gravel", 0.6f).gravity();
    earth("clay", "clay", 0.6f).drop("clay_ball", 4, 4).snd(SoundType::Gravel);
    earth("soul_sand", "soul_sand", 0.5f).snd(SoundType::Sand);
    earth("farmland", "dirt", 0.6f).drop("dirt");
    earth("moss_block", "moss_block", 0.1f).snd(SoundType::Grass);
    // ---- snow / ice
    add("snow").all("snow").hard(0.1f).shovel().mat(Material::Snow).snd(SoundType::Snow).boxed(0, 0, 0, 1, 0.125f, 1).opac(1).drop("snowball");
    add("snow_block").all("snow").hard(0.2f).shovel().mat(Material::Snow).snd(SoundType::Snow).drop("snowball", 4, 4);
    add("ice").all("ice").hard(0.5f).mat(Material::Ice).snd(SoundType::Glass).layer(RenderLayer::Translucent).opac(1).cullSame().drop("-");
    add("packed_ice").all("packed_ice").hard(0.5f).mat(Material::Ice).snd(SoundType::Glass);
    add("blue_ice").all("blue_ice").hard(2.8f).mat(Material::Ice).snd(SoundType::Glass);
    // ---- fluids
    { D d = add("water").all("water_still").tint(Tint::Water).layer(RenderLayer::Translucent).mat(Material::Water).snd(SoundType::Water).hard(-1.0f).opac(1).cullSame().replaceable().drop("-"); d.b.shape = Shape::Fluid; d.b.solid = false; d.b.fluid = true; d.b.targetable = false; }
    { D d = add("lava").all("lava_still").mat(Material::Lava).snd(SoundType::Lava).hard(-1.0f).opac(2).light(15).replaceable().drop("-"); d.b.shape = Shape::Fluid; d.b.solid = false; d.b.fluid = true; d.b.targetable = false; d.b.cullSame = true; }
    // ---- ores
    ore("coal_ore", 3.0f, 0, "coal"); ore("iron_ore", 3.0f, 1, "raw_iron"); ore("copper_ore", 3.0f, 1, "raw_copper", 2, 5); ore("gold_ore", 3.0f, 2, "raw_gold");
    ore("redstone_ore", 3.0f, 2, "redstone", 4, 5); ore("lapis_ore", 3.0f, 1, "lapis_lazuli", 4, 9); ore("diamond_ore", 3.0f, 2, "diamond"); ore("emerald_ore", 3.0f, 2, "emerald");
    ore("deepslate_coal_ore", 4.5f, 0, "coal"); ore("deepslate_iron_ore", 4.5f, 1, "raw_iron"); ore("deepslate_copper_ore", 4.5f, 1, "raw_copper", 2, 5); ore("deepslate_gold_ore", 4.5f, 2, "raw_gold");
    ore("deepslate_redstone_ore", 4.5f, 2, "redstone", 4, 5); ore("deepslate_lapis_ore", 4.5f, 1, "lapis_lazuli", 4, 9); ore("deepslate_diamond_ore", 4.5f, 2, "diamond"); ore("deepslate_emerald_ore", 4.5f, 2, "emerald");
    ore("nether_quartz_ore", 3.0f, 0, "quartz");
    // ---- wood
    log("oak_log", "oak_log_top", "oak_log"); log("spruce_log", "spruce_log_top", "spruce_log"); log("birch_log", "birch_log_top", "birch_log");
    log("jungle_log", "jungle_log_top", "jungle_log"); log("acacia_log", "acacia_log_top", "acacia_log"); log("dark_oak_log", "dark_oak_log_top", "dark_oak_log");
    planks("oak_planks", "oak_planks"); planks("spruce_planks", "spruce_planks"); planks("birch_planks", "birch_planks"); planks("jungle_planks", "jungle_planks");
    planks("acacia_planks", "acacia_planks"); planks("dark_oak_planks", "dark_oak_planks");
    leaves("oak_leaves", "oak_leaves"); leaves("spruce_leaves", "spruce_leaves").tint(Tint::None).fixed(0x619961); leaves("birch_leaves", "birch_leaves").tint(Tint::None).fixed(0x80A755);
    leaves("jungle_leaves", "jungle_leaves"); leaves("acacia_leaves", "acacia_leaves"); leaves("dark_oak_leaves", "dark_oak_leaves");
    plant("oak_sapling", "oak_sapling"); plant("spruce_sapling", "spruce_sapling"); plant("birch_sapling", "birch_sapling");
    plant("jungle_sapling", "jungle_sapling"); plant("acacia_sapling", "acacia_sapling"); plant("dark_oak_sapling", "dark_oak_sapling");
    // ---- man-made
    add("glass").all("glass").hard(0.3f).mat(Material::Glass).snd(SoundType::Glass).layer(RenderLayer::Translucent).clear().cullSame().drop("-");
    add("bookshelf").tsb("oak_planks", "bookshelf", "oak_planks").hard(1.5f).axe().mat(Material::Wood).snd(SoundType::Wood).drop("book", 3, 3);
    { D d = add("crafting_table").hard(2.5f).axe().mat(Material::Wood).snd(SoundType::Wood); d.tsb("crafting_table_top", "crafting_table_side", "oak_planks"); d.front("crafting_table_front"); }
    { D d = add("furnace").hard(3.5f).pick().mat(Material::Stone); d.tsb("furnace_top", "furnace_side", "furnace_top"); d.front("furnace_front"); }
    { D d = add("chest").tsb("chest_top", "chest_side", "chest_top").hard(2.5f).axe().mat(Material::Wood).snd(SoundType::Wood).boxed(0.0625f, 0, 0.0625f, 0.9375f, 0.875f, 0.9375f); d.front("chest_front"); }
    { D d = add("torch").all("torch").hard(0.0f).mat(Material::Misc).snd(SoundType::Wood).layer(RenderLayer::Cutout).clear().light(14).boxed(0.4375f, 0, 0.4375f, 0.5625f, 0.625f, 0.5625f); d.b.solid = false; }
    add("tnt").tsb("tnt_top", "tnt_side", "tnt_bottom").hard(0.0f).mat(Material::Misc).snd(SoundType::Grass);
    add("spawner").all("spawner").hard(5.0f).pick().mat(Material::Metal).layer(RenderLayer::Cutout).opac(0).drop("-");
    add("sponge").all("sponge").hard(0.6f).mat(Material::Organic).snd(SoundType::Grass);
    { D d = add("pumpkin").tsb("pumpkin_top", "pumpkin_side", "pumpkin_top").hard(1.0f).axe().mat(Material::Organic).snd(SoundType::Wood); }
    { D d = add("carved_pumpkin").tsb("pumpkin_top", "pumpkin_side", "pumpkin_top").hard(1.0f).axe().mat(Material::Organic).snd(SoundType::Wood); d.front("carved_pumpkin"); }
    { D d = add("jack_o_lantern").tsb("pumpkin_top", "pumpkin_side", "pumpkin_top").hard(1.0f).axe().mat(Material::Organic).snd(SoundType::Wood).light(15); d.front("jack_o_lantern"); }
    add("melon").tsb("melon_top", "melon_side", "melon_top").hard(1.0f).axe().mat(Material::Organic).snd(SoundType::Wood).drop("melon_slice", 3, 7);
    add("hay_block").tsb("hay_block_top", "hay_block_side", "hay_block_top").hard(0.5f).mat(Material::Organic).snd(SoundType::Grass);
    add("cactus").tsb("cactus_top", "cactus_side", "cactus_bottom").hard(0.4f).mat(Material::Organic).snd(SoundType::Cloth).boxed(0.0625f, 0, 0.0625f, 0.9375f, 1, 0.9375f);
    { D d = plant("sugar_cane", "sugar_cane").tint(Tint::Grass).drop(""); }
    add("lily_pad").all("lily_pad").hard(0.0f).mat(Material::Plant).snd(SoundType::Grass).layer(RenderLayer::Cutout).clear().boxed(0, 0, 0, 1, 0.015625f, 1).fixed(0x208030).drop("");
    { D d = plant("cobweb", "cobweb").hard(4.0f); d.b.tool = ToolType::Sword; d.b.lightOpacity = 1; d.b.plant = false; d.b.dropItem = "string"; }
    plant("dead_bush", "dead_bush").replaceable();
    { D d = plant("grass", "grass").replaceable().tint(Tint::Grass); }
    { D d = plant("fern", "fern").replaceable().tint(Tint::Grass); }
    flower("dandelion", "dandelion"); flower("poppy", "poppy"); flower("blue_orchid", "blue_orchid"); flower("allium", "allium"); flower("azure_bluet", "azure_bluet");
    flower("red_tulip", "red_tulip"); flower("orange_tulip", "orange_tulip"); flower("white_tulip", "white_tulip"); flower("pink_tulip", "pink_tulip");
    flower("oxeye_daisy", "oxeye_daisy"); flower("cornflower", "cornflower");
    flower("brown_mushroom", "brown_mushroom").light(1); flower("red_mushroom", "red_mushroom");
    add("brown_mushroom_block").all("brown_mushroom_block").hard(0.2f).axe().mat(Material::Wood).snd(SoundType::Wood).drop("-");
    add("red_mushroom_block").all("red_mushroom_block").hard(0.2f).axe().mat(Material::Wood).snd(SoundType::Wood).drop("-");
    add("mushroom_stem").all("mushroom_stem").hard(0.2f).axe().mat(Material::Wood).snd(SoundType::Wood).drop("-");
    // ---- storage / decorative blocks
    rock("coal_block", "coal_block", 5.0f); rock("iron_block", "iron_block", 5.0f, 1).snd(SoundType::Metal); rock("gold_block", "gold_block", 3.0f, 2).snd(SoundType::Metal);
    rock("diamond_block", "diamond_block", 5.0f, 2).snd(SoundType::Metal); rock("emerald_block", "emerald_block", 5.0f, 2).snd(SoundType::Metal);
    rock("lapis_block", "lapis_block", 3.0f, 1); rock("redstone_block", "redstone_block", 5.0f);
    slab("oak_slab", "oak_planks", true); slab("cobblestone_slab", "cobblestone", false); slab("stone_slab", "stone", false);
    slab("stone_brick_slab", "stone_bricks", false); slab("sandstone_slab", "sandstone", false);
    wool("white_wool"); wool("red_wool"); wool("blue_wool"); wool("green_wool"); wool("yellow_wool");
    { D d = add("ladder").all("ladder").hard(0.4f).axe().mat(Material::Wood).snd(SoundType::Wood).layer(RenderLayer::Cutout).clear().boxed(0, 0, 0.8125f, 1, 1, 1); d.b.solid = false; }

    for (auto& b : table_) {
        // slabs / chests / torches etc. keep a lightOpacity chosen above; cubes default to fully opaque.
        if (b.shape == Shape::Cube && b.layer == RenderLayer::Opaque) { b.opaque = true; b.lightOpacity = 15; }
        if (b.id == 0) { b.opaque = false; b.lightOpacity = 0; }
    }
    // Bind the C++ id constants.
#define X(ID, NAME) { auto it = g_byName.find(NAME); if (it == g_byName.end()) std::fprintf(stderr, "[blocks] missing block '%s'\n", NAME); else B::ID = it->second; }
    MC_BLOCK_IDS(X)
#undef X
    // Fix-ups that depend on final ids.
    table_[B::SNOW_LAYER].opaque = false;
    table_[B::WATER].lightOpacity = 1; table_[B::LAVA].lightOpacity = 2;
}

} // namespace mc
