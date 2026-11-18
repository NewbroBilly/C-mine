#pragma once

#include <global.h>
#include <camera.h>
#include <blocks.h>
#include <raycast.h>
#include <hitbox.h>
#include <world.h>
#include <hotbar.h>
#include <inventory.h>
#include <vector>
#include <algorithm>
#include <gtx/rotate_vector.hpp>
#include <gtx/vector_angle.hpp>

using namespace glm;

struct Player
{
    dvec3 pos;
    dvec3 vel;

    bool grounded;
    int third_person = 0;
    double accumulator = 0, walk_phase = 0;
    int jump_cooldown = 0;
    dvec3 previous_position{0};
    dvec3 render_position() const;
    Camera view_camera();

    Camera cam;

    Hitbox hitbox;

    Hotbar hotbar;
    Inventory inventory;

    HitInfo hit_info;
    Items::Vitals vitals;
    bool flying=false;
    int air=300;
    bool fishing=false;
    dvec3 bobber{0};
    double fish_timer=0,fish_bite=0;
    double burn_clock=0;
    void update_fishing();
    double flight_tap=-1,vital_clock=0,break_progress=0,break_duration=0,action_delay=0,eat_progress=0;
    int eating_item=-1;
    ivec3 mining_position{-1000000};
    int mining_tool=-1;
    struct Dropped {Items::Stack stack;dvec3 position,velocity;double age=0;};
    std::vector<Dropped> drops;
    void drop_item(Items::Stack stack);
    void update_drops();
    void draw_hud();
    void draw_drops();
    bool place_selected(Chunk* chunk,ivec3 local,ivec3 normal);

    Player() {}
    Player(const dvec3& _pos);

    void handle_keys();
    void handle_clicks();
    void move();

    void update();
    void draw();
    void destroy();
};
