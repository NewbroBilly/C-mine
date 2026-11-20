#include <vector>
#include <avatar.h>
#include <gtc/matrix_transform.hpp>
#include <model_animation.h>
namespace {
Shader shader;
Texture skin,mob_skins[Mobs::Count],fur,armor_skins[5][2];
vec2 texture_size(64);
GLuint vao,vbo;
struct Vertex { vec3 position; vec2 uv; float shade; };
std::vector<Vertex> vertices;
// ModelRenderer applies its rotations Z, Y, X, after its pivot translation.
mat4 part(const mat4& root,const ModelAnimation::Part& p) {
    mat4 m=translate(root,vec3(p.px,p.py,p.pz));
    m=rotate(m,p.z,vec3(0,0,1));m=rotate(m,p.y,vec3(0,1,0));return rotate(m,p.x,vec3(1,0,0));
}
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
        for(int n:{0,1,2,0,2,3})vertices.push_back({p[faces[f][n]],tex[n]/texture_size,shades[f]});}
}
void flush(Texture& texture,const mat4& camera,float flash=0){
    texture.bind();shader.bind();shader.upload_texture("skin",texture);shader.upload_mat4("cam_mat",camera);shader.upload_float("flash",flash);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
    glDisable(GL_CULL_FACE);glDrawArrays(GL_TRIANGLES,0,(GLsizei)vertices.size());glEnable(GL_CULL_FACE);vertices.clear();
}
Texture load_skin(const std::string& name){std::string path="resource_pack/"+name+".png";
#ifdef __SWITCH__
    path="sdmc:/switch/C-mine/resource_pack/"+name+".png";
#endif
    if(!std::filesystem::exists(path))path="textures/"+name+".png";
    return Texture(path.c_str(),6);
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
    const char* names[]={"pig","cow","sheep","zombie","creeper","chicken","skeleton","spider","enderman","slime","ghast","blaze","witch","wither","bat","rabbit","wolf","ocelot","horse"};
    for(int i=0;i<Mobs::Count;++i)mob_skins[i]=load_skin(names[i]);fur=load_skin("sheep_fur");
    const char* mats[]={"leather","chainmail","iron","gold","diamond"};
    for(int m=0;m<5;++m)for(int l=0;l<2;++l)armor_skins[m][l]=load_skin(std::string(mats[m])+"_layer_"+std::to_string(l+1));
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,position));glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,uv));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,shade));glEnableVertexAttribArray(2);
}
void Avatar::draw(dvec3 position,vec3 forward,double phase,const mat4& camera,float amount,float attack,const std::array<Items::Stack,4>* armor,float age,bool sneak,bool held,bool blocking,bool bow) {
    vertices.clear();texture_size=vec2(64);
    mat4 body=translate(mat4(1),vec3(position));
    body=rotate(body,atan2(forward.x,forward.z),vec3(0,1,0));
    body=scale(body,vec3(.9375f));body=translate(body,vec3(0,1.5,0));body=scale(body,vec3(1,-1,-1)/16.f);
    if(sneak)body=translate(body,vec3(0,3.2f,0));
    auto pose=ModelAnimation::biped(float(phase),amount,age,-asin(clamp(forward.y,-1.f,1.f)),attack>=1?0:attack,sneak,held,blocking,bow);
    mat4 head=part(body,pose.head),torso=part(body,pose.body);
    box(head,vec3(-4,-8,-4),ivec3(8),ivec2(0,0));
    box(head,vec3(-4,-8,-4),ivec3(8),ivec2(32,0),.5f);
    box(torso,vec3(-4,0,-2),ivec3(8,12,4),ivec2(16,16));
    box(torso,vec3(-4,0,-2),ivec3(8,12,4),ivec2(16,32),.25f);
    mat4 right_arm=part(body,pose.right_arm),left_arm=part(body,pose.left_arm);
    box(right_arm,vec3(-3,-2,-2),ivec3(4,12,4),ivec2(40,16));
    box(right_arm,vec3(-3,-2,-2),ivec3(4,12,4),ivec2(40,32),.25f);
    box(left_arm,vec3(-1,-2,-2),ivec3(4,12,4),ivec2(32,48));
    box(left_arm,vec3(-1,-2,-2),ivec3(4,12,4),ivec2(48,48),.25f);
    mat4 right_leg=part(body,pose.right_leg),left_leg=part(body,pose.left_leg);
    box(right_leg,vec3(-2,0,-2),ivec3(4,12,4),ivec2(0,16));
    box(right_leg,vec3(-2,0,-2),ivec3(4,12,4),ivec2(0,32),.25f);
    box(left_leg,vec3(-2,0,-2),ivec3(4,12,4),ivec2(16,48));
    box(left_leg,vec3(-2,0,-2),ivec3(4,12,4),ivec2(0,48),.25f);
    flush(skin,camera);
    if(armor){texture_size=vec2(64,32);
        for(int i=0;i<4;++i){auto& item=(*armor)[i];int material=Items::armor_material(item.id);if(item.empty() || material<0 || Items::armor_slot(item.id)!=i)continue;
            float f=i==2?.5f:1.f;
            if(i==0)box(head,vec3(-4,-8,-4),ivec3(8),ivec2(0),f);
            if(i==1 || i==2)box(torso,vec3(-4,0,-2),ivec3(8,12,4),ivec2(16,16),f);
            if(i==1){box(right_arm,vec3(-3,-2,-2),ivec3(4,12,4),ivec2(40,16),f);box(left_arm,vec3(-1,-2,-2),ivec3(4,12,4),ivec2(40,16),f);}
            if(i==2 || i==3){box(right_leg,vec3(-2,0,-2),ivec3(4,12,4),ivec2(0,16),f);box(left_leg,vec3(-2,0,-2),ivec3(4,12,4),ivec2(0,16),f);}
            flush(armor_skins[material][i==2?1:0],camera);
        }
    }texture_size=vec2(64);
}
void Avatar::hand(const mat4& matrix,bool left){
    vertices.clear();texture_size=vec2(64);
    mat4 m=translate(scale(mat4(1),vec3(1.f/16)),vec3(left?5:-5,2,0));
    box(m,vec3(left?-1:-3,-2,-2),ivec3(4,12,4),ivec2(left?32:40, left?48:16));
    box(m,vec3(left?-1:-3,-2,-2),ivec3(4,12,4),ivec2(left?48:40, left?48:32),.25f);
    skin.bind();shader.bind();shader.upload_texture("skin",skin);shader.upload_mat4("cam_mat",matrix);shader.upload_float("flash",0);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
    glDisable(GL_CULL_FACE);glDrawArrays(GL_TRIANGLES,0,(GLsizei)vertices.size());glEnable(GL_CULL_FACE);
}
// ModelPig/ModelCow/ModelSheep1+2/ModelZombie/ModelCreeper, 1.12.2
// ModelRenderer pivots and ModelBox UV rectangles, converted to our Y-up world.
void Avatar::mob(const Mobs::Entity& e,const mat4& camera,double partial){
    vertices.clear();auto p=mix(dvec3(e.previous[0],e.previous[1],e.previous[2]),dvec3(e.position[0],e.position[1],e.position[2]),std::clamp(partial,0.,1.));
    float alpha=float(std::clamp(partial,0.,1.)),phase=e.walk.render_phase(alpha),amount=e.walk.render_amount(alpha);
    mat4 root=translate(mat4(1),vec3(p));root=rotate(root,ModelAnimation::interpolate_yaw(e.previous_yaw,e.yaw,alpha),vec3(0,1,0));root=translate(root,vec3(0,1.5,0));root=scale(root,vec3(1,-1,-1)/16.f);
    float walk=ModelAnimation::leg(phase,amount,0);
    auto pivot=[&](vec3 pos,float angle=0.f){return rotate(translate(root,pos),angle,vec3(1,0,0));};
    texture_size=vec2(64,e.kind==Mobs::Zombie?64:32);
    if(e.kind==Mobs::Zombie){
        box(root,vec3(-4,-8,-4),ivec3(8),ivec2(0));box(root,vec3(-4,0,-2),ivec3(8,12,4),ivec2(16,16));
        auto pose=ModelAnimation::zombie(phase,amount,float(e.age_ticks)+alpha,e.attack_ticks?std::clamp((6-e.attack_ticks+alpha)/6.f,0.f,1.f):0,e.arms_raised);
        box(part(root,pose.right_arm),vec3(-3,-2,-2),ivec3(4,12,4),ivec2(40,16));
        box(part(root,pose.left_arm),vec3(-1,-2,-2),ivec3(4,12,4),ivec2(40,16));
        box(pivot(vec3(-1.9,12,0),walk),vec3(-2,0,-2),ivec3(4,12,4),ivec2(0,16));
        box(pivot(vec3(1.9,12,0),-walk),vec3(-2,0,-2),ivec3(4,12,4),ivec2(0,16));
    }else if(e.kind==Mobs::Creeper){
        box(pivot(vec3(0,6,0)),vec3(-4,-8,-4),ivec3(8),ivec2(0));
        box(pivot(vec3(0,6,0)),vec3(-4,0,-2),ivec3(8,12,4),ivec2(16,16));
        for(int i=0;i<4;++i)box(pivot(vec3(i%2?2:-2,18,i<2?4:-4),(i==0 || i==3)?walk:-walk),vec3(-2,0,-2),ivec3(4,6,4),ivec2(0,16));
    }else if(e.kind==Mobs::Chicken){
        box(pivot(vec3(0,8,-4)),vec3(-3,-3,-4),ivec3(6,6,6),ivec2(0));
        box(pivot(vec3(0,8,-8)),vec3(-2,-2,-3),ivec3(4,4,3),ivec2(14,0));
        for(int i=0;i<2;++i)box(pivot(vec3(i?2:-2,10,0),i?-.25f:.25f),vec3(-1,-2,-3),ivec3(2,4,6),ivec2(0,16));
        for(int i=0;i<2;++i)box(pivot(vec3(i?1:-1,2,0),i?walk:-walk),vec3(-1,0,-1),ivec3(2,6,2),ivec2(0,22));
    }else if(e.kind==Mobs::Spider){
        box(pivot(vec3(0,6,0)),vec3(-4,-3,-8),ivec3(8,6,16),ivec2(0));
        box(pivot(vec3(0,7,-8)),vec3(-4,-4,-4),ivec3(8),ivec2(32,0));
        for(int i=0;i<8;++i){float side=i%2?-1.f:1.f;float z=(i/2-1.5f)*3.f;box(pivot(vec3(side*5,5,z),side*(i%4<2?.35f:-.35f)+walk*side),vec3(-1,0,-1),ivec3(2,1,8),ivec2(0,16));}
    }else if(e.kind==Mobs::Enderman){
        box(root,vec3(-4,-8,-4),ivec3(8),ivec2(0));box(root,vec3(-3,0,-2),ivec3(6,24,4),ivec2(16,16));
        box(pivot(vec3(-5,0,0),walk),vec3(-2,0,-2),ivec3(4,24,4),ivec2(40,16));box(pivot(vec3(5,0,0),-walk),vec3(-2,0,-2),ivec3(4,24,4),ivec2(40,16));
    }else if(e.kind==Mobs::Slime){
        box(root,vec3(-6,-6,-6),ivec3(12),ivec2(0),.15f);box(root,vec3(-3,-3,-6.5),ivec3(2),ivec2(0));box(root,vec3(3,-3,-6.5),ivec3(2),ivec2(0));
    }else if(e.kind==Mobs::Ghast){
        box(root,vec3(-16,-16,-16),ivec3(32),ivec2(0));
        for(int i=0;i<8;++i)box(pivot(vec3((i%4-1.5f)*5,-19,(i/4?7:-7)),walk),vec3(-1,0,-1),ivec3(2,8,2),ivec2(0,16));
    }else if(e.kind==Mobs::Wither){
        // Three-headed Wither silhouette from the 1.12 model.
        box(root,vec3(-10,-3,-4),ivec3(20,10,8),ivec2(0));
        box(pivot(vec3(-10,4,0)),vec3(-5,-5,-5),ivec3(10),ivec2(0));
        box(pivot(vec3(0,5,0)),vec3(-5,-5,-5),ivec3(10),ivec2(0));
        box(pivot(vec3(10,4,0)),vec3(-5,-5,-5),ivec3(10),ivec2(0));
        for(int i=0;i<3;++i)box(pivot(vec3((i-1)*10,-8,0),i%2?walk:-walk),vec3(-1,0,-1),ivec3(2,8,2),ivec2(0,16));
    }else if(e.kind==Mobs::Blaze){
        box(root,vec3(-4,-8,-4),ivec3(8,16,8),ivec2(0));for(int i=0;i<12;++i){float a=i*.5236f;box(pivot(vec3(std::cos(a)*8,5+std::sin(a*2)*3,std::sin(a)*8),a+phase*.15f),vec3(-1,-1,-1),ivec3(2),ivec2(0,16));}
    }else if(e.kind==Mobs::Bat){
        box(pivot(vec3(0,4,0)),vec3(-2,-3,-2),ivec3(4,6,4),ivec2(0));for(int i=0;i<2;++i)box(pivot(vec3(i?3:-3,5,0),i?-.25f:.25f),vec3(-1,-1,-4),ivec3(2,2,8),ivec2(0,16));
    }else if(e.kind==Mobs::Rabbit){
        box(pivot(vec3(0,5,-3)),vec3(-3,-3,-4),ivec3(6,6,8),ivec2(0));box(pivot(vec3(0,11,-5)),vec3(-2,-4,-2),ivec3(4,6,4),ivec2(24,0));for(int i=0;i<4;++i)box(pivot(vec3(i%2?2:-2,0,i<2?2:-2),walk),vec3(-1,0,-1),ivec3(2,4,2),ivec2(0,16));
    }else if(e.kind==Mobs::Witch || e.kind==Mobs::Skeleton){
        box(root,vec3(-4,-8,-4),ivec3(8),ivec2(0));box(root,vec3(-4,0,-2),ivec3(8,12,4),ivec2(16,16));auto pose=ModelAnimation::biped(phase,amount,float(e.age_ticks)+alpha,0,0,false,e.kind==Mobs::Witch);box(part(root,pose.right_arm),vec3(-3,-2,-2),ivec3(4,12,4),ivec2(40,16));box(part(root,pose.left_arm),vec3(-1,-2,-2),ivec3(4,12,4),ivec2(40,16));box(pivot(vec3(-1.9,12,0),walk),vec3(-2,0,-2),ivec3(4,12,4),ivec2(0,16));box(pivot(vec3(1.9,12,0),-walk),vec3(-2,0,-2),ivec3(4,12,4),ivec2(0,16));
    }else if(e.kind==Mobs::Wolf){
        auto h=pivot(vec3(0,8,-6));box(h,vec3(-3,-4,-4),ivec3(6,6,6),ivec2(0));box(h,vec3(-2,-2,-7),ivec3(4,3,3),ivec2(14,0));
        auto body=pivot(vec3(0,7,2),1.57079633f);box(body,vec3(-3,-7,-5),ivec3(6,12,8),ivec2(18,4));
        for(int i=0;i<4;++i)box(pivot(vec3(i%2?2.4f:-2.4f,4,i<2?5:-3),i%2?walk:-walk),vec3(-1,0,-1),ivec3(2,6,2),ivec2(0,16));
        box(pivot(vec3(0,8,7),-.35f+walk),vec3(-1,-1,0),ivec3(2,2,6),ivec2(0,22));
    }else if(e.kind==Mobs::Ocelot){
        auto h=pivot(vec3(0,9,-7));box(h,vec3(-3,-4,-4),ivec3(6,6,6),ivec2(0));box(h,vec3(-2,-2,-8),ivec3(4,3,2),ivec2(14,0));
        auto body=pivot(vec3(0,7,2),1.57079633f);box(body,vec3(-3,-7,-5),ivec3(6,12,8),ivec2(18,4));
        for(int i=0;i<4;++i)box(pivot(vec3(i%2?2.4f:-2.4f,4,i<2?5:-3),i%2?walk:-walk),vec3(-1,0,-1),ivec3(2,6,2),ivec2(0,16));
        box(pivot(vec3(0,8,7),-.55f+walk),vec3(-1,-1,0),ivec3(2,2,10),ivec2(0,22));
    }else if(e.kind==Mobs::Horse){
        // Compact horse proportions: one body, a connected neck/head and four legs.
        box(root,vec3(-6,-6,-8),ivec3(12,12,16),ivec2(18,4));
        box(pivot(vec3(0,8,-8),-.12f),vec3(-3,-8,-3),ivec3(6,14,6),ivec2(0));
        box(pivot(vec3(0,18,-10)),vec3(-4,-4,-5),ivec3(8,8,8),ivec2(0));
        for(int i=0;i<4;++i)box(pivot(vec3(i%2?4:-4,0,i<2?5:-5),i%2?walk:-walk),vec3(-2,-1,-2),ivec3(4,12,4),ivec2(0,16));
        box(pivot(vec3(0,7,9),.3f+walk),vec3(-1,-1,0),ivec3(2,2,8),ivec2(0,22));
    }else{
        bool cow=e.kind==Mobs::Cow,sheep=e.kind==Mobs::Sheep;
        if(cow){auto h=pivot(vec3(0,4,-8));box(h,vec3(-4,-4,-6),ivec3(8,8,6),ivec2(0));
            box(h,vec3(-5,-5,-4),ivec3(1,3,1),ivec2(22,0));box(h,vec3(4,-5,-4),ivec3(1,3,1),ivec2(22,0));
            auto body=pivot(vec3(0,5,2),1.57079633f);box(body,vec3(-6,-10,-7),ivec3(12,18,10),ivec2(18,4));box(body,vec3(-2,2,-8),ivec3(4,6,1),ivec2(52,0));
        }else if(sheep){box(pivot(vec3(0,6,-8)),vec3(-3,-4,-6),ivec3(6,6,8),ivec2(0));box(pivot(vec3(0,5,2),1.57079633f),vec3(-4,-10,-7),ivec3(8,16,6),ivec2(28,8));}
        else{auto h=pivot(vec3(0,12,-6));box(h,vec3(-4,-4,-8),ivec3(8),ivec2(0));box(h,vec3(-2,0,-9),ivec3(4,3,1),ivec2(16,16));box(pivot(vec3(0,11,2),1.57079633f),vec3(-5,-10,-7),ivec3(10,16,8),ivec2(28,8));}
        int height=e.kind==Mobs::Pig?6:12;
        for(int i=0;i<4;++i)box(pivot(vec3((i%2?1:-1)*(cow?4:3),24-height,i<2?7:(cow?-6:-5)),(i==0 || i==3)?walk:-walk),vec3(-2,0,-2),ivec3(4,height,4),ivec2(0,16));
    }
    float flash=e.hurt>0?.4f:0.f;if(e.kind==Mobs::Creeper && e.fuse>0 && int(e.fuse*10)%2)flash=.65f;
    flush(mob_skins[e.kind],camera,flash);
    if(e.kind==Mobs::Sheep && !e.sheared){
        box(pivot(vec3(0,6,-8)),vec3(-3,-4,-4),ivec3(6),ivec2(0),.6f);
        box(pivot(vec3(0,5,2),1.57079633f),vec3(-4,-10,-7),ivec3(8,16,6),ivec2(28,8),1.75f);
        for(int i=0;i<4;++i)box(pivot(vec3(i%2?3:-3,12,i<2?7:-5),(i==0 || i==3)?walk:-walk),vec3(-2,0,-2),ivec3(4,6,4),ivec2(0,16),.5f);
        flush(fur,camera,flash);
    }
    texture_size=vec2(64);
}
void Avatar::destroy() {for(auto& t:mob_skins)t.destroy();fur.destroy();for(auto& a:armor_skins)for(auto& t:a)t.destroy();skin.destroy();glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);shader.destroy();}
