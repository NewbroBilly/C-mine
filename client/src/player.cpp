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
    BlockState bs{char(stack.id),0,0};
    if(bs.id==LOG)bs.data1=normal.x?0:(normal.y?1:2);
    if(bs.id==TORCH){if(normal.y<0)return false;bs.data1=normal.y?0:(normal.z==1?1:(normal.z==-1?2:(normal.x==1?3:4)));}
    Hitbox cube(dvec3(local)+chunk_pos_to_world_hp(c->pos)-pos,dvec3(1));
    if(intersect(cube,hitbox))return false;
    if(!world->place_block(c,local,&bs))return false;
    swing();if(survival_mode){--stack.count;Items::normalize(stack);}return true;
}
void Player::hurt(float damage,bool bypass){if(!survival_mode || vitals.hurt_ticks>0 || damage<=0)return;vitals.hurt(bypass?damage:Items::absorb_damage(inventory.data().armor,damage));}
void Player::handle_clicks(){
    action_delay=std::max(0.,action_delay-delta_time);
    if(show_inventory){hit_info.hit=false;break_progress=eat_progress=0;return;}
    vec3 origin=vec3(pos)+cam.pos;
    hit_info=raycast(origin,cam.forward,survival_mode?4.5f:5.f,world->get_chunk_from_pos(origin));
    bool attack=is_button_down(0) || is_button_pressed(0);
    if(attack)swing();
    double reach=hit_info.hit?double(length(hit_info.hit_point-origin)):(survival_mode?4.5:5.);
    int mob=world->target_mob(origin,cam.forward,reach);
    if(mob>=0){
        if(is_button_down(2) && hotbar.get_item()==Items::SHEARS && world->mobs[mob].kind==Mobs::Sheep && !world->mobs[mob].sheared){world->mobs[mob].sheared=true;drop_item({WHITE_WOOL,2});if(survival_mode)Items::wear(hotbar.slots[hotbar.index].value());swing();}
        if(attack && action_delay<=0){world->hit_mob(mob,Mobs::melee_damage(hotbar.get_item()),dvec3(cam.forward));if(survival_mode && Items::durability(hotbar.get_item()))Items::wear(hotbar.slots[hotbar.index].value(),Items::tool_kind(hotbar.get_item())==Items::Tool::Sword?1:2);action_delay=.6;}
        hit_info.hit=false;break_progress=0;return;
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
    bool use=is_button_down(2) || is_button_pressed(2);
    int held=hotbar.get_item();
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
            BlockState b{char(replacement),0,0};c->set(p,&b);world->notify_water(c,p);world->rebuild_dirty();
            swing();if(survival_mode)Items::wear(hotbar.slots[hotbar.index].value());action_delay=.25;return;
        }
        if(held==Items::FLINT_AND_STEEL){
            BlockState fire{FIRE,0,0};
            auto target=p+hit_info.face_normal;auto* n=get_neighbor_chunk(c,target);
            if(n && n->get(target)->id!=WATER && world->place_block(c,p+hit_info.face_normal,&fire)){
                swing();if(survival_mode)Items::wear(hotbar.slots[hotbar.index].value());action_delay=.25;
            }return;
        }
    }
    if(use && action_delay<=0 && (held==Items::BUCKET || held==Items::WATER_BUCKET)){
        auto hit=raycast(origin,cam.forward,survival_mode?4.5f:5.f,world->get_chunk_from_pos(origin),held==Items::BUCKET);
        if(hit.hit){auto& stack=hotbar.slots[hotbar.index].value();bool done=false;int result=Items::BUCKET;
            if(held==Items::BUCKET && hit.chunk->get(hit.block_pos)->id==WATER){world->remove_block(hit.chunk,hit.block_pos,true);done=true;result=Items::WATER_BUCKET;}
            if(held==Items::WATER_BUCKET){BlockState b{WATER,0,0};done=world->place_block(hit.chunk,hit.block_pos+hit.face_normal,&b);}
            if(done){if(survival_mode){--stack.count;Items::normalize(stack);Items::Stack out{result,1};if(stack.empty())stack=out;else if(inventory.data().add(out))drop_item(out);}else if(held==Items::BUCKET)stack={Items::WATER_BUCKET,1};action_delay=.25;}
        }return;
    }
    bool edible=survival_mode && Items::food_points(held)>0 && vitals.food<20;
    if(use && edible){
        if(eating_item!=held){eating_item=held;eat_progress=0;}
        eat_progress+=delta_time;
        if(eat_progress>=1.6){auto& stack=hotbar.slots[hotbar.index].value();if(vitals.eat(stack.id)){--stack.count;Items::normalize(stack);}eat_progress=0;}
        return;
    }
    eat_progress=0;
    if(use && action_delay<=0){
        if(hit_info.hit && place_selected(hit_info.chunk,hit_info.block_pos+hit_info.face_normal,hit_info.face_normal))action_delay=.2;
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
    for(auto it=drops.begin();it!=drops.end();){
        it->age+=delta_time;if(it->age>300){it=drops.erase(it);continue;}
        dvec3 next=it->position+it->velocity*delta_time;it->velocity.y-=9.8*delta_time;
        auto* c=world->get_chunk_from_pos(next);
        if(c && next.y>=0 && next.y<CHUNK_HEIGHT){
            ivec3 local=ivec3(floor(next))-ivec3(chunk_pos_to_world_hp(c->pos));int id=c->get(local)->id;
            if(id==WATER){auto flow=Water::flow({int(floor(next.x)),int(floor(next.y)),int(floor(next.z))},[&](Water::Pos p){return world->fluid(p);});it->velocity*=pow(.8,delta_time*20);it->velocity+=dvec3(flow[0],flow[1],flow[2])*delta_time*5.;}
            else if(id!=AIR && id!=GRASS && id!=TORCH && id!=FIRE){if(it->velocity.y<0){next.y=floor(next.y)+1.15;it->velocity=dvec3(0);}}
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
        return id != AIR && id != GRASS && id != TORCH && id!=WATER && id!=FIRE;
    };
    const bool sneak=!show_inventory && is_key_pressed(SDLK_LSHIFT);
    const bool sprint=!show_inventory && is_key_pressed(SDLK_LCTRL) && input.z>0 && !sneak && (!survival_mode || vitals.food>6);
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
        if (id!=AIR && id!=GRASS && id!=TORCH && id!=WATER && id!=FIRE) break;
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
    animate();handle_keys();handle_clicks();move();update_drops();update_fishing();
    bool burning=false;
    if(survival_mode)for(int x=int(floor(pos.x-.3));x<=int(floor(pos.x+.3));++x)
        for(int y=int(floor(pos.y));y<=int(floor(pos.y+1.8));++y)for(int z=int(floor(pos.z-.3));z<=int(floor(pos.z+.3));++z){
            auto* c=world->get_chunk_from_pos(dvec3(x,y,z));if(c && y>=0 && y<256){ivec3 p=ivec3(x,y,z)-ivec3(chunk_pos_to_world_hp(c->pos));if(c->get(p)->id==FIRE)burning=true;}}
    if(burning){burn_clock+=delta_time;if(burn_clock>=1){hurt(1);burn_clock=0;}}else burn_clock=0;
    vital_clock+=delta_time;while(vital_clock>=.05){
        if(survival_mode){vitals.tick();if(world->underwater(pos+dvec3(cam.pos))){if(--air<=-20){air=0;hurt(2,true);}}else air=300;}
        else{air=300;if(vitals.hurt_ticks>0)--vitals.hurt_ticks;}
        vital_clock-=.05;
    }
    if(survival_mode){if(pos.y<0)hurt(4,true);if(vitals.health<=0)Game::die();}
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
    if (third_person) Avatar::draw(render_position(),cam.forward,walk_phase,cam_mat,flying?0.f:float(length(dvec2(vel.x,vel.z))/5.6),float(swing_progress),&inventory.data().armor);
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
    glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
    for(auto& d:drops){
        vec3 target=vec3(d.position)+vec3(0,.12f*sin(float(d.age)*3),0);
        vec4 clip=matrix*vec4(target,1);if(clip.w<.1)continue;vec3 ndc=vec3(clip)/clip.w;
        if(abs(ndc.x)>1.1 || abs(ndc.y)>1.1 || ndc.z>1)continue;
        vec3 origin=vec3(pos)+view.pos,direction=target-origin;float distance=length(direction);if(distance>32)continue;
        auto hit=raycast(origin,normalize(direction),std::max(.01f,distance-.3f),world->get_chunk_from_pos(origin));if(hit.hit)continue;
        float size=clamp(240.f/std::max(1.f,distance),10.f,48.f);
        Ui::item(d.stack,(ndc.x*.5f+.5f)*win_size.x-size/2,(.5f-ndc.y*.5f)*win_size.y-size/2,size,false);
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
    if(eat_progress>0){
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
    else{
        // Flat sprite adaptation of the vanilla generated/handheld display pose.
        m=rotate(m,radians(-45.f),vec3(0,1,0));m=rotate(m,radians(25.f),vec3(0,0,1));m=scale(m,vec3(.68f));
    }
    ItemModel::draw(equipped_item,Ui::items_texture(),Ui::item_frame(equipped_item),m);
}
