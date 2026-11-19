#include <language.h>
#include <items.h>
#include <algorithm>
#include <cmath>
#include <cstring>
namespace Items {
static bool wool(int id){return id==WHITE_WOOL || id==RED_WOOL || id==BLUE_WOOL || (id>=ORANGE_WOOL && id<=BLACK_WOOL);}
static bool wood(int id){return id==LOG || id==SPRUCE_LOG || id==BIRCH_LOG || id==PLANKS || id==SPRUCE_PLANKS || id==BIRCH_PLANKS || (id>=JUNGLE_PLANKS && id<=DARK_OAK_LOG) || id==CRAFTING_TABLE || id==BOOKSHELF;}
static bool stone_material(int id){return (id>=GRANITE && id<=POLISHED_ANDESITE) || (id>=WHITE_CONCRETE && id<=BLUE_TERRACOTTA && id!=RED_SAND) || (id>=PURPUR_BLOCK && id<=PRISMARINE_ROUGH);}
int food_points(int id){switch(id){
case APPLE:return 4;case BREAD:case BAKED_POTATO:case COOKED_FISH:case COOKED_RABBIT:return 5;
case RAW_FISH:case MELON_SLICE:case COOKIE:case RAW_CHICKEN:case RAW_MUTTON:return 2;
case ROTTEN_FLESH:return 4;
case RAW_SALMON:case POISONOUS_POTATO:case SPIDER_EYE:return 2;
case CLOWNFISH:case PUFFERFISH:return 1;
case COOKED_SALMON:case GOLDEN_CARROT:case MUSHROOM_STEW:case BEETROOT_SOUP:return 6;
case RABBIT_STEW:return 10;
case GOLDEN_APPLE:case ENCHANTED_GOLDEN_APPLE:case CHORUS_FRUIT:return 4;
case CARROT:case RAW_BEEF:case RAW_PORKCHOP:case RABBIT:return 3;
case POTATO:case BEETROOT:return 1;
case STEAK:case COOKED_PORKCHOP:case PUMPKIN_PIE:return 8;
case COOKED_CHICKEN:case COOKED_MUTTON:return 6;
default:return 0;}}
float food_saturation(int id){switch(id){
case RAW_FISH:case COOKIE:return .1f;
case RAW_SALMON:case CLOWNFISH:case PUFFERFISH:return .1f;
case ROTTEN_FLESH:case RAW_CHICKEN:return .1f;
case RAW_MUTTON:case POISONOUS_POTATO:case CHORUS_FRUIT:return .3f;
case COOKED_SALMON:return .8f;
case GOLDEN_APPLE:case ENCHANTED_GOLDEN_APPLE:case GOLDEN_CARROT:return 1.2f;
case SPIDER_EYE:return .8f;
case APPLE:case PUMPKIN_PIE:case POTATO:case MELON_SLICE:case RAW_BEEF:case RAW_PORKCHOP:case RABBIT:return .3f;
case STEAK:case COOKED_PORKCHOP:case COOKED_MUTTON:return .8f;
default:return .6f;}}
int limit(int id){switch(id){
#define ITEM(id,label,texture,max,damage) case id:return max;
#include <items.def>
#undef ITEM
default:return 64;}}
int durability(int id){switch(id){
#define ITEM(id,label,texture,max,damage) case id:return damage;
#include <items.def>
#undef ITEM
default:return 0;}}
const char* english_name(int id){switch(id){
#define BASE_BLOCK(id,label,...) case id:return label;
#define EXTRA_BLOCK BASE_BLOCK
#include <extra_blocks.def>
#undef BASE_BLOCK
#undef EXTRA_BLOCK
case STONE:return "Stone";case DIRT:return "Dirt";case GRASS_BLOCK:return "Grass Block";
case LOG:return "Oak Log";case LEAVES:return "Oak Leaves";case GLASS:return "Glass";
case GRASS:return "Grass";case TORCH:return "Torch";case PLANKS:return "Oak Planks";case BEDROCK:return "Bedrock";
#define ITEM(id,label,...) case id:return label;
#include <items.def>
#undef ITEM
default:return "";}}
const char* name(int id){return Language::tr(english_name(id));}
bool block(int id){return id>AIR && id<BLOCK_COUNT && id!=WATER && id!=FIRE;}
bool valid(int id){return block(id) || (id>ITEM_START && id<ITEM_END);}
int from_vanilla_name(const char* name){
    if(!name)return -1;
    if(std::strncmp(name,"minecraft:",10)==0)name+=10;
#define VANILLA_ITEM(key,id) if(std::strcmp(name,#key)==0)return id;
#include <vanilla_items.def>
#undef VANILLA_ITEM
    return -1;
}
bool always_edible(int id){return id==GOLDEN_APPLE || id==ENCHANTED_GOLDEN_APPLE || id==CHORUS_FRUIT;}
int container_after_use(int id){switch(id){
case MUSHROOM_STEW:case RABBIT_STEW:case BEETROOT_SOUP:return BOWL;
case MILK_BUCKET:return BUCKET;case POTION:case DRAGON_BREATH:return GLASS_BOTTLE;
default:return -1;}}
int spawn_kind(int id){return id>=PIG_SPAWN_EGG && id<=HORSE_SPAWN_EGG?id-PIG_SPAWN_EGG:-1;}
Projectile projectile(int id){switch(id){
case ARROW:case SPECTRAL_ARROW:case TIPPED_ARROW:return Projectile::Arrow;
case SNOWBALL:return Projectile::Snowball;case EGG:return Projectile::Egg;case ENDER_PEARL:return Projectile::Pearl;
case SPLASH_POTION:case LINGERING_POTION:case EXPERIENCE_BOTTLE:return Projectile::Potion;
default:return Projectile::None;}}
float bow_charge(float seconds){float t=std::max(0.f,seconds)*20/20;return std::min(1.f,(t*t+2*t)/3);}
float projectile_speed(Projectile kind,float charge){return kind==Projectile::Arrow?60.f*charge:(kind==Projectile::Potion?24.f:30.f);}
float projectile_gravity(Projectile kind){return kind==Projectile::Arrow?20.f:12.f;} // blocks/second^2, .05/.03 per tick
Tool tool_kind(int id){
    if((id>=WOOD_PICKAXE && id<=DIAMOND_PICKAXE) || id==GOLD_PICKAXE)return Tool::Pickaxe;
    if(id>=WOOD_AXE && id<=GOLD_AXE)return Tool::Axe;
    if(id>=WOOD_SHOVEL && id<=GOLD_SHOVEL)return Tool::Shovel;
    if(id>=WOOD_HOE && id<=GOLD_HOE)return Tool::Hoe;
    if(id>=WOOD_SWORD && id<=GOLD_SWORD)return Tool::Sword;
    return id==SHEARS?Tool::Shears:Tool::None;
}
bool pickaxe(int id){return tool_kind(id)==Tool::Pickaxe;}
int tool_tier(int id){
    int material=-1;
    switch(tool_kind(id)){
    case Tool::Pickaxe:material=id==GOLD_PICKAXE?4:id-WOOD_PICKAXE;break;
    case Tool::Axe:material=id-WOOD_AXE;break;case Tool::Shovel:material=id-WOOD_SHOVEL;break;
    case Tool::Hoe:material=id-WOOD_HOE;break;case Tool::Sword:material=id-WOOD_SWORD;break;
    default:break;}
    return material==4?0:material;
}
double tool_speed(int id){
    if(tool_kind(id)==Tool::None || tool_kind(id)==Tool::Shears)return 1;
    bool gold=id==GOLD_PICKAXE || id==GOLD_AXE || id==GOLD_SHOVEL || id==GOLD_HOE || id==GOLD_SWORD;
    return gold?12.:2.+2.*tool_tier(id);
}
int armor_slot(int id){return id>=LEATHER_HELMET && id<=DIAMOND_BOOTS?(id-LEATHER_HELMET)%4:-1;}
int armor_material(int id){return armor_slot(id)<0?-1:(id-LEATHER_HELMET)/4;}
int armor_points(int id){static const int points[5][4]={{1,3,2,1},{2,5,4,1},{2,6,5,2},{2,5,3,1},{3,8,6,3}};int m=armor_material(id);return m<0?0:points[m][armor_slot(id)];}
float armor_toughness(int id){return armor_material(id)==4?2.f:0.f;}
float absorb_damage(std::array<Stack,4>& armor,float damage){
    if(damage<=0)return 0;float points=0,tough=0;
    for(int i=0;i<4;++i)if(!armor[i].empty() && armor_slot(armor[i].id)==i){points+=armor_points(armor[i].id);tough+=armor_toughness(armor[i].id);}
    float effective=std::min(20.f,std::max(points*.2f,points-damage/(2.f+tough/4.f)));
    for(int i=0;i<4;++i)if(!armor[i].empty() && armor_slot(armor[i].id)==i)wear(armor[i],std::max(1,int(damage/4.f)));
    return damage*(1.f-effective/25.f);
}
bool equip(std::array<Stack,4>& armor,Stack& source){int slot=armor_slot(source.id);if(source.empty() || slot<0 || !armor[slot].empty())return false;armor[slot]=source;armor[slot].count=1;--source.count;normalize(source);return true;}
void armor_click(Stack& slot,Stack& cursor,int part,bool right){if(cursor.empty() || armor_slot(cursor.id)==part)click(slot,cursor,right);}
void wear(Stack& s,int amount){int max=durability(s.id);if(max && amount>0 && (s.damage+=amount)>max)s.clear();}
int mining_wear(int id,int tool){
    if(tool==SHEARS && id==GRASS)return 1;
    if(hardness(id)<=0)return 0;
    switch(tool_kind(tool)){
    case Tool::Pickaxe:case Tool::Axe:case Tool::Shovel:return 1;
    case Tool::Sword:return 2;
    case Tool::Shears:return id==LEAVES || id==GRASS || wool(id)?1:0;
    default:return 0;}
}
int use_on_block(int id,int tool,bool air,bool bottom){
    if(!air || bottom)return -1;
    if(tool_kind(tool)==Tool::Hoe && (id==DIRT || id==GRASS_BLOCK || id==GRASS_PATH))return FARMLAND;
    if(tool_kind(tool)==Tool::Shovel && id==GRASS_BLOCK)return GRASS_PATH;
    return -1;
}
int clock_frame(double ticks){return int(std::floor(ticks/24000.*32.+24))&31;}
int compass_frame(double dx,double dz,double fx,double fz){
    return int(std::floor((std::atan2(dx,dz)-std::atan2(fx,fz))/6.283185307179586*32.+16.5))&31;
}
void normalize(Stack& s){if(!valid(s.id) || s.count<=0)s.clear();else s.count=std::min(s.count,limit(s.id));}
void click(Stack& s,Stack& c,bool right){
    normalize(s);normalize(c);
    if(c.empty()){
        if(s.empty())return;
        c=s;c.count=right?(s.count+1)/2:s.count;s.count-=c.count;normalize(s);
    }else if(s.empty() || (s.id==c.id && s.damage==c.damage)){
        int n=std::min(right?1:c.count,limit(c.id)-(s.empty()?0:s.count));
        if(s.empty()){s=c;s.count=0;}s.count+=n;c.count-=n;normalize(c);
    }else if(!right)std::swap(s,c);
}
int insert(Stack* slots,int count,Stack v){
    if(v.empty() || !valid(v.id))return 0;
    for(int pass=0;pass<2;++pass)for(int i=0;i<count && v.count>0;++i){
        auto& s=slots[i];
        if((pass==0 && !s.empty() && s.id==v.id && s.damage==v.damage) || (pass==1 && s.empty())){
            int amount=std::min(v.count,limit(v.id)-(s.empty()?0:s.count));
            if(s.empty()){s=v;s.count=0;}s.count+=amount;v.count-=amount;
        }
    }return v.count;
}
void Inventory::quick_move(int i){
    if(i<0 || i>=36 || main[i].empty())return;
    if(equip(armor,main[i]))return;
    main[i].count=insert(main.data()+(i<9?9:0),i<9?27:9,main[i]);normalize(main[i]);
}
static bool plank(int id){return id==PLANKS || id==BIRCH_PLANKS || id==SPRUCE_PLANKS || id==JUNGLE_PLANKS || id==ACACIA_PLANKS || id==DARK_OAK_PLANKS;}
Recipe recipe(const std::array<Stack,9>& grid,int width){
    Recipe r;int left=width,top=width,right=-1,bottom=-1,count=0;
    for(int y=0;y<width;++y)for(int x=0;x<width;++x)if(!grid[y*width+x].empty()){
        left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);++count;r.used[y*width+x]=true;
    }
    if(!count)return r;
    int w=right-left+1,h=bottom-top+1;
    auto at=[&](int x,int y){return grid[(top+y)*width+left+x].id;};
    auto all=[&](int id){for(int y=0;y<h;++y)for(int x=0;x<w;++x)if(at(x,y)!=id)return false;return true;};
    auto all_planks=[&](){for(int y=0;y<h;++y)for(int x=0;x<w;++x)if(!plank(at(x,y)))return false;return true;};
    if(count==1 && at(0,0)==LOG)r.output={PLANKS,4};
    if(count==1 && at(0,0)==SPRUCE_LOG)r.output={SPRUCE_PLANKS,4};
    if(count==1 && at(0,0)==BIRCH_LOG)r.output={BIRCH_PLANKS,4};
    if(count==1){
        if(at(0,0)==JUNGLE_LOG)r.output={JUNGLE_PLANKS,4};
        if(at(0,0)==ACACIA_LOG)r.output={ACACIA_PLANKS,4};
        if(at(0,0)==DARK_OAK_LOG)r.output={DARK_OAK_PLANKS,4};
        if(at(0,0)==SUGAR_CANE)r.output={SUGAR,1};
        if(at(0,0)==BONE)r.output={BONE_MEAL,3};
        if(at(0,0)==BLAZE_ROD)r.output={BLAZE_POWDER,2};
    }
    if(w==1 && h==2 && count==2 && all_planks())r.output={STICK,4};
    if(w==2 && h==2 && count==4){
        if(all_planks())r.output={CRAFTING_TABLE,1};
        if(all(SAND))r.output={SANDSTONE,1};
        if(all(BRICK_ITEM))r.output={BRICKS,1};
        if(all(SNOWBALL))r.output={SNOW_BLOCK,1};
        if(all(GLOWSTONE_DUST))r.output={GLOWSTONE,1};
        if(all(STONE))r.output={STONE_BRICKS,4};
        if(all(RED_SAND))r.output={RED_SANDSTONE,1};
        if(all(QUARTZ))r.output={QUARTZ_BLOCK,1};
        for(int id:{GRANITE,DIORITE,ANDESITE})if(all(id))r.output={id+1,4};
    }
    if(w==1 && h==2 && count==2 && at(0,0)==COAL && at(0,1)==STICK)r.output={TORCH,4};
    if(w==1 && h==2 && count==2 && at(0,0)==CHARCOAL && at(0,1)==STICK)r.output={TORCH,4};
    if(w==3 && h==1 && count==3 && all(WHEAT))r.output={BREAD,1};
    if(w==3 && h==1 && count==3 && all(SUGAR_CANE))r.output={PAPER,3};
    if(w==3 && h==2 && count==3 && at(0,0)==IRON_INGOT && at(2,0)==IRON_INGOT && at(1,1)==IRON_INGOT)r.output={BUCKET,1};
    auto shape=[&](const char* pattern,int sw,int sh,int material,bool mirror=false){
        if(w!=sw || h!=sh)return false;
        for(int y=0;y<sh;++y)for(int x=0;x<sw;++x){
            char c=pattern[y*sw+(mirror?sw-1-x:x)];auto s=grid[(top+y)*width+left+x];
            if(c==' '){if(!s.empty())return false;}
            else if(s.empty() || (c=='S'?s.id!=STICK:c=='T'?s.id!=STRING:material==PLANKS?!plank(s.id):s.id!=material))return false;
        }return true;
    };
    const int armor_materials[]={LEATHER,-1,IRON_INGOT,GOLD_INGOT,DIAMOND};
    for(int m=0;m<5;++m)if(armor_materials[m]>0){int base=LEATHER_HELMET+m*4;
        if(shape("RRRR R",3,2,armor_materials[m]))r.output={base,1};
        if(shape("R RRRRRRR",3,3,armor_materials[m]))r.output={base+1,1};
        if(shape("RRRR RR R",3,3,armor_materials[m]))r.output={base+2,1};
        if(shape("R RR R",3,2,armor_materials[m]))r.output={base+3,1};
    }
    const int materials[]={PLANKS,COBBLESTONE,IRON_INGOT,DIAMOND,GOLD_INGOT};
    const int picks[]={WOOD_PICKAXE,STONE_PICKAXE,IRON_PICKAXE,DIAMOND_PICKAXE,GOLD_PICKAXE};
    for(int m=0;m<5;++m){
        if(shape("RRR S  S ",3,3,materials[m]))r.output={picks[m],1};
        if(shape("RRRS S",2,3,materials[m]) || shape("RRRS S",2,3,materials[m],true))r.output={WOOD_AXE+m,1};
        if(shape("RSS",1,3,materials[m]))r.output={WOOD_SHOVEL+m,1};
        if(shape("RR S S",2,3,materials[m]) || shape("RR S S",2,3,materials[m],true))r.output={WOOD_HOE+m,1};
        if(shape("RRS",1,3,materials[m]))r.output={WOOD_SWORD+m,1};
    }
    if(shape(" RR ",2,2,IRON_INGOT) || shape("R  R",2,2,IRON_INGOT))r.output={SHEARS,1};
    if(shape("  S STS T",3,3,STICK) || shape("  S STS T",3,3,STICK,true))r.output={FISHING_ROD,1};
    if(shape(" STR T ST",3,3,STICK) || shape(" STR T ST",3,3,STICK,true))r.output={BOW,1};
    if(w==1 && h==3 && count==3 && at(0,0)==FLINT && at(0,1)==STICK && at(0,2)==FEATHER)r.output={ARROW,4};
    if(w==3 && h==2 && count==3 && plank(at(0,0)) && plank(at(2,0)) && plank(at(1,1)))r.output={BOWL,4};
    if(w==3 && h==3 && count==7 && at(1,0)==IRON_INGOT){
        bool shield=true;
        for(int y=0;y<3;++y)for(int x=0;x<3;++x){
            if(y==0 && x==1)continue;
            if(y==2 && x!=1){if(!grid[(top+y)*width+left+x].empty())shield=false;}
            else if(!plank(at(x,y)))shield=false;
        }if(shield)r.output={SHIELD,1};
    }
    if(w==3 && h==3 && count==5 && at(1,1)==REDSTONE_DUST){
        bool iron=true,gold=true;
        for(auto p:{std::array<int,2>{1,0},{0,1},{2,1},{1,2}}){iron&=at(p[0],p[1])==IRON_INGOT;gold&=at(p[0],p[1])==GOLD_INGOT;}
        if(iron)r.output={COMPASS,1};if(gold)r.output={CLOCK,1};
    }
    if(count==2){
        Stack a,b;for(int i=0;i<width*width;++i)if(!grid[i].empty()){if(a.empty())a=grid[i];else b=grid[i];}
        if((a.id==IRON_INGOT && b.id==FLINT) || (b.id==IRON_INGOT && a.id==FLINT))r.output={FLINT_AND_STEEL,1};
        auto pair=[&](int x,int y){return (a.id==x && b.id==y) || (a.id==y && b.id==x);};
        if(pair(SLIME_BALL,BLAZE_POWDER))r.output={MAGMA_CREAM,1};
        if(pair(ENDER_PEARL,BLAZE_POWDER))r.output={ENDER_EYE,1};
        int max=durability(a.id);
        if(a.id==b.id && a.count==1 && b.count==1 && max)r.output={a.id,1,std::max(0,a.damage+b.damage-max-max*5/100)};
    }
    if(w==3 && h==3 && count==9){
        const int raw[]={IRON_INGOT,GOLD_INGOT,DIAMOND,EMERALD,REDSTONE_DUST,LAPIS,COAL};
        const int packed[]={IRON_BLOCK,GOLD_BLOCK,DIAMOND_BLOCK,EMERALD_BLOCK,REDSTONE_BLOCK,LAPIS_BLOCK,COAL_BLOCK};
        for(int i=0;i<7;++i)if(all(raw[i]))r.output={packed[i],1};
        if(all(IRON_NUGGET))r.output={IRON_INGOT,1};if(all(GOLD_NUGGET))r.output={GOLD_INGOT,1};
        if(all(WHEAT))r.output={HAY_BLOCK,1};if(all(MELON_SLICE))r.output={MELON_BLOCK,1};
    }
    if(count==1){const int packed[]={IRON_BLOCK,GOLD_BLOCK,DIAMOND_BLOCK,EMERALD_BLOCK,REDSTONE_BLOCK,LAPIS_BLOCK};
        const int raw[]={IRON_INGOT,GOLD_INGOT,DIAMOND,EMERALD,REDSTONE_DUST,LAPIS};
        for(int i=0;i<6;++i)if(at(0,0)==packed[i])r.output={raw[i],9};
    }
    if(count==1){switch(at(0,0)){
        case PUMPKIN:r.output={PUMPKIN_SEEDS,4};break;case MELON_SLICE:r.output={MELON_SEEDS,1};break;
        case COAL_BLOCK:r.output={COAL,9};break;case HAY_BLOCK:r.output={WHEAT,9};break;
        case IRON_INGOT:r.output={IRON_NUGGET,9};break;case GOLD_INGOT:r.output={GOLD_NUGGET,9};break;default:break;}}
    if(count==3 || count==4){
        int paper=0,leather=0,pumpkin=0,sugar=0,egg=0;
        for(int i=0;i<width*width;++i)if(!grid[i].empty()){
            int id=grid[i].id;paper+=id==PAPER;leather+=id==LEATHER;pumpkin+=id==PUMPKIN;sugar+=id==SUGAR;egg+=id==EGG;
        }
        if(count==4 && paper==3 && leather==1)r.output={BOOK,1};
        if(count==3 && pumpkin==1 && sugar==1 && egg==1)r.output={PUMPKIN_PIE,1};
    }
    return r;
}
bool Inventory::take_result(int width,bool quick){
    auto r=recipe(craft,width);if(r.output.empty())return false;
    if(quick){auto copy=main;if(insert(copy.data(),36,r.output))return false;main=copy;}
    else {if(!cursor.empty() && (cursor.id!=r.output.id || cursor.damage || cursor.count+r.output.count>limit(cursor.id)))return false;
        if(cursor.empty())cursor=r.output;else cursor.count+=r.output.count;}
    for(int i=0;i<width*width;++i)if(r.used[i]){--craft[i].count;normalize(craft[i]);}
    return true;
}
std::vector<Stack> Inventory::close(){
    std::vector<Stack> rest;
    auto return_stack=[&](Stack& s){if(!s.empty()){s.count=add(s);if(!s.empty())rest.push_back(s);s.clear();}};
    return_stack(cursor);for(auto& s:craft)return_stack(s);return rest;
}
double hardness(int id){
if(wool(id))return .8;if(wood(id))return id==CRAFTING_TABLE?2.5:(id==BOOKSHELF?1.5:2.);
if(id>=WHITE_CONCRETE && id<=BLACK_CONCRETE)return 1.8;
if(id==COAL_BLOCK)return 5.;if(id==RED_SAND)return .5;
if(id==HAY_BLOCK)return .5;if(id==PUMPKIN)return 1.;if(id==MELON_BLOCK)return 1.;
if(id==SEA_LANTERN)return .3;
if(id==CACTUS)return .4;
if(id==RED_SANDSTONE)return .8;if(id>=HARDENED_CLAY && id<=BLUE_TERRACOTTA)return 1.25;
switch(id){
case BEDROCK:return -1;case GRASS:case TORCH:case FIRE:return 0;
case LEAVES:return .2;case GLASS:case GLOWSTONE:return .3;
case DIRT:case SAND:case SOUL_SAND:return .5;case GRASS_BLOCK:case GRAVEL:case CLAY:case FARMLAND:case GRASS_PATH:return .6;
case LOG:case PLANKS:case SPRUCE_PLANKS:case BIRCH_PLANKS:return 2;case CRAFTING_TABLE:return 2.5;
case OBSIDIAN:return 50;case STONE:return 1.5;
case COAL_ORE:case IRON_ORE:case GOLD_ORE:case DIAMOND_ORE:case EMERALD_ORE:case REDSTONE_ORE:case LAPIS_ORE:return 3;
case WHITE_WOOL:case RED_WOOL:case BLUE_WOOL:return .8;case SNOW_BLOCK:return .2;
case NETHERRACK:return .4;case END_STONE:return 3;
default:return 1.5;}}
static bool needs_pick(int id){if(stone_material(id))return true;switch(id){
case STONE:case COBBLESTONE:case MOSSY_COBBLESTONE:case STONE_BRICKS:case BRICKS:case OBSIDIAN:
case COAL_ORE:case IRON_ORE:case GOLD_ORE:case DIAMOND_ORE:case REDSTONE_ORE:case LAPIS_ORE:case EMERALD_ORE:
case IRON_BLOCK:case GOLD_BLOCK:case DIAMOND_BLOCK:case EMERALD_BLOCK:case REDSTONE_BLOCK:case LAPIS_BLOCK:
case SANDSTONE:case NETHER_BRICKS:case NETHERRACK:case END_STONE:case QUARTZ_BLOCK:case PRISMARINE:return true;
default:return false;}}
bool harvestable(int id,int tool){
    if(id==BEDROCK || id==GLASS || id==FIRE)return false;
    if(id==GRASS)return tool==SHEARS;
    if(id==SNOW_BLOCK)return tool_kind(tool)==Tool::Shovel;
    if(!needs_pick(id))return true;if(!pickaxe(tool))return false;
    int tier=tool_tier(tool);
    if(id==OBSIDIAN)return tier>=3;
    if(id==DIAMOND_ORE || id==EMERALD_ORE || id==REDSTONE_ORE || id==GOLD_ORE || id==GOLD_BLOCK || id==DIAMOND_BLOCK || id==EMERALD_BLOCK)return tier>=2;
    if(id==IRON_ORE || id==IRON_BLOCK || id==LAPIS_ORE || id==LAPIS_BLOCK)return tier>=1;
    return true;
}
double break_seconds(int id,int tool,bool grounded){
    double h=hardness(id);if(h<0)return -1;if(h==0)return 0;
    Tool kind=tool_kind(tool);double speed=1.;
    bool soil=id==DIRT || id==GRASS_BLOCK || id==PODZOL || id==MYCELIUM || id==SAND || id==RED_SAND || id==GRAVEL || id==CLAY || id==SNOW_BLOCK || id==SOUL_SAND || id==FARMLAND || id==GRASS_PATH;
    if((needs_pick(id) && kind==Tool::Pickaxe) || ((wood(id) || id==PUMPKIN || id==MELON_BLOCK) && kind==Tool::Axe) || (soil && kind==Tool::Shovel))speed=tool_speed(tool);
    if(kind==Tool::Sword)speed=1.5;
    if(tool==SHEARS){if(id==LEAVES)speed=15.;if(wool(id))speed=5.;}
    if(!grounded)speed/=5.;return h*(harvestable(id,tool)?30.:100.)/speed/20.;
}
Stack drop(int id,int tool,unsigned r){
    if(id==LEAVES)return tool==SHEARS?Stack{LEAVES,1}:(r%200==0?Stack{APPLE,1}:Stack{});
    if(!harvestable(id,tool))return {};
    switch(id){case GRASS_BLOCK:case FARMLAND:case GRASS_PATH:return {DIRT,1};case STONE:return {COBBLESTONE,1};
    case SNOW_BLOCK:return {SNOWBALL,4};
    case COAL_ORE:return {COAL,1};case DIAMOND_ORE:return {DIAMOND,1};case EMERALD_ORE:return {EMERALD,1};
    case REDSTONE_ORE:return {REDSTONE_DUST,4+int(r%2)};case LAPIS_ORE:return {LAPIS,4+int(r%5)};
    case GLOWSTONE:return {GLOWSTONE_DUST,2+int(r%3)};case GRAVEL:return {r%10==0?int(FLINT):int(GRAVEL),1};
    case MELON_BLOCK:return {MELON_SLICE,3+int(r%5)};case SEA_LANTERN:return {PRISMARINE_CRYSTALS,2+int(r%2)};
    default:return {id,1};}
}
void Vitals::hurt(float amount){if(amount>0 && hurt_ticks==0){health=std::max(0.f,health-amount);hurt_ticks=10;}}
void Vitals::exhaust(float amount){exhaustion=std::min(40.f,exhaustion+amount);}
bool Vitals::eat(int id){if(food>=20 && !always_edible(id))return false;int n=food_points(id);if(!n)return false;
    food=std::min(20,food+n);saturation=std::min(float(food),saturation+n*food_saturation(id)*2);return true;}
void Vitals::tick(){
    if(hurt_ticks>0)--hurt_ticks;
    if(exhaustion>4){exhaustion-=4;if(saturation>0)saturation=std::max(0.f,saturation-1);else food=std::max(0,food-1);}
    if(health<=0)return;
    if(food==20 && saturation>0 && health<20){if(++timer>=10){float n=std::min(saturation,6.f);health=std::min(20.f,health+n/6);exhaust(n);timer=0;}}
    else if(food>=18 && health<20){if(++timer>=80){health=std::min(20.f,health+1);exhaust(6);timer=0;}}
    else if(food==0){if(++timer>=80){if(health>1)hurt(1);timer=0;}}
    else timer=0;
}
void Vitals::landing(double downward,bool grounded){
    if(downward>0)fallen+=downward;
    if(grounded){if(fallen>3)hurt(float(std::ceil(fallen-3-1e-5)));fallen=0;}
}
}
