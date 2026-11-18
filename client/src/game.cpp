#include <game.h>
#include <vector>
#include <keys.h>
#include <menu.h>
#include <avatar.h>
#include <settings.h>
#include <ui.h>

SDL_Window* win;
uvec2 win_size = uvec2(800, 600);
char win_name[] = "C-mine";

bool closed = false;
bool mouse_control = false;
bool is_fullscreen = false;
bool show_wireframe = false;
bool show_ao = true;
bool show_debug = false;
bool show_inventory = false;
bool survival_mode = true;

double delta_time;

Shader line_shader;
Shader block_shader;
Shader font_shader;
Shader gui_shader;

Texture texture_atlas;
Texture day_lightmap;
Texture night_lightmap;
Texture font;
Texture gui;

Player* player;
World* world;

Text debug_text;
Sprite crosshair;

int global_light = 15;

static std::string options_path();

void Game::init()
{
    Settings::load(options_path(),Settings::options);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    line_shader = Shader("shaders/line_vert.glsl", "shaders/line_frag.glsl");
    block_shader = Shader("shaders/block_vert.glsl", "shaders/block_frag.glsl");
    font_shader = Shader("shaders/font_vert.glsl", "shaders/font_frag.glsl");
    gui_shader = Shader("shaders/gui_vert.glsl", "shaders/gui_frag.glsl");

    std::string pack = "resource_pack/";
#ifdef __SWITCH__
    if (std::filesystem::exists("sdmc:/switch/C-mine/resource_pack/texture_atlas.png"))
        pack = "sdmc:/switch/C-mine/resource_pack/";
#endif
    const bool custom_pack = std::filesystem::exists(pack+"texture_atlas.png") &&
                             std::filesystem::exists(pack+"texture_atlas_mipmap.png");
    texture_atlas = Texture(custom_pack ? (pack+"texture_atlas.png").c_str() : "textures/texture_atlas.png", 0);
    texture_atlas.use_mipmap(custom_pack ? (pack+"texture_atlas_mipmap.png").c_str() : "textures/texture_atlas_mipmap.png", 1);
    SDL_Log("Block textures: %s", custom_pack ? "imported resource pack" : "supplied atlas");

    day_lightmap = Texture("textures/day_lightmap.png", 1);
    night_lightmap = Texture("textures/night_lightmap.png", 2);

    font = Texture("textures/font.png", 3);
    gui = Texture("textures/gui.png", 4);

    texture_atlas.bind();
    day_lightmap.bind();
    night_lightmap.bind();
    font.bind();
    gui.bind();

    block_shader.bind();
    block_shader.upload_vec4("fog_color", vec4(192, 216, 255, 255) / 255.0f);
    block_shader.upload_texture("tex0", texture_atlas);
    block_shader.upload_texture("tex1", day_lightmap);
    block_shader.upload_texture("tex2", night_lightmap);

    font_shader.bind();
    font_shader.upload_texture("tex3", font);
    font_shader.upload_int("win_width", win_size.x);
    font_shader.upload_int("win_height", win_size.y);

    gui_shader.bind();
    gui_shader.upload_int("win_width", win_size.x);
    gui_shader.upload_int("win_height", win_size.y);

    apply_options();
    blocks_init();
    Ui::init();

    Menu::init();
    Avatar::init();
    debug_text = Text(ivec2((int)win_size.x / -2 + 6, (int)win_size.y / 2 - 6));
    crosshair = Sprite(vec2(-9 / 2 * 3 - 1, -9 / 2 * 3 - 2), vec2(9, 9), vec2(15 * 16 + 3, 3 + 9), &gui);

}
Game::Screen Game::screen = Game::Screen::Title;
static void capture(bool value) {
    mouse_control = value;
    SDL_ShowCursor(!value);
    SDL_SetRelativeMouseMode(value ? SDL_TRUE : SDL_FALSE);
    std::fill(keys, keys + KEY_MAX, Input::NONE);
    std::fill(buttons, buttons + BUTTON_MAX, Input::NONE);
    SDL_GetRelativeMouseState(nullptr, nullptr);
    Menu::reset();
}
static std::string options_path(){
    if(!Settings::storage_override.empty())return Settings::storage_override;
#ifdef __SWITCH__
    return "sdmc:/switch/C-mine/options.txt";
#else
    char* path=SDL_GetPrefPath("C-mine","C-mine");
    std::string result=path?std::string(path)+"options.txt":"options.txt";
    if(path)SDL_free(path);return result;
#endif
}
void Game::save_options(){if(!Settings::save(options_path(),Settings::options))SDL_Log("Could not save options");}
void Game::apply_options(){
    auto& o=Settings::options;Settings::validate(o);show_ao=o.ao;
#ifndef __SWITCH__
    if(is_fullscreen!=o.fullscreen){
        if(SDL_SetWindowFullscreen(win,o.fullscreen?SDL_WINDOW_FULLSCREEN_DESKTOP:0)==0)is_fullscreen=o.fullscreen;
        else o.fullscreen=is_fullscreen;
        if(!is_fullscreen)SDL_SetWindowSize(win,800,600);
    }
#endif
    int w,h;SDL_GetWindowSize(win,&w,&h);win_size=uvec2(w,h);glViewport(0,0,w,h);
    SDL_GL_SetSwapInterval(o.vsync?1:0);
    if(player){player->cam.fov=player->cam.target_fov=float(o.fov);player->cam.aspect_ratio=w/float(h);}
    font_shader.bind();font_shader.upload_int("win_width",w);font_shader.upload_int("win_height",h);
    gui_shader.bind();gui_shader.upload_int("win_width",w);gui_shader.upload_int("win_height",h);
}
void Game::open_options(){screen=Screen::Options;capture(false);}
void Game::close_options(){save_options();screen=world?Screen::Paused:Screen::Title;capture(false);}
void Game::start(bool survival) {
    survival_mode=survival;
    player = new Player(dvec3(8.5,122,8.5));
    player->inventory.bind_hotbar();
    world = new World();
    show_inventory = false;
    screen = Screen::Playing;
    capture(true);
}
void Game::inventory(bool open,bool table){
    if(!player)return;
    if(open)player->inventory.open(table);else player->inventory.close();
    show_inventory=open;capture(!open);
}
void Game::set_mode(bool survival){
    if(!player || survival_mode==survival)return;
    if(show_inventory)inventory(false);
    survival_mode=survival;player->flying=false;player->vel=dvec3(0);
    player->accumulator=0;player->previous_position=player->pos;player->vitals.fallen=0;
    player->break_progress=0;player->inventory.bind_hotbar();
}
void Game::die(){
    if(screen!=Screen::Playing)return;
    if(show_inventory)inventory(false);
    auto& d=player->inventory.survival;
    for(auto& s:d.main){player->drop_item(s);s.clear();}
    player->drop_item(d.offhand);d.offhand.clear();
    screen=Screen::Dead;capture(false);
}
void Game::respawn(){
    player->pos=player->previous_position=dvec3(8.5,122,8.5);player->vel=dvec3(0);player->grounded=false;
    player->accumulator=player->vital_clock=0;player->vitals={};player->break_progress=0;player->hit_info.hit=false;
    world->spawned=false;screen=Screen::Playing;capture(true);
}
void Game::pause() {
    if (screen != Screen::Playing) return;
    if(show_inventory)inventory(false);
    screen = Screen::Paused;
    capture(false);
}
void Game::resume() { screen = Screen::Playing; capture(true); }
void Game::title() {
    if (world) world->destroy();
    if (player) player->destroy();
    world = nullptr; player = nullptr;
    Ui::clear_text();
    screen = Screen::Title;
    capture(false);
}

