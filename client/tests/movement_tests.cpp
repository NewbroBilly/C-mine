#include <movement.h>
#include <block_geometry.h>
#include <voxel_trace.h>
#include <oak_tree.h>
#include <map>
#include <tuple>
#include <cmath>
#include <iostream>
#include <cstdlib>
static void check(bool condition,const char* message) {if(!condition){std::cerr<<message<<'\n';std::exit(1);}}
int main() {
    auto floor=[](int,int y,int){return y<0;};
    Movement::State s{{.5,8,.5},{0,0,0},false};
    for(int i=0;i<360;++i) Movement::tick(s,{0,0},false,false,false,floor);
    check(s.grounded && std::abs(s.position.y)<1e-5,"fall must land on surface");
    Movement::tick(s,{0,0},true,false,false,floor);
    check(s.velocity.y>0 && !s.grounded,"jump must leave ground");
    double max_y=s.position.y;
    for(int i=0;i<240;++i){ Movement::tick(s,{0,0},false,false,false,floor);max_y=std::max(max_y,s.position.y); }
    check(max_y>1 && max_y<1.4 && s.grounded,"jump arc must land");
    s={{.5,0,.5},{0,0,0},true};
    auto wall=[](int x,int y,int){return y<0 || (x==2 && y<4);};
    for(int i=0;i<360;++i) Movement::tick(s,{1,0},false,false,false,wall);
    check(s.position.x<=1.70001 && s.position.x>1.69,"wall must stop body");
    check(!Movement::overlaps(s.position,wall),"body must not overlap wall");
    s={{-.5,10,-.5},{0,-52,0},false};
    for(int i=0;i<60;++i) Movement::tick(s,{0,0},false,false,false,floor);
    check(s.grounded && std::abs(s.position.y)<1e-5,"terminal fall must not tunnel at negative coordinates");
    auto ceiling=[](int,int y,int){return y<0 || y==2;};
    s={{.5,0,.5},{0,0,0},true};
    Movement::tick(s,{0,0},true,false,false,ceiling);
    for(int i=0;i<240;++i){Movement::tick(s,{0,0},false,false,false,ceiling);check(s.position.y<=.20001,"head must not cross ceiling");}
    check(s.grounded,"head collision must allow landing");
    // Direct land/air recurrence from the 1.12.2 travel method.
    s={{.5,0,.5},{0,0,0},true};
    double expected_y=0,expected_motion=.42f;
    for(int i=0;i<10;++i){
        Movement::tick(s,{0,0},i==0,false,false,floor);
        expected_y+=expected_motion;expected_motion=(expected_motion-.08)*double(.98f);
        check(std::abs(s.position.y-expected_y)<1e-8,"vertical trajectory must match 20 Hz reference");
    }
    s={{.5,0,.5},{0,0,0},true};
    for(int i=0;i<80;++i)Movement::tick(s,{1,0},false,false,false,floor);
    double start_x=s.position.x;
    for(int i=0;i<20;++i)Movement::tick(s,{1,0},false,false,false,floor);
    check(s.position.x-start_x>4.30 && s.position.x-start_x<4.34,"vanilla walking speed");
    for(int i=0;i<80;++i)Movement::tick(s,{1,0},false,true,false,floor);
    start_x=s.position.x;
    for(int i=0;i<20;++i)Movement::tick(s,{1,0},false,true,false,floor);
    check(s.position.x-start_x>5.59 && s.position.x-start_x<5.64,"vanilla sprint speed");
    s={{.5,0,.5},{0,0,0},true};
    auto ledge=[](int x,int y,int){return y<0 && x<=0;};
    for(int i=0;i<100;++i)Movement::tick(s,{1,0},false,false,true,ledge);
    check(s.position.x<1.3 && s.position.y>-.01,"sneaking must protect ledge");
    Trees::Random java_random(0);check(java_random.next(100)==60,"Java random sequence");
    using Key=std::tuple<int,int,int>;
    std::map<Key,Trees::Cell> tree;
    auto get=[&](int x,int y,int z){auto it=tree.find({x,y,z});return it!=tree.end()?it->second:(y==0?Trees::Grass:Trees::Air);};
    auto set=[&](int x,int y,int z,Trees::Cell c){tree[{x,y,z}]=c;};
    Trees::Random r(0);check(Trees::oak(0,1,0,r,get,set),"oak on grass");
    for(int y=1;y<=4;++y)check(get(0,y,0)==Trees::Log,"oak trunk height and continuity");
    check(get(0,0,0)==Trees::Dirt && get(0,5,0)==Trees::Leaves,"oak dirt and crown");
    check(get(1,5,1)==Trees::Air && get(1,5,0)==Trees::Leaves,"vanilla top crown corners");
    tree.clear();tree[{0,3,0}]=Trees::Other;r=Trees::Random(0);
    check(!Trees::oak(0,1,0,r,get,set) && tree.size()==1,"obstructed oak must not partially write");
    auto flat=[](int,int y,int){return y==0?Trees::Grass:Trees::Air;};
    tree.clear();r=Trees::Random(0);Trees::oak(15,1,0,r,flat,set);auto full_tree=tree;
    tree.clear();
    for(int side=0;side<2;++side){r=Trees::Random(0);Trees::oak(15,1,0,r,flat,[&](int x,int y,int z,Trees::Cell c){if((x<16)==(side==0))set(x,y,z,c);});}
    check(tree==full_tree,"clipped tree must join identically across chunks");
    for (int i=0;i<8;++i) {
        glm::vec3 p=torch_corner(glm::vec3(i&1,(i>>1)&1,(i>>2)&1));
        check(p.x>=7.f/16 && p.x<=9.f/16 && p.z>=7.f/16 && p.z<=9.f/16,"torch faces must stay inside narrow shaft");
        check(p.y>=0 && p.y<=10.f/16,"torch height must be ten pixels");
    }
    auto target=[](glm::ivec3 p){return p==glm::ivec3(0,0,5);};
    auto empty=[](int,int,int){return false;};
    s={{0,50,0},{0,0,0},false};double water_y=50,water_v=0,water_x=0,water_vx=0;
    for(int i=0;i<100;++i){
        Movement::swim_tick(s,{1,0},true,{0,0,0},empty);
        water_v+=.04;water_y+=water_v;water_v=water_v*.8-.02;
        water_vx+=.0196;water_x+=water_vx;water_vx*=.8;
        check(std::abs(s.position.y-water_y)<1e-8 && std::abs(s.position.x-water_x)<1e-8,"water motion must follow vanilla recurrence");
    }
    s={{0,50,0},{0,0,0},false};Movement::swim_tick(s,{0,0},false,{1,0,0},empty);
    check(std::abs(s.position.x-.014)<1e-9,"water current must accelerate by .014 per tick");
    s={{.5,0,.5},{0,-100,0},false};Movement::swim_tick(s,{0,0},false,{0,0,0},floor);
    check(s.position.y>=-1e-6 && s.grounded,"swimming must retain swept floor collision");
    check(std::abs(Movement::hurt_roll(10,0))<1e-8 && Movement::hurt_roll(8,0)<-10 && Movement::hurt_roll(0,0)==0,"hurt camera must tilt and recover");
    auto hit=trace_voxels(glm::vec3(.5),glm::vec3(0,0,1),4.5f,target);
    check(hit.hit && hit.distance==4.5f && hit.normal==glm::ivec3(0,0,-1),"axis aligned ray must hit at reach");
    check(!trace_voxels(glm::vec3(.5),glm::vec3(0,0,1),4.49f,target).hit,"ray must not hit beyond reach");
    check(!trace_voxels(glm::vec3(.5),glm::vec3(0),5.f,target).hit,"zero direction must terminate");
    hit=trace_voxels(glm::vec3(-1,0.5,.5),glm::vec3(-1,0,0),1.f,[](glm::ivec3 p){return p.x==-2;});
    check(hit.hit && hit.distance==0 && hit.normal.x==1,"negative ray on grid boundary");
    std::cout<<"Movement: fall, jump, wall, terminal velocity, ceiling passed\n";
}
