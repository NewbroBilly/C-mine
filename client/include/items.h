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
struct Stack {
    int id=-1,count=0,damage=0;
    bool empty() const {return id<1 || count<=0;}
    void clear(){id=-1;count=damage=0;}
    bool operator==(const Stack& b) const{return id==b.id && count==b.count && damage==b.damage;}
};
int limit(int id);
int durability(int id);
const char* name(int id);
bool valid(int id);
bool block(int id);
bool pickaxe(int id);
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
