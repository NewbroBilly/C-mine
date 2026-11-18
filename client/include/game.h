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
    enum class Screen { Title, Playing, Paused, Options };
    extern Screen screen;
    void open_options();
    void close_options();
    void apply_options();
    void save_options();
    void start();
    void pause();
    void resume();
    void title();
    void init();
    void update();
    void draw();
    void destroy();
};