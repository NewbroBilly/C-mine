#include <movement.h>
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
    std::cout<<"Movement: fall, jump, wall, terminal velocity, ceiling passed\n";
}
