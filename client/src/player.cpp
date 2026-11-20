#include <language.h>
#include <player.h>
#include <movement.h>
#include <avatar.h>
#include <settings.h>
#include <game.h>
#include <ui.h>
#include <item_model.h>
#include <gtc/matrix_transform.hpp>
#include <random>

Player::Player(const dvec3& _pos)
{
    pos = previous_position = _pos;
    vel = dvec3(0);

    grounded = false;

    cam = Camera(vec3(0, 1.62, 0), Settings::options.fov, win_size.x / (float)win_size.y, 0.1f, 1000.0f);

    hitbox = Hitbox(dvec3(-0.3, 0, -0.3), dvec3(0.6, 1.8, 0.6));

    hit_info.hit = false;


}

vec3 get_input()
{
    vec3 input = vec3(0);

    if (is_key_pressed(SDLK_w))         input += vec3(0, 0, 1);
    if (is_key_pressed(SDLK_s))         input += vec3(0, 0, -1);
    if (is_key_pressed(SDLK_a))         input += vec3(-1, 0, 0);
    if (is_key_pressed(SDLK_d))         input += vec3(1, 0, 0);
    if (is_key_pressed(SDLK_LCTRL))     input += vec3(0, -1, 0);
    if (is_key_pressed(SDLK_SPACE))     input += vec3(0, 1, 0);

    return input;
}

