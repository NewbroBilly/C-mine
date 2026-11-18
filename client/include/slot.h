#pragma once

#include <global.h>
#include <sprite.h>
#include <blocks.h>
#include <items.h>

struct Slot
{
    vec2 pos;

    Items::Stack owned;
    Items::Stack* stack=nullptr;
    Items::Stack& value(){return stack?*stack:owned;}

    Slot() {}
    Slot(const vec2& _pos);
    void bind(Items::Stack& value){stack=&value;}

    bool is_empty();
    int get_item();
    void set_item(int id,int count=1);
    bool is_point_over(vec2 p);

    void draw();
};
