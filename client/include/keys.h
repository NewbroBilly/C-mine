#pragma once

#include <game.h>
#include <settings.h>

void toggle_mouse_control()
{
    mouse_control = !mouse_control;

    SDL_ShowCursor(!mouse_control);
    SDL_SetRelativeMouseMode((SDL_bool)mouse_control);
    SDL_WarpMouseInWindow(win, win_size.x / 2, win_size.y / 2);
}

void toggle_fullscreen()
{
    Settings::options.fullscreen=!is_fullscreen;
    Game::apply_options();Game::save_options();
}

void handle_misc_keys()
{
    if (is_key_down(SDLK_F3))
        show_debug = !show_debug;

    if (is_key_down(SDLK_F5)) player->third_person = (player->third_person + 1) % 3;

    if (is_key_down(SDLK_F7))
        show_wireframe = !show_wireframe;

    if (is_key_down(SDLK_F6))
        {show_ao = !show_ao;Settings::options.ao=show_ao;Game::save_options();}

    if (!show_inventory && is_key_down(SDLK_RIGHTBRACKET))
    {
        if (global_light < 15)
            global_light++;
    }

    if (!show_inventory && is_key_down(SDLK_LEFTBRACKET))
    {
        if (global_light > 0)
            global_light--;
    }

    if (!(show_inventory && !survival_mode && player->inventory.catalog_page==5) && (is_key_down(SDLK_i) || is_key_down(SDLK_e)))
    {
        Game::inventory(!show_inventory);
    }

    if (!(show_inventory && !survival_mode && player->inventory.catalog_page==5) && is_key_down(SDLK_TAB))
        Game::set_mode(!survival_mode);
}
