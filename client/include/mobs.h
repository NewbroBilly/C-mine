#pragma once
#include <array>
#include <functional>
#include <cstdint>
#include <model_animation.h>
namespace Mobs {
enum Kind {Pig,Cow,Sheep,Zombie,Creeper,Count};
struct Entity {
    int kind=Pig;
    std::array<double,3> position{},previous{},velocity{};
    double yaw=0,phase=0,decision=0,flee=0,fuse=0,cooldown=0,hurt=0;
    float health=10;
    bool grounded=false,sheared=false;
    uint32_t random=1;
    ModelAnimation::Walk walk;
    double previous_yaw=0;
    int age_ticks=0,attack_ticks=0;
    bool arms_raised=false;
};
struct Traits {double width,height,speed;float health;bool hostile;};
Traits traits(int kind);
Entity create(int kind,std::array<double,3> position,uint32_t seed);
using Query=std::function<int(int,int,int)>; // 0 air, 1 solid, 2 water, 3 unavailable
bool fits(const Entity& entity,const std::array<double,3>& position,const Query& query);
// Fixed 20Hz movement and simple local steering; no chunks are generated here.
// Returns 1 for melee contact, 2 for a completed creeper fuse.
int tick(Entity& entity,const std::array<double,3>& player,bool survival,bool visible,const Query& query);
double ray_distance(const Entity& entity,const std::array<double,3>& origin,const std::array<double,3>& direction,double reach);
float melee_damage(int item);
}
