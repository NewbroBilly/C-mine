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

struct World
{
    std::unordered_map<ivec2, Chunk*> chunk_map;
    std::vector<Chunk*> chunk_vector;
    std::queue<ivec2> load_q;
    std::queue<Chunk*> dirty_chunks;

    ivec2 old_p;
    int old_distance=-1;
    bool spawned = false;

    World();

    Chunk* get_chunk_from_pos(const vec3 pos);

    void remove_block(Chunk* chunk, const ivec3& pos,bool creative=false);
    bool place_block(Chunk* chunk, const ivec3& pos, BlockState* bs);

    void update();
    void update2();
    void rebuild_dirty();
    void draw();
    void destroy();
};
