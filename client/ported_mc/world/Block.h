#pragma once
// Block registry: every block type with its physical, visual and sound properties.
#include "../core/Types.h"
#include <string>
#include <vector>

namespace mc {

using BlockId = uint16_t;
using BlockState = uint16_t;                     // (id << 4) | meta
inline BlockId stateId(BlockState s) { return (BlockId)(s >> 4); }
inline int stateMeta(BlockState s) { return s & 15; }
inline BlockState makeState(BlockId id, int meta = 0) { return (BlockState)((id << 4) | (meta & 15)); }

enum class Shape : uint8_t { None, Cube, Cross, Box, Fluid };
enum class RenderLayer : uint8_t { Opaque, Cutout, Translucent };
enum class Material : uint8_t { Air, Stone, Earth, Wood, Plant, Leaves, Sand, Glass, Water, Lava, Ice, Snow, Metal, Cloth, Organic, Misc };
enum class ToolType : uint8_t { None, Pickaxe, Axe, Shovel, Hoe, Shears, Sword };
enum class SoundType : uint8_t { Stone, Wood, Gravel, Grass, Sand, Glass, Cloth, Snow, Metal, Water, Lava, Silent };
enum class Tint : uint8_t { None, Grass, Foliage, Water };

struct BlockInfo {
    BlockId id = 0;
    std::string name;
    Shape shape = Shape::Cube;
    RenderLayer layer = RenderLayer::Opaque;
    Material material = Material::Stone;
    ToolType tool = ToolType::None;
    int toolTier = 0;              // minimum tool tier to harvest (0 wood/hand, 1 stone, 2 iron, 3 diamond)
    bool requiresTool = false;     // drops nothing without the right tool
    float hardness = 1.0f;         // < 0 : unbreakable
    bool solid = true;             // has collision
    bool opaque = true;            // full opaque cube: hides neighbour faces
    uint8_t lightOpacity = 15;     // light attenuation (0 transparent .. 15 opaque)
    uint8_t emission = 0;          // emitted block light 0..15
    bool replaceable = false;      // placing a block here replaces it (air, grass, fluids)
    bool cullSame = false;         // hide faces between two identical blocks (glass, leaves, water)
    bool gravity = false;          // sand / gravel
    bool targetable = true;        // can be selected by the crosshair
    bool fluid = false;
    bool hasFacing = false;        // furnace / pumpkin: front texture follows meta (0=N 1=S 2=W 3=E)
    bool plant = false;            // needs support below, destroyed by fluids
    Tint tint = Tint::None;
    uint32_t fixedTint = 0xFFFFFF; // for birch/spruce leaves
    bool tintSideMask = false;     // grass block sides: tint only where texture alpha says so
    SoundType sound = SoundType::Stone;
    uint16_t tex[6] = {0, 0, 0, 0, 0, 0};   // texture layer per face: E W U D S N
    uint16_t frontTex = 0;
    float box[6] = {0, 0, 0, 1, 1, 1};
    std::string dropItem;          // "" = own item, "-" = nothing
    int dropMin = 1, dropMax = 1;
    bool isAir() const { return id == 0; }
};

class Blocks {
public:
    static void init();                                   // idempotent
    static const BlockInfo& get(BlockId id) { return table_[id]; }
    static BlockId byName(const std::string& name);       // 0 if unknown
    static int count() { return (int)table_.size(); }
    static const std::vector<std::string>& textureNames() { return texNames_; }
    static int textureIndex(const std::string& name);     // interns a texture name like "block/stone"
    // Geometry that depends on meta (slabs, snow layers). Returns false if block has no box.
    static bool getBox(BlockState s, float out[6]);
    static bool getCollisionBox(BlockState s, float out[6]);
private:
    static std::vector<BlockInfo> table_;
    static std::vector<std::string> texNames_;
};

inline const BlockInfo& blockOf(BlockState s) { return Blocks::get(stateId(s)); }

// Fluid helpers: meta 0 = source, 1..7 flowing (amount 8-meta), 8 = falling column.
inline int fluidAmount(BlockState s) {
    if (!Blocks::get(stateId(s)).fluid) return 0;
    int m = stateMeta(s); return (m == 0 || m >= 8) ? 8 : 8 - m;
}
inline bool isFluidSource(BlockState s) { return Blocks::get(stateId(s)).fluid && stateMeta(s) == 0; }

#define MC_BLOCK_IDS(X) \
    X(AIR,"air") X(STONE,"stone") X(GRANITE,"granite") X(DIORITE,"diorite") X(ANDESITE,"andesite") \
    X(COBBLESTONE,"cobblestone") X(MOSSY_COBBLESTONE,"mossy_cobblestone") X(DIRT,"dirt") X(GRASS_BLOCK,"grass_block") \
    X(PODZOL,"podzol") X(MYCELIUM,"mycelium") X(SAND,"sand") X(RED_SAND,"red_sand") X(GRAVEL,"gravel") X(CLAY,"clay") \
    X(BEDROCK,"bedrock") X(DEEPSLATE,"deepslate") X(TUFF,"tuff") X(SNOW_LAYER,"snow") X(SNOW_BLOCK,"snow_block") \
    X(ICE,"ice") X(PACKED_ICE,"packed_ice") X(OBSIDIAN,"obsidian") X(NETHERRACK,"netherrack") X(SOUL_SAND,"soul_sand") \
    X(GLOWSTONE,"glowstone") X(MAGMA_BLOCK,"magma_block") X(END_STONE,"end_stone") X(SANDSTONE,"sandstone") X(RED_SANDSTONE,"red_sandstone") \
    X(TERRACOTTA,"terracotta") X(ORANGE_TERRACOTTA,"orange_terracotta") X(YELLOW_TERRACOTTA,"yellow_terracotta") \
    X(BROWN_TERRACOTTA,"brown_terracotta") X(RED_TERRACOTTA,"red_terracotta") X(WHITE_TERRACOTTA,"white_terracotta") X(LIGHT_GRAY_TERRACOTTA,"light_gray_terracotta") \
    X(WATER,"water") X(LAVA,"lava") \
    X(COAL_ORE,"coal_ore") X(IRON_ORE,"iron_ore") X(COPPER_ORE,"copper_ore") X(GOLD_ORE,"gold_ore") X(REDSTONE_ORE,"redstone_ore") \
    X(LAPIS_ORE,"lapis_ore") X(DIAMOND_ORE,"diamond_ore") X(EMERALD_ORE,"emerald_ore") \
    X(DEEPSLATE_COAL_ORE,"deepslate_coal_ore") X(DEEPSLATE_IRON_ORE,"deepslate_iron_ore") X(DEEPSLATE_COPPER_ORE,"deepslate_copper_ore") \
    X(DEEPSLATE_GOLD_ORE,"deepslate_gold_ore") X(DEEPSLATE_REDSTONE_ORE,"deepslate_redstone_ore") X(DEEPSLATE_LAPIS_ORE,"deepslate_lapis_ore") \
    X(DEEPSLATE_DIAMOND_ORE,"deepslate_diamond_ore") X(DEEPSLATE_EMERALD_ORE,"deepslate_emerald_ore") \
    X(OAK_LOG,"oak_log") X(SPRUCE_LOG,"spruce_log") X(BIRCH_LOG,"birch_log") X(JUNGLE_LOG,"jungle_log") X(ACACIA_LOG,"acacia_log") X(DARK_OAK_LOG,"dark_oak_log") \
    X(OAK_PLANKS,"oak_planks") X(SPRUCE_PLANKS,"spruce_planks") X(BIRCH_PLANKS,"birch_planks") X(JUNGLE_PLANKS,"jungle_planks") X(ACACIA_PLANKS,"acacia_planks") X(DARK_OAK_PLANKS,"dark_oak_planks") \
    X(OAK_LEAVES,"oak_leaves") X(SPRUCE_LEAVES,"spruce_leaves") X(BIRCH_LEAVES,"birch_leaves") X(JUNGLE_LEAVES,"jungle_leaves") X(ACACIA_LEAVES,"acacia_leaves") X(DARK_OAK_LEAVES,"dark_oak_leaves") \
    X(GLASS,"glass") X(BOOKSHELF,"bookshelf") X(BRICKS,"bricks") X(STONE_BRICKS,"stone_bricks") X(MOSSY_STONE_BRICKS,"mossy_stone_bricks") X(CRACKED_STONE_BRICKS,"cracked_stone_bricks") \
    X(CRAFTING_TABLE,"crafting_table") X(FURNACE,"furnace") X(CHEST,"chest") X(TORCH,"torch") X(TNT,"tnt") X(SPAWNER,"spawner") X(SPONGE,"sponge") \
    X(PUMPKIN,"pumpkin") X(CARVED_PUMPKIN,"carved_pumpkin") X(JACK_O_LANTERN,"jack_o_lantern") X(MELON,"melon") X(HAY_BLOCK,"hay_block") X(CACTUS,"cactus") \
    X(SUGAR_CANE,"sugar_cane") X(LILY_PAD,"lily_pad") X(COBWEB,"cobweb") X(DEAD_BUSH,"dead_bush") X(GRASS,"grass") X(FERN,"fern") \
    X(DANDELION,"dandelion") X(POPPY,"poppy") X(BLUE_ORCHID,"blue_orchid") X(ALLIUM,"allium") X(AZURE_BLUET,"azure_bluet") X(RED_TULIP,"red_tulip") \
    X(ORANGE_TULIP,"orange_tulip") X(WHITE_TULIP,"white_tulip") X(PINK_TULIP,"pink_tulip") X(OXEYE_DAISY,"oxeye_daisy") X(CORNFLOWER,"cornflower") \
    X(BROWN_MUSHROOM,"brown_mushroom") X(RED_MUSHROOM,"red_mushroom") X(BROWN_MUSHROOM_BLOCK,"brown_mushroom_block") X(RED_MUSHROOM_BLOCK,"red_mushroom_block") X(MUSHROOM_STEM,"mushroom_stem") \
    X(OAK_SAPLING,"oak_sapling") X(SPRUCE_SAPLING,"spruce_sapling") X(BIRCH_SAPLING,"birch_sapling") X(JUNGLE_SAPLING,"jungle_sapling") X(ACACIA_SAPLING,"acacia_sapling") X(DARK_OAK_SAPLING,"dark_oak_sapling") \
    X(COAL_BLOCK,"coal_block") X(IRON_BLOCK,"iron_block") X(GOLD_BLOCK,"gold_block") X(DIAMOND_BLOCK,"diamond_block") X(EMERALD_BLOCK,"emerald_block") X(LAPIS_BLOCK,"lapis_block") X(REDSTONE_BLOCK,"redstone_block") \
    X(OAK_SLAB,"oak_slab") X(COBBLESTONE_SLAB,"cobblestone_slab") X(STONE_SLAB,"stone_slab") X(STONE_BRICK_SLAB,"stone_brick_slab") X(SANDSTONE_SLAB,"sandstone_slab") \
    X(WHITE_WOOL,"white_wool") X(RED_WOOL,"red_wool") X(BLUE_WOOL,"blue_wool") X(GREEN_WOOL,"green_wool") X(YELLOW_WOOL,"yellow_wool") \
    X(LADDER,"ladder") X(NETHER_BRICKS,"nether_bricks") X(PRISMARINE,"prismarine") X(DARK_PRISMARINE,"dark_prismarine") X(SEA_LANTERN,"sea_lantern") \
    X(QUARTZ_ORE,"nether_quartz_ore") X(CRYING_OBSIDIAN,"crying_obsidian") X(BLUE_ICE,"blue_ice") X(MOSS_BLOCK,"moss_block") X(FARMLAND,"farmland")

namespace B {
#define X(ID, NAME) inline BlockId ID = 0;
MC_BLOCK_IDS(X)
#undef X
}

} // namespace mc
