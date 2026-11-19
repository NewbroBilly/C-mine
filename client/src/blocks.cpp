#include <blocks.h>
#include <block_geometry.h>
#include <water.h>

#define RIGHT_FACE  0
#define LEFT_FACE   1
#define TOP_FACE    2
#define BOTTOM_FACE 3
#define BACK_FACE   4
#define FRONT_FACE  5

ivec3 vertex_offset[6][4] =
{
    { ivec3(1, 0, 0), ivec3(1, 0, 1), ivec3(1, 1, 1), ivec3(1, 1, 0) },
    { ivec3(0, 0, 1), ivec3(0, 0, 0), ivec3(0, 1, 0), ivec3(0, 1, 1) },
    { ivec3(0, 1, 0), ivec3(1, 1, 0), ivec3(1, 1, 1), ivec3(0, 1, 1) },
    { ivec3(0, 0, 1), ivec3(1, 0, 1), ivec3(1, 0, 0), ivec3(0, 0, 0) },
    { ivec3(1, 0, 1), ivec3(0, 0, 1), ivec3(0, 1, 1), ivec3(1, 1, 1) },
    { ivec3(0, 0, 0), ivec3(1, 0, 0), ivec3(1, 1, 0), ivec3(0, 1, 0) }
};

const vec3 cross_vertex_offset[] =
{
    vec3(0, 0, 0),
    vec3(1, 0, 1),
    vec3(1, 0.875f, 1),
    vec3(0, 0.875f, 0),

    vec3(1, 0, 1),
    vec3(0, 0, 0),
    vec3(0, 0.875f, 0),
    vec3(1, 0.875f, 1),

    vec3(0, 0, 1),
    vec3(1, 0, 0),
    vec3(1, 0.875f, 0),
    vec3(0, 0.875f, 1),

    vec3(1, 0, 0),
    vec3(0, 0, 1),
    vec3(0, 0.875f, 1),
    vec3(1, 0.875f, 0)
};

ivec3 ao_offset[6][4] =
{
    { ivec3(0, 0, -1), ivec3(0, -1, 0), ivec3(0, 0, 1), ivec3(0, 1, 0) },
    { ivec3(0, 0, 1), ivec3(0, -1, 0), ivec3(0, 0, -1), ivec3(0, 1, 0) },
    { ivec3(-1, 0, 0), ivec3(0, 0, -1), ivec3(1, 0, 0), ivec3(0, 0, 1) },
    { ivec3(-1, 0, 0), ivec3(0, 0, 1), ivec3(1, 0, 0), ivec3(0, 0, -1) },
    { ivec3(1, 0, 0), ivec3(0, -1, 0), ivec3(-1, 0, 0), ivec3(0, 1, 0) },
    { ivec3(-1, 0, 0), ivec3(0, -1, 0), ivec3(1, 0, 0), ivec3(0, 1, 0) }
};

ivec3 face_normal[] =
{
    ivec3(1, 0, 0),
    ivec3(-1, 0, 0),
    ivec3(0, 1, 0),
    ivec3(0, -1, 0),
    ivec3(0, 0, 1),
    ivec3(0, 0, -1)
};

vec2 uv_offsets[4] =
{
    vec2(0.0f, 1 / 32.0f),
    vec2(1 / 32.0f, 1 / 32.0f),
    vec2(1 / 32.0f, 0.0f),
    vec2(0.0f, 0.0f)
};

Block* blocks[BLOCK_COUNT];
void build_cross(Block* block, Chunk* chunk, const ivec3& pos,
                 std::vector<Vertex>& vertices, std::vector<int>& indices, int& index);

/*bool add_block_light(Chunk* chunk, ivec3& pos, int& sky_sum, int& block_sum, int& n)
{
    if (pos.y < 0 || pos.y >= CHUNK_HEIGHT)
        return false;

    chunk = get_neighbor_chunk(chunk, pos);

    if (chunk == NULL)
        return false;

    if (chunk->get(pos).id != BlockID::AIR && chunk->get(pos).id != BlockID::GLASS)
        return false;

    sky_sum += chunk->get_sky_light(pos);
    block_sum += chunk->get_block_light(pos);
    n++;

    return true;
}

int get_vertex_light(Chunk* chunk, const ivec3& pos, int face, int vertex)
{
    ivec3 normal = pos + face_normal[face];
    ivec3 side1 = pos + face_normal[face] + ao_offset[face][vertex];
    ivec3 side2 = pos + face_normal[face] + ao_offset[face][(vertex + 1) & 3];
    ivec3 corner = pos + face_normal[face] + ao_offset[face][vertex] + ao_offset[face][(vertex + 1) & 3];

    int sky_sum = 0;
    int block_sum = 0;
    int n = 0;

    add_block_light(chunk, normal, sky_sum, block_sum, n);

    bool s1 = add_block_light(chunk, side1, sky_sum, block_sum, n);
    bool s2 = add_block_light(chunk, side2, sky_sum, block_sum, n);

    if (s1 || s2)
        add_block_light(chunk, corner, sky_sum, block_sum, n);

    int sky = sky_sum / (float)n;
    int block = block_sum / (float)n;

    return (sky & 0x0f) | ((block & 0x0f) << 4);
}*/

