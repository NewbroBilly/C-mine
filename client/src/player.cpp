#include <player.h>
#include <movement.h>
#include <avatar.h>
#include <settings.h>
#include <game.h>
#include <ui.h>
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
        if(flight_tap>=0 && now-flight_tap<.35){flying=!flying;vel=dvec3(0);accumulator=0;previous_position=pos;flight_tap=-1;}
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
    if(survival_mode){--stack.count;Items::normalize(stack);}return true;
}
void Player::handle_clicks(){
    action_delay=std::max(0.,action_delay-delta_time);
    if(show_inventory){hit_info.hit=false;break_progress=eat_progress=0;return;}
    vec3 origin=vec3(pos)+cam.pos;
    hit_info=raycast(origin,cam.forward,survival_mode?4.5f:5.f,world->get_chunk_from_pos(origin));
    bool attack=is_button_down(0) || is_button_pressed(0);
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
                    if(survival_mode){vitals.exhaust(.005f);auto& tool=hotbar.slots[hotbar.index].value();int max=Items::durability(tool.id);if(max && ++tool.damage>max)tool.clear();}
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
    bool edible=survival_mode && (held==Items::APPLE || held==Items::BREAD) && vitals.food<20;
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
void Player::update_drops(){
    for(auto it=drops.begin();it!=drops.end();){
        it->age+=delta_time;if(it->age>300){it=drops.erase(it);continue;}
        dvec3 next=it->position+it->velocity*delta_time;it->velocity.y-=9.8*delta_time;
        auto* c=world->get_chunk_from_pos(next);
        if(c && next.y>=0 && next.y<CHUNK_HEIGHT){
            ivec3 local=ivec3(floor(next))-ivec3(chunk_pos_to_world_hp(c->pos));int id=c->get(local)->id;
            if(id!=AIR && id!=GRASS && id!=TORCH){if(it->velocity.y<0){next.y=floor(next.y)+1.15;it->velocity=dvec3(0);}}
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
    double dt = std::min(delta_time,.05);
    dvec3 direction = dvec3(right)*double(input.x) + normalize(dvec3(cam.forward.x,0,cam.forward.z))*double(input.z);
    if (length(direction)>1) direction = normalize(direction);
    auto solid = [](int x,int y,int z) {
        if (y<0) return true;
        if (y>=CHUNK_HEIGHT) return false;
        Chunk* c = world->get_chunk_from_pos(dvec3(x,y,z));
        if (!c) return true;
        ivec3 local = dvec3(x,y,z)-chunk_pos_to_world_hp(c->pos);
        int id = c->get(local)->id;
        return id != AIR && id != GRASS && id != TORCH;
    };
    if(!survival_mode && flying){
        dvec3 flight=dvec3(right)*double(input.x)+normalize(dvec3(cam.forward.x,0,cam.forward.z))*double(input.z);
        flight.y=(!show_inventory && is_key_pressed(SDLK_SPACE)?1.:0.)-(!show_inventory && is_key_pressed(SDLK_LSHIFT)?1.:0.);
        if(length(flight)>1)flight=normalize(flight);
        flight*=is_key_pressed(SDLK_LCTRL)?21.8:10.9;
        previous_position=pos;
        for(int axis=0;axis<3;++axis){double amount=flight[axis]*dt;int n=std::max(1,int(ceil(abs(amount)/.1)));for(int i=0;i<n;++i){auto next=pos;next[axis]+=amount/n;if(!Movement::overlaps(next,solid))pos=next;}}
        vel=flight;vitals.fallen=0;return;
    }
    const bool sneak=!show_inventory && is_key_pressed(SDLK_LSHIFT);
    const bool sprint=!show_inventory && is_key_pressed(SDLK_LCTRL) && input.z>0 && !sneak && (!survival_mode || vitals.food>6);
    const bool jump=!show_inventory && is_key_pressed(SDLK_SPACE);
    accumulator += dt;
    Movement::State state{pos,vel,grounded,jump_cooldown};
    while (accumulator >= .05) {
        previous_position=state.position;
        double old_y=state.position.y;
        bool was_grounded=state.grounded;
        Movement::tick(state,dvec2(direction.x,direction.z),jump,sprint,sneak,solid);
        if(survival_mode){
            vitals.landing(old_y-state.position.y,state.grounded);
            if(jump && was_grounded && state.velocity.y>0)vitals.exhaust(sprint?.2f:.05f);
            if(sprint && state.grounded)vitals.exhaust(float(length(dvec2(state.velocity.x,state.velocity.z))*.05*.1));
        }
        accumulator -= .05;
    }
    pos = state.position; vel = state.velocity; grounded = state.grounded;jump_cooldown=state.jump_cooldown;
    walk_phase += length(dvec2(vel.x,vel.z))*dt*4.0;
}

dvec3 Player::render_position() const {
    if(flying || length(pos-previous_position)>3.)return pos;
    return mix(previous_position,pos,clamp(accumulator/.05,0.,1.));
}

Camera Player::view_camera() {
    Camera view = cam;
    if (!third_person) return view;
    vec3 offset = cam.forward * (third_person == 1 ? -3.5f : 3.5f);
    float fraction = 0;
    for (float t=.05f;t<=1.f;t+=.05f) {
        dvec3 p = pos + dvec3(cam.pos + offset*t);
        Chunk* c = world->get_chunk_from_pos(p);
        if (!c || p.y<0 || p.y>=CHUNK_HEIGHT) break;
        ivec3 local = floor(p)-chunk_pos_to_world_hp(c->pos);
        int id=c->get(local)->id;
        if (id!=AIR && id!=GRASS && id!=TORCH) break;
        fraction=t;
    }
    view.pos += offset*fraction;
    if (third_person==2) view.forward = -cam.forward;
    return view;
}

void Player::update()
{
    if (!world->spawned) return;
    handle_keys();handle_clicks();move();update_drops();
    if(survival_mode){vital_clock+=delta_time;while(vital_clock>=.05){vitals.tick();vital_clock-=.05;}if(pos.y<0)vitals.hurt(4);if(vitals.health<=0)Game::die();}
}

void Player::draw()
{
    Camera view = view_camera();
    mat4 cam_mat = view.matrix((vec3)render_position());
    if (third_person) Avatar::draw(render_position(),cam.forward,walk_phase,cam_mat,float(length(dvec2(vel.x,vel.z))/5.6));
    line_shader.bind();
    line_shader.upload_mat4("cam_mat", cam_mat);

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
        for(int i=0;i<10;++i){int x=int(win_size.x)/2-270+i*24;
            Ui::image(Ui::icons_texture(),float(x),float(bottom),24,24,vec4(16,0,9,9));
            if(vitals.health>i*2)Ui::image(Ui::icons_texture(),float(x),float(bottom),24,24,vec4(vitals.health>=i*2+2?52:61,0,9,9));
            x=int(win_size.x)/2+246-i*24;Ui::image(Ui::icons_texture(),float(x),float(bottom),24,24,vec4(16,27,9,9));
            if(vitals.food>i*2)Ui::image(Ui::icons_texture(),float(x),float(bottom),24,24,vec4(vitals.food>=i*2+2?52:61,27,9,9));
        }
    }
    Ui::text("mode",survival_mode?"Survival":"Creative",10,10,2);
    if(break_progress>0 && break_duration>0){float f=float(std::min(1.,break_progress/break_duration));Ui::rect(win_size.x/2.f-75,win_size.y/2.f+25,150,6,vec4(0,0,0,.8));Ui::rect(win_size.x/2.f-75,win_size.y/2.f+25,150*f,6,vec4(1));}
    if(eat_progress>0){Ui::rect(win_size.x/2.f-75,win_size.y/2.f+25,150,6,vec4(0,0,0,.8));Ui::rect(win_size.x/2.f-75,win_size.y/2.f+25,float(eat_progress/1.6)*150,6,vec4(.5,1,.3,1));}
    int id=hotbar.get_item();std::string title=Items::name(id);Ui::text("hand",title,int(win_size.x)/2-int(title.size())*6,bottom-26,2);
    glEnable(GL_CULL_FACE);glEnable(GL_DEPTH_TEST);
}
