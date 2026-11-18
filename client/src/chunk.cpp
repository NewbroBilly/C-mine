#include <chunk.h>
#include <oak_tree.h>
#include <world.h>

Chunk::Chunk(const ivec2& _pos)
{
    pos = _pos;

    north = NULL;
    south = NULL;
    east = NULL;
    west = NULL;

    dirty = false;
    can_draw = false;

    mesh = ChunkMesh(this);
    std::memset(blocks, 0, sizeof(blocks));
    std::memset(light, 0, sizeof(light));
}

BlockState* Chunk::get(const ivec3& p)
{
    return &blocks[p.x][p.y][p.z];
}

void Chunk::set(const ivec3& p, BlockState* bs)
{
    if (!in_chunk(p) || !bs) return;
    const BlockState& previous = blocks[p.x][p.y][p.z];
    if (previous.id == bs->id && previous.data1 == bs->data1 && previous.data2 == bs->data2) return;
    dirty = true;
    modified = true;

    blocks[p.x][p.y][p.z] = *bs;

    if (p.x == 0 && west != NULL)
        west->dirty = true;
    else if (p.x == (CHUNK_WIDTH - 1) && east != NULL)
        east->dirty = true;

    if (p.z == 0 && south != NULL)
        south->dirty = true;
    else if (p.z == (CHUNK_WIDTH - 1) && north != NULL)
        north->dirty = true;
}

int Chunk::get_light(const ivec3& p)
{
    return (int)light[p.x][p.y][p.z];
}

void Chunk::set_light(const ivec3& p, int value)
{
    if (light[p.x][p.y][p.z] == (unsigned char)value) return;
    dirty = true;
    for (int dx = -1; dx <= 1; ++dx)
        for (int dz = -1; dz <= 1; ++dz) {
            ivec3 neighbor_pos = p + ivec3(dx, 0, dz);
            Chunk* neighbor = get_neighbor_chunk(this, neighbor_pos);
            if (neighbor) neighbor->dirty = true;
        }
    light[p.x][p.y][p.z] = (unsigned char)value;
}

int Chunk::get_sky_light(const ivec3& p)
{
    return (int)(light[p.x][p.y][p.z] & 0x0f);
}

void Chunk::set_sky_light(const ivec3& p, int value)
{
    set_light(p, (light[p.x][p.y][p.z] & 0xf0) | ((unsigned char)value & 0x0f));
}

int Chunk::get_block_light(const ivec3& p)
{
    return (int)(light[p.x][p.y][p.z] >> 4);
}

void Chunk::set_block_light(const ivec3& p, int value)
{
    set_light(p, (light[p.x][p.y][p.z] & 0x0f) | ((unsigned char)value << 4));
}

