#include <item_model.h>
#include <blocks.h>
#include <items.h>
#include <gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>
namespace {
Shader shader;GLuint vao=0,vbo=0;
struct ModelVertex {vec3 p;vec2 uv;float light;};
struct Range {int first=0,count=0;};
Range block_mesh[BLOCK_COUNT],sprites[Items::ATLAS_CAPACITY];
Range damage[10];
std::vector<ModelVertex> vertices;
void face(const vec3* p,vec2 tile,vec2 extent,float light){
    vec2 uv[4]={{tile.x,tile.y+extent.y},{tile.x+extent.x,tile.y+extent.y},{tile.x+extent.x,tile.y},{tile.x,tile.y}};
    for(int n:{0,1,2,0,2,3})vertices.push_back({p[n],uv[n],light});
}
}
bool ItemModel::cube(int id){return Items::block(id) && id!=TORCH && id!=GRASS;}
void ItemModel::init(){
    shader=Shader("shaders/avatar_vert.glsl","shaders/item_frag.glsl");
    // Outward CCW faces; model origin is the centre, as in vanilla baked models.
    const vec3 corners[6][4]={
        {{.5,-.5,.5},{.5,-.5,-.5},{.5,.5,-.5},{.5,.5,.5}},
        {{-.5,-.5,-.5},{-.5,-.5,.5},{-.5,.5,.5},{-.5,.5,-.5}},
        {{-.5,.5,.5},{.5,.5,.5},{.5,.5,-.5},{-.5,.5,-.5}},
        {{-.5,-.5,-.5},{.5,-.5,-.5},{.5,-.5,.5},{-.5,-.5,.5}},
        {{-.5,-.5,.5},{.5,-.5,.5},{.5,.5,.5},{-.5,.5,.5}},
        {{.5,-.5,-.5},{-.5,-.5,-.5},{-.5,.5,-.5},{.5,.5,-.5}}};
    const float shades[]={.78f,.78f,1.f,.5f,.9f,.65f};
    const vec3 flat[4]={{-.5,-.5,0},{.5,-.5,0},{.5,.5,0},{-.5,.5,0}};
    for(int id=1;id<BLOCK_COUNT;++id){
        auto& range=block_mesh[id];range.first=int(vertices.size());
        if(cube(id))for(int f=0;f<6;++f)face(corners[f],get_block_by_id(id)->get_texture(nullptr,ivec3(0),f)/32.f,vec2(1.f/32),shades[f]);
        else{auto& icon=get_block_by_id(id)->icon;face(flat,(icon.uv-vec2(0,16))/512.f,vec2(1.f/32),1);}
        range.count=int(vertices.size())-range.first;
    }
    for(int n=0;n<10;++n){damage[n]={int(vertices.size()),36};for(int f=0;f<6;++f)face(corners[f],vec2(n,10)/32.f,vec2(1.f/32),1);}
    for(int n=0;n<Items::ATLAS_CAPACITY;++n){sprites[n]={int(vertices.size()),6};face(flat,vec2(n%Items::ATLAS_COLUMNS,n/Items::ATLAS_COLUMNS)/float(Items::ATLAS_COLUMNS),vec2(1.f/Items::ATLAS_COLUMNS),1);}
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(ModelVertex),vertices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(ModelVertex),(void*)offsetof(ModelVertex,p));glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(ModelVertex),(void*)offsetof(ModelVertex,uv));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,sizeof(ModelVertex),(void*)offsetof(ModelVertex,light));glEnableVertexAttribArray(2);
    vertices.clear();vertices.shrink_to_fit();
}
void ItemModel::draw(int id,Texture& items,int frame,const mat4& matrix){
    bool block=Items::block(id);if(!block && !Items::valid(id))return;
    GLboolean cull=glIsEnabled(GL_CULL_FACE);
    if(cube(id))glEnable(GL_CULL_FACE);else glDisable(GL_CULL_FACE);
    Texture& tex=block?texture_atlas:items;tex.bind();shader.bind();shader.upload_texture("tex",tex);shader.upload_mat4("cam_mat",matrix);
    Range r=block?block_mesh[id]:sprites[std::clamp(frame,0,Items::ATLAS_CAPACITY-1)];
    glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,r.first,r.count);
    if(cull)glEnable(GL_CULL_FACE);else glDisable(GL_CULL_FACE);
}
void ItemModel::gui(int id,float x,float y,float size){
    mat4 m=ortho(0.f,float(win_size.x),float(win_size.y),0.f,-1000.f,1000.f);
    m=translate(m,vec3(x+size/2,y+size/2,0));m=scale(m,vec3(size,-size,size));
    if(cube(id)){m=rotate(m,radians(30.f),vec3(1,0,0));m=rotate(m,radians(225.f),vec3(0,1,0));m=scale(m,vec3(.625f));}
    else {m=rotate(m,radians(0.f),vec3(0,1,0));m=scale(m,vec3(.82f));}
    draw(id,texture_atlas,0,m);
}
void ItemModel::destroy(){glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);shader.destroy();}
void ItemModel::cracks(int stage,const mat4& matrix){
    glDisable(GL_CULL_FACE);glDepthMask(GL_FALSE);
    shader.bind();texture_atlas.bind();shader.upload_texture("tex",texture_atlas);shader.upload_mat4("cam_mat",matrix);
    Range r=damage[std::clamp(stage,0,9)];glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,r.first,r.count);
    glDepthMask(GL_TRUE);glEnable(GL_CULL_FACE);
}
