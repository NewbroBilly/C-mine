#include <inventory.h>
#include <player.h>

Inventory::Inventory()
{
    s_inventory = Sprite(vec2(-176 / 2 * 3, -122 / 2 * 3), vec2(176, 122), vec2(0, 228), &gui);
    s_highlight = Sprite(vec2(0, 0), vec2(16, 16), vec2(0, 256), &gui);

    vec2 off = s_inventory.pos + vec2(0, 122 * 3 - 18 * 3) + vec2(8 * 3, -6 * 3);

    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 9; j++)
            slots[i][j] = Slot(off + vec2(j, -i) * 18.0f * 3.0f);

    over = NULL;
    tooltip.font_size=2;
}

void Inventory::set_item(int i, int j, int id)
{
    slots[i][j].set_item(id);
}

int Inventory::get_item(int i, int j)
{
    return slots[i][j].get_item();
}

void Inventory::update()
{
    over = NULL;

    ivec2 int_mouse;

    SDL_GetMouseState(&int_mouse.x, &int_mouse.y);

    vec2 mouse = (vec2)int_mouse - 0.5f * (vec2)win_size;
    mouse.y *= -1;

    if(int_mouse!=last_mouse){keyboard_navigation=false;last_mouse=int_mouse;}
    int move=0;
    if(is_key_down(SDLK_LEFT))move=-1;if(is_key_down(SDLK_RIGHT))move=1;
    if(is_key_down(SDLK_UP))move=-9;if(is_key_down(SDLK_DOWN))move=9;
    if(move){selected=(selected+move+54)%54;keyboard_navigation=true;}
    if(keyboard_navigation)over=&slots[selected/9][selected%9];
    else for(int i=0;i<6;++i)for(int j=0;j<9;++j)if(slots[i][j].is_point_over(mouse)){over=&slots[i][j];selected=i*9+j;}
    if(over && !over->is_empty() && is_key_down(SDLK_RETURN))player->hotbar.set_item(over->get_item());
    std::string label=over && !over->is_empty()?block_name(over->get_item()):"";
    ivec2 position(-int(label.size())*6,-192);
    if(tooltip.pos!=position){tooltip.destroy();tooltip.str.clear();tooltip.pos=position;}
    tooltip.set_str(label);
}

void Inventory::draw()
{
    glClear(GL_DEPTH_BUFFER_BIT);
    s_inventory.draw();

    glClear(GL_DEPTH_BUFFER_BIT);
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 9; j++)
            slots[i][j].draw();

    tooltip.draw();
    if (over != NULL)
    {
        glClear(GL_DEPTH_BUFFER_BIT);
        s_highlight.pos = over->pos;
        s_highlight.draw();
    }
}