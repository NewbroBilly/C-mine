#include <item_smoke.h>
#include <game.h>
#include <player.h>

bool item_smoke(const std::function<void(const char*)>& save_frame) {
    auto check=[](bool ok,const char* name){
        if(!ok)SDL_Log("Item regression FAILED: %s",name);
        return ok;
    };
    Game::update();
    auto* c=world->get_chunk_from_pos(player->pos);
    if(!check(c && c->can_draw,"loaded test chunk"))return false;
    dvec3 base=chunk_pos_to_world_hp(c->pos);
    for(int x=1;x<15;++x)for(int z=1;z<15;++z){
        BlockState floor{STONE,0,0};c->set(ivec3(x,229,z),&floor);
        for(int y=230;y<240;++y){BlockState air{AIR,0,0};c->set(ivec3(x,y,z),&air);}
    }
    world->mobs.clear();world->initial_mobs=true;world->rebuild_dirty();
    Game::set_mode(true);player->inventory.survival={};
    player->hotbar.set_index(0);player->third_person=0;
    player->pos=player->previous_position=base+dvec3(8.5,230,4.5);
    player->vel=dvec3(0);player->grounded=true;player->vitals={};
    std::fill(buttons,buttons+BUTTON_MAX,Input::NONE);
    delta_time=.25;player->action_delay=0;
    auto& held=player->hotbar.slots[0].value();
    auto cow=Mobs::create(Mobs::Cow,{base.x+8.5,230,base.z+7.5},1);
    world->mobs.push_back(cow);
    player->cam.forward=normalize(vec3(0,-.72,3));

    held={Items::BUCKET,1};buttons[2]=Input::DOWN;player->handle_clicks();
    if(!check(held.id==Items::MILK_BUCKET && world->mobs.size()==1,"cow milking while targeting entity"))return false;
    held={Items::BOW,1};player->inventory.data().main[1]={Items::ARROW,3};
    for(int n=0;n<5;++n){buttons[2]=n?Input::PRESSED:Input::DOWN;player->handle_clicks();}
    Game::draw();save_frame("smoke-bow-drawn.bmp");
    buttons[2]=Input::UP;player->handle_clicks();
    if(!check(player->shots.size()==1 && held.damage==1 && player->inventory.data().main[1].count==2,"bow release spends one arrow and durability"))return false;
    delta_time=.05;
    for(int n=0;n<6;++n)player->update_projectiles();
    if(!check(player->shots.empty() && world->mobs[0].health<cow.health,"swept arrow hits aimed-at cow"))return false;

    held={Items::SHIELD,1};buttons[2]=Input::DOWN;player->handle_clicks();
    Game::draw();save_frame("smoke-shield-block.bmp");
    player->vitals.hurt_ticks=0;float health=player->vitals.health;
    player->hurt(4);
    if(!check(player->blocking && player->vitals.health==health && held.damage==5,"held shield blocks ordinary damage"))return false;
    buttons[2]=Input::NONE;player->handle_clicks();player->hurt(4,true);
    if(!check(player->vitals.health==health-4,"released shield does not block bypass damage"))return false;

    held={Items::SNOWBALL,2};player->action_delay=0;buttons[2]=Input::DOWN;player->handle_clicks();
    if(!check(held.count==1 && player->shots.size()==1,"throwable spends one"))return false;
    player->shots.clear();world->mobs.clear();
    held={Items::MUSHROOM_STEW,1};player->vitals.food=5;player->eat_progress=0;
    delta_time=.2;
    for(int n=0;n<9;++n){buttons[2]=n?Input::PRESSED:Input::DOWN;player->handle_clicks();}
    if(!check(held.id==Items::BOWL && player->vitals.food==11,"timed soup returns bowl"))return false;
    // Switching away and back must not preserve a partially eaten item.
    held={Items::APPLE,2};player->vitals.food=5;buttons[2]=Input::PRESSED;player->handle_clicks();
    held={Items::BOW,1};player->handle_clicks();
    held={Items::APPLE,2};player->handle_clicks();
    if(!check(player->eat_progress<=.201 && held.count==2,"switching items cancels eating"))return false;
    player->bow_time=0;
    for(auto& s:player->inventory.data().main)s={STONE,64};
    held={Items::BUCKET,2};auto before=player->drops.size();player->consume_held(Items::MILK_BUCKET);
    if(!check(held.count==1 && player->drops.size()==before+1 && player->drops.back().stack.id==Items::MILK_BUCKET,"container overflow is dropped, not lost"))return false;

    player->inventory.survival={};held={Items::PIG_SPAWN_EGG,2};
    player->cam.forward=normalize(vec3(0,-1,1));player->action_delay=0;buttons[2]=Input::DOWN;
    player->handle_clicks();
    if(!check(world->mobs.size()==1 && world->mobs[0].kind==Mobs::Pig && held.count==1,"spawn egg validates space and spends one"))return false;
    world->mobs.clear();
    player->cam.forward=normalize(vec3(0,-1.12,3));
    BlockState water{WATER,1,0};c->set(ivec3(8,230,7),&water);
    held={Items::BUCKET,1};player->action_delay=0;player->handle_clicks();
    if(!check(held.id==Items::BUCKET && c->get(ivec3(8,230,7))->id==WATER,"bucket refuses flowing water"))return false;
    water.data1=0;c->set(ivec3(8,230,7),&water);player->action_delay=0;player->handle_clicks();
    if(!check(held.id==Items::WATER_BUCKET && c->get(ivec3(8,230,7))->id==AIR,"bucket accepts source water"))return false;
    auto position=player->pos;
    if(!check(!player->teleport_near(position+dvec3(100000,0,100000)) && player->pos==position,"pearl refuses unloaded destination"))return false;
    for(int x=7;x<=10;++x)for(int y=220;y<=225;++y)for(int z=7;z<=10;++z){
        BlockState solid{STONE,0,0};c->set(ivec3(x,y,z),&solid);
    }
    if(!check(!player->teleport_near(base+dvec3(8.5,220,8.5)),"pearl refuses solid destination"))return false;

    // Entity rendering must not leak its shader into the translucent water pass.
    water.data1=0;
    for(int x=5;x<12;++x)for(int z=8;z<13;++z)c->set(ivec3(x,230,z),&water);
    world->mobs.push_back(cow);world->rebuild_dirty();
    player->cam.forward=normalize(vec3(0,-.35,1));
    world->draw();GLint program=0;glGetIntegerv(GL_CURRENT_PROGRAM,&program);
    GLboolean depth_mask=GL_FALSE;glGetBooleanv(GL_DEPTH_WRITEMASK,&depth_mask);
    if(!check(GLuint(program)==block_shader.id && depth_mask==GL_TRUE && glIsEnabled(GL_BLEND),"water restores block shader, alpha blending and depth mask"))return false;
    Game::draw();save_frame("smoke-water-with-mobs.bmp");
    Game::set_mode(false);Game::inventory(true);player->inventory.tab(3);
    Game::draw();save_frame("smoke-transport-catalog.bmp");
    player->inventory.tab(8);Game::draw();save_frame("smoke-expanded-combat.bmp");
    Game::inventory(false);Game::set_mode(true);
    buttons[2]=Input::NONE;player->inventory.data().offhand={Items::TOTEM,1};
    player->vitals.health=0;player->vitals.food=0;player->vel=dvec3(0);
    player->blocking=false;player->update();
    if(!check(player->vitals.health==1 && player->inventory.data().offhand.empty() && Game::screen==Game::Screen::Playing,"offhand totem saves from lethal damage"))return false;
    SDL_Log("Item regressions passed: entity-targeted bow/milk, swept arrow hit, shield, throwing, soup/container overflow, use cancellation, spawn egg, source-only bucket, safe pearl, totem, water shader with mobs");
    return true;
}
