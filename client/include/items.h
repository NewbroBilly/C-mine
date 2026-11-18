#pragma once
#include <block_ids.h>
#include <array>
#include <vector>
namespace Items {
enum ID { ITEM_START=255,
#define ITEM(id,...) id,
#include <items.def>
#undef ITEM
    ITEM_END
};
static_assert(ITEM_END-ITEM_START-1<=160,"Static icons overlap the animated item atlas region");
struct Stack {
    int id=-1,count=0,damage=0;
    bool empty() const {return id<1 || count<=0;}
    void clear(){id=-1;count=damage=0;}
    bool operator==(const Stack& b) const{return id==b.id && count==b.count && damage==b.damage;}
};
int limit(int id);
int durability(int id);
const char* name(int id);
const char* english_name(int id);
bool valid(int id);
int food_points(int id);
float food_saturation(int id);
bool block(int id);
bool pickaxe(int id);
enum class Tool {None,Pickaxe,Axe,Shovel,Hoe,Sword,Shears};
Tool tool_kind(int id);
int tool_tier(int id);
double tool_speed(int id);
void wear(Stack& stack,int amount=1);
// Equipment is ordered as the GUI: helmet, chestplate, leggings, boots.
int armor_slot(int id);
int armor_material(int id);
int armor_points(int id);
float armor_toughness(int id);
float absorb_damage(std::array<Stack,4>& armor,float damage);
bool equip(std::array<Stack,4>& armor,Stack& source);
void armor_click(Stack& slot,Stack& cursor,int part,bool right);
int mining_wear(int block_id,int tool);
int use_on_block(int block_id,int tool,bool air_above,bool bottom_face);
int clock_frame(double day_ticks);
int compass_frame(double dx,double dz,double forward_x,double forward_z);
void normalize(Stack& s);
void click(Stack& slot,Stack& cursor,bool right);
// Returns the number still uninserted; merges existing stacks before empty slots.
int insert(Stack* slots,int count,Stack value);
struct Recipe {Stack output;std::array<bool,9> used{};};
Recipe recipe(const std::array<Stack,9>& grid,int width);
struct Inventory {
    std::array<Stack,36> main{};
    std::array<Stack,9> craft{};
    std::array<Stack,4> armor{};
    Stack offhand,cursor;
    int add(Stack s){return insert(main.data(),36,s);}
    void quick_move(int index);
    bool take_result(int width,bool quick);
    std::vector<Stack> close();
};
double hardness(int id);
bool harvestable(int block_id,int tool);
double break_seconds(int block_id,int tool,bool grounded);
Stack drop(int block_id,int tool,unsigned random);
struct Vitals {
    float health=20,saturation=5,exhaustion=0;
    int food=20,timer=0,hurt_ticks=0;
    double fallen=0;
    void hurt(float amount);
    void exhaust(float amount);
    bool eat(int id);
    void tick();
    void landing(double downward,bool grounded);
};
}