void Player::handle_keys()
{
    if(show_inventory)return;
    for(int i=0;i<9;++i)if(is_key_down(SDLK_1+i))hotbar.set_index(i);
    if(is_key_down(SDLK_q)){
        auto& s=hotbar.slots[hotbar.index].value();
        if(!s.empty()){auto one=s;one.count=is_key_pressed(SDLK_LCTRL)?s.count:1;drop_item(one);s.count-=one.count;Items::normalize(s);}
    }
    if(is_key_down(SDLK_f)){
        auto& off=inventory.data().offhand;
        auto& main=hotbar.slots[hotbar.index].value();
        std::swap(main,off);
        equipped_item=main.id;
    }
    if(is_key_down(SDLK_w)){
        double now=SDL_GetTicks()/1000.;
        if(w_tap>=0 && now-w_tap<.35)sprinting=true;
        w_tap=now;
    }
    if(!is_key_pressed(SDLK_w))sprinting=false;
    if(!survival_mode && is_key_down(SDLK_SPACE)){
        double now=SDL_GetTicks()/1000.;
        if(flight_tap>=0 && now-flight_tap<.35){
            // Toggling flight changes the travel rules, not the current momentum
            // or interpolation clock. In particular, leaving flight must not
            // erase the horizontal motion used by the following falling ticks.
            flying=!flying;
            if(flying){grounded=false;jump_cooldown=0;vitals.fallen=0;}
            flight_tap=-1;
        }
        else flight_tap=now;
    }
}
bool Player::place_selected(Chunk* c,ivec3 local,ivec3 normal){
    auto& stack=hotbar.slots[hotbar.index].value();
    if(stack.empty() || !Items::block(stack.id))return false;
    BlockState bs{static_cast<unsigned char>(stack.id),0,0};
    if(bs.id==LOG)bs.data1=normal.x?0:(normal.y?1:2);
    if(bs.id>=OAK_STAIRS && bs.id<=PRISMARINE_STAIRS){
        // Store the vanilla horizontal facing plus the upper-half bit. The
        // face clicked above a block creates an upper stair, matching Java.
        int facing=normal.x? (normal.x>0?1:3) : (normal.z>0?0:2);
        bs.data1=char(facing | (normal.y<0?4:0));
    }
    if(bs.id==TORCH){if(normal.y<0)return false;bs.data1=normal.y?0:(normal.z==1?1:(normal.z==-1?2:(normal.x==1?3:4)));}
    Hitbox cube(dvec3(local)+chunk_pos_to_world_hp(c->pos)-pos,dvec3(1));
    if(intersect(cube,hitbox))return false;
    if(!world->place_block(c,local,&bs))return false;
    swing();if(survival_mode){--stack.count;Items::normalize(stack);}return true;
}
void Player::apply_item_effect(int id){
    // The clone has one deterministic effect profile for each bottle family;
    // this keeps the action useful even though potion NBT is not serialized yet.
    if(id==Items::POTION){speed_effect=240;regeneration_effect=100;}
    else if(id==Items::GOLDEN_APPLE || id==Items::ENCHANTED_GOLDEN_APPLE)regeneration_effect=200;
    else if(id==Items::ROTTEN_FLESH || id==Items::POISONOUS_POTATO || id==Items::SPIDER_EYE || id==Items::PUFFERFISH)poison_effect=100;
    else if(id==Items::MILK_BUCKET){speed_effect=strength_effect=regeneration_effect=poison_effect=0;fire_resistance_effect=night_vision_effect=0;}
}
bool Player::use_item_action(int id,Chunk* c,ivec3 local,ivec3 normal){
    auto message=[&](const char* ru,const char* en){item_message=Language::russian?ru:en;item_message_time=2.;};
    auto consume=[&](){if(survival_mode)consume_held();swing();action_delay=.25;};
    auto place_simple=[&](int block_id,const char* ru,const char* en,int data=0){
        if(!hit_info.hit)return false;
        auto target=local+normal;BlockState b{static_cast<unsigned char>(block_id),static_cast<char>(data&7),0};
        if(!world->place_block(c,target,&b))return false;
        message(ru,en);consume();return true;
    };
    if(hit_info.hit && (c->get(local)->id==DOOR_BLOCK)){
        auto state=c->get(local)->data1;state^=4;BlockState door{DOOR_BLOCK,state,0};c->set(local,&door);world->rebuild_dirty();message("Дверь открыта","Door toggled");action_delay=.2;return true;
    }
    if(id==Items::FLINT_AND_STEEL && hit_info.hit && c && c->get(local)->id==TNT_BLOCK){
        dvec3 blast=chunk_pos_to_world_hp(c->pos)+dvec3(local)+dvec3(.5);
        world->remove_block(c,local,true);world->explode(blast);message("TNT взорван","TNT ignited");
        if(survival_mode)Items::wear(hotbar.slots[hotbar.index].value(),1);action_delay=.5;return true;
    }
    const bool seed=id==Items::WHEAT_SEEDS || id==Items::BEETROOT_SEEDS || id==Items::PUMPKIN_SEEDS || id==Items::MELON_SEEDS;
    if(seed && hit_info.hit){
        auto support=c->get(local)->id;auto target=local+normal;auto* n=get_neighbor_chunk(c,target);
        bool supported=support==FARMLAND || ((id==Items::PUMPKIN_SEEDS || id==Items::MELON_SEEDS) && (support==DIRT || support==GRASS_BLOCK));
        if(supported && n && n->get(target)->id==AIR){BlockState crop{GRASS,0,char(id&7)};n->set(target,&crop);world->notify_water(n,target);world->rebuild_dirty();message("Семена посажены","Seeds planted");consume();return true;}
    }
    if(id==Items::BONE_MEAL && hit_info.hit){
        auto support=c->get(local)->id;auto target=local+normal;auto* n=get_neighbor_chunk(c,target);
        if((support==FARMLAND || support==GRASS_BLOCK || support==DIRT) && n && n->get(target)->id==AIR){BlockState crop{GRASS,0,7};n->set(target,&crop);world->notify_water(n,target);world->rebuild_dirty();message("Растение выросло","Plant grew");consume();return true;}
    }
    if(id==Items::LAVA_BUCKET && hit_info.hit){
        auto target=local+normal;auto* n=get_neighbor_chunk(c,target);
        // Lava simulation is intentionally represented by a lit source block
        // until the world fluid format gains a second fluid type.
        if(n && n->get(target)->id==AIR){BlockState lava{LAVA_BLOCK,0,0};if(world->place_block(c,target,&lava)){if(survival_mode){auto& s=hotbar.slots[hotbar.index].value();--s.count;Items::normalize(s);Items::Stack empty{Items::BUCKET,1};if(s.empty())s=empty;else if(inventory.data().add(empty))drop_item(empty);}message("Лава размещена","Lava placed");action_delay=.25;return true;}}
    }
    if(id==Items::BED && hit_info.hit && normal.y==1){
        const int orientation=normal.x? (normal.x>0?1:3) : (normal.z>0?2:0);
        if(place_simple(BED_BLOCK,"Точка возрождения установлена","Respawn point set",orientation)){
            const ivec3 target=local+normal;
            world->spawn_point=dvec3(chunk_pos_to_world_hp(c->pos))+dvec3(target)+dvec3(.5,0,.5);
            world->spawn_known=true;return true;
        }
    }
    // Items with a physical placement action use a simple solid anchor in the
    // compact renderer; they still obey collision, stack and survival rules.
    if(id==Items::OAK_DOOR || (id>=Items::SPRUCE_DOOR && id<=Items::DARK_OAK_DOOR)){
        int facing=normal.x?1:0;return place_simple(DOOR_BLOCK,"Дверь установлена","Door placed",facing);
    }
    if(id==Items::REDSTONE_DUST)return place_simple(REDSTONE_DUST_BLOCK,"Редстоун установлен","Redstone placed");
    if(id==Items::FLOWER_POT)return place_simple(FLOWER_POT_BLOCK,"Цветочный горшок установлен","Flower pot placed");
    if(id==Items::BANNER){
        const int orientation=normal.y?0:(normal.x? (normal.x>0?1:3) : (normal.z>0?2:4));
        return place_simple(BANNER_BLOCK,"Баннер установлен","Banner placed",orientation);
    }
    if(id==Items::PAINTING || id==Items::SIGN || id==Items::ITEM_FRAME || id==Items::ARMOR_STAND || id==Items::SKULL)return place_simple(PLANKS,"Предмет установлен","Item placed");
    if(id==Items::OAK_BOAT || (id>=Items::SPRUCE_BOAT && id<=Items::DARK_OAK_BOAT) || id==Items::MINECART || id==Items::CHEST_MINECART || id==Items::FURNACE_MINECART || id==Items::TNT_MINECART || id==Items::HOPPER_MINECART || id==Items::COMMAND_BLOCK_MINECART)return place_simple(PLANKS,"Транспорт установлен","Vehicle placed");
    if(id==Items::EMPTY_MAP){if(survival_mode){auto& s=hotbar.slots[hotbar.index].value();s.id=Items::FILLED_MAP;s.damage=0;}message("Карта открыта","Map opened");action_delay=.25;return true;}
    if(id==Items::WRITABLE_BOOK || id==Items::WRITTEN_BOOK || id==Items::ENCHANTED_BOOK || id==Items::KNOWLEDGE_BOOK){message("Книга открыта","Book opened");action_delay=.25;return true;}
    if(id==Items::FIREWORK_ROCKET || id==Items::FIREWORK_STAR){message("Фейерверк запущен","Firework launched");consume();return true;}
    if(id==Items::RECORD_13 || (id>=Items::RECORD_CAT && id<=Items::RECORD_WAIT)){message("Музыка играет","Music disc playing");action_delay=.5;return true;}
    if(id==Items::SADDLE || id==Items::IRON_HORSE_ARMOR || id==Items::GOLD_HORSE_ARMOR || id==Items::DIAMOND_HORSE_ARMOR || id==Items::LEAD || id==Items::NAME_TAG || id==Items::CARROT_ON_A_STICK){message("Предмет применён","Item used");action_delay=.25;return true;}
    if(id==Items::CAKE){message("Торт съеден","Cake eaten");consume();return true;}
    return false;
}
void Player::hurt(float damage,bool bypass){
    if(!survival_mode || vitals.hurt_ticks>0 || damage<=0)return;
    if(blocking && hotbar.get_item()==Items::SHIELD && !bypass){Items::wear(hotbar.slots[hotbar.index].value(),std::max(1,int(damage)+1));return;}
    vitals.hurt(bypass?damage:Items::absorb_damage(inventory.data().armor,damage));
}
void Player::consume_held(int result){
    if(!survival_mode)return;
    auto& stack=hotbar.slots[hotbar.index].value();--stack.count;Items::normalize(stack);
    if(result>0){Items::Stack out{result,1};if(stack.empty())stack=out;else {out.count=inventory.data().add(out);if(out.count)drop_item(out);}}
}
bool Player::teleport_near(dvec3 target){
    // Never generate chunks for pearls. Test the whole standing player box,
    // including headroom, and refuse unloaded/water/lava destinations.
    for(int dy=0;dy<=3;++dy)for(int dx=-1;dx<=1;++dx)for(int dz=-1;dz<=1;++dz){
        dvec3 p=target+dvec3(dx,dy,dz);bool safe=true;
        for(int x=int(floor(p.x-.3));x<=int(floor(p.x+.3));++x)
        for(int y=int(floor(p.y));y<=int(floor(p.y+1.8));++y)
        for(int z=int(floor(p.z-.3));z<=int(floor(p.z+.3));++z)
            if(world->mob_cell(x,y,z)!=0)safe=false;
        if(safe && p.y>=0 && p.y<254){pos=previous_position=p;vel=dvec3(0);accumulator=0;vitals.fallen=0;return true;}
    }return false;
}
void Player::launch(int item,float charge){
    auto kind=Items::projectile(item);float speed=Items::projectile_speed(kind,charge);
    dvec3 origin=pos+dvec3(cam.pos),velocity=dvec3(cam.forward)*double(speed)+dvec3(vel.x,grounded?0:vel.y,vel.z);
    shots.push_back({item,origin,velocity,0,kind==Items::Projectile::Arrow?float(std::ceil(speed/20*2)):0});
}
void Player::update_projectiles(){
    double remaining=std::min(delta_time,.25);
    while(remaining>1e-8){
        double dt=std::min(remaining,.01);remaining-=dt;
        for(auto it=shots.begin();it!=shots.end();){
            auto kind=Items::projectile(it->item);dvec3 travel=it->velocity*dt;double distance=length(travel);
            bool erase=(it->age+=dt)>15 || it->position.y<0 || it->position.y>=256;
            if(!erase && distance>1e-8){
                dvec3 direction=travel/distance;
                auto hit=raycast(vec3(it->position),vec3(direction),float(distance),world->get_chunk_from_pos(vec3(it->position)));
                double reach=hit.hit?std::min(distance,double(length(dvec3(hit.hit_point)-it->position))):distance;
                int mob=world->target_mob(it->position,direction,reach+1e-5);
                // Projectiles stop at unloaded frontiers instead of tunnelling.
                dvec3 next=it->position+travel;
                if(world->mob_cell(int(floor(next.x)),int(floor(next.y)),int(floor(next.z)))==3)erase=true;
                else if(mob>=0 || hit.hit){
                    if(mob>=0){auto& e=world->mobs[mob];double t=Mobs::ray_distance(e,{it->position.x,it->position.y,it->position.z},{direction.x,direction.y,direction.z},reach+1e-5);
                        next=it->position+direction*std::max(0.,t);if(it->damage>0)world->hit_mob(mob,it->damage,direction);}
                    else next=dvec3(hit.hit_point)-direction*.1;
                    if(kind==Items::Projectile::Potion)apply_item_effect((it->item==Items::EXPERIENCE_BOTTLE || it->item==Items::SPLASH_POTION || it->item==Items::LINGERING_POTION)?Items::POTION:it->item);
                    if(kind==Items::Projectile::Pearl && teleport_near(next))hurt(5,true);
                    erase=true;
                }
                if(!erase){it->position=next;it->velocity*=std::pow(.99,dt*20);it->velocity.y-=Items::projectile_gravity(kind)*dt;}
            }
            if(erase)it=shots.erase(it);else ++it;
        }
    }
}
void Player::handle_clicks(){
    action_delay=std::max(0.,action_delay-delta_time);
    item_message_time=std::max(0.,item_message_time-delta_time);
    if(show_inventory){hit_info.hit=false;break_progress=eat_progress=bow_time=0;blocking=false;return;}
    vec3 origin=vec3(pos)+cam.pos;
    hit_info=raycast(origin,cam.forward,survival_mode?4.5f:5.f,world->get_chunk_from_pos(origin));
    bool attack=is_button_down(0) || is_button_pressed(0);
    bool use=is_button_down(2) || is_button_pressed(2);
    int held=hotbar.get_item();
    if(!use || eating_item!=held){eat_progress=0;eating_item=held;}
    if(held!=Items::BOW)bow_time=0;
    blocking=held==Items::SHIELD && use;
    if(attack)swing();
    double reach=hit_info.hit?double(length(hit_info.hit_point-origin)):(survival_mode?4.5:5.);
    int mob=world->target_mob(origin,cam.forward,reach);
    if(mob>=0){
        if(is_button_down(2) && hotbar.get_item()==Items::SHEARS && world->mobs[mob].kind==Mobs::Sheep && !world->mobs[mob].sheared){world->mobs[mob].sheared=true;drop_item({WHITE_WOOL,2});if(survival_mode)Items::wear(hotbar.slots[hotbar.index].value());swing();}
        if(attack && action_delay<=0){float damage=Mobs::melee_damage(hotbar.get_item())+(strength_effect>0?3.f:0.f);world->hit_mob(mob,damage,dvec3(cam.forward));if(survival_mode && Items::durability(hotbar.get_item()))Items::wear(hotbar.slots[hotbar.index].value(),Items::tool_kind(hotbar.get_item())==Items::Tool::Sword?1:2);action_delay=.6;}
        // Entity targeting blocks mining/placement, not item use. Returning here
        // used to prevent drawing/releasing a bow and made cow milking unreachable.
        hit_info.hit=false;break_progress=0;
    }
    if(hit_info.hit){
        int id=hit_info.chunk->get(hit_info.block_pos)->id;
        ivec3 target=ivec3(chunk_pos_to_world_hp(hit_info.chunk->pos))+hit_info.block_pos;
        if(attack && action_delay<=0){
            if(mining_position!=target || mining_tool!=hotbar.get_item()){break_progress=0;mining_position=target;mining_tool=hotbar.get_item();}
            break_duration=survival_mode?Items::break_seconds(id,hotbar.get_item(),grounded):0;
            if(break_duration>=0){break_progress+=delta_time;
                if(break_progress>=break_duration){
                    static std::mt19937 random(81421);
                    auto loot=survival_mode?Items::drop(id,hotbar.get_item(),random()):Items::Stack{};
                    for(int n=0;n<7;++n){double a=(random()%6283)/1000.;particles.push_back({id,dvec3(target)+dvec3(.5),dvec3(cos(a)*.8,1.1+(random()%100)/120.,sin(a)*.8),0});}
                    world->remove_block(hit_info.chunk,hit_info.block_pos,!survival_mode);
                    if(!loot.empty())drops.push_back({loot,dvec3(target)+dvec3(.5),dvec3(0,2,0),0});
                    if(survival_mode){vitals.exhaust(.005f);auto& tool=hotbar.slots[hotbar.index].value();Items::wear(tool,Items::mining_wear(id,tool.id));}
                    break_progress=0;action_delay=.25;
                }
            }
        }
        if(is_button_down(1)){
            int i=hotbar.find_item(id);if(i!=-1)hotbar.set_index(i);else if(!survival_mode)hotbar.set_item(id);
        }
        if(is_button_down(2) && id==CRAFTING_TABLE && survival_mode && !is_key_pressed(SDLK_LSHIFT)){Game::inventory(true,true);return;}
    }else break_progress=0;
    if(!attack)break_progress=0;
    if(held==Items::BOW){
        eat_progress=0;
        int ammo=-1;
        for(int i=0;i<36;++i)if(!inventory.data().main[i].empty() && Items::projectile(inventory.data().main[i].id)==Items::Projectile::Arrow){ammo=i;break;}
        if(use && (!survival_mode || ammo>=0))bow_time+=std::min(delta_time,.25);
        else if(bow_time>0){
            float charge=Items::bow_charge(float(bow_time));bow_time=0;
            if(charge>=.1f && (!survival_mode || ammo>=0)){
                int arrow=ammo>=0?inventory.data().main[ammo].id:int(Items::ARROW);launch(arrow,charge);
                if(survival_mode){auto& s=inventory.data().main[ammo];--s.count;Items::normalize(s);Items::wear(hotbar.slots[hotbar.index].value());}
            }
        }return;
    }
    auto throwable=Items::projectile(held);
    if(is_button_down(2) && action_delay<=0 && throwable!=Items::Projectile::None && throwable!=Items::Projectile::Arrow){
        launch(held);consume_held();swing();action_delay=held==Items::ENDER_PEARL?1.:.25;return;
    }
    int spawn=Items::spawn_kind(held);
    if(is_button_down(2) && hit_info.hit && spawn>=0 && action_delay<=0){
        auto point=dvec3(hit_info.block_pos+hit_info.face_normal)+chunk_pos_to_world_hp(hit_info.chunk->pos)+dvec3(.5,0,.5);
        auto e=Mobs::create(spawn,{point.x,point.y,point.z},SDL_GetTicks()+unsigned(world->mobs.size()));
        if(world->mobs.size()<128 && Mobs::fits(e,e.position,[&](int x,int y,int z){return world->mob_cell(x,y,z);})){
            world->mobs.push_back(e);consume_held();swing();action_delay=.25;
        }return;
    }
    if(blocking)return;
    if(is_button_down(2) && Items::armor_slot(held)>=0){if(Items::equip(inventory.data().armor,hotbar.slots[hotbar.index].value()))swing();return;}
    if(is_button_down(2) && action_delay<=0 && held==Items::FISHING_ROD){
        if(fishing){
            if(fish_bite>0){Items::Stack catch_item{Items::RAW_FISH,1};if(inventory.data().add(catch_item))drop_item(catch_item);if(survival_mode)Items::wear(hotbar.slots[hotbar.index].value());}
            fishing=false;
        }else{
            for(double t=.5;t<=12.;t+=.2){
                auto p=dvec3(origin)+dvec3(cam.forward)*t;int level=world->fluid({int(floor(p.x)),int(floor(p.y)),int(floor(p.z))});
                if(Water::liquid(level)){
                    bobber=dvec3(p.x,floor(p.y)+Water::height(level),p.z);
                    while(Water::liquid(world->fluid({int(floor(bobber.x)),int(floor(bobber.y))+1,int(floor(bobber.z))})))bobber.y+=1.;
                    fishing=true;fish_timer=5.+(SDL_GetTicks()%7000)/1000.;fish_bite=0;break;
                }
                if(level==Water::Solid || level==Water::Unloaded)break;
            }
        }swing();action_delay=.3;return;
    }
    if(use && hit_info.hit && action_delay<=0){
        auto* c=hit_info.chunk;auto p=hit_info.block_pos;
        auto above=p+ivec3(0,1,0);auto* top=get_neighbor_chunk(c,above);
        bool air=top && top->get(above)->id==AIR;
        int replacement=Items::use_on_block(c->get(p)->id,held,air,hit_info.face_normal.y<0);
        if(replacement!=-1){
            BlockState b{static_cast<unsigned char>(replacement),0,0};c->set(p,&b);world->notify_water(c,p);world->rebuild_dirty();
            swing();if(survival_mode)Items::wear(hotbar.slots[hotbar.index].value());action_delay=.25;return;
        }
        if(held==Items::FLINT_AND_STEEL || held==Items::FIRE_CHARGE){
            BlockState fire{FIRE,0,0};
            auto target=p+hit_info.face_normal;auto* n=get_neighbor_chunk(c,target);
            if(n && n->get(target)->id!=WATER && world->place_block(c,p+hit_info.face_normal,&fire)){
                swing();if(held==Items::FIRE_CHARGE)consume_held();else if(survival_mode)Items::wear(hotbar.slots[hotbar.index].value());action_delay=.25;
            }return;
        }
    }
    if(is_button_down(2) && held==Items::BUCKET && mob>=0 && world->mobs[mob].kind==Mobs::Cow && action_delay<=0){consume_held(Items::MILK_BUCKET);swing();action_delay=.25;return;}
    if(use && action_delay<=0 && held==Items::GLASS_BOTTLE){
        auto hit=raycast(origin,cam.forward,survival_mode?4.5f:5.f,world->get_chunk_from_pos(origin),true);
        if(hit.hit && hit.chunk->get(hit.block_pos)->id==WATER){consume_held(Items::POTION);swing();action_delay=.25;}return;
    }
    if(use && action_delay<=0 && (held==Items::BUCKET || held==Items::WATER_BUCKET || held==Items::LAVA_BUCKET)){
        auto hit=raycast(origin,cam.forward,survival_mode?4.5f:5.f,world->get_chunk_from_pos(origin),held==Items::BUCKET);
        if(hit.hit){auto& stack=hotbar.slots[hotbar.index].value();bool done=false;int result=Items::BUCKET;
            if(held==Items::BUCKET && hit.chunk->get(hit.block_pos)->id==WATER && (hit.chunk->get(hit.block_pos)->data1&15)==0){world->remove_block(hit.chunk,hit.block_pos,true);done=true;result=Items::WATER_BUCKET;}
            if(held==Items::BUCKET && hit.chunk->get(hit.block_pos)->id==LAVA_BLOCK){world->remove_block(hit.chunk,hit.block_pos,true);done=true;result=Items::LAVA_BUCKET;}
            if(held==Items::WATER_BUCKET){BlockState b{WATER,0,0};done=world->place_block(hit.chunk,hit.block_pos+hit.face_normal,&b);}
            if(held==Items::LAVA_BUCKET){BlockState b{LAVA_BLOCK,0,0};done=world->place_block(hit.chunk,hit.block_pos+hit.face_normal,&b);}
            if(done){if(survival_mode){--stack.count;Items::normalize(stack);Items::Stack out{result,1};if(stack.empty())stack=out;else if(inventory.data().add(out))drop_item(out);}else if(held==Items::BUCKET)stack={Items::WATER_BUCKET,1};action_delay=.25;}
        }return;
    }
    bool drink=held==Items::MILK_BUCKET || held==Items::POTION;
    bool edible=survival_mode && Items::food_points(held)>0 && (vitals.food<20 || Items::always_edible(held));
    if(use && (edible || drink)){
        if(eating_item!=held){eating_item=held;eat_progress=0;}
        eat_progress+=delta_time;
        if(eat_progress>=1.6){
            if(drink || vitals.eat(held)){
                apply_item_effect(held);
                consume_held(Items::container_after_use(held));
                if(held==Items::CHORUS_FRUIT)for(int attempt=0;attempt<16;++attempt){double angle=(SDL_GetTicks()+attempt*937)*.013;
                    if(teleport_near(pos+dvec3(sin(angle)*8,(attempt%5)-2,cos(angle)*8)))break;}
            }eat_progress=0;
        }
        return;
    }
    eat_progress=0;
    if(use && action_delay<=0){
        if(use_item_action(held,hit_info.chunk,hit_info.block_pos,hit_info.face_normal))return;
        if(hit_info.hit && place_selected(hit_info.chunk,hit_info.block_pos+hit_info.face_normal,hit_info.face_normal))action_delay=.2;
        else if(is_button_down(2) && held>Items::ITEM_START){
            item_message=Language::russian?"Предмет применён":"Item used";item_message_time=2;
        }
    }
}
void Player::drop_item(Items::Stack s){if(!s.empty())drops.push_back({s,pos+dvec3(cam.forward)*.7+dvec3(0,1.2,0),dvec3(cam.forward)*2.+dvec3(0,1,0),-1});}
void Player::update_fishing(){
    if(!fishing)return;
    Water::Pos p{int(floor(bobber.x)),int(floor(bobber.y-.01)),int(floor(bobber.z))};
    if(hotbar.get_item()!=Items::FISHING_ROD || length(pos-bobber)>32. || !Water::liquid(world->fluid(p))){fishing=false;return;}
    if(fish_bite>0){fish_bite-=delta_time;if(fish_bite<=0)fish_timer=5.+(SDL_GetTicks()%7000)/1000.;}
    else{fish_timer-=delta_time;if(fish_timer<=0)fish_bite=1.5;}
}
void Player::update_drops(){
    for(auto it=particles.begin();it!=particles.end();){it->age+=delta_time;if(it->age>.8){it=particles.erase(it);continue;}it->velocity.y-=8.*delta_time;it->position+=it->velocity*delta_time;++it;}
    for(auto it=drops.begin();it!=drops.end();){
        it->age+=delta_time;if(it->age>300){it=drops.erase(it);continue;}
        dvec3 next=it->position+it->velocity*delta_time;it->velocity.y-=9.8*delta_time;
        auto* c=world->get_chunk_from_pos(next);
        if(c && next.y>=0 && next.y<CHUNK_HEIGHT){
            ivec3 local=ivec3(floor(next))-ivec3(chunk_pos_to_world_hp(c->pos));int id=c->get(local)->id;
            if(id==WATER){auto flow=Water::flow({int(floor(next.x)),int(floor(next.y)),int(floor(next.z))},[&](Water::Pos p){return world->fluid(p);});it->velocity*=pow(.8,delta_time*20);it->velocity+=dvec3(flow[0],flow[1],flow[2])*delta_time*5.;}
            else if(id!=AIR && id!=GRASS && id!=TORCH && id!=LAVA_BLOCK && id!=FIRE){if(it->velocity.y<0){next.y=floor(next.y)+1.15;it->velocity=dvec3(0);}}
            it->position=next;
        }
        if(it->age>.4 && length(it->position-(pos+dvec3(0,.8,0)))<1.6){it->stack.count=inventory.data().add(it->stack);if(it->stack.count==0){it=drops.erase(it);continue;}}
        ++it;
    }
}

