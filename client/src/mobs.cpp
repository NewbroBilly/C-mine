#include <mobs.h>
#include <items.h>
#include <algorithm>
#include <cmath>
#include <limits>
namespace Mobs {
Traits traits(int kind){static const Traits t[]={{.9,.9,.25f,10,false},{.9,1.4,.2f,10,false},{.9,1.3,.23f,8,false},{.6,1.95,.23f,20,true},{.6,1.7,.25f,20,true},{.5,.8,.25f,4,false},{.6,1.99,.23f,20,true},{1.4,.9,.3f,16,true},{.6,2.9,.23f,40,true},{.8,.8,.2f,16,true},{4,4,.1f,10,true},{.7,1.8,.22f,20,true},{.6,1.95,.2f,26,true},{.9,3.5,.2f,300,true},{.5,.5,.18f,6,false},{.6,.7,.28f,3,false},{.8,.9,.25f,8,false},{.8,.8,.25f,10,false},{1.4,1.6,.25f,30,false}};return t[std::clamp(kind,0,int(Count)-1)];}
Entity create(int kind,std::array<double,3> p,uint32_t seed){Entity e;e.kind=kind;e.position=e.previous=p;e.health=traits(kind).health;e.random=seed?seed:1;e.yaw=e.previous_yaw=(seed%6283)/1000.;return e;}
static uint32_t random(Entity& e){e.random^=e.random<<13;e.random^=e.random>>17;e.random^=e.random<<5;return e.random;}
bool fits(const Entity& e,const std::array<double,3>& p,const Query& query){auto t=traits(e.kind);double w=t.width/2;
    for(int x=int(floor(p[0]-w+1e-7));x<=int(floor(p[0]+w-1e-7));++x)
    for(int y=int(floor(p[1]+1e-7));y<=int(floor(p[1]+t.height-1e-7));++y)
    for(int z=int(floor(p[2]-w+1e-7));z<=int(floor(p[2]+w-1e-7));++z){int b=query(x,y,z);if(b==1 || b==3)return false;}return true;
}
int tick(Entity& e,const std::array<double,3>& target,bool survival,bool visible,const Query& q){
    e.previous=e.position;e.previous_yaw=e.yaw;++e.age_ticks;if(e.attack_ticks>0)--e.attack_ticks;auto t=traits(e.kind);
    e.cooldown=std::max(0.,e.cooldown-.05);e.hurt=std::max(0.,e.hurt-.05);e.flee=std::max(0.,e.flee-.05);e.decision-=.05;
    double dx=target[0]-e.position[0],dz=target[2]-e.position[2],distance=std::hypot(dx,dz),dy=target[1]-e.position[1];
    bool chasing=t.hostile && survival && visible && distance<24 && std::abs(dy)<12;
    e.arms_raised=chasing;
    bool moving=chasing || e.flee>0;
    if(chasing)e.yaw=std::atan2(dx,dz);
    else{if(e.decision<=0){e.yaw+=(int(random(e)%240)-120)*.025;e.decision=1.+random(e)%80/20.;}moving|=e.decision>1.5;}
    if(e.kind==Creeper){if(chasing && distance<3 && std::abs(dy)<2.5)e.fuse=std::min(1.5,e.fuse+.05);else e.fuse=std::max(0.,e.fuse-.05);if(e.fuse>0)moving=false;if(e.fuse>=1.5)return 2;}
    int event=0;if(e.kind==Zombie && chasing && distance<1.4 && std::abs(dy)<1.8 && e.cooldown<=0){event=1;e.cooldown=1.;e.attack_ticks=6;}
    double vx=moving?std::sin(e.yaw)*t.speed*(e.flee>0?1.6:1.):0,vz=moving?std::cos(e.yaw)*t.speed*(e.flee>0?1.6:1.):0;
    // Land acceleration and inertia use the same 20Hz convention as the player.
    double acceleration=e.grounded?.1:.02;
    e.velocity[0]=e.velocity[0]*.546+vx*acceleration*40;
    e.velocity[2]=e.velocity[2]*.546+vz*acceleration*40;
    auto next=e.position;next[0]+=e.velocity[0]*.05;next[2]+=e.velocity[2]*.05;
    bool water=q(int(floor(e.position[0])),int(floor(e.position[1]+.2)),int(floor(e.position[2])))==2;
    if(e.grounded && moving){auto probe=next;probe[0]+=std::sin(e.yaw)*.3;probe[2]+=std::cos(e.yaw)*.3;probe[1]-=.6;
        if(!fits(e,next,q)){e.velocity[1]=8.4;e.grounded=false;}
        else if(fits(e,probe,q) && !chasing){e.yaw+=3.14159265;e.velocity[0]=e.velocity[2]=0;}}
    if(water){e.velocity[1]=e.velocity[1]*.8+.5;if(e.velocity[1]<2)e.velocity[1]+=0.8;}
    else e.velocity[1]=(e.velocity[1]-1.6)*.98;
    for(int axis:{1,0,2}){double travel=e.velocity[axis]*.05;int steps=std::max(1,int(std::ceil(std::abs(travel)/.15)));double step=travel/steps;
        if(axis==1)e.grounded=false;
        for(int i=0;i<steps;++i){auto p=e.position;p[axis]+=step;
            if(fits(e,p,q))e.position=p;
            else{double low=0,high=1;for(int j=0;j<16;++j){double mid=(low+high)/2;p=e.position;p[axis]+=step*mid;if(fits(e,p,q))low=mid;else high=mid;}e.position[axis]+=step*low;if(axis==1 && step<0)e.grounded=true;e.velocity[axis]=0;break;}}
    }
    e.walk.tick(e.position[0]-e.previous[0],e.position[2]-e.previous[2]);e.phase=e.walk.phase;
    return event;
}
double ray_distance(const Entity& e,const std::array<double,3>& origin,const std::array<double,3>& direction,double reach){auto t=traits(e.kind);double near=0,far=reach;
    for(int a=0;a<3;++a){double low=e.position[a]-(a==1?0:t.width/2),high=e.position[a]+(a==1?t.height:t.width/2);
        if(std::abs(direction[a])<1e-12){if(origin[a]<low || origin[a]>high)return -1;continue;}
        double x=(low-origin[a])/direction[a],y=(high-origin[a])/direction[a];if(x>y)std::swap(x,y);near=std::max(near,x);far=std::min(far,y);if(near>far)return -1;}return near;
}
float melee_damage(int item){if(Items::tool_kind(item)==Items::Tool::Sword)return 4+Items::tool_tier(item);if(Items::tool_kind(item)==Items::Tool::Axe)return item==Items::WOOD_AXE || item==Items::GOLD_AXE?7:9;return 1;}
}
