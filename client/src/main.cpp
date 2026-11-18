#include <global.h>
#include <game.h>
#include <settings.h>
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

            if (index >= 0 && index < KEY_MAX && keys[index] == Input::NONE)
                keys[index] = Input::DOWN;
        }
        else if (event.type == SDL_KEYUP)
        {
            int index = event.key.keysym.sym & (~0x40000000);

            if (index >= 0 && index < KEY_MAX)
                keys[index] = Input::UP;
        }
        else if(event.type==SDL_MOUSEWHEEL && Game::screen==Game::Screen::Playing && player && !show_inventory) {
            int wheel=event.wheel.direction==SDL_MOUSEWHEEL_FLIPPED?-event.wheel.y:event.wheel.y;
            if(wheel) player->hotbar.set_index(player->hotbar.get_index()-(wheel>0?1:-1));
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
