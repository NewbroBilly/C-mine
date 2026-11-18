#include <world.h>
#include <limits>
#include <settings.h>

// World data and GL objects belong to the main thread. Generate nearest first.

World::World(int64_t value,std::filesystem::path path) : old_p(std::numeric_limits<int>::max()),seed(value),save_path(path) {}
int World::fluid(Water::Pos p)const{
    if(p[1]<0 || p[1]>=CHUNK_HEIGHT)return Water::Solid;
    ivec2 cp(int(floor(p[0]/16.)),int(floor(p[2]/16.)));auto it=chunk_map.find(cp);
    if(it==chunk_map.end())return Water::Unloaded;
    auto* bs=it->second->get(ivec3(p[0]-cp.x*16,p[1],p[2]-cp.y*16));
    if(bs->id==WATER)return bs->data1&15;
    return bs->id==AIR || bs->id==GRASS || bs->id==TORCH?Water::Air:Water::Solid;
}
void World::notify_water(Chunk* c,ivec3 p){ivec3 q=ivec3(chunk_pos_to_world_hp(c->pos))+p;water.changed({q.x,q.y,q.z});}
void World::set_fluid(Water::Pos p,int level){
    auto* c=get_chunk_from_pos(vec3(p[0],p[1],p[2]));if(!c || p[1]<0 || p[1]>=256)return;
    ivec3 local=ivec3(p[0],p[1],p[2])-ivec3(chunk_pos_to_world_hp(c->pos));auto old=*c->get(local);
    if(level!=Water::Air && old.id!=AIR && old.id!=WATER && old.id!=GRASS && old.id!=TORCH)return;
    if(old.id==TORCH || old.id==GRASS){auto loot=Items::drop(old.id,-1,0);if(!loot.empty())player->drops.push_back({loot,dvec3(p[0]+.5,p[1]+.5,p[2]+.5),dvec3(0),0});}
    BlockState b{char(level==Water::Air?AIR:WATER),char(level&15),0};c->set(local,&b);
    if(old.id==TORCH)update_lighting(c,local,false,&old);
}
bool World::underwater(dvec3 p)const{return Water::contains({p.x,p.y,p.z},[&](Water::Pos q){return fluid(q);});}
bool World::submerged(dvec3 p,dvec3* current)const{
    bool found=false;dvec3 flow(0);
    auto get=[&](Water::Pos q){return fluid(q);};
    for(int x=int(floor(p.x-.3));x<=int(floor(p.x+.3));++x)
    for(int y=int(floor(p.y+.001));y<=int(floor(p.y+1.4));++y)
    for(int z=int(floor(p.z-.3));z<=int(floor(p.z+.3));++z){Water::Pos q{x,y,z};int level=fluid(q);
        if(Water::liquid(level) && p.y+.001<y+Water::height(level)){found=true;auto v=Water::flow(q,get);flow+=dvec3(v[0],v[1],v[2]);}}
    if(current)*current=length(flow)>1e-9?normalize(flow):dvec3(0);return found;
}
bool World::save_chunk(Chunk* c){
    if(save_path.empty() || !c->modified)return true;
    if(!Saves::save_chunk(save_path,c->pos.x,c->pos.y,reinterpret_cast<unsigned char*>(c->blocks),sizeof(c->blocks)))return false;
    c->modified=false;return true;
}
bool World::save_chunks(){bool ok=true;for(auto* c:chunk_vector)if(!save_chunk(c))ok=false;return ok;}

