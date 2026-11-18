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
bool block(int id){return id>AIR && id<BLOCK_COUNT && id!=WATER;}
bool valid(int id){return block(id) || (id>ITEM_START && id<ITEM_END);}
bool pickaxe(int id){return id>=WOOD_PICKAXE && id<=DIAMOND_PICKAXE;}
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
    if(w==3 && h==3 && count==5 && at(1,1)==STICK && at(1,2)==STICK){
        bool p=plank(at(0,0)) && plank(at(1,0)) && plank(at(2,0));
        if(p)r.output={WOOD_PICKAXE,1};
        if(at(0,0)==COBBLESTONE && at(1,0)==COBBLESTONE && at(2,0)==COBBLESTONE)r.output={STONE_PICKAXE,1};
        if(at(0,0)==IRON_INGOT && at(1,0)==IRON_INGOT && at(2,0)==IRON_INGOT)r.output={IRON_PICKAXE,1};
        if(at(0,0)==DIAMOND && at(1,0)==DIAMOND && at(2,0)==DIAMOND)r.output={DIAMOND_PICKAXE,1};
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
case BEDROCK:return -1;case GRASS:case TORCH:return 0;
case LEAVES:return .2;case GLASS:case GLOWSTONE:return .3;
case DIRT:case SAND:case SOUL_SAND:return .5;case GRASS_BLOCK:case GRAVEL:case CLAY:return .6;
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
    if(id==BEDROCK || id==GLASS || id==GRASS || id==SNOW_BLOCK)return false;
    if(!needs_pick(id))return true;if(!pickaxe(tool))return false;
    int tier=tool-WOOD_PICKAXE;
    if(id==OBSIDIAN)return tier>=3;
    if(id==DIAMOND_ORE || id==EMERALD_ORE || id==REDSTONE_ORE || id==GOLD_ORE || id==GOLD_BLOCK || id==DIAMOND_BLOCK || id==EMERALD_BLOCK)return tier>=2;
    if(id==IRON_ORE || id==IRON_BLOCK || id==LAPIS_ORE || id==LAPIS_BLOCK)return tier>=1;
    return true;
}
double break_seconds(int id,int tool,bool grounded){
    double h=hardness(id);if(h<0)return -1;if(h==0)return 0;
    double speed=needs_pick(id) && pickaxe(tool)?2.+2.*(tool-WOOD_PICKAXE):1.;
    if(!grounded)speed/=5.;return h*(harvestable(id,tool)?30.:100.)/speed/20.;
}
Stack drop(int id,int tool,unsigned r){
    if(id==LEAVES)return r%200==0?Stack{APPLE,1}:Stack{};
    if(!harvestable(id,tool))return {};
    switch(id){case GRASS_BLOCK:return {DIRT,1};case STONE:return {COBBLESTONE,1};
    case COAL_ORE:return {COAL,1};case DIAMOND_ORE:return {DIAMOND,1};case EMERALD_ORE:return {EMERALD,1};
    case REDSTONE_ORE:return {REDSTONE_DUST,4+int(r%2)};case LAPIS_ORE:return {LAPIS,4+int(r%5)};
    case GLOWSTONE:return {GLOWSTONE_DUST,2+int(r%3)};case GRAVEL:return {r%10==0?int(FLINT):int(GRAVEL),1};
    default:return {id,1};}
}
void Vitals::hurt(float amount){if(amount>0 && hurt_ticks==0){health=std::max(0.f,health-amount);hurt_ticks=10;}}
void Vitals::exhaust(float amount){exhaustion=std::min(40.f,exhaustion+amount);}
bool Vitals::eat(int id){if(food>=20)return false;int n=id==APPLE?4:(id==BREAD?5:0);if(!n)return false;
    food=std::min(20,food+n);saturation=std::min(float(food),saturation+n*(id==APPLE?.6f:1.2f));return true;}
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
