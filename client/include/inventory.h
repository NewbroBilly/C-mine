#pragma once

#include <slot.h>
#include <text.h>

struct Inventory
{
    Slot slots[6][9];
    Slot* over;
    int selected=0;
    bool keyboard_navigation=false;
    ivec2 last_mouse{-1};
    Text tooltip;

    Sprite s_inventory;
    Sprite s_highlight;

    Inventory();

    void set_item(int i, int j, int id);
    int get_item(int i, int j);

    void update();
    void draw();
};