Chunk* World::get_chunk_from_pos(const vec3 pos) {
    const auto it = chunk_map.find(world_pos_to_chunk(pos));
    return it == chunk_map.end() ? nullptr : it->second;
}
void World::remove_block(Chunk* chunk, const ivec3& pos,bool creative) {
    if (!chunk || !chunk->can_draw || !in_chunk(pos)) return;
    BlockState previous = *chunk->get(pos);
    if (previous.id == AIR || (previous.id == BEDROCK && !creative)) return;
    BlockState air = { AIR, 0, 0 };
    chunk->set(pos, &air);
    notify_water(chunk,pos);
    update_lighting(chunk, pos, false, &previous);
    rebuild_dirty();
}
bool World::place_block(Chunk* chunk, const ivec3& pos, BlockState* bs) {
    if (!chunk || !bs || pos.y < 0 || pos.y >= CHUNK_HEIGHT) return false;
    ivec3 local = pos;
    chunk = get_neighbor_chunk(chunk, local);
    if (!chunk || (chunk->get(local)->id != AIR && chunk->get(local)->id != WATER)) return false;
    auto old=*chunk->get(local);
    chunk->set(local, bs);
    if(old.id!=AIR)update_lighting(chunk,local,false,&old);
    notify_water(chunk,local);
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
    const int render_distance=Settings::options.distance;
    if (center != old_p || old_distance!=render_distance) {
        old_distance=render_distance;
        old_p = center;
        load_q = {};
        for (auto it = chunk_vector.begin(); it != chunk_vector.end();) {
            Chunk* chunk = *it;
            if (chebyshev_distance(center, chunk->pos) > render_distance) {
                if(!save_chunk(chunk)){++it;continue;}
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
    water_clock+=std::min(delta_time,.05);
    while(water_clock>=.05){water.tick([&](Water::Pos p){return fluid(p);},[&](Water::Pos p,int l){set_fluid(p,l);});water_clock-=.05;}
    rebuild_dirty();
    if (!spawned) {
        // Try the starting column first, then dry surface columns in loaded chunks.
        auto spawn_at=[&](Chunk* c,int x,int z){
            for(int y=253;y>=0;--y){int id=c->blocks[x][y][z].id;
                if(id==AIR || id==WATER || id==GRASS || id==TORCH)continue;
                int a=c->blocks[x][y+1][z].id,b=c->blocks[x][y+2][z].id;
                if((a!=AIR && a!=GRASS) || (b!=AIR && b!=GRASS))return false;
                player->pos=chunk_pos_to_world_hp(c->pos)+dvec3(x+.5,y+1,z+.5);
                player->previous_position=player->pos;player->vel=dvec3(0);player->grounded=true;spawned=true;
                SDL_Log("Surface spawn: %.1f %.1f %.1f",player->pos.x,player->pos.y,player->pos.z);return true;
            }return false;
        };
        auto* c=get_chunk_from_pos(player->pos);
        if(c){ivec3 p=ivec3(floor(player->pos-chunk_pos_to_world_hp(c->pos)));spawn_at(c,p.x,p.z);}
        if(!spawned)for(auto* chunk:chunk_vector){for(int x=0;x<16 && !spawned;++x)for(int z=0;z<16 && !spawned;++z)spawn_at(chunk,x,z);if(spawned)break;}
    }
}
void World::update2() {
    if (load_q.empty()) return;
    const ivec2 p = load_q.front();
    load_q.pop();
    Chunk* chunk = new Chunk(p);
    chunk->load();
    if(!save_path.empty()){
        if(Saves::load_chunk(save_path,p.x,p.y,reinterpret_cast<unsigned char*>(chunk->blocks),sizeof(chunk->blocks))){
            for(auto& column:chunk->blocks)for(auto& row:column)for(auto& b:row)if(b.id<0 || b.id>=BLOCK_COUNT)b={AIR,0,0};
        }
    }
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
    // Resume saved water, including both sides of newly loaded chunk seams.
    for(auto* c:{chunk,chunk->north,chunk->south,chunk->east,chunk->west})if(c)
        for(int x=0;x<16;++x)for(int y=0;y<256;++y)for(int z=0;z<16;++z)if(c->blocks[x][y][z].id==WATER){
            bool active=(c->blocks[x][y][z].data1&15)!=0;
            for(int f=0;f<6;++f){if(f==2)continue;ivec3 p=ivec3(x,y,z)+face_normal[f];auto* n=get_neighbor_chunk(c,p);if(n && (n->get(p)->id==AIR || n->get(p)->id==GRASS || n->get(p)->id==TORCH))active=true;}
            if(active)notify_water(c,ivec3(x,y,z));
        }
    // Saved emitting blocks need their block light restored too.
    for(int x=0;x<16;++x)for(int y=0;y<256;++y)for(int z=0;z<16;++z){auto b=chunk->blocks[x][y][z];if(get_block_by_id(b.id)->light>0)update_lighting(chunk,ivec3(x,y,z),true,&b);}
}
void World::draw() {
    Camera view = player->view_camera();
    const mat4 cam_mat = view.matrix(player->render_position());
    texture_atlas.bind();
    block_shader.bind();
    block_shader.upload_mat4("cam_mat", cam_mat);
    block_shader.upload_vec3("eye_pos", view.pos + (vec3)player->render_position());
    block_shader.upload_int("global_light", global_light);
    block_shader.upload_int("show_ao", (int)show_ao);
    block_shader.upload_int("underwater",underwater(player->render_position()+dvec3(view.pos))?1:0);
    block_shader.upload_float("water_time",SDL_GetTicks()/1000.f);
    for (Chunk* chunk : chunk_vector) chunk->draw();
    std::vector<Chunk*> sorted=chunk_vector;
    std::sort(sorted.begin(),sorted.end(),[&](Chunk* a,Chunk* b){return distance(chunk_pos_to_world(a->pos),vec3(player->pos))>distance(chunk_pos_to_world(b->pos),vec3(player->pos));});
    glDepthMask(GL_FALSE);glDisable(GL_CULL_FACE);
    for(auto* chunk:sorted)if(chunk->can_draw)chunk->mesh.draw(true);
    glDepthMask(GL_TRUE);glEnable(GL_CULL_FACE);
}
void World::destroy() {
    for (Chunk* chunk : chunk_vector) chunk->mesh.destroy();
    for (Chunk* chunk : chunk_vector) delete chunk;
    delete this;
}
