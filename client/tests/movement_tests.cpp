#include <movement.h>
#include <block_geometry.h>
#include <voxel_trace.h>
#include <cmath>
#include <iostream>
#include <cstdlib>
static void check(bool condition,const char* message) {if(!condition){std::cerr<<message<<'\n';std::exit(1);}}
int main() {
    auto floor=[](int,int y,int){return y<0;};
    Movement::State s{{.5,8,.5},{0,0,0},false};
    for(int i=0;i<360;++i) Movement::tick(s,{0,0},false,1./120.,floor);
    check(s.grounded && std::abs(s.position.y)<1e-5,"fall must land on surface");
    Movement::tick(s,{0,0},true,1./120.,floor);
    check(s.velocity.y>0 && !s.grounded,"jump must leave ground");
    double max_y=s.position.y;
    for(int i=0;i<240;++i){ Movement::tick(s,{0,0},false,1./120.,floor);max_y=std::max(max_y,s.position.y); }
    check(max_y>1 && max_y<1.4 && s.grounded,"jump arc must land");
    s={{.5,0,.5},{0,0,0},true};
    auto wall=[](int x,int y,int){return y<0 || (x==2 && y<4);};
    for(int i=0;i<360;++i) Movement::tick(s,{5.6,0},false,1./120.,wall);
    check(s.position.x<=1.70001 && s.position.x>1.69,"wall must stop body");
    check(!Movement::overlaps(s.position,wall),"body must not overlap wall");
    s={{-.5,10,-.5},{0,-52,0},false};
    for(int i=0;i<60;++i) Movement::tick(s,{0,0},false,1./120.,floor);
    check(s.grounded && std::abs(s.position.y)<1e-5,"terminal fall must not tunnel at negative coordinates");
    auto ceiling=[](int,int y,int){return y<0 || y==2;};
    s={{.5,0,.5},{0,0,0},true};
    Movement::tick(s,{0,0},true,1./120.,ceiling);
    for(int i=0;i<240;++i){Movement::tick(s,{0,0},false,1./120.,ceiling);check(s.position.y<=.20001,"head must not cross ceiling");}
    check(s.grounded,"head collision must allow landing");
    for (int i=0;i<8;++i) {
        glm::vec3 p=torch_corner(glm::vec3(i&1,(i>>1)&1,(i>>2)&1));
        check(p.x>=7.f/16 && p.x<=9.f/16 && p.z>=7.f/16 && p.z<=9.f/16,"torch faces must stay inside narrow shaft");
        check(p.y>=0 && p.y<=10.f/16,"torch height must be ten pixels");
    }
    auto target=[](glm::ivec3 p){return p==glm::ivec3(0,0,5);};
    auto hit=trace_voxels(glm::vec3(.5),glm::vec3(0,0,1),4.5f,target);
    check(hit.hit && hit.distance==4.5f && hit.normal==glm::ivec3(0,0,-1),"axis aligned ray must hit at reach");
    check(!trace_voxels(glm::vec3(.5),glm::vec3(0,0,1),4.49f,target).hit,"ray must not hit beyond reach");
    check(!trace_voxels(glm::vec3(.5),glm::vec3(0),5.f,target).hit,"zero direction must terminate");
    hit=trace_voxels(glm::vec3(-1,0.5,.5),glm::vec3(-1,0,0),1.f,[](glm::ivec3 p){return p.x==-2;});
    check(hit.hit && hit.distance==0 && hit.normal.x==1,"negative ray on grid boundary");
    std::cout<<"Movement: fall, jump, wall, terminal velocity, ceiling passed\n";
}