void Chunk::load()
{
    SimplexNoise noise;
    vec3 world_pos = chunk_pos_to_world(pos);
    const uint64_t seed=world?uint64_t(world->seed):0;
    float ox=float(seed&0xffff)*.25f,oz=float((seed>>16)&0xffff)*.25f;

    auto terrain=[&](int x,int y,int z)->int {
        if(y<0 || y>=CHUNK_HEIGHT)return AIR;
        if(y==0)return BEDROCK;
        int height=int(noise.fractal(6,(x+ox)*.002f,(z+oz)*.002f)*40+120);
        if(y>=height)return y<112?WATER:AIR;
        if(noise.fractal(3,(x+ox)*.01f,y*.01f,(z+oz)*.01f)>=.3f)return AIR;
        if(y==height-1)return GRASS_BLOCK;
        return y>=height-4?DIRT:STONE;
    };
    for(int x=0;x<CHUNK_WIDTH;++x)for(int z=0;z<CHUNK_WIDTH;++z)
        for(int y=0;y<CHUNK_HEIGHT;++y)blocks[x][y][z].id=terrain(int(world_pos.x)+x,y,int(world_pos.z)+z);
    // Each nearby eight-block cell owns one candidate. Query base terrain in
    // world coordinates and clip writes, so crowns continue across chunk seams.
    for(int gx=pos.x*2-1;gx<=pos.x*2+2;++gx)for(int gz=pos.y*2-1;gz<=pos.y*2+2;++gz){
        Trees::Random random(uint64_t(int64_t(gx))*341873128712ULL+uint64_t(int64_t(gz))*132897987541ULL+1122+seed);
        if(random.next(3)==0)continue;
        int tx=gx*8+2+random.next(4),tz=gz*8+2+random.next(4);
        int height=int(noise.fractal(6,(tx+ox)*.002f,(tz+oz)*.002f)*40+120);
        auto get=[&](int x,int y,int z)->Trees::Cell {
            switch(terrain(x,y,z)) {case AIR:return Trees::Air;case GRASS_BLOCK:return Trees::Grass;case DIRT:return Trees::Dirt;default:return Trees::Other;}
        };
        auto set=[&](int x,int y,int z,Trees::Cell cell){
            int lx=x-int(world_pos.x),lz=z-int(world_pos.z);
            if(!in_chunk(lx,y,lz))return;
            int id=cell==Trees::Log?LOG:(cell==Trees::Leaves?LEAVES:DIRT);
            blocks[lx][y][lz]={(char)id,(char)(id==LOG?1:0),0};
        };
        Trees::oak(tx,height,tz,random,get,set);
    }

    // Caves must never open into the void at the bottom of the world.
    for (int x=0;x<CHUNK_WIDTH;++x)
        for (int z=0;z<CHUNK_WIDTH;++z) blocks[x][0][z].id = BEDROCK;

    dirty = true;
    can_draw = true;
}

void Chunk::draw()
{
    if (!can_draw)
        return;

    mesh.draw();
}

void Chunk::destroy()
{
    mesh.destroy();

    if (north != NULL)
    {
        north->south = NULL;
        north->dirty = true;
    }

    if (south != NULL)
    {
        south->north = NULL;
        south->dirty = true;
    }

    if (east != NULL)
    {
        east->west = NULL;
        east->dirty = true;
    }

    if (west != NULL)
    {
        west->east = NULL;
        west->dirty = true;
    }

    delete this;
}

vec3 chunk_pos_to_world(const ivec2& pos)
{
    return vec3(pos.x, 0, pos.y) * (float)CHUNK_WIDTH;
}

dvec3 chunk_pos_to_world_hp(const ivec2& pos)
{
    return dvec3(pos.x, 0, pos.y) * (double)CHUNK_WIDTH;
}

ivec2 world_pos_to_chunk(const vec3& pos)
{
    return ivec2(floorf(pos.x / CHUNK_WIDTH), floorf(pos.z / CHUNK_WIDTH));
}

bool in_chunk(int x, int y, int z)
{
    if (x < 0 || y < 0 || z < 0 || x >= CHUNK_WIDTH || y >= CHUNK_HEIGHT || z >= CHUNK_WIDTH)
        return false;

    return true;
}

bool in_chunk(const ivec3& pos)
{
    return in_chunk(pos.x, pos.y, pos.z);
}

Chunk* get_neighbor_chunk(Chunk* chunk, ivec3& pos)
{
    if (!chunk || pos.y < 0 || pos.y >= CHUNK_HEIGHT) return NULL;
    if (pos.x < 0)
    {
        pos.x += CHUNK_WIDTH;
        chunk = chunk->west;
    }
    else if (pos.x >= CHUNK_WIDTH)
    {
        pos.x -= CHUNK_WIDTH;
        chunk = chunk->east;
    }

    if (!chunk || chunk->can_draw == false)
        return NULL;

    if (!chunk) return NULL;

    if (pos.z < 0)
    {
        pos.z += CHUNK_WIDTH;
        chunk = chunk->south;
    }
    else if (pos.z >= CHUNK_WIDTH)
    {
        pos.z -= CHUNK_WIDTH;
        chunk = chunk->north;
    }

    if (!chunk || chunk->can_draw == false)
        return NULL;

    return chunk;
}
