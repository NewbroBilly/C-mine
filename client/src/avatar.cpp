#include <vector>
#include <avatar.h>
#include <gtc/matrix_transform.hpp>
namespace {
Shader shader;
Texture skin;
GLuint vao,vbo;
struct Vertex { vec3 position; vec2 uv; float shade; };
std::vector<Vertex> vertices;
// ModelBox's six rectangles on a 64x64 skin; coordinates remain in model pixels.
void box(mat4 transform,vec3 minimum,ivec3 size,ivec2 uv,float inflate=0) {
    vec3 p[8];
    for(int i=0;i<8;++i)p[i]=vec3(transform*vec4(minimum-vec3(inflate)+(vec3(size)+2.f*inflate)*vec3(i&1,(i>>1)&1,(i>>2)&1),1));
    int faces[6][4]={{5,1,3,7},{0,4,6,2},{5,4,0,1},{3,2,6,7},{1,0,2,3},{4,5,7,6}};
    int w=size.x,h=size.y,d=size.z,u=uv.x,v=uv.y;
    vec4 rects[6]={{u+d+w,v+d,u+d+w+d,v+d+h},{u,v+d,u+d,v+d+h},
        {u+d,v,u+d+w,v+d},{u+d+w,v+d,u+d+w+w,v},
        {u+d,v+d,u+d+w,v+d+h},{u+d+w+d,v+d,u+d+w+d+w,v+d+h}};
    float shades[]={.78f,.78f,1.f,.65f,.9f,.8f};
    for(int f=0;f<6;++f){vec4 r=rects[f];vec2 tex[4]={{r.z,r.y},{r.x,r.y},{r.x,r.w},{r.z,r.w}};
        for(int n:{0,1,2,0,2,3})vertices.push_back({p[faces[f][n]],tex[n]/64.f,shades[f]});}
}
}
void Avatar::init() {
    shader=Shader("shaders/avatar_vert.glsl","shaders/avatar_frag.glsl");
    std::string path="resource_pack/player.png";
#ifdef __SWITCH__
    if(std::filesystem::exists("sdmc:/switch/C-mine/resource_pack/player.png"))path="sdmc:/switch/C-mine/resource_pack/player.png";
#endif
    if(!std::filesystem::exists(path))path="textures/player.png";
    skin=Texture(path.c_str(),6);
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,position));glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,uv));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,shade));glEnableVertexAttribArray(2);
}
void Avatar::draw(dvec3 position,vec3 forward,double phase,const mat4& camera,float amount,float attack) {
    vertices.clear();
    mat4 body=translate(mat4(1),vec3(position));
    body=rotate(body,atan2(forward.x,forward.z),vec3(0,1,0));
    body=scale(body,vec3(.9375f));body=translate(body,vec3(0,1.5,0));body=scale(body,vec3(1,-1,-1)/16.f);
    mat4 head=rotate(body,-asin(clamp(forward.y,-1.f,1.f)),vec3(1,0,0));
    box(head,vec3(-4,-8,-4),ivec3(8),ivec2(0,0));
    box(head,vec3(-4,-8,-4),ivec3(8),ivec2(32,0),.5f);
    box(body,vec3(-4,0,-2),ivec3(8,12,4),ivec2(16,16));
    box(body,vec3(-4,0,-2),ivec3(8,12,4),ivec2(16,32),.25f);
    float swing=cos(float(phase)*.6662f)*clamp(amount,0.f,1.f);
    mat4 right_arm=rotate(translate(body,vec3(-5,2,0)),-swing,vec3(1,0,0));
    right_arm=rotate(right_arm,-sin(sqrt(attack)*3.14159265f)*1.2f,vec3(1,0,0));
    mat4 left_arm=rotate(translate(body,vec3(5,2,0)),swing,vec3(1,0,0));
    box(right_arm,vec3(-3,-2,-2),ivec3(4,12,4),ivec2(40,16));
    box(right_arm,vec3(-3,-2,-2),ivec3(4,12,4),ivec2(40,32),.25f);
    box(left_arm,vec3(-1,-2,-2),ivec3(4,12,4),ivec2(32,48));
    box(left_arm,vec3(-1,-2,-2),ivec3(4,12,4),ivec2(48,48),.25f);
    mat4 right_leg=rotate(translate(body,vec3(-1.9f,12,0)),swing*1.4f,vec3(1,0,0));
    mat4 left_leg=rotate(translate(body,vec3(1.9f,12,0)),-swing*1.4f,vec3(1,0,0));
    box(right_leg,vec3(-2,0,-2),ivec3(4,12,4),ivec2(0,16));
    box(right_leg,vec3(-2,0,-2),ivec3(4,12,4),ivec2(0,32),.25f);
    box(left_leg,vec3(-2,0,-2),ivec3(4,12,4),ivec2(16,48));
    box(left_leg,vec3(-2,0,-2),ivec3(4,12,4),ivec2(0,48),.25f);
    skin.bind();shader.bind();shader.upload_texture("skin",skin);shader.upload_mat4("cam_mat",camera);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
    glDisable(GL_CULL_FACE);glDrawArrays(GL_TRIANGLES,0,(GLsizei)vertices.size());glEnable(GL_CULL_FACE);
}
void Avatar::hand(const mat4& matrix){
    vertices.clear();
    mat4 m=translate(scale(mat4(1),vec3(1.f/16)),vec3(-5,2,0));
    box(m,vec3(-3,-2,-2),ivec3(4,12,4),ivec2(40,16));
    box(m,vec3(-3,-2,-2),ivec3(4,12,4),ivec2(40,32),.25f);
    skin.bind();shader.bind();shader.upload_texture("skin",skin);shader.upload_mat4("cam_mat",matrix);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
    glDisable(GL_CULL_FACE);glDrawArrays(GL_TRIANGLES,0,(GLsizei)vertices.size());glEnable(GL_CULL_FACE);
}
void Avatar::destroy() {skin.destroy();glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);shader.destroy();}
