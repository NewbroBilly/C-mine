#include <items.h>
#include <algorithm>
#include <cmath>
namespace Items {
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
const char* name(int id){switch(id){
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
bool block(int id){return id>AIR && id<BLOCK_COUNT && id!=WATER && id!=FIRE;}
bool valid(int id){return block(id) || (id>ITEM_START && id<ITEM_END);}
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
void wear(Stack& s,int amount){int max=durability(s.id);if(max && amount>0 && (s.damage+=amount)>max)s.clear();}
int mining_wear(int id,int tool){
    if(tool==SHEARS && id==GRASS)return 1;
    if(hardness(id)<=0)return 0;
    switch(tool_kind(tool)){
    case Tool::Pickaxe:case Tool::Axe:case Tool::Shovel:return 1;
    case Tool::Sword:return 2;
    case Tool::Shears:return id==LEAVES || id==GRASS || id==WHITE_WOOL || id==RED_WOOL || id==BLUE_WOOL?1:0;
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
    main[i].count=insert(main.data()+(i<9?9:0),i<9?27:9,main[i]);normalize(main[i]);
}
static bool plank(int id){return id==PLANKS || id==BIRCH_PLANKS || id==SPRUCE_PLANKS;}
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
    if(w==1 && h==2 && count==2 && all_planks())r.output={STICK,4};
    if(w==2 && h==2 && count==4){
        if(all_planks())r.output={CRAFTING_TABLE,1};
        if(all(SAND))r.output={SANDSTONE,1};
        if(all(BRICK_ITEM))r.output={BRICKS,1};
        if(all(SNOWBALL))r.output={SNOW_BLOCK,1};
        if(all(GLOWSTONE_DUST))r.output={GLOWSTONE,1};
        if(all(STONE))r.output={STONE_BRICKS,4};
    }
    if(w==1 && h==2 && count==2 && at(0,0)==COAL && at(0,1)==STICK)r.output={TORCH,4};
    if(w==3 && h==1 && count==3 && all(WHEAT))r.output={BREAD,1};
    if(w==3 && h==2 && count==3 && at(0,0)==IRON_INGOT && at(2,0)==IRON_INGOT && at(1,1)==IRON_INGOT)r.output={BUCKET,1};
    auto shape=[&](const char* pattern,int sw,int sh,int material,bool mirror=false){
        if(w!=sw || h!=sh)return false;
        for(int y=0;y<sh;++y)for(int x=0;x<sw;++x){
            char c=pattern[y*sw+(mirror?sw-1-x:x)];auto s=grid[(top+y)*width+left+x];
            if(c==' '){if(!s.empty())return false;}
            else if(s.empty() || (c=='S'?s.id!=STICK:c=='T'?s.id!=STRING:material==PLANKS?!plank(s.id):s.id!=material))return false;
        }return true;
    };
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
    if(w==3 && h==3 && count==5 && at(1,1)==REDSTONE_DUST){
        bool iron=true,gold=true;
        for(auto p:{std::array<int,2>{1,0},{0,1},{2,1},{1,2}}){iron&=at(p[0],p[1])==IRON_INGOT;gold&=at(p[0],p[1])==GOLD_INGOT;}
        if(iron)r.output={COMPASS,1};if(gold)r.output={CLOCK,1};
    }
    if(count==2){
        Stack a,b;for(int i=0;i<width*width;++i)if(!grid[i].empty()){if(a.empty())a=grid[i];else b=grid[i];}
        if((a.id==IRON_INGOT && b.id==FLINT) || (b.id==IRON_INGOT && a.id==FLINT))r.output={FLINT_AND_STEEL,1};
        int max=durability(a.id);
        if(a.id==b.id && a.count==1 && b.count==1 && max)r.output={a.id,1,std::max(0,a.damage+b.damage-max-max*5/100)};
    }
    if(w==3 && h==3 && count==9){
        const int raw[]={IRON_INGOT,GOLD_INGOT,DIAMOND,EMERALD,REDSTONE_DUST,LAPIS,COAL};
        const int packed[]={IRON_BLOCK,GOLD_BLOCK,DIAMOND_BLOCK,EMERALD_BLOCK,REDSTONE_BLOCK,LAPIS_BLOCK,-1};
        for(int i=0;i<6;++i)if(all(raw[i]))r.output={packed[i],1};
    }
    if(count==1){const int packed[]={IRON_BLOCK,GOLD_BLOCK,DIAMOND_BLOCK,EMERALD_BLOCK,REDSTONE_BLOCK,LAPIS_BLOCK};
        const int raw[]={IRON_INGOT,GOLD_INGOT,DIAMOND,EMERALD,REDSTONE_DUST,LAPIS};
        for(int i=0;i<6;++i)if(at(0,0)==packed[i])r.output={raw[i],9};
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
double hardness(int id){switch(id){
case BEDROCK:return -1;case GRASS:case TORCH:case FIRE:return 0;
case LEAVES:return .2;case GLASS:case GLOWSTONE:return .3;
case DIRT:case SAND:case SOUL_SAND:return .5;case GRASS_BLOCK:case GRAVEL:case CLAY:case FARMLAND:case GRASS_PATH:return .6;
case LOG:case PLANKS:case SPRUCE_PLANKS:case BIRCH_PLANKS:return 2;case CRAFTING_TABLE:return 2.5;
case OBSIDIAN:return 50;case STONE:return 1.5;
case COAL_ORE:case IRON_ORE:case GOLD_ORE:case DIAMOND_ORE:case EMERALD_ORE:case REDSTONE_ORE:case LAPIS_ORE:return 3;
case WHITE_WOOL:case RED_WOOL:case BLUE_WOOL:return .8;case SNOW_BLOCK:return .2;
case NETHERRACK:return .4;case END_STONE:return 3;
default:return 1.5;}}
static bool needs_pick(int id){switch(id){
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
    bool wood=id==LOG || id==PLANKS || id==SPRUCE_PLANKS || id==BIRCH_PLANKS || id==CRAFTING_TABLE || id==BOOKSHELF;
    bool soil=id==DIRT || id==GRASS_BLOCK || id==SAND || id==GRAVEL || id==CLAY || id==SNOW_BLOCK || id==SOUL_SAND || id==FARMLAND || id==GRASS_PATH;
    if((needs_pick(id) && kind==Tool::Pickaxe) || (wood && kind==Tool::Axe) || (soil && kind==Tool::Shovel))speed=tool_speed(tool);
    if(kind==Tool::Sword)speed=1.5;
    if(tool==SHEARS){if(id==LEAVES)speed=15.;if(id==WHITE_WOOL || id==RED_WOOL || id==BLUE_WOOL)speed=5.;}
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
    default:return {id,1};}
}
void Vitals::hurt(float amount){if(amount>0 && hurt_ticks==0){health=std::max(0.f,health-amount);hurt_ticks=10;}}
void Vitals::exhaust(float amount){exhaustion=std::min(40.f,exhaustion+amount);}
bool Vitals::eat(int id){if(food>=20)return false;int n=id==APPLE?4:(id==BREAD?5:(id==RAW_FISH?2:0));if(!n)return false;
    food=std::min(20,food+n);saturation=std::min(float(food),saturation+n*(id==APPLE?.6f:(id==RAW_FISH?.2f:1.2f)));return true;}
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