void Player::move()
{
    vec3 right = normalize(cross(cam.up, cam.forward));
    if (mouse_control && !show_inventory) {
        int x,y; SDL_GetRelativeMouseState(&x,&y);
        vec3 f = rotate(cam.forward, radians(.07f*y*(Settings::options.invert_y?-1:1)*Settings::options.sensitivity/100.f), right);
        if (dot(right,cross(cam.up,f)) > .01f) cam.forward = f;
        cam.forward = rotate(cam.forward, radians(.07f*x*Settings::options.sensitivity/100.f), cam.up);
    }
    vec3 input = show_inventory ? vec3(0) : get_input();
    // Catch up up to five simulation ticks after a slow render frame.
    double dt = std::min(delta_time,.25);
    dvec3 direction = dvec3(right)*double(input.x) + normalize(dvec3(cam.forward.x,0,cam.forward.z))*double(input.z);
    if (length(direction)>1) direction = normalize(direction);
    auto solid = [](int x,int y,int z) {
        if (y<0) return true;
        if (y>=CHUNK_HEIGHT) return false;
        Chunk* c = world->get_chunk_from_pos(dvec3(x,y,z));
        if (!c) return true;
        ivec3 local = dvec3(x,y,z)-chunk_pos_to_world_hp(c->pos);
        int id = c->get(local)->id;
        if(id==DOOR_BLOCK && (c->get(local)->data1&4))return false;
        const bool passable=id==AIR || id==GRASS || id==TORCH || id==WATER || id==LAVA_BLOCK || id==FIRE ||
            id==SAPLING || id==DEADBUSH || id==FERN || id==WHEAT_CROP || id==CARROT_CROP || id==POTATO_CROP ||
            id==BEETROOT_CROP || id==NETHER_WART_CROP || id==VINE || id==CHORUS_FLOWER || id==WATERLILY ||
            id==BANNER_BLOCK || id==FLOWER_POT_BLOCK || id==RAIL || id==POWERED_RAIL || id==DETECTOR_RAIL ||
            id==ACTIVATOR_RAIL || id==LADDER || id==REDSTONE_DUST_BLOCK;
        return !passable;
    };
    const bool sneak=!show_inventory && is_key_pressed(SDLK_LSHIFT);
    const bool sprint=!show_inventory && (is_key_pressed(SDLK_LCTRL) || sprinting || speed_effect>0) && input.z>0 && !sneak && (!survival_mode || vitals.food>6);
    const bool jump=!show_inventory && is_key_pressed(SDLK_SPACE);
    accumulator += dt;
    Movement::State state{pos,vel,grounded,jump_cooldown};
    while (accumulator >= .05-1e-9) {
        previous_position=state.position;
        double old_y=state.position.y;
        bool was_grounded=state.grounded;
        dvec3 flow;bool swimming=world->submerged(state.position,&flow);
        if(!survival_mode && flying){
            Movement::fly_tick(state,dvec2(direction.x,direction.z),double(jump)-double(sneak),sprint,solid);
            vitals.fallen=0;if(state.grounded)flying=false;
        }
        else if(swimming){Movement::swim_tick(state,dvec2(direction.x,direction.z),jump,flow,solid);vitals.fallen=0;}
        else Movement::tick(state,dvec2(direction.x,direction.z),jump,sprint,sneak,solid);
        if(survival_mode){
            if(!swimming)vitals.landing(old_y-state.position.y,state.grounded);
            if(jump && was_grounded && state.velocity.y>0)vitals.exhaust(sprint?.2f:.05f);
            if(sprint && state.grounded)vitals.exhaust(float(length(dvec2(state.velocity.x,state.velocity.z))*.05*.1));
        }
        model_walk.tick(state.position.x-previous_position.x,state.position.z-previous_position.z);++model_age;
        accumulator=std::max(0.,accumulator-.05);
    }
    pos = state.position; vel = state.velocity; grounded = state.grounded;jump_cooldown=state.jump_cooldown;
    walk_phase += length(dvec2(vel.x,vel.z))*dt*4.0;
}

