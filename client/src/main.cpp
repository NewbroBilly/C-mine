#include <global.h>
#include <game.h>
#include <settings.h>
#include <menu.h>
#ifdef __SWITCH__
#include <switch.h>
#include <unistd.h>
void update_switch_input();
#endif

void handle_events()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            closed = true;
            break;
        }
        else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) Game::pause();
        else if (event.type == SDL_KEYDOWN)
        {
            int index = event.key.keysym.sym & (~0x40000000);

            if (!event.key.repeat && index >= 0 && index < KEY_MAX &&
                (keys[index] == Input::NONE || keys[index] == Input::UP))
                keys[index] = Input::DOWN;
        }
        else if(event.type==SDL_TEXTINPUT){if(Game::screen==Game::Screen::Playing && player && show_inventory)player->inventory.text_input(event.text.text);else Menu::text_input(event.text.text);}
        else if (event.type == SDL_KEYUP)
        {
            int index = event.key.keysym.sym & (~0x40000000);

            if (index >= 0 && index < KEY_MAX)
                keys[index] = Input::UP;
        }
        else if(event.type==SDL_MOUSEWHEEL){
            int wheel=event.wheel.direction==SDL_MOUSEWHEEL_FLIPPED?-event.wheel.y:event.wheel.y;
            if(Game::screen==Game::Screen::Playing && player){
                if(show_inventory)player->inventory.scroll(-wheel);
                else if(wheel)player->hotbar.set_index(player->hotbar.get_index()-(wheel>0?1:-1));
            }else Menu::wheel(wheel);
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN)
        {
            int index = event.button.button - 1;

            if (index >= 0 && index < BUTTON_MAX && buttons[index] == Input::NONE)
                buttons[index] = Input::DOWN;
        }
        else if (event.type == SDL_MOUSEBUTTONUP)
        {
            int index = event.button.button - 1;

            if (index >= 0 && index < BUTTON_MAX)
                buttons[index] = Input::UP;
        }
    }
}

int main(int argc, char** argv)
{
    const bool smoke_test = argc == 2 && std::string(argv[1]) == "--smoke-test";
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }
#ifdef __SWITCH__
    if (R_FAILED(romfsInit())) return 1;
    chdir("romfs:/");
    win_size = uvec2(1280, 720);
#else
    char* base = SDL_GetBasePath();
    if (base) { std::filesystem::current_path(base); SDL_free(base); }