void Game::update()
{
    if (is_key_down(SDLK_F11)) toggle_fullscreen();
    if (is_key_down(SDLK_ESCAPE)) {
        if (screen == Screen::Options) { close_options(); return; }
        if (screen == Screen::Paused) { resume(); return; }
        if (screen == Screen::Playing) {
            if (show_inventory) { inventory(false); }
            else pause();
            return;
        }
    }
    if (screen != Screen::Playing) {
        const bool options=screen==Screen::Options;
        int button = Menu::update(world!=nullptr);
        if(options){
            if(button==7)close_options();
            else if(button>=0){
                auto& o=Settings::options;int direction=is_key_down(SDLK_LEFT)?-1:1;
                if(button==0)o.fov=o.fov+direction*10<50?110:(o.fov+direction*10>110?50:o.fov+direction*10);
                if(button==1)o.sensitivity=o.sensitivity+direction*10<10?200:(o.sensitivity+direction*10>200?10:o.sensitivity+direction*10);
                if(button==2)o.distance=o.distance+direction*2<2?8:(o.distance+direction*2>8?2:o.distance+direction*2);
                if(button==3)o.vsync=!o.vsync;if(button==4)o.ao=!o.ao;
                if(button==5)o.fullscreen=!o.fullscreen;if(button==6)o.invert_y=!o.invert_y;
                apply_options();save_options();
            }
        }else if(screen==Screen::Dead){if(button==0)respawn();else if(button==1)title();}
        else if(screen==Screen::Title){if(button==0)start(true);else if(button==1)start(false);else if(button==2)open_options();else if(button==3)closed=true;}
        else if(screen==Screen::Paused){if(button==0)resume();else if(button==1)set_mode(!survival_mode);else if(button==2)open_options();else if(button==3)title();else if(button==4)closed=true;}
        return;
    }
    handle_misc_keys();

    world->update();
    player->update();

    if (show_inventory)
        player->inventory.update();

    if (show_debug)
    {
        std::stringstream buf;

        buf << "fps: " << (int)(1 / delta_time) << "\n\n";

        buf << "load_q: " << world->load_q.size() << "\n" << "dirty: " << world->dirty_chunks.size() << "\n\n";

        buf << "pos: " << std::fixed << std::setprecision(4) << player->pos.x << " / " << std::fixed << std::setprecision(4) << player->pos.y << " / " << std::fixed << std::setprecision(4) << player->pos.z << "\n";
        buf << "vel: " << std::fixed << std::setprecision(4) << player->vel.x << " / " << std::fixed << std::setprecision(4) << player->vel.y << " / " << std::fixed << std::setprecision(4) << player->vel.z << "   " << length(player->vel) << "\n\n";
        buf << "hand: " << player->hotbar.get_item() << " in slot " << player->hotbar.get_index() << "\n";

        buf << "mode: ";

        if (survival_mode)
            buf << "survival\n";
        else
            buf << "creative\n";

        Chunk* chunk = world->get_chunk_from_pos(player->pos);

        if (chunk != NULL)
        {
            buf << "chunk: " << chunk->pos.x << " " << chunk->pos.y << "\n";

            ivec3 lp = player->pos - chunk_pos_to_world_hp(chunk->pos);
            lp.y++;

            if (in_chunk(lp))
                buf << "light: " << chunk->get_sky_light(lp) << " sky " << chunk->get_block_light(lp) << " block\n";
        }

        if (player->hit_info.hit)
        {
            buf << "\nblock: " << player->hit_info.block_pos.x << " " << player->hit_info.block_pos.y << " " << player->hit_info.block_pos.z << " in " << player->hit_info.chunk->pos.x << " " << player->hit_info.chunk->pos.y;

            BlockState* bs = player->hit_info.chunk->get(player->hit_info.block_pos);

            buf << "\nid: " << (int)bs->id << "\ndata1: " << std::setfill('0') << std::setw(2) << std::hex << (int)bs->data1 << "\ndata2: " << std::setfill('0') << std::setw(2) << std::hex << (int)bs->data2;
        }

        debug_text.set_str(buf.str());
    }
}

