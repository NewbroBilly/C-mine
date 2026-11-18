#include <water.h>
#include <iostream>
#include <cstdlib>
#include <map>
#include <cmath>
static void check(bool c,const char* s){if(!c){std::cerr<<s<<'\n';std::exit(1);}}
int main(){
    std::map<Water::Pos,int> cells;
    auto get=[&](Water::Pos p){auto i=cells.find(p);return i!=cells.end()?i->second:(p[1]<=0?Water::Solid:Water::Air);};
    auto set=[&](Water::Pos p,int l){if(l==Water::Air)cells.erase(p);else cells[p]=l;};
    Water::Simulator sim;cells[{0,1,0}]=0;sim.changed({0,1,0});
    for(int i=0;i<4;++i)sim.tick(get,set);
    check(cells.size()==1,"water must wait five ticks");sim.tick(get,set);
    check(get({1,1,0})==1 && get({0,1,-1})==1,"source spreads one level per cell");
    for(int i=0;i<200;++i)sim.tick(get,set);
    check(get({7,1,0})==7 && get({8,1,0})==Water::Air,"flat water must stop after seven horizontal cells");
    check(get({3,1,3})==6,"flat water levels follow horizontal distance");
    auto f=Water::flow({1,1,0},get);check(f[0]>.99 && std::abs(f[2])<.01,"flow vector must point away from source");
    cells.erase({0,1,0});sim.changed({0,1,0});for(int i=0;i<400;++i)sim.tick(get,set);
    check(cells.empty(),"removing the only source must drain all flowing water");
    sim={};cells[{0,5,0}]=0;sim.changed({0,5,0});for(int i=0;i<5;++i)sim.tick(get,set);
    check(get({1,5,0})==Water::Air,"an initial water tick must prefer falling over sideways spread");
    for(int i=0;i<20;++i)sim.tick(get,set);
    check(get({0,4,0})==8 && get({0,1,0})==8,"downward water must carry falling metadata");
    check(get({1,5,0})==1,"a source updates sideways when water already occupies the cell below");
    for(int i=0;i<100;++i)sim.tick(get,set);check(Water::liquid(get({1,1,0})) && get({6,1,0})!=Water::Air,"falling water must spread across supported ground");
    cells.clear();sim={};cells[{-1,1,0}]=0;cells[{1,1,0}]=0;cells[{0,1,0}]=1;sim.changed({0,1,0});
    for(int i=0;i<5;++i)sim.tick(get,set);check(get({0,1,0})==0,"two sources regenerate a source over solid support");
    cells.clear();sim={};cells[{0,1,0}]=0;cells[{2,0,0}]=Water::Air;sim.changed({0,1,0});
    for(int i=0;i<5;++i)sim.tick(get,set);check(get({1,1,0})==1 && get({-1,1,0})==Water::Air && get({0,1,1})==Water::Air,"water must choose the shortest downhill route");
    cells.clear();sim={};cells[{-1,1,0}]=2;cells[{-2,1,0}]=Water::Unloaded;sim.changed({-1,1,0});
    for(int i=0;i<20;++i)sim.tick(get,set);check(get({-1,1,0})==2,"unloaded chunk frontier must not drain water");
    cells[{-2,1,0}]=0;sim.changed({-2,1,0});for(int i=0;i<10;++i)sim.tick(get,set);check(get({-1,1,0})==1,"water must resume at negative chunk coordinates");
    check(std::abs(Water::height(0)-8./9)<1e-9 && Water::height(8)==Water::height(0),"source and falling surface height");
    std::cout<<"Water scheduling, levels, range, falling, drainage, source regeneration, path selection and chunk frontier passed\n";
}
