#include <vector>
#include <avatar.h>
#include <gtc/matrix_transform.hpp>
namespace {
Shader shader;
GLuint vao,vbo;
struct Vertex { vec3 position,color; };
std::vector<Vertex> vertices;
void box(mat4 transform,vec3 center,vec3 size,vec3 color) {
    vec3 p[8];
    for(int i=0;i<8;++i) p[i]=vec3(transform*vec4(center+size*(vec3((i&1)?1:0,(i&2)?1:0,(i&4)?1:0)-.5f),1));
    int faces[6][4]={{0,1,3,2},{4,6,7,5},{0,4,5,1},{2,3,7,6},{0,2,6,4},{1,5,7,3}};
    for(int f=0;f<6;++f) for(int n:{0,1,2,0,2,3}) vertices.push_back({p[faces[f][n]],color*(.7f+.05f*f)});
}
}
void Avatar::init() {
    shader=Shader("shaders/avatar_vert.glsl","shaders/avatar_frag.glsl");
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)0);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)sizeof(vec3));glEnableVertexAttribArray(1);
}
void Avatar::draw(dvec3 position,vec3 forward,double phase,const mat4& camera) {
    vertices.clear();
    mat4 body=translate(mat4(1),vec3(position));
    body=rotate(body,atan2(forward.x,forward.z),vec3(0,1,0));
    vec3 skin(.72,.48,.31),shirt(.05,.65,.67),pants(.2,.2,.62),hair(.19,.10,.06);
    box(body,vec3(0,1.05,0),vec3(.5,.6,.25),shirt);
    mat4 head=translate(body,vec3(0,1.6,0));
    head=rotate(head,-asin(clamp(forward.y,-1.f,1.f)),vec3(1,0,0));
    box(head,vec3(0),vec3(.45,.45,.45),skin);
    box(head,vec3(0,.17,-.01),vec3(.46,.13,.46),hair);
    for(float side:{-1.f,1.f}) {
        box(head,vec3(side*.11,.01,.229),vec3(.09,.065,.012),vec3(.85));
        box(head,vec3(side*.10,.01,.237),vec3(.04,.06,.01),vec3(.15,.2,.7));
        mat4 arm=rotate(translate(body,vec3(side*.375,1.32,0)),float(sin(phase)*.65*side),vec3(1,0,0));
        box(arm,vec3(0,-.13,0),vec3(.23,.25,.25),shirt);
        box(arm,vec3(0,-.42,0),vec3(.23,.33,.25),skin);
        mat4 leg=rotate(translate(body,vec3(side*.13,.75,0)),float(-sin(phase)*.65*side),vec3(1,0,0));
        box(leg,vec3(0,-.30,0),vec3(.24,.6,.25),pants);
        box(leg,vec3(0,-.68,0),vec3(.24,.15,.28),vec3(.17));
    }
    shader.bind();shader.upload_mat4("cam_mat",camera);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
    glDisable(GL_CULL_FACE);glDrawArrays(GL_TRIANGLES,0,(GLsizei)vertices.size());glEnable(GL_CULL_FACE);
}
void Avatar::destroy() {glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);shader.destroy();}