void Game::draw()
{
    vec3 day_color = vec3(121, 166, 255) / 255.0f;
    vec3 night_color = vec3(10, 15, 25) / 255.0f;

    float t = global_light / 15.0f;
    vec3 sky_color = t * day_color + (1 - t) * night_color;

    glClearColor(sky_color.x, sky_color.y, sky_color.z, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!world) { Menu::draw(false); return; }
    world->draw();
    player->draw();
    player->draw_drops();

    player->hotbar.draw();
    player->draw_hud();

    if (show_inventory)
        player->inventory.draw();
    else
    {
        glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);

        crosshair.draw();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glClear(GL_DEPTH_BUFFER_BIT);

    if (show_debug)
        debug_text.draw();
    if (screen == Screen::Paused || screen == Screen::Options || screen==Screen::Dead) Menu::draw(true);
}

void Game::destroy()
{
    if (world) world->destroy();
    if (player) player->destroy();
    Menu::destroy();
    Avatar::destroy();
    Ui::destroy();

    debug_text.destroy();
    crosshair.destroy();

    line_shader.destroy();
    block_shader.destroy();
    font_shader.destroy();
    gui_shader.destroy();

    texture_atlas.destroy();
    day_lightmap.destroy();
    night_lightmap.destroy();
    font.destroy();
    gui.destroy();
}