bool is_block(Chunk* chunk, ivec3& pos)
{
    if (pos.y < 0 || pos.y >= CHUNK_HEIGHT)
        return false;

    chunk = get_neighbor_chunk(chunk, pos);

    if (chunk == NULL)
        return false;

    int id = chunk->get(pos)->id;

    Block* b = get_block_by_id(id);

    if (id == BlockID::LEAVES)
        return true;

    return b->opacity > 0;
}

int get_ao(Chunk* chunk, const ivec3& pos, int face, int vertex)
{
    ivec3 pos1 = pos + face_normal[face] + ao_offset[face][vertex];
    ivec3 pos2 = pos + face_normal[face] + ao_offset[face][(vertex + 1) & 3];
    ivec3 pos3 = pos + face_normal[face] + ao_offset[face][vertex] + ao_offset[face][(vertex + 1) & 3];

    int side1 = is_block(chunk, pos1);
    int side2 = is_block(chunk, pos2);
    int corner = is_block(chunk, pos3);

    if (side1 && side2)
        return 3;

    return side1 + side2 + corner;
}

bool face_is_visible(Chunk* chunk, const ivec3& pos, int face, int& light)
{
    ivec3 target_pos = pos + face_normal[face];

    if (target_pos.y >= CHUNK_HEIGHT)
    {
        light = 15;
        return true;
    }

    if (target_pos.y < 0)
        return false;

    Chunk* _chunk = get_neighbor_chunk(chunk, target_pos);

    if (_chunk == NULL) {
        // Expose the frontier until a neighbor is loaded; rebuild on arrival.
        light = chunk->get_light(pos);
        return true;
    }

    light = _chunk->get_light(target_pos);

    BlockState* bs = _chunk->get(target_pos);
    Block* b = get_block_by_id(bs->id);

    if (chunk->get(pos)->id == GLASS && bs->id == GLASS)
        return false;

    return b->opacity == 0;
}