#endif

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    win = SDL_CreateWindow
    (
        win_name,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        win_size.x,
        win_size.y,
        SDL_WINDOW_OPENGL | (is_fullscreen ? SDL_WINDOW_FULLSCREEN : 0)
    );

    SDL_GLContext context = SDL_GL_CreateContext(win);
    if (!win || !context || !gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_Log("OpenGL 3.3 initialization failed: %s", SDL_GetError());
        return 2;
    }
    SDL_GL_SetSwapInterval(1);
    glViewport(0, 0, win_size.x, win_size.y);

    if(smoke_test){Settings::storage_override="smoke-options.txt";Settings::save(Settings::storage_override,Settings::Options{});}
    Game::init();
    if (smoke_test) {
        mouse_control = false;
        show_debug = false;
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }
    int frame = 0;
    Chunk* edited_chunk = nullptr;
    ivec3 edited_pos(0);
    bool edit_placed = false;
    dvec3 paused_position(0);
    size_t paused_chunks=0;
    ivec2 water_chunk(0);dvec3 saved_position(0);std::filesystem::path smoke_world;
    auto save_frame = [](const char* path) {
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, win_size.x, win_size.y, 32, SDL_PIXELFORMAT_RGBA32);
        glReadPixels(0, 0, win_size.x, win_size.y, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
        // OpenGL reads from the bottom left; flip for a normal image.
        auto* pixels = static_cast<unsigned char*>(surface->pixels);
        for (int y = 0; y < surface->h / 2; ++y)
            for (int x = 0; x < surface->pitch; ++x)
                std::swap(pixels[y * surface->pitch + x], pixels[(surface->h - 1 - y) * surface->pitch + x]);
        SDL_SaveBMP(surface, path);
        SDL_FreeSurface(surface);
    };

    Uint64 this_frame = SDL_GetPerformanceCounter();
    Uint64 last_frame = 0;

    while (!closed)
    {
#ifdef __SWITCH__
        if (!appletMainLoop()) break;
#endif
        last_frame = this_frame;
        this_frame = SDL_GetPerformanceCounter();

        delta_time = std::min(0.05, (this_frame - last_frame) / (double)SDL_GetPerformanceFrequency());

        update_inputs();
        handle_events();
#ifdef __SWITCH__
        update_switch_input();
#endif

        if (smoke_test) delta_time = 1./60.;
        Game::update();
        if (smoke_test) {
            ++frame;

            if (frame == 2) Game::start();
            if (frame == 4) {
                player->pos.y += 5;
                player->cam.forward = normalize(vec3(0, -0.65f, 1));
            }
            if(frame==10 || frame==11){SDL_Event wheel{};wheel.type=SDL_MOUSEWHEEL;wheel.wheel.y=frame==10?1:-1;SDL_PushEvent(&wheel);}
            if(frame==11 && player->hotbar.get_index()!=8)return 10;
            if(frame==12 && player->hotbar.get_index()!=0)return 11;
            if (frame == 60) { Game::pause(); paused_position=player->pos; paused_chunks=world->chunk_vector.size(); }
            if (frame > 60 && frame < 64 && (player->pos!=paused_position || world->chunk_vector.size()!=paused_chunks)) return 5;
            if (frame == 64) Game::resume();
            if (frame == 70) player->third_person=2;
            if (frame == 90) {
                edited_chunk = world->get_chunk_from_pos(player->pos);
                for (int y = CHUNK_HEIGHT - 3; y >= 0; --y)
                    if (edited_chunk->get(ivec3(8, y, 12))->id != AIR) {
                        edited_pos = ivec3(8, y + 1, 12);
                        break;
                    }
                BlockState stone = { STONE, 0, 0 };
                edit_placed = world->place_block(edited_chunk, edited_pos, &stone);
            }
            if (frame == 91 && edit_placed) world->remove_block(edited_chunk, edited_pos);
        }
        Game::draw();
        if (smoke_test) {
            if (frame == 1) save_frame("smoke-title.bmp");
            if (glGetError() != GL_NO_ERROR) { SDL_Log("Smoke test: OpenGL error"); return 3; }
            if (frame == 62) save_frame("smoke-pause.bmp");
            if (frame == 80) save_frame("smoke-avatar.bmp");
            if (frame == 89) save_frame("smoke-before.bmp");
            if (frame == 90) save_frame("smoke-placed.bmp");
            if (frame == 91) save_frame("smoke-after.bmp");
            if (frame == 92) {
                if (!world->spawned || !edit_placed || edited_chunk->get(edited_pos)->id != AIR) return 4;
                // Reproduce the screenshot at the world's lowest cave, with a torch.
                Chunk* cave=world->get_chunk_from_pos(vec3(8,1,8));
                for(int x=4;x<=12;++x) for(int z=4;z<=14;++z) {
                    if(cave->get(ivec3(x,0,z))->id!=BEDROCK) return 7;
                    for(int y=1;y<=5;++y) { BlockState bs{(char)((z==14 || x==4 || x==12 || y==5)?STONE:AIR),0,0};cave->set(ivec3(x,y,z),&bs); }
                }
                BlockState torch{TORCH,0,0};
                world->place_block(cave,ivec3(8,1,11),&torch);
                world->remove_block(cave,ivec3(8,0,8));
                if(cave->get(ivec3(8,0,8))->id!=BEDROCK) return 8;
                player->pos=dvec3(8.5,1,8.5);player->vel=dvec3(0);
                player->third_person=0;player->cam.forward=normalize(vec3(0,-.3f,1));
            }
            if(frame==100) save_frame("smoke-cave.bmp");
            if(frame==105) {
                if(player->pos.y<.9999 || !player->grounded) return 9;
                Game::title();Game::start();Game::update();
                if(!world->spawned) return 6;
                SDL_Log("Smoke test passed: mouse wheel wrapping, title, pause, Steve, trees, restart, spawn, block edits, bedrock and torch, no GL errors");
                Game::pause();Game::open_options();paused_position=player->pos;paused_chunks=world->chunk_vector.size();
            }
            if(frame>105 && frame<=108 && (player->pos!=paused_position || world->chunk_vector.size()!=paused_chunks))return 12;
            if(frame==106) {save_frame("smoke-options.bmp");keys[SDLK_RIGHT&~0x40000000]=Input::DOWN;Game::update();}
            if(frame==107 && (Settings::options.fov!=90 || player->cam.fov!=90))return 13;
            if(frame==108) {Settings::Options loaded;if(!Settings::load(Settings::storage_override,loaded) || !(loaded==Settings::options))return 14;Game::close_options();}
            if(frame==109) {if(Game::screen!=Game::Screen::Paused)return 15;Game::resume();}
            if(frame==110){
                Chunk* c=world->get_chunk_from_pos(player->pos);
                // Every registered inventory block must build, place, and remove safely.
                for(int id=1;id<BLOCK_COUNT;++id){
                    if(id==WATER || id==BEDROCK)continue;
                    if(get_block_by_id(id)==get_block_by_id(AIR))return 16;
                    BlockState block{char(id),0,0};ivec3 p(8,240,8);
                    if(!world->place_block(c,p,&block))return 17;
                    if(id==GLOWSTONE && c->get_block_light(ivec3(9,240,8))<14)return 18;
                    world->remove_block(c,p);if(c->get(p)->id!=AIR)return 19;
                }
                Game::set_mode(false);Game::inventory(true);
            }
            if(frame==111){
                player->inventory.selected=0;player->inventory.keyboard_navigation=true;
                keys[SDLK_RIGHT&~0x40000000]=Input::DOWN;Game::update();
                save_frame("smoke-blocks.bmp");
            }
            if(frame==112){
                if(player->inventory.selected!=1)return 20;
                keys[SDLK_RETURN&~0x40000000]=Input::DOWN;Game::update();
                if(player->inventory.data().cursor.id!=DIRT)return 21;
                Game::title();Game::open_options();Game::draw();save_frame("smoke-options-title.bmp");
                keys[SDLK_ESCAPE&~0x40000000]=Input::DOWN;Game::update();
                if(Game::screen!=Game::Screen::Title || world!=nullptr)return 22;
                SDL_Log("Settings save/reload, options pause freeze, camera FOV, all inventory blocks and glowstone lighting passed");
                Game::start(true);Game::update();
            }
            if(frame==113){
                auto& data=player->inventory.survival;
                for(auto& s:data.main)if(!s.empty())return 23;
                data.main[0]={DIRT,2};
                Chunk* c=world->get_chunk_from_pos(player->pos);ivec3 p(8,240,8);
                if(!player->place_selected(c,p,ivec3(0,1,0)) || data.main[0].count!=1)return 24;
                if(player->place_selected(c,p,ivec3(0,1,0)) || data.main[0].count!=1)return 25;
                Game::set_mode(false);player->hotbar.set_item(0,DIRT);
                if(!player->place_selected(c,p+ivec3(1,0,0),ivec3(0,1,0)) || player->hotbar.slots[0].value().count!=1)return 26;
                Game::set_mode(true);if(data.main[0].count!=1)return 27;
                data.main[1]={LOG,16};data.main[2]={PLANKS,64};data.main[9]={Items::APPLE,4};
                data.craft[0]={LOG,2};Game::inventory(true);Game::draw();save_frame("smoke-survival-inventory.bmp");
            }
            if(frame==114){
                auto& data=player->inventory.survival;
                player->inventory.activate(40,false,false);
                if(data.cursor.id!=PLANKS || data.cursor.count!=4 || data.craft[0].count!=1)return 28;
                Game::inventory(false);Game::inventory(true,true);
                data.craft={};for(int i=0;i<3;++i)data.craft[i]={PLANKS,1};data.craft[4]={Items::STICK,1};data.craft[7]={Items::STICK,1};
                Game::draw();save_frame("smoke-workbench.bmp");
                player->inventory.activate(45,false,false);
                if(data.cursor.id!=Items::WOOD_PICKAXE)return 29;
                Game::inventory(false);
            }
            if(frame==115){
                Game::set_mode(false);Game::inventory(true);player->inventory.catalog_page=1;
                Game::draw();save_frame("smoke-creative-items.bmp");Game::inventory(false);Game::set_mode(true);
                Chunk* c=world->get_chunk_from_pos(player->pos);
                // A level test arena, away from terrain and player overlap.
                for(int x=5;x<=12;++x)for(int z=5;z<=13;++z){
                    BlockState floor{DIRT,0,0};c->set(ivec3(x,199,z),&floor);
                    for(int y=200;y<=204;++y){BlockState air{AIR,0,0};c->set(ivec3(x,y,z),&air);}
                }
                world->rebuild_dirty();BlockState dirt{DIRT,0,0};world->place_block(c,ivec3(8,201,10),&dirt);
                player->pos=player->previous_position=dvec3(8.5,200,8.5);player->vel=dvec3(0);player->vitals={};player->grounded=true;
                player->hotbar.set_index(0);player->inventory.survival.main={};player->cam.forward=vec3(0,0,1);
                buttons[0]=Input::DOWN;
            }
            if(frame==125){
                auto* c=world->get_chunk_from_pos(player->pos);if(c->get(ivec3(8,201,10))->id!=DIRT)return 30;
            }
            if(frame==200){
                buttons[0]=Input::NONE;
                auto* c=world->get_chunk_from_pos(player->pos);if(c->get(ivec3(8,201,10))->id!=AIR || player->drops.empty())return 31;
                player->pos=player->previous_position=dvec3(8.5,200,10.5);
            }
            if(frame==230){
                int dirt=0;for(auto& s:player->inventory.survival.main)if(s.id==DIRT)dirt+=s.count;
                if(dirt!=1)return 32;
                Game::draw();save_frame("smoke-survival-hud.bmp");
                player->vitals.health=0;Game::update();
                if(Game::screen!=Game::Screen::Dead)return 33;
                Game::draw();save_frame("smoke-death.bmp");
                Game::respawn();Game::update();
                if(Game::screen!=Game::Screen::Playing || player->vitals.health!=20 || !world->spawned)return 34;
                for(auto& s:player->inventory.survival.main)if(!s.empty())return 35;
                Game::set_mode(false);player->pos.y+=6;auto y=player->pos.y;
                keys[SDLK_SPACE]=Input::DOWN;Game::update();keys[SDLK_SPACE]=Input::NONE;Game::update();
                keys[SDLK_SPACE]=Input::DOWN;Game::update();keys[SDLK_SPACE]=Input::NONE;
                if(!player->flying)return 36;
                player->vitals.health=15;player->vitals.fallen=50;Game::update();
                if(player->vitals.health!=15)return 37;
                Game::set_mode(true);if(player->flying)return 38;
                SDL_Log("Inventory smoke passed: empty survival, stack spending, failed placement conservation, separate creative inventory, crafting, timed mining, drops, pickup, death/respawn and creative flight");
                Game::set_mode(true);player->pos=player->previous_position=dvec3(8.5,241,8.5);player->vel=dvec3(0);player->grounded=true;
                player->vitals={};player->vitals.food=10;player->vitals.saturation=0;
                player->hotbar.set_index(0);player->hotbar.slots[0].value()={Items::APPLE,2};buttons[2]=Input::DOWN;
            }
            if(frame==245 && (player->vitals.food!=10 || player->hotbar.slots[0].value().count!=2))return 39;
            if(frame==340){
                buttons[2]=Input::NONE;
                if(player->vitals.food!=14 || player->hotbar.slots[0].value().count!=1)return 40;
                SDL_Log("Timed food consumption passed: no early consumption, one apple after 32 ticks");
                Game::title();
                Saves::root_override=Saves::unique_path(std::filesystem::current_path()/"smoke-saves").string();
                Game::select_worlds();Game::draw();save_frame("smoke-worlds-empty.bmp");
                keys[SDLK_RETURN]=Input::DOWN;Game::update();
                if(Game::screen!=Game::Screen::CreateWorld)return 41;
            }
            if(frame==341){
                Game::world_name.clear();Menu::text_input("Water Test");
                keys[SDLK_DOWN&~0x40000000]=Input::DOWN;Game::update();
                Menu::text_input("12345");keys[SDLK_DOWN&~0x40000000]=Input::DOWN;Game::update();
                keys[SDLK_DOWN&~0x40000000]=Input::NONE;keys[SDLK_RETURN]=Input::DOWN;Game::update();
                if(Game::new_survival || Game::world_name!="Water Test" || Game::world_seed!="12345")return 42;
                Game::draw();save_frame("smoke-create-world.bmp");
                keys[SDLK_RETURN]=Input::NONE;keys[SDLK_DOWN&~0x40000000]=Input::DOWN;Game::update();
                keys[SDLK_DOWN&~0x40000000]=Input::NONE;keys[SDLK_RETURN]=Input::DOWN;Game::update();
                if(!world || world->seed!=12345 || survival_mode)return 43;
                smoke_world=world->save_path;
            }
            if(frame==355){
                if(!world->spawned)return 44;
                size_t natural_water=0;
                for(auto* c:world->chunk_vector)
                    for(auto& column:c->blocks)for(auto& row:column)for(auto b:row)if(b.id==WATER){
                        ++natural_water;if(b.data1!=0 || b.data2!=0)return 60;
                    }
                if(!natural_water || world->water.pending()!=0)return 61;
                SDL_Log("Generation regression passed: %zu settled water blocks, zero scheduled fluid updates",natural_water);
                int ores[BLOCK_COUNT]={};
                for(auto* c:world->chunk_vector)for(auto& column:c->blocks)for(auto& row:column)for(auto b:row)++ores[int(b.id)];
                for(int id:{COAL_ORE,IRON_ORE,GOLD_ORE,DIAMOND_ORE,REDSTONE_ORE,LAPIS_ORE,EMERALD_ORE})if(ores[id]==0)return 62;
                Game::inventory(true);player->inventory.tab(7);Game::draw();save_frame("smoke-all-tools.bmp");
                player->inventory.tab(8);Game::draw();save_frame("smoke-swords.bmp");Game::inventory(false);
                auto* c=world->get_chunk_from_pos(player->pos);water_chunk=c->pos;
                for(int x=2;x<=14;++x)for(int z=2;z<=14;++z){BlockState b{STONE,0,0};c->set(ivec3(x,229,z),&b);
                    for(int y=230;y<=236;++y){BlockState a{AIR,0,0};c->set(ivec3(x,y,z),&a);}}
                BlockState water{WATER,0,0};world->place_block(c,ivec3(8,232,8),&water);
                player->flying=true;player->pos=player->previous_position=chunk_pos_to_world_hp(c->pos)+dvec3(8.5,233,3.5);
                player->cam.forward=normalize(vec3(0,-.5,1));player->inventory.creative.main[0]={Items::WATER_BUCKET,1};player->hotbar.set_index(0);
            }
            if(frame==410){
                auto* c=world->chunk_map.at(water_chunk);
                if(c->get(ivec3(8,231,8))->id!=WATER || (c->get(ivec3(8,231,8))->data1&15)!=8 || c->get(ivec3(9,230,8))->id!=WATER)return 45;
                Game::draw();save_frame("smoke-water.bmp");
                Game::inventory(true);player->inventory.tab(4);Game::draw();save_frame("smoke-creative-water.bmp");
                player->inventory.tab(5);player->inventory.text_input("water");
                if(player->inventory.catalog[0].id!=Items::WATER_BUCKET || !player->inventory.catalog[1].empty())return 46;
                keys[SDLK_e]=Input::DOWN;Game::update();keys[SDLK_e]=Input::NONE;
                if(!show_inventory || player->inventory.catalog_page!=5)return 47;
                Game::draw();save_frame("smoke-creative-search.bmp");Game::inventory(false);
                player->inventory.survival.main[2]={Items::IRON_PICKAXE,1,17};player->vitals.health=13;player->vitals.food=10;
                player->vitals.hurt_ticks=8;auto hurt=player->view_camera();if(abs(hurt.up.x)<.1)return 48;
                Game::draw();save_frame("smoke-hurt.bmp");
                player->vitals.hurt_ticks=0;
                // Swim the camera through two complete water cells. Fog and
                // FOV must not switch to air near internal block boundaries.
                int old_view=player->third_person;player->third_person=0;
                for(int i=0;i<2000;++i){
                    dvec3 eye=chunk_pos_to_world_hp(c->pos)+dvec3(8.5,230.+i*.001,8.5);
                    player->pos=player->previous_position=eye-dvec3(player->cam.pos);
                    auto view=player->view_camera();
                    if(!world->underwater(eye) || abs(view.fov-player->cam.fov*60.f/70.f)>.001f)return 59;
                }
                player->third_person=old_view;
                SDL_Log("Swimming regression passed: continuous underwater fog/FOV across water block boundaries");
                player->pos=player->previous_position=chunk_pos_to_world_hp(c->pos)+dvec3(8.5,230,8.5);player->vel=dvec3(0);player->flying=false;
                if(!world->submerged(player->pos) || !world->underwater(player->pos+dvec3(0,1.62,0)))return 49;
                Game::set_mode(true);player->air=0;player->vital_clock=0;
            }
            if(frame==415){
                double original_dt=delta_time;delta_time=.05;
                for(int i=0;i<20;++i)player->update();delta_time=original_dt;
                if(player->vitals.health!=11)return 57;
                Game::draw();save_frame("smoke-underwater.bmp");
                // Exercise bucket pickup and replacement through normal click handling.
                Game::set_mode(false);player->flying=true;auto* c=world->chunk_map.at(water_chunk);
                player->pos=player->previous_position=chunk_pos_to_world_hp(c->pos)+dvec3(8.5,233,8.5);player->cam.forward=vec3(0,-.99,.01);player->cam.forward=normalize(player->cam.forward);
                player->inventory.creative.main[0]={Items::BUCKET,1};player->action_delay=0;buttons[2]=Input::DOWN;player->handle_clicks();buttons[2]=Input::NONE;
                if(c->get(ivec3(8,232,8))->id==WATER || player->hotbar.get_item()!=Items::WATER_BUCKET)return 50;
                Game::set_mode(true);player->pos=player->previous_position=chunk_pos_to_world_hp(c->pos)+dvec3(8.5,232.5,8.5);
                player->inventory.survival.main[0]={Items::WATER_BUCKET,1};player->action_delay=0;buttons[2]=Input::DOWN;player->handle_clicks();buttons[2]=Input::NONE;
                if(player->hotbar.get_item()!=Items::BUCKET || c->get(ivec3(8,230,8))->data1!=0)return 58;
                // Exercise right-click tool actions on a safe, isolated pad.
                auto use_tool=[&](int id){player->inventory.survival.main[0]={id,1};player->action_delay=0;buttons[2]=Input::DOWN;player->handle_clicks();buttons[2]=Input::NONE;};
                player->pos=player->previous_position=chunk_pos_to_world_hp(c->pos)+dvec3(5.5,232,5.5);player->cam.forward=normalize(vec3(0,-1,.001));
                BlockState grass{GRASS_BLOCK,0,0};c->set(ivec3(5,230,5),&grass);
                use_tool(Items::IRON_HOE);
                if(c->get(ivec3(5,230,5))->id!=FARMLAND || player->inventory.survival.main[0].damage!=1)return 63;
                c->set(ivec3(5,230,5),&grass);use_tool(Items::DIAMOND_SHOVEL);
                if(c->get(ivec3(5,230,5))->id!=GRASS_PATH || player->inventory.survival.main[0].damage!=1)return 64;
                use_tool(Items::FLINT_AND_STEEL);
                if(c->get(ivec3(5,231,5))->id!=FIRE || player->inventory.survival.main[0].damage!=1 || c->get_block_light(ivec3(5,232,5))<14)return 65;
                player->pos=player->previous_position=chunk_pos_to_world_hp(c->pos)+dvec3(5.5,232,2.5);player->cam.forward=normalize(vec3(0,-.65,1));player->vitals.hurt_ticks=0;
                Game::draw();save_frame("smoke-tools-fire.bmp");player->cam.forward=normalize(vec3(0,-1,.001));
                ivec3 flame=ivec3(chunk_pos_to_world_hp(c->pos))+ivec3(5,231,5);
                world->set_fluid({flame.x,flame.y,flame.z},0);
                if(c->get(ivec3(5,231,5))->id!=WATER || c->get_block_light(ivec3(5,232,5))!=0)return 66;
                BlockState source{WATER,0,1};c->set(ivec3(12,230,12),&source);
                player->pos=player->previous_position=chunk_pos_to_world_hp(c->pos)+dvec3(12.5,232,12.5);
                use_tool(Items::FISHING_ROD);if(!player->fishing)return 67;
                player->fish_timer=.001;delta_time=.05;player->update_fishing();delta_time=original_dt;
                if(player->fish_bite<=0)return 68;
                Game::draw();save_frame("smoke-fishing.bmp");
                int old_fish=0;for(auto s:player->inventory.survival.main)if(s.id==Items::RAW_FISH)old_fish+=s.count;
                player->action_delay=0;buttons[2]=Input::DOWN;player->handle_clicks();buttons[2]=Input::NONE;
                int new_fish=0;for(auto s:player->inventory.survival.main)if(s.id==Items::RAW_FISH)new_fish+=s.count;
                if(player->fishing || new_fish!=old_fish+1 || player->inventory.survival.main[0].damage!=1)return 69;
                SDL_Log("Tools/ores smoke passed: all seven ore kinds, creative tools/swords, tilling, paths, fire/light/extinguishing and fishing catch/durability");
                Game::set_mode(false);player->flying=true;player->vitals.health=13;player->vitals.hurt_ticks=0;
                // Integrated flight must follow identical ticks at 10/20/60/144 FPS.
                dvec3 reference_position(0),reference_velocity(0);
                bool old_mouse=mouse_control;mouse_control=false;
                keys[SDLK_w]=keys[SDLK_SPACE]=Input::PRESSED;
                for(int fps:{10,20,60,144}){
                    player->pos=player->previous_position=chunk_pos_to_world_hp(c->pos)+dvec3(8.5,240,8.5);
                    player->vel=dvec3(0);player->cam.forward=vec3(0,0,1);player->accumulator=0;player->flying=true;
                    delta_time=1./fps;for(int i=0;i<fps;++i)player->move();
                    if(fps==10){reference_position=player->pos;reference_velocity=player->vel;}
                    else if(length(player->pos-reference_position)>1e-7 || length(player->vel-reference_velocity)>1e-7)return 71;
                }
                keys[SDLK_w]=keys[SDLK_SPACE]=Input::NONE;
                // Exercise actual SDL edges: a release/press in one render frame
                // still counts as the second tap; key repeats never toggle flight.
                player->flying=false;player->flight_tap=-1;player->grounded=false;
                player->vel=dvec3(3,-2,4);player->accumulator=.023;
                player->previous_position=player->pos-dvec3(.1);
                auto momentum=player->vel,interpolation_start=player->previous_position;
                auto space_event=[](Uint32 type,bool repeat=false){SDL_Event e{};e.type=type;e.key.keysym.sym=SDLK_SPACE;e.key.repeat=repeat;SDL_PushEvent(&e);};
                space_event(SDL_KEYDOWN);handle_events();player->handle_keys();
                space_event(SDL_KEYUP);space_event(SDL_KEYDOWN);handle_events();player->handle_keys();
                if(!player->flying || player->vel!=momentum || player->accumulator!=.023 || player->previous_position!=interpolation_start)return 73;
                update_inputs();space_event(SDL_KEYDOWN,true);handle_events();player->handle_keys();
                if(!player->flying || player->flight_tap!=-1)return 74;
                for(int tap=0;tap<2;++tap){space_event(SDL_KEYUP);space_event(SDL_KEYDOWN);handle_events();player->handle_keys();}
                if(player->flying || player->vel!=momentum || player->accumulator!=.023)return 75;
                keys[SDLK_SPACE]=Input::NONE;
                for(bool survival:{false,true}){
                    Game::set_mode(survival);
                    player->pos=player->previous_position=chunk_pos_to_world_hp(c->pos)+dvec3(8.5,250,4.5);
                    player->vel=dvec3(0,-4,0);player->grounded=false;player->accumulator=0;
                    auto falling_start=player->pos;delta_time=.05;
                    keys[SDLK_w]=keys[SDLK_d]=Input::PRESSED;
                    for(int i=0;i<12;++i)player->move();
                    if(player->pos.x<=falling_start.x+.7 || player->pos.z<=falling_start.z+.7 || player->pos.y>=falling_start.y-5 || player->grounded)return 76;
                    keys[SDLK_w]=keys[SDLK_d]=Input::NONE;
                }
                Game::set_mode(false);player->flying=true;
                mouse_control=old_mouse;delta_time=original_dt;
                SDL_Log("Flight toggle/fall regression passed: SDL double taps and repeats, momentum and interpolation preserved, WASD while falling in both modes");
                player->pos=player->previous_position=chunk_pos_to_world_hp(c->pos)+dvec3(8.5,234,4.5);
                player->vel=dvec3(0);player->accumulator=0;player->third_person=0;player->cam.forward=normalize(vec3(0,-.5,1));
                auto old_stack=player->inventory.creative.main[0];
                player->inventory.creative.main[0]={GRASS_BLOCK,64};player->hotbar.set_index(0);
                player->equipped_item=GRASS_BLOCK;player->equip=1;player->swing_progress=1;
                Game::draw();save_frame("smoke-hand-block.bmp");
                player->swing_progress=.4;Game::draw();save_frame("smoke-hand-swing.bmp");
                player->equipped_item=Items::IRON_PICKAXE;player->swing_progress=1;Game::draw();save_frame("smoke-hand-tool.bmp");
                player->equipped_item=Items::APPLE;player->eat_progress=.6;Game::draw();save_frame("smoke-hand-eating.bmp");
                player->eat_progress=0;player->equipped_item=-1;Game::draw();save_frame("smoke-hand-empty.bmp");
                player->swing_progress=.4;Game::draw();save_frame("smoke-hand-empty-swing.bmp");
                player->swing_progress=1;
                player->cam.forward=normalize(vec3(0,-1,1));
                auto target=raycast(vec3(player->pos)+player->cam.pos,player->cam.forward,20.f,world->get_chunk_from_pos(player->pos));
                if(!target.hit)return 72;
                player->hit_info=target;player->break_progress=.65;player->break_duration=1;
                Game::draw();save_frame("smoke-mining-cracks.bmp");player->break_progress=0;
                Game::inventory(true);player->inventory.tab(0);player->inventory.scroll(100);Game::draw();save_frame("smoke-expanded-blocks.bmp");
                player->inventory.tab(6);Game::draw();save_frame("smoke-expanded-food.bmp");
                player->inventory.tab(10);Game::draw();save_frame("smoke-expanded-materials.bmp");Game::inventory(false);
                player->inventory.creative.main[0]=old_stack;
                SDL_Log("Flight/icons/animation smoke passed: identical flight at 10/20/60/144 FPS; expanded catalog, isometric menus, held block/tool/empty hand, swing/eating and mining cracks rendered");
                // Restore a known source and persist the edits, both bags and player pose.
                BlockState water{WATER,0,0};world->place_block(c,ivec3(8,232,8),&water);
                player->cam.forward=normalize(vec3(0,-.5,1));player->pos=player->previous_position=chunk_pos_to_world_hp(c->pos)+dvec3(8.5,234,4.5);
                saved_position=player->pos;player->hotbar.set_index(2);
                if(!Game::save_world())return 51;
                player->pos+=dvec3(1000,0,1000);Game::update();
                if(world->chunk_map.count(water_chunk))return 52;
                player->pos=saved_position;Game::update();
                c=world->get_chunk_from_pos(saved_position);
                if(!c || c->get(ivec3(8,232,8))->id!=WATER || !(c->get(ivec3(8,232,8))->data2&1) || c->get(ivec3(3,229,3))->id!=STONE)return 53;
                Game::title();Game::select_worlds();Game::draw();save_frame("smoke-worlds.bmp");
                if(Game::saved_worlds.size()!=1)return 54;
                Game::load_world(0);Game::update();
                if(survival_mode || length(player->pos-saved_position)>.01 || player->inventory.survival.main[2].damage!=17 || player->hotbar.index!=2 || player->vitals.health!=13)return 55;
                c=world->get_chunk_from_pos(player->pos);if(!c || c->get(ivec3(8,232,8))->id!=WATER || world->water.pending()==0)return 56;
                SDL_Log("Water/menu/save smoke passed: create/name/seed/mode, flowing/falling water, creative categories/search, source bucket pickup, hurt roll, underwater, chunk unload/reload, world save/reopen and both inventories");
                if(glGetError()!=GL_NO_ERROR)return 70;
                closed=true;
            }
        }

        SDL_GL_SwapWindow(win);
    }

    Game::destroy();

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(win);
    SDL_Quit();
#ifdef __SWITCH__
    romfsExit();
#endif

    return 0;
}
