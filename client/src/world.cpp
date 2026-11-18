#include <world.h>
#include <limits>

// World data and GL objects belong to the main thread. Generate nearest first.
constexpr int render_distance = 4;
World::World() : old_p(std::numeric_limits<int>::max()) {}

Chunk* World::get_chunk_from_pos(const vec3 pos) {
    const auto it = chunk_map.find(world_pos_to_chunk(pos));
    return it == chunk_map.end() ? nullptr : it->second;
}
void World::remove_block(Chunk* chunk, const ivec3& pos) {
    if (!chunk || !chunk->can_draw || !in_chunk(pos)) return;
    BlockState previous = *chunk->get(pos);
    if (previous.id == AIR) return;
    BlockState air = { AIR, 0, 0 };
    chunk->set(pos, &air);
    update_lighting(chunk, pos, false, &previous);
    rebuild_dirty();
}
bool World::place_block(Chunk* chunk, const ivec3& pos, BlockState* bs) {
    if (!chunk || !bs || pos.y < 0 || pos.y >= CHUNK_HEIGHT) return false;
    ivec3 local = pos;
    chunk = get_neighbor_chunk(chunk, local);
    if (!chunk || chunk->get(local)->id != AIR) return false;
    chunk->set(local, bs);
    update_lighting(chunk, local, true, bs);
    // Commit every affected seam before the next draw, with no empty frame.
    rebuild_dirty();
    return true;
}
void World::rebuild_dirty() {
    for (Chunk* chunk : chunk_vector)
        if (chunk->can_draw && chunk->dirty) {
            chunk->mesh.build();
            chunk->dirty = false;
        }
}
void World::update() {
    const ivec2 center = world_pos_to_chunk(player->pos);
    if (center != old_p) {
        old_p = center;
        load_q = {};
        for (auto it = chunk_vector.begin(); it != chunk_vector.end();) {
            Chunk* chunk = *it;
            if (chebyshev_distance(center, chunk->pos) > render_distance) {
                chunk_map.erase(chunk->pos);
                it = chunk_vector.erase(it);
                chunk->destroy();
            } else ++it;
        }
        for (int radius = 0; radius <= render_distance; ++radius)
            for (int x = -radius; x <= radius; ++x)
                for (int z = -radius; z <= radius; ++z) {
                    const ivec2 p = center + ivec2(x, z);
                    if (chebyshev_distance(center, p) == radius && !chunk_map.count(p))
                        load_q.push(p);
                }
    }
    update2();
    rebuild_dirty();
    if (!spawned) {
        Chunk* chunk = get_chunk_from_pos(player->pos);
        if (chunk && chunk->can_draw) {
            const ivec3 local = ivec3(floor(player->pos - chunk_pos_to_world_hp(chunk->pos)));
            // Stop at the highest block: never search for a cave floor.
            for (int y = CHUNK_HEIGHT - 1; y >= 0; --y) {
                if (chunk->get(ivec3(local.x, y, local.z))->id == AIR) continue;
                if (y <= CHUNK_HEIGHT - 3 &&
                    get_block_by_id(chunk->get(ivec3(local.x, y, local.z))->id)->opacity > 0) {
                    player->pos.y = y + 1.0;
                    player->vel = dvec3(0);
                    player->grounded = true;
                    spawned = true;
                    SDL_Log("Surface spawn: %.1f %.1f %.1f", player->pos.x, player->pos.y, player->pos.z);
                }
                break;
            }
        }
    }
}
void World::update2() {
    if (load_q.empty()) return;
    const ivec2 p = load_q.front();
    load_q.pop();
    Chunk* chunk = new Chunk(p);
    chunk->load();
    auto neighbor = [&](ivec2 offset) -> Chunk* {
        const auto it = chunk_map.find(p + offset);
        return it == chunk_map.end() ? nullptr : it->second;
    };
    chunk->north = neighbor(ivec2(0, 1));
    chunk->south = neighbor(ivec2(0, -1));
    chunk->east = neighbor(ivec2(1, 0));
    chunk->west = neighbor(ivec2(-1, 0));
    if (chunk->north) { chunk->north->south = chunk; chunk->north->dirty = true; }
    if (chunk->south) { chunk->south->north = chunk; chunk->south->dirty = true; }
    if (chunk->east) { chunk->east->west = chunk; chunk->east->dirty = true; }
    if (chunk->west) { chunk->west->east = chunk; chunk->west->dirty = true; }
    chunk_map.emplace(p, chunk);
    chunk_vector.push_back(chunk);
    calculate_lighting(chunk);
}
void World::draw() {
    Camera view = player->view_camera();
    const mat4 cam_mat = view.matrix(player->pos);
    texture_atlas.bind();
    block_shader.bind();
    block_shader.upload_mat4("cam_mat", cam_mat);
    block_shader.upload_vec3("eye_pos", view.pos + (vec3)player->pos);
    block_shader.upload_int("global_light", global_light);
    block_shader.upload_int("show_ao", (int)show_ao);
    for (Chunk* chunk : chunk_vector) chunk->draw();
}
void World::destroy() {
    for (Chunk* chunk : chunk_vector) chunk->mesh.destroy();
    for (Chunk* chunk : chunk_vector) delete chunk;
    delete this;
}