void build_cube(Block* block, Chunk* chunk, const ivec3& pos, std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
{
    for (int f = 0; f < 6; f++)
    {
        int light;

        if (face_is_visible(chunk, pos, f, light))
        {
            vec2 tex_coord = block->get_texture(chunk, pos, f);
            vec3 vert_pos = (vec3)pos + chunk_pos_to_world(chunk->pos);

            for (int v = 0; v < 4; v++)
            {
                int ao = get_ao(chunk, pos, f, v);

                Vertex vert = Vertex(vert_pos + (vec3)vertex_offset[f][v], f, light, ao, tex_coord, v);

                vertices.push_back(vert);
            }

            indices.push_back(index + 0);
            indices.push_back(index + 1);
            indices.push_back(index + 2);

            indices.push_back(index + 0);
            indices.push_back(index + 2);
            indices.push_back(index + 3);

            index += 4;
        }
    }
}

// A small axis-aligned box helper for catalog blocks that are thinner than a
// full voxel (beds). It keeps the normal atlas UVs while retaining the same
// culling and ambient-occlusion rules as ordinary cubes.
void build_box(Block* block, Chunk* chunk, const ivec3& pos,
               const vec3& minimum, const vec3& maximum,
               std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
{
    for (int f = 0; f < 6; ++f) {
        int light;
        if (!face_is_visible(chunk, pos, f, light)) continue;
        const vec2 tex_coord = block->get_texture(chunk, pos, f);
        const vec3 origin = (vec3)pos + chunk_pos_to_world(chunk->pos);
        for (int v = 0; v < 4; ++v) {
            const vec3 corner = vec3(vertex_offset[f][v]);
            const vec3 p = minimum + corner * (maximum - minimum);
            Vertex vert(origin + p, f, light, get_ao(chunk, pos, f, v), tex_coord, v);
            vertices.push_back(vert);
        }
        for (int v : {0, 1, 2, 0, 2, 3}) indices.push_back(index + v);
        index += 4;
    }
}

void build_banner(Block* block, Chunk* chunk, const ivec3& pos,
                  std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
{
    // A banner is a narrow hanging cloth with a slim central pole. The
    // orientation is stored by placement so side-mounted banners face away
    // from the supporting block instead of becoming a full voxel cross.
    const int orientation=chunk->get(pos)->data1&7;
    const bool along_x=orientation==0 || orientation==2 || orientation==4;
    if (along_x) {
        build_box(block,chunk,pos,vec3(.46f,0,.47f),vec3(.54f,1,.53f),vertices,indices,index);
        build_box(block,chunk,pos,vec3(.10f,.25f,.47f),vec3(.90f,.86f,.53f),vertices,indices,index);
    } else {
        build_box(block,chunk,pos,vec3(.47f,0,.46f),vec3(.53f,1,.54f),vertices,indices,index);
        build_box(block,chunk,pos,vec3(.47f,.25f,.10f),vec3(.53f,.86f,.90f),vertices,indices,index);
    }
}

void build_slab(Block* block, Chunk* chunk, const ivec3& pos,
                std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
{
    build_box(block,chunk,pos,vec3(0,0,0),vec3(1,.5f,1),vertices,indices,index);
}

void build_stairs(Block* block, Chunk* chunk, const ivec3& pos,
                  std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
{
    // Default state is the vanilla east-facing lower stair. Placement states
    // are preserved and can be rotated by the renderer when directional
    // placement is enabled.
    build_box(block,chunk,pos,vec3(0,0,0),vec3(1,.5f,1),vertices,indices,index);
    build_box(block,chunk,pos,vec3(.5f,.5f,0),vec3(1,1,1),vertices,indices,index);
}

void build_pane(Block* block, Chunk* chunk, const ivec3& pos,
                std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
{
    build_box(block,chunk,pos,vec3(.4375f,0,.4375f),vec3(.5625f,1,.5625f),vertices,indices,index);
    build_box(block,chunk,pos,vec3(0,.0f,.4375f),vec3(1,1,.5625f),vertices,indices,index);
}

void build_plant(Block* block, Chunk* chunk, const ivec3& pos,
                 std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
{
    build_cross(block,chunk,pos,vertices,indices,index);
}

void build_oriented_cube(Block* block, Chunk* chunk, const ivec3& pos, std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
{
    int orientation = chunk->get(pos)->data1 & 3;

    for (int f = 0; f < 6; f++)
    {
        int light;

        int r = 0;

        if (orientation == 0) // x
        {
            if (f != RIGHT_FACE && f != LEFT_FACE)
                r = 1;
        }
        else if (orientation == 2) // z
        {
            if (f == RIGHT_FACE || f == LEFT_FACE)
                r = 1;
        }

        if (face_is_visible(chunk, pos, f, light))
        {
            vec2 tex_coord = block->get_texture(chunk, pos, f);
            vec3 vert_pos = (vec3)pos + chunk_pos_to_world(chunk->pos);

            for (int v = 0; v < 4; v++)
            {
                int ao = get_ao(chunk, pos, f, (v + r) & 3);

                Vertex vert = Vertex(vert_pos + (vec3)vertex_offset[f][(v + r) & 3], f, light, ao, tex_coord, v);

                vertices.push_back(vert);
            }

            indices.push_back(index);
            indices.push_back(index + 1);
            indices.push_back(index + 2);

            indices.push_back(index + 0);
            indices.push_back(index + 2);
            indices.push_back(index + 3);

            index += 4;
        }
    }
}

void build_cross(Block* block, Chunk* chunk, const ivec3& pos, std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
{
    vec2 tex_coord = block->get_texture(chunk, pos, 0);
    int light = chunk->get_light(pos);
    vec3 vert_pos = (vec3)pos + chunk_pos_to_world(chunk->pos);

    for (int i = 0; i < 16; i++)
    {
        Vertex vert = Vertex(vert_pos + (vec3)cross_vertex_offset[i], TOP_FACE, light, 0, tex_coord, i % 4);

        vertices.push_back(vert);
    }

    for (int i = 0; i < 4; i++)
    {
        indices.push_back(index);
        indices.push_back(index + 1);
        indices.push_back(index + 2);

        indices.push_back(index + 0);
        indices.push_back(index + 2);
        indices.push_back(index + 3);

        index += 4;
    }
}

void build_torch(Block* block, Chunk* chunk, const ivec3& pos, std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
{
    int orientation = chunk->get(pos)->data1 & 7;

    for (int f = 0; f < 6; f++)
    {
        if (f == BOTTOM_FACE)
            continue;

        int light = chunk->get_light(pos);

        vec2 tex_coord = block->get_texture(chunk, pos, f);
        vec3 vert_pos = (vec3)pos + chunk_pos_to_world(chunk->pos);

        for (int v = 0; v < 4; v++)
        {
            vec3 offset = torch_corner(vec3(vertex_offset[f][v]));

            if (orientation != 0)
            {
                offset -= vec3(0.5f);

                vec3 normal;

                if (orientation == 1)
                {
                    offset = rotateX(offset, radians(24.0f));
                    normal = vec3(0, 0, 1);
                }
                else if (orientation == 2)
                {
                    offset = rotateX(offset, radians(-24.0f));
                    normal = vec3(0, 0, -1);
                }
                else if (orientation == 3)
                {
                    offset = rotateZ(offset, radians(-24.0f));
                    normal = vec3(1, 0, 0);
                }
                else
                {
                    offset = rotateZ(offset, radians(24.0f));
                    normal = vec3(-1, 0, 0);
                }

                offset += vec3(0.5f);
                offset += vec3(0, 0.15f, 0);
                offset -= normal * 0.295f;
            }

            Vertex vert = Vertex(vert_pos + offset, TOP_FACE, light, 0, tex_coord, v);

            vertices.push_back(vert);
        }

        indices.push_back(index);
        indices.push_back(index + 1);
        indices.push_back(index + 2);

        indices.push_back(index + 0);
        indices.push_back(index + 2);
        indices.push_back(index + 3);

        index += 4;
    }
}

struct Air : public Block
{
    Air()
    {
        opacity = 0;
        light = 0;
        icon = Sprite(vec2(0, 0), vec2(0, 0), vec2(0, 0), &texture_atlas);
    }

    vec2 get_texture(Chunk* chunk, const ivec3& pos, int face)
    {
        return vec2(0);
    }

    void build(Chunk* chunk, const ivec3& pos, std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
    {
        return;
    }
};

struct Stone : public Block
{
    Stone()
    {
        opacity = 15;
        light = 0;
        icon = Sprite(vec2(0, 0), vec2(16, 16), vec2(0, 16), &texture_atlas);
    }

    vec2 get_texture(Chunk* chunk, const ivec3& pos, int face)
    {
        return vec2(0, 0);
    }

    void build(Chunk* chunk, const ivec3& pos, std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
    {
        return build_cube(this, chunk, pos, vertices, indices, index);
    }
};

struct Dirt : public Block
{
    Dirt()
    {
        opacity = 15;
        light = 0;
        icon = Sprite(vec2(0, 0), vec2(16, 16), vec2(16, 16), &texture_atlas);
    }

    vec2 get_texture(Chunk* chunk, const ivec3& pos, int face)
    {
        return vec2(1, 0);
    }

    void build(Chunk* chunk, const ivec3& pos, std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
    {
        return build_cube(this, chunk, pos, vertices, indices, index);
    }
};

struct Grass_Block : public Block
{
    Grass_Block()
    {
        opacity = 15;
        light = 0;
        icon = Sprite(vec2(0, 0), vec2(16, 16), vec2(32, 16), &texture_atlas);
    }

    vec2 get_texture(Chunk* chunk, const ivec3& pos, int face)
    {
        if (face == TOP_FACE)
            return vec2(3, 0);

        if (face == BOTTOM_FACE)
            return vec2(1, 0);

        return vec2(2, 0);
    }

    void build(Chunk* chunk, const ivec3& pos, std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
    {
        return build_cube(this, chunk, pos, vertices, indices, index);
    }
};

struct Log : public Block
{
    Log()
    {
        opacity = 15;
        light = 0;
        icon = Sprite(vec2(0, 0), vec2(16, 16), vec2(6 * 16, 16), &texture_atlas);
    }

    vec2 get_texture(Chunk* chunk, const ivec3& pos, int face)
    {
        int orientation = chunk?2 * (chunk->get(pos)->data1 & 3):TOP_FACE;

        if (face == orientation || face == (orientation + 1))
            return vec2(7, 0);
        else
            return vec2(6, 0);
    }

    void build(Chunk* chunk, const ivec3& pos, std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
    {
        return build_oriented_cube(this, chunk, pos, vertices, indices, index);
    }
};

struct Leaves : public Block
{
    Leaves()
    {
        opacity = 0;
        light = 0;
        icon = Sprite(vec2(0, 0), vec2(16, 16), vec2(8 * 16, 16), &texture_atlas);
    }

    vec2 get_texture(Chunk* chunk, const ivec3& pos, int face)
    {
        return vec2(8, 0);
    }

    void build(Chunk* chunk, const ivec3& pos, std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
    {
        return build_cube(this, chunk, pos, vertices, indices, index);
    }
};

struct Glass : public Block
{
    Glass()
    {
        opacity = 0;
        light = 0;
        icon = Sprite(vec2(0, 0), vec2(16, 16), vec2(10 * 16, 16), &texture_atlas);
    }

    vec2 get_texture(Chunk* chunk, const ivec3& pos, int face)
    {
        return vec2(10, 0);
    }

    void build(Chunk* chunk, const ivec3& pos, std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
    {
        return build_cube(this, chunk, pos, vertices, indices, index);
    }
};

struct Grass : public Block
{
    Grass()
    {
        opacity = 0;
        light = 0;
        icon = Sprite(vec2(0, 0), vec2(16, 16), vec2(0, 2 * 16), &texture_atlas);
    }

    vec2 get_texture(Chunk* chunk, const ivec3& pos, int face)
    {
        return vec2(0, 1);
    }

    void build(Chunk* chunk, const ivec3& pos, std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
    {
        build_cross(this, chunk, pos, vertices, indices, index);
    }
};

struct Torch : public Block
{
    Torch()
    {
        opacity = 0;
        light = 14;
        icon = Sprite(vec2(0, 0), vec2(16, 16), vec2(3 * 16, 2 * 16), &texture_atlas);
    }

    vec2 get_texture(Chunk* chunk, const ivec3& pos, int face)
    {
        if (face == TOP_FACE)
            return vec2(2, 1);
        else
            return vec2(1, 1);
    }

    void build(Chunk* chunk, const ivec3& pos, std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
    {
        return build_torch(this, chunk, pos, vertices, indices, index);
    }
};

struct Planks : public Block
{
    Planks()
    {
        opacity = 15;
        light = 0;
        icon = Sprite(vec2(0, 0), vec2(16, 16), vec2(6 * 16, 2 * 16), &texture_atlas);
    }

    vec2 get_texture(Chunk* chunk, const ivec3& pos, int face)
    {
        return vec2(6, 1);
    }

    void build(Chunk* chunk, const ivec3& pos, std::vector<Vertex>& vertices, std::vector<int>& indices, int& index)
    {
        return build_cube(this, chunk, pos, vertices, indices, index);
    }
};

struct Bedrock : public Stone {
    Bedrock() { icon.destroy(); icon = Sprite(vec2(0),vec2(16),vec2(4*16,2*16),&texture_atlas); }
    vec2 get_texture(Chunk*,const ivec3&,int) override { return vec2(4,1); }
};

struct WaterBlock : public Block {
    WaterBlock(){opacity=0;light=0;icon=Sprite(vec2(0),vec2(16),vec2(0,96),&texture_atlas);}
    vec2 get_texture(Chunk*,const ivec3&,int)override{return vec2(0,5);}
    void build(Chunk* chunk,const ivec3& p,std::vector<Vertex>& vertices,std::vector<int>& indices,int& index)override{
        auto at=[&](ivec3 q)->int{auto* c=get_neighbor_chunk(chunk,q);if(!c)return Water::Unloaded;auto b=*c->get(q);return b.id==WATER?(b.data1&15):(get_block_by_id(b.id)->opacity>0?Water::Solid:Water::Air);};
        auto corner=[&](int cx,int cz){double sum=0,weight=0;
            for(int dx=-1;dx<=0;++dx)for(int dz=-1;dz<=0;++dz){ivec3 q=p+ivec3(cx+dx,0,cz+dz);if(Water::liquid(at(q+ivec3(0,1,0))))return 1.;int l=at(q);
                if(Water::liquid(l)){double w=(l==0 || l>=8)?11:1;sum+=Water::height(l)*w;weight+=w;}
                else if(l!=Water::Solid){++weight;}}
            return weight?sum/weight:Water::height(chunk->get(p)->data1&15);
        };
        int raw=chunk->get(p)->data1&15;bool flowing=raw!=0;
        double heights[2][2]={{corner(0,0),corner(0,1)},{corner(1,0),corner(1,1)}};
        for(int f=0;f<6;++f){int neighbor=at(p+face_normal[f]);if(Water::liquid(neighbor) || neighbor==Water::Solid)continue;
            for(int v=0;v<4;++v){auto o=vertex_offset[f][v];vec3 offset(o);if(o.y)offset.y=float(heights[o.x][o.z]);
                Vertex vert(vec3(p)+chunk_pos_to_world(chunk->pos)+offset,f,chunk->get_light(p),0,ivec2(0,(flowing || f!=2)?6:5),v);
                vert.data|=1<<25;vertices.push_back(vert);}
            for(int v:{0,1,2,0,2,3})indices.push_back(index+v);index+=4;
        }
    }
};

struct CatalogBlock : public Block {
    int id;
    ivec2 side,top,bottom;
    CatalogBlock(int block_id,ivec2 s,ivec2 t,ivec2 b,int emission):id(block_id),side(s),top(t),bottom(b){
        opacity=block_id==BANNER_BLOCK?0:15;light=char(emission);icon=Sprite(vec2(0),vec2(16),vec2(s.x*16,(s.y+1)*16),&texture_atlas);
    }
    vec2 get_texture(Chunk*,const ivec3&,int face) override {return face==TOP_FACE?top:(face==BOTTOM_FACE?bottom:side);}
    void build(Chunk* c,const ivec3& p,std::vector<Vertex>& v,std::vector<int>& i,int& n) override {
        if(c->get(p)->id==FIRE){auto start=v.size();build_cross(this,c,p,v,i,n);for(size_t a=start;a<v.size();++a)v[a].data|=1<<26;}
        else if(id==BED_BLOCK)build_box(this,c,p,vec3(0,0,0),vec3(1,.5625f,1),v,i,n);
        else if(id==BANNER_BLOCK)build_banner(this,c,p,v,i,n);
        else if(id>=OAK_STAIRS && id<=PRISMARINE_STAIRS)build_stairs(this,c,p,v,i,n);
        else if(id==OAK_SLAB || id==STONE_SLAB || id==SANDSTONE_SLAB)build_slab(this,c,p,v,i,n);
        else if(id==GLASS_PANE || id==IRON_BARS)build_pane(this,c,p,v,i,n);
        else if(id==SAPLING || id==DEADBUSH || id==FERN || id==WHEAT_CROP || id==CARROT_CROP || id==POTATO_CROP || id==BEETROOT_CROP || id==NETHER_WART_CROP || id==VINE || id==WATERLILY || id==CHORUS_FLOWER)build_plant(this,c,p,v,i,n);
        else build_cube(this,c,p,v,i,n);
    }
};

void blocks_init()
{
    blocks[AIR] = new Air();
    blocks[WATER] = new WaterBlock();
    blocks[STONE] = new Stone();
    blocks[DIRT] = new Dirt();
    blocks[GRASS_BLOCK] = new Grass_Block();
    blocks[LOG] = new Log();
    blocks[LEAVES] = new Leaves();
    blocks[GLASS] = new Glass();
    blocks[GRASS] = new Grass();
    blocks[TORCH] = new Torch();
    blocks[PLANKS] = new Planks();
    blocks[BEDROCK] = new Bedrock();
#define BASE_BLOCK(id,label,sx,sy,sname,tx,ty,tname,bx,by,bname,emission) blocks[id]=new CatalogBlock(id,ivec2(sx,sy),ivec2(tx,ty),ivec2(bx,by),emission);
#define EXTRA_BLOCK BASE_BLOCK
#include <extra_blocks.def>
#undef EXTRA_BLOCK
#undef BASE_BLOCK
}

Block* get_block_by_id(int id)
{
    return id>=0 && id<BLOCK_COUNT && blocks[id]?blocks[id]:blocks[AIR];
}
const char* block_name(int id){
    switch(id){
    case AIR:return "Air";case STONE:return "Stone";case DIRT:return "Dirt";case GRASS_BLOCK:return "Grass Block";
    case LOG:return "Oak Log";case LEAVES:return "Oak Leaves";case GLASS:return "Glass";case GRASS:return "Grass";
    case TORCH:return "Torch";case PLANKS:return "Oak Planks";case BEDROCK:return "Bedrock";
#define BASE_BLOCK(id,label,...) case id:return label;
#define EXTRA_BLOCK BASE_BLOCK
#include <extra_blocks.def>
#undef EXTRA_BLOCK
#undef BASE_BLOCK
    default:return "";
    }
}
