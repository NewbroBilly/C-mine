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
    Items::Inventory survival,creative;
    std::array<Items::Stack,54> catalog{};
    Slot storage[36],craft_slots[9],result_slot,offhand,armor[4];
    Items::Stack result;
    bool workbench=false;
    int catalog_page=0;
    Items::Inventory& data();
    void bind_hotbar();
    void open(bool table=false);
    void close();
    void layout();
    void activate(int index,bool right,bool quick);
    int result_index()const{return workbench?45:40;}
    Slot* slot_at(int index);

    Sprite s_inventory;
    Sprite s_highlight;

    Inventory();

    void set_item(int i, int j, int id);
    int get_item(int i, int j);

    void update();
    void draw();
};