dvec3 Player::render_position() const {
    if(length(pos-previous_position)>3.)return pos;
    return mix(previous_position,pos,clamp(accumulator/.05,0.,1.));
}

Camera Player::view_camera() {
    Camera view = cam;
    float roll=float(Movement::hurt_roll(vitals.hurt_ticks,vital_clock/.05));
    view.up=rotate(cam.up,radians(roll),cam.forward);
    if (!third_person) {
        float phase=float(walk_phase)*.6662f;
        vec3 right=normalize(cross(cam.up,cam.forward));
        view.pos+=right*(sin(phase)*bob_amount*.05f)-cam.up*(abs(cos(phase))*bob_amount*.05f);
        view.up=rotate(view.up,radians(sin(phase)*bob_amount*2.f),cam.forward);
        if(world->underwater(render_position()+dvec3(view.pos)))view.fov*=60.f/70.f;
        return view;
    }
    vec3 offset = cam.forward * (third_person == 1 ? -3.5f : 3.5f);
    float fraction = 0;
    for (float t=.05f;t<=1.f;t+=.05f) {
        dvec3 p = pos + dvec3(cam.pos + offset*t);
        Chunk* c = world->get_chunk_from_pos(p);
        if (!c || p.y<0 || p.y>=CHUNK_HEIGHT) break;
        ivec3 local = floor(p)-chunk_pos_to_world_hp(c->pos);
        int id=c->get(local)->id;
        if (id!=AIR && id!=GRASS && id!=TORCH && id!=WATER && id!=LAVA_BLOCK && id!=FIRE) break;
        fraction=t;
    }
    view.pos += offset*fraction;
    if (third_person==2) view.forward = -cam.forward;
    if(world->underwater(render_position()+dvec3(view.pos)))view.fov*=60.f/70.f;
    return view;
}

