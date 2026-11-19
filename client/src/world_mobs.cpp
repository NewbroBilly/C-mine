#include <world.h>
#include <avatar.h>
#include <settings.h>
namespace {
dvec3 position(const Mobs::Entity& e){return {e.position[0],e.position[1],e.position[2]};}
uint32_t next_random(uint32_t& r){if(!r)r=1;r^=r<<13;r^=r>>17;r^=r<<5;return r;}
}
int World::mob_cell(int x,int y,int z) const{
    if(y<0)return 1;if(y>=256)return 0;
    ivec2 cp(int(floor(x/16.)),int(floor(z/16.)));auto it=chunk_map.find(cp);if(it==chunk_map.end() || !it->second->can_draw)return 3;
    int id=it->second->get(ivec3(x-cp.x*16,y,z-cp.y*16))->id;
    if(id==WATER || id==LAVA_BLOCK)return 2;if(id==AIR || id==GRASS || id==TORCH || id==FIRE)return 0;return 1;
}
int World::target_mob(dvec3 origin,dvec3 direction,double reach){
    int found=-1;for(int i=0;i<int(mobs.size());++i){double t=Mobs::ray_distance(mobs[i],{origin.x,origin.y,origin.z},{direction.x,direction.y,direction.z},reach);if(t>=0 && t<reach){found=i;reach=t;}}return found;
}
void World::hit_mob(int index,float damage,dvec3 direction){
    if(index<0 || index>=int(mobs.size()))return;auto& e=mobs[index];if(e.hurt>0)return;
    e.health-=damage;e.hurt=.5;e.flee=Mobs::traits(e.kind).hostile?0:4;
    if(length(dvec2(direction.x,direction.z))>1e-6){direction=normalize(dvec3(direction.x,0,direction.z));e.velocity[0]+=direction.x*8;e.velocity[2]+=direction.z*8;e.yaw=atan2(direction.x,direction.z);}
    e.velocity[1]=5;e.grounded=false;
    if(e.health>0)return;
    auto loot=[&](int id,int count){player->drops.push_back({{id,count},position(e)+dvec3(0,.4,0),dvec3(0,2,0),0});};
    if(e.kind==Mobs::Pig)loot(Items::RAW_PORKCHOP,1+next_random(e.random)%3);
    if(e.kind==Mobs::Cow){loot(Items::RAW_BEEF,1+next_random(e.random)%3);loot(Items::LEATHER,1);}
    if(e.kind==Mobs::Sheep){loot(Items::RAW_MUTTON,1+next_random(e.random)%2);if(!e.sheared)loot(WHITE_WOOL,1);}
    if(e.kind==Mobs::Zombie)loot(Items::ROTTEN_FLESH,1+next_random(e.random)%2);
    if(e.kind==Mobs::Creeper)loot(Items::GUNPOWDER,1+next_random(e.random)%2);
    mobs.erase(mobs.begin()+index);
}
void World::explode(dvec3 center){
    dvec3 delta=player->pos+dvec3(0,.9,0)-center;double distance=length(delta);
    if(distance<6){bool obstructed=distance>.001 && raycast(vec3(center),vec3(delta/distance),float(std::max(.01,distance-.3)),get_chunk_from_pos(vec3(center))).hit;
        double exposure=obstructed?.15:1.;player->hurt(float(((1-distance/6)*(1-distance/6)+(1-distance/6))*21*exposure+1));
        if(survival_mode && distance>.01)player->vel+=normalize(delta)*(1-distance/6)*12.;}
    // A bounded sphere, with all affected chunks rebuilt once after the blast.
    for(int x=-3;x<=3;++x)for(int y=-3;y<=3;++y)for(int z=-3;z<=3;++z){
        ivec3 p=ivec3(floor(center))+ivec3(x,y,z);if(x*x+y*y+z*z>7 || p.y<1 || p.y>=256)continue;
        auto* c=get_chunk_from_pos(vec3(p));if(!c)continue;ivec3 local=p-ivec3(chunk_pos_to_world_hp(c->pos));auto old=*c->get(local);
        if(old.id==AIR || old.id==WATER || old.id==BEDROCK || old.id==OBSIDIAN || Items::hardness(old.id)>5)continue;
        BlockState air{AIR,0,0};c->set(local,&air);notify_water(c,local);update_lighting(c,local,false,&old);
    }
    for(auto& e:mobs){double d=length(position(e)-center);if(d<4)e.health-=float((4-d)*6);}
}
void World::update_mobs(){
    if(!initial_mobs && chunk_vector.size()>=9){mob_random=uint32_t(seed)^uint32_t(uint64_t(seed)>>32)^0x73af12;mob_spawn_clock=20;}
    mob_spawn_clock+=std::min(delta_time,.25);
    if(mob_spawn_clock>=20 && chunk_vector.size()>=9){
        mob_spawn_clock=0;bool first=!initial_mobs;initial_mobs=true;
        int nearby=0,passive=0;for(auto& e:mobs)if(length(position(e)-player->pos)<64){++nearby;if(!Mobs::traits(e.kind).hostile)++passive;}
        auto query=[&](int x,int y,int z){return mob_cell(x,y,z);};
        // Sample loaded columns only. This never generates terrain or schedules fluids.
        for(int attempt=0;attempt<100 && nearby<24 && mobs.size()<128;++attempt){
            auto r=next_random(mob_random);bool hostile=day_ticks>13000 && day_ticks<23000 && (passive>=6 || attempt%3==0);
            if(!hostile && passive>=12)continue;
            static const int hostile_kinds[]={Mobs::Zombie,Mobs::Creeper,Mobs::Skeleton,Mobs::Spider,Mobs::Enderman,Mobs::Slime,Mobs::Ghast,Mobs::Blaze,Mobs::Witch,Mobs::Wither};
            static const int passive_kinds[]={Mobs::Pig,Mobs::Cow,Mobs::Sheep,Mobs::Chicken,Mobs::Rabbit,Mobs::Wolf,Mobs::Ocelot,Mobs::Horse};
            int kind=hostile?hostile_kinds[r%(sizeof(hostile_kinds)/sizeof(hostile_kinds[0]))]:passive_kinds[r%(sizeof(passive_kinds)/sizeof(passive_kinds[0]))];
            double angle=next_random(mob_random)/4294967296.*6.2831853;
            double radius=(first?9:24)+next_random(mob_random)%24;
            int x=int(floor(player->pos.x+sin(angle)*radius)),z=int(floor(player->pos.z+cos(angle)*radius));
            auto* c=get_chunk_from_pos(vec3(x,0,z));if(!c || !c->can_draw)continue;
            int lx=x-c->pos.x*16,lz=z-c->pos.y*16;
            for(int y=252;y>0;--y){int id=c->get(ivec3(lx,y,lz))->id;if(id==AIR || id==GRASS || id==TORCH)continue;
                bool ground=hostile?(mob_cell(x,y,z)==1 && id!=LEAVES && id!=CACTUS):(id==GRASS_BLOCK || id==DIRT || id==PODZOL);
                if(ground){auto e=Mobs::create(kind,{x+.5,double(y+1),z+.5},next_random(mob_random));
                    bool crowded=false;for(auto& other:mobs)if(length(position(other)-position(e))<3)crowded=true;
                    if(!crowded && Mobs::fits(e,e.position,query)){e.grounded=true;mobs.push_back(e);++nearby;if(!hostile)++passive;}}
                break;
            }
        }
    }
    mob_clock+=std::min(delta_time,.25);
    while(mob_clock>=.05){mob_clock-=.05;
        for(size_t i=0;i<mobs.size();){auto& e=mobs[i];dvec3 p=position(e);
            if(e.health<=0 || p.y<0 || (Mobs::traits(e.kind).hostile && length(p-player->pos)>128)){mobs.erase(mobs.begin()+i);continue;}
            auto* c=get_chunk_from_pos(vec3(p));if(!c || !c->can_draw){e.previous=e.position;++i;continue;}
            dvec3 eye=p+dvec3(0,Mobs::traits(e.kind).height*.8,0),direction=player->pos+dvec3(0,1,0)-eye;double range=length(direction);
            bool visible=false;if(range<25 && range>.001)visible=!raycast(vec3(eye),vec3(direction/range),float(range),c).hit;
            int action=Mobs::tick(e,{player->pos.x,player->pos.y,player->pos.z},survival_mode && player->vitals.health>0,visible,[&](int x,int y,int z){return mob_cell(x,y,z);});
            if(action==1)player->hurt(3);
            if(action==2){dvec3 blast=position(e)+dvec3(0,.6,0);mobs.erase(mobs.begin()+i);explode(blast);continue;}
            if(e.kind==Mobs::Zombie && day_ticks<12000 && !underwater(p+dvec3(0,1,0))){
                ivec3 local=ivec3(floor(p))-ivec3(chunk_pos_to_world_hp(c->pos));if(in_chunk(local) && c->get_sky_light(local)>=14){e.health-=.05f;if(int(day_ticks)%20<10)e.hurt=.08;}}
            ++i;
        }
    }
    mobs.erase(std::remove_if(mobs.begin(),mobs.end(),[](const Mobs::Entity& e){return e.health<=0;}),mobs.end());
}
void World::draw_mobs(const mat4& camera){for(auto& e:mobs)if(get_chunk_from_pos(vec3(position(e))) && length(position(e)-player->render_position())<Settings::options.distance*16)Avatar::mob(e,camera,mob_clock/.05);}
