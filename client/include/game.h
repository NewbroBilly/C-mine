#pragma once

#include <global.h>
#include <player.h>
#include <shader.h>
#include <world.h>
#include <text.h>
#include <sprite.h>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace Game
{
    enum class Screen { Title, Worlds, CreateWorld, Playing, Paused, Options, Dead };
    extern Screen screen;
    extern std::vector<Saves::Entry> saved_worlds;
    extern int world_index;
    extern std::string world_name,world_seed,message;
    extern bool new_survival,debug_world;
    void select_worlds();
    void create_world();
    void load_world(int index);
    bool save_world();
    void open_options();
    void close_options();
    void apply_options();
    void save_options();
    void start(bool survival=true);
    void set_mode(bool survival);
    void inventory(bool open,bool table=false);
    void die();
    void respawn();
    void pause();
    void resume();
    void title();
    void init();
    void update();
    void draw();
    void destroy();
};