void Player::update()
{
    if (!world->spawned) return;
    animate();handle_keys();handle_clicks();move();update_drops();update_fishing();update_projectiles();
    bool burning=false;
    if(survival_mode)for(int x=int(floor(pos.x-.3));x<=int(floor(pos.x+.3));++x)
        for(int y=int(floor(pos.y));y<=int(floor(pos.y+1.8));++y)for(int z=int(floor(pos.z-.3));z<=int(floor(pos.z+.3));++z){
            auto* c=world->get_chunk_from_pos(dvec3(x,y,z));if(c && y>=0 && y<256){ivec3 p=ivec3(x,y,z)-ivec3(chunk_pos_to_world_hp(c->pos));if(c->get(p)->id==FIRE)burning=true;}}
    if(burning && fire_resistance_effect<=0){burn_clock+=delta_time;if(burn_clock>=1){hurt(1);burn_clock=0;}}else burn_clock=0;
    vital_clock+=delta_time;while(vital_clock>=.05){
        for(int* effect:{&speed_effect,&strength_effect,&regeneration_effect,&poison_effect,&fire_resistance_effect,&night_vision_effect})if(*effect>0)--*effect;
        if(poison_effect>0 && poison_effect%20==0)hurt(1,true);
        if(regeneration_effect>0 && regeneration_effect%20==0 && vitals.health<20)vitals.health=std::min(20.f,vitals.health+1.f);
        if(survival_mode){vitals.tick();if(world->underwater(pos+dvec3(cam.pos))){if(--air<=-20){air=0;hurt(2,true);}}else air=300;}
        else{air=300;if(vitals.hurt_ticks>0)--vitals.hurt_ticks;}
        vital_clock-=.05;
    }
    if(survival_mode){if(pos.y<0)hurt(4,true);
        if(vitals.health<=0 && pos.y>=0){auto& main=hotbar.slots[hotbar.index].value();auto& off=inventory.data().offhand;
            auto* totem=main.id==Items::TOTEM && !main.empty()?&main:off.id==Items::TOTEM && !off.empty()?&off:nullptr;
            if(totem){--totem->count;Items::normalize(*totem);vitals.health=1;vitals.hurt_ticks=20;vitals.fallen=0;}
        }
        if(vitals.health<=0)Game::die();}
}

