#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <gtx/hash.hpp>

#include <unordered_map>
#include <chunk.h>
#include <lighting.h>
#include <player.h>
#include <misc.h>
#include <water.h>
#include <saves.h>

struct World
{
    std::unordered_map<ivec2, Chunk*> chunk_map;
    std::vector<Chunk*> chunk_vector;
    std::queue<ivec2> load_q;
    std::queue<Chunk*> dirty_chunks;

    ivec2 old_p;
    int old_distance=-1;
    bool spawned = false;
    bool spawn_known=false;
    dvec3 spawn_point{8.5,122,8.5};
    double day_ticks=6000;
    int64_t seed=0;
    std::filesystem::path save_path;
    Water::Simulator water;
    double water_clock=0;
    double fire_clock=0;
    std::map<Water::Pos,int> fires;
    int fluid(Water::Pos p) const;
    void set_fluid(Water::Pos p,int level);
    void notify_water(Chunk* chunk,ivec3 pos);
    bool save_chunks();
    bool save_chunk(Chunk* chunk);
    bool submerged(dvec3 feet,dvec3* current=nullptr)const;
    bool underwater(dvec3 point)const;

    World(int64_t seed=0,std::filesystem::path path={});

    Chunk* get_chunk_from_pos(const vec3 pos);

    void remove_block(Chunk* chunk, const ivec3& pos,bool creative=false);
    bool place_block(Chunk* chunk, const ivec3& pos, BlockState* bs);

    void update();
    void update2();
    void rebuild_dirty();
    void draw();
    void destroy();
};
