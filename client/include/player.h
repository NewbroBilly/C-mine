#pragma once

#include <global.h>
#include <model_animation.h>
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
    ModelAnimation::Walk model_walk;
    int model_age=0;
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
    // Short-lived vanilla-style item effects. Timers are in simulation ticks.
    int speed_effect=0, strength_effect=0, regeneration_effect=0, poison_effect=0;
    int fire_resistance_effect=0, night_vision_effect=0;
    void update_fishing();
    void hurt(float damage,bool bypass=false);
    double flight_tap=-1,vital_clock=0,break_progress=0,break_duration=0,action_delay=0,eat_progress=0;
    int eating_item=-1;
    double bow_time=0;
    bool blocking=false;
    double item_message_time=0;
    std::string item_message;
    struct Shot {int item;dvec3 position,velocity;double age=0;float damage=0;};
    std::vector<Shot> shots;
    void update_projectiles();
    void launch(int item,float charge=1);
    void consume_held(int result=-1);
    bool teleport_near(dvec3 target);
    ivec3 mining_position{-1000000};
    int mining_tool=-1;
    struct Dropped {Items::Stack stack;dvec3 position,velocity;double age=0;};
    std::vector<Dropped> drops;
    void drop_item(Items::Stack stack);
    void update_drops();
    void draw_hud();
    void draw_drops();
    double swing_progress=1;
    float equip=0,bob_amount=0;
    int equipped_item=-1;
    void swing();
    void animate();
    void draw_hand();
    bool place_selected(Chunk* chunk,ivec3 local,ivec3 normal);
    void apply_item_effect(int id);
    bool use_item_action(int id,Chunk* chunk,ivec3 local,ivec3 normal);

    Player() {}
    Player(const dvec3& _pos);

    void handle_keys();
    void handle_clicks();
    void move();

    void update();
    void draw();
    void destroy();
};