void Player::draw()
{
    Camera view = view_camera();
    mat4 cam_mat = view.matrix((vec3)render_position());
    if(hit_info.hit && break_progress>0 && break_duration>0 && ItemModel::cube(hit_info.chunk->get(hit_info.block_pos)->id)){
        vec3 center=vec3(hit_info.block_pos)+chunk_pos_to_world(hit_info.chunk->pos)+vec3(.5);
        mat4 m=scale(translate(cam_mat,center),vec3(1.002f));
        ItemModel::cracks(int(break_progress/break_duration*10),m);
    }
    if (third_person) Avatar::draw(render_position(),cam.forward,model_walk.render_phase(float(accumulator/.05)),cam_mat,
        flying?0.f:model_walk.render_amount(float(accumulator/.05)),float(swing_progress),&inventory.data().armor,
        float(model_age+accumulator/.05),is_key_pressed(SDLK_LSHIFT) && !flying,Items::valid(hotbar.get_item()),blocking,bow_time>0);
    line_shader.bind();
    line_shader.upload_mat4("cam_mat", cam_mat);
    if(fishing){
        dvec3 tip=bobber+dvec3(0,fish_bite>0?-.15:std::sin(SDL_GetTicks()*.004)*.03,0);
        line_shader.upload_vec4("color",vec4(1,.15,.1,1));Hitbox(tip-dvec3(.07,0,.07),dvec3(.14)).draw();
        vec3 line[2]={vec3(render_position())+cam.pos+cam.forward*.5f-vec3(0,.2,0),vec3(tip)};
        GLuint vao,vbo;glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,sizeof(line),line,GL_STREAM_DRAW);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(vec3),nullptr);glEnableVertexAttribArray(0);
        line_shader.upload_vec4("color",vec4(.15,.15,.15,1));glDrawArrays(GL_LINES,0,2);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);
    }

    if (hit_info.hit)
    {
        line_shader.upload_vec4("color", vec4(0, 0, 0, 0.5f));

        Hitbox block_hitbox;

        block_hitbox.pos = (dvec3)hit_info.block_pos + chunk_pos_to_world_hp(hit_info.chunk->pos) - (dvec3)cam.forward * 0.005;
        block_hitbox.size = dvec3(1);
        if (hit_info.chunk->get(hit_info.block_pos)->id == TORCH &&
            hit_info.chunk->get(hit_info.block_pos)->data1 == 0) {
            block_hitbox.pos += dvec3(7./16,0,7./16);
            block_hitbox.size = dvec3(2./16,10./16,2./16);
        }

        block_hitbox.draw();
    }

    if (show_debug)
    {
        line_shader.upload_vec4("color", vec4(0, 0, 1, 1));

        Hitbox _hitbox = hitbox;
        _hitbox.pos += pos;
        _hitbox.draw();
    }
}

