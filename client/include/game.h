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
    enum class Screen { Title, Playing, Paused };
    extern Screen screen;
    void start();
    void pause();
    void resume();
    void title();
    void init();
    void update();
    void draw();
    void destroy();
};