void Player::destroy()
{
    hotbar.s_hotbar.destroy();
    hotbar.s_selector.destroy();
    inventory.tooltip.destroy();
    delete this;
}

void Player::draw_drops(){
    Camera view=view_camera();mat4 matrix=view.matrix(vec3(render_position()));
    // World-space projectile sprites use the depth buffer, unlike the legacy
    // billboard pickup labels below. Camera-facing basis is shared by all shots.
    vec3 right=normalize(cross(view.forward,view.up)),up=normalize(cross(right,view.forward));
    for(auto& shot:shots){
        mat4 m=translate(matrix,vec3(shot.position));
        mat4 facing(1);facing[0]=vec4(right,0);facing[1]=vec4(up,0);facing[2]=vec4(-view.forward,0);
        m=m*facing;m=scale(m,vec3(shot.item==Items::ARROW?.35f:.25f));
        ItemModel::draw(shot.item,Ui::items_texture(),Ui::item_frame(shot.item),m);
    }
    glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
    for(auto& d:drops){
        dvec3 relative=d.position-render_position();float distance=float(length(relative));if(distance>32)continue;
        vec4 clip=matrix*vec4(vec3(d.position),1);if(clip.w<.1)continue;
        mat4 m=translate(matrix,vec3(d.position)+vec3(0,.12f*sin(float(d.age)*3),0));
        m=rotate(m,float(d.age*2.7),vec3(0,1,0));m=rotate(m,float(sin(d.age*1.7)*.18),vec3(1,0,0));
        float item_scale=Items::block(d.stack.id)?.32f:.42f;
        for(int copy=0;copy<std::min(4,std::max(1,d.stack.count));++copy){
            mat4 item=m*translate(mat4(1),vec3((copy%2-.5f)*.12f,(copy/2)*.1f,0));
            ItemModel::draw(d.stack.id,Ui::items_texture(),Ui::item_frame(d.stack.id),scale(item,vec3(item_scale)));
        }
    }
    for(auto& p:particles){
        dvec3 relative=p.position-render_position();if(length(relative)>32)continue;
        mat4 m=translate(matrix,vec3(p.position));m=rotate(m,float(p.age*5),vec3(0,1,0));m=scale(m,vec3(.08f));
        ItemModel::draw(p.item,Ui::items_texture(),Ui::item_frame(p.item),m);
    }
    glEnable(GL_CULL_FACE);glEnable(GL_DEPTH_TEST);
}
void Player::draw_hud(){
    glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
    int bottom=int(win_size.y)-100;
    if(survival_mode){
        int armor_points=0;for(auto& s:inventory.data().armor)if(!s.empty())armor_points+=Items::armor_points(s.id);
        if(armor_points)for(int i=0;i<10;++i)Ui::image(Ui::icons_texture(),win_size.x/2.f-270+i*24,float(bottom-26),24,24,vec4(armor_points>=i*2+2?34:(armor_points>i*2?25:16),9,9,9));
        if(air<300)for(int i=0;i<int(ceil(std::max(0,air)/30.));++i)Ui::image(Ui::icons_texture(),win_size.x/2.f+246-i*24,float(bottom-26),24,24,vec4(16,18,9,9));
        for(int i=0;i<10;++i){int x=int(win_size.x)/2-270+i*24;
            Ui::image(Ui::icons_texture(),float(x),float(bottom),24,24,vec4(16,0,9,9));
            if(vitals.health>i*2)Ui::image(Ui::icons_texture(),float(x),float(bottom),24,24,vec4(vitals.health>=i*2+2?52:61,0,9,9));
            x=int(win_size.x)/2+246-i*24;Ui::image(Ui::icons_texture(),float(x),float(bottom),24,24,vec4(16,27,9,9));
            if(vitals.food>i*2)Ui::image(Ui::icons_texture(),float(x),float(bottom),24,24,vec4(vitals.food>=i*2+2?52:61,27,9,9));
        }
    }
    Ui::text("mode",survival_mode?Language::tr("Survival"):Language::tr("Creative"),10,10,2);
    Ui::text("fishing",fishing?(fish_bite>0?Language::tr("Bite! Right click to reel in"):Language::tr("Fishing...")):"",10,36,2);
    Ui::text("item-action",item_message_time>0?item_message:"",10,62,2);
    if(break_progress>0 && break_duration>0){float f=float(std::min(1.,break_progress/break_duration));Ui::rect(win_size.x/2.f-75,win_size.y/2.f+25,150,6,vec4(0,0,0,.8));Ui::rect(win_size.x/2.f-75,win_size.y/2.f+25,150*f,6,vec4(1));}
    if(eat_progress>0){Ui::rect(win_size.x/2.f-75,win_size.y/2.f+25,150,6,vec4(0,0,0,.8));Ui::rect(win_size.x/2.f-75,win_size.y/2.f+25,float(eat_progress/1.6)*150,6,vec4(.5,1,.3,1));}
    int id=hotbar.get_item();std::string title=Items::name(id);Ui::text("hand",title,int(win_size.x)/2-Text::measure(title,2)/2,bottom-26,2);
    glEnable(GL_CULL_FACE);glEnable(GL_DEPTH_TEST);
}
void Player::swing(){if(swing_progress>=1)swing_progress=0;}
void Player::animate(){
    float dt=float(std::min(delta_time,.25));
    swing_progress=std::min(1.,swing_progress+dt/.3);
    int selected=hotbar.get_item();
    float target=selected==equipped_item?1.f:0.f;
    equip+=std::clamp(target-equip,-8.f*dt,8.f*dt);
    if(equip<.1f)equipped_item=selected;
    float walking=grounded && !flying && !world->submerged(pos)?std::min(1.f,float(length(dvec2(vel.x,vel.z))/5.6)):0.f;
    bob_amount+=(walking-bob_amount)*std::min(1.f,dt*12);
}
void Player::draw_hand(){
    if(third_person || show_inventory || vitals.health<=0)return;
    glClear(GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);
    const float pi=3.14159265359f,s=float(swing_progress),root=sqrt(s);
    mat4 m=perspective(radians(70.f),float(win_size.x)/win_size.y,.05f,10.f);
    float phase=float(walk_phase)*.6662f;
    m=translate(m,vec3(sin(phase)*bob_amount*.025f,-abs(cos(phase))*bob_amount*.025f,0));
    if(!Items::valid(equipped_item)){
        // 1.12.2 ItemRenderer's right-arm pose, with the ModelBiped shoulder
        // pivot retained in Avatar::hand instead of centering the arm mesh.
        m=translate(m,vec3(.64000005f-.3f*sin(root*pi),-.6f+.4f*sin(root*2*pi)-(1-equip)*.6f,-.71999997f-.4f*sin(s*pi)));
        m=rotate(m,radians(45.f+70.f*sin(root*pi)),vec3(0,1,0));
        m=rotate(m,radians(-20.f*sin(s*s*pi)),vec3(0,0,1));
        m=translate(m,vec3(-1.f,3.6f,3.5f));
        m=rotate(m,radians(120.f),vec3(0,0,1));
        m=rotate(m,radians(200.f),vec3(1,0,0));
        m=rotate(m,radians(-135.f),vec3(0,1,0));
        m=translate(m,vec3(5.6f,0,0));Avatar::hand(m);return;
    }
    if(blocking && equipped_item==Items::SHIELD){
        m=translate(m,vec3(.35f,-.2f-(1-equip)*.6f,-.5f));
        m=rotate(m,radians(-25.f),vec3(0,1,0));
    }else if(bow_time>0 && equipped_item==Items::BOW){
        // ItemRenderer 1.12.2 BOW use branch (main/right hand).
        m=translate(m,vec3(.56f,-.52f-(1-equip)*.6f,-.72f));
        m=translate(m,vec3(-.2785682f,.18344387f,.15731531f));
        m=rotate(m,radians(-13.935f),vec3(1,0,0));
        m=rotate(m,radians(35.3f),vec3(0,1,0));m=rotate(m,radians(-9.785f),vec3(0,0,1));
        float charge=Items::bow_charge(float(bow_time)),ticks=float(bow_time)*20;
        if(charge>.1f)m=translate(m,vec3(0,sin((ticks-.1f)*1.3f)*(charge-.1f)*.004f,0));
        m=translate(m,vec3(0,0,charge*.04f));m=scale(m,vec3(1,1,1+charge*.2f));
        m=rotate(m,radians(-45.f),vec3(0,1,0));
    }else if(eat_progress>0){
        float remaining=32.f-float(eat_progress)*20.f+1;
        float f=remaining/32.f;
        if(f<.8f)m=translate(m,vec3(0,abs(cos(remaining/4*pi))*.1f,0));
        float bite=1-pow(f,27.f);
        m=translate(m,vec3(bite*.6f,bite*-.5f,0));
        m=rotate(m,radians(bite*90),vec3(0,1,0));m=rotate(m,radians(bite*10),vec3(1,0,0));m=rotate(m,radians(bite*30),vec3(0,0,1));
        m=translate(m,vec3(.56f,-.52f-(1-equip)*.6f,-.72f));
    }else{
        m=translate(m,vec3(-.4f*sin(root*pi),.2f*sin(root*2*pi),-.2f*sin(s*pi)));
        m=translate(m,vec3(.56f,-.52f-(1-equip)*.6f,-.72f));
        m=rotate(m,radians(45.f-20.f*sin(s*s*pi)),vec3(0,1,0));
        m=rotate(m,radians(-20.f*sin(root*pi)),vec3(0,0,1));
        m=rotate(m,radians(-80.f*sin(root*pi)),vec3(1,0,0));
        m=rotate(m,radians(-45.f),vec3(0,1,0));
    }
    if(ItemModel::cube(equipped_item)){m=rotate(m,radians(45.f),vec3(0,1,0));m=scale(m,vec3(.4f));}
    else if(equipped_item==Items::SHIELD){m=rotate(m,radians(-12.f),vec3(1,0,0));m=rotate(m,radians(-18.f),vec3(0,1,0));m=scale(m,vec3(.52f));}
    else{
        // Flat sprite adaptation of the vanilla generated/handheld display pose.
        m=rotate(m,radians(-45.f),vec3(0,1,0));m=rotate(m,radians(25.f),vec3(0,0,1));m=scale(m,vec3(.68f));
    }
    // Vanilla keeps the right arm visible while holding an item.  Draw it
    // with the same pose before the item so the hand stays attached to tools
    // and blocks instead of disappearing behind the model.
    ItemModel::draw(equipped_item,Ui::items_texture(),Ui::item_frame(equipped_item),m);
    auto& off=inventory.data().offhand;
    if(!off.empty() && Items::valid(off.id)){
        mat4 held=perspective(radians(70.f),float(win_size.x)/win_size.y,.05f,10.f);
        held=translate(held,vec3(-.56f,-.52f-(1-equip)*.6f,-.72f));
        if(ItemModel::cube(off.id)){held=rotate(held,radians(-45.f),vec3(0,1,0));held=scale(held,vec3(.4f));}
        else{held=rotate(held,radians(45.f),vec3(0,1,0));held=rotate(held,radians(-25.f),vec3(0,0,1));held=scale(held,vec3(.68f));}
        ItemModel::draw(off.id,Ui::items_texture(),Ui::item_frame(off.id),held);
    }
}
