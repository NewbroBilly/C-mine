#include <generation.h>
#include <block_ids.h>
#include "../ported_mc/terrain/TerrainGenerator.h"
#include <iostream>
#include <cstdlib>
#include <set>
#include <chrono>
#include <algorithm>
void check(bool ok,const char* why){if(!ok){std::cerr<<why<<'\n';std::exit(1);}}
int cell(const std::vector<unsigned char>& v,int x,int y,int z){return v[((x*256+y)*16+z)*3];}
int main(){
    auto start=std::chrono::steady_clock::now();
    auto a=Generation::generate(12345,-1,0),b=Generation::generate(12345,0,0);
    Generation::clear_cache();
    auto reverse_b=Generation::generate(12345,0,0),reverse_a=Generation::generate(12345,-1,0);
    check(a==reverse_a && b==reverse_b,"neighboring chunks must be independent of generation order");
    check(Generation::generate(54321,0,0)!=b,"seed changes terrain, even after switching cached worlds");
    size_t water=0,cave=0,ore=0,logs=0;
    std::set<int> biomes;mc::TerrainGenerator terrain(12345);int forest_x=0,forest_z=0;bool forest=false;
    for(int x=-8192;x<=8192;x+=512)for(int z=-8192;z<=8192;z+=512){auto b=terrain.column(x,z).biome;biomes.insert(int(b));if(!forest && b==mc::Biome::BirchForest){forest_x=x/16;forest_z=z/16;forest=true;}}
    check(biomes.size()>=8,"climate must produce varied biomes");
    // Include distant climates, negative coordinates and a value using the high seed bits.
    for(auto p:{std::pair<int,int>{0,0},{-1,0},{120,-80},{-240,200}}){
        auto data=Generation::generate(12345,p.first,p.second);
        check(data.size()==16*256*16*3,"stable chunk save shape");
        for(int x=0;x<16;++x)for(int z=0;z<16;++z){
            check(cell(data,x,0,z)==BEDROCK,"unbroken floor");
            check(cell(data,x,255,z)==AIR,"upper world boundary remains open");
            for(int y=0;y<256;++y){auto i=((x*256+y)*16+z)*3;int id=data[i];
                check(id<BLOCK_COUNT,"all imported block IDs must be mapped");
                if(id==WATER){++water;check(data[i+1]==0 && data[i+2]==0,"natural water is settled and unscheduled");}
                if(id==AIR && y>5 && y<60)++cave;
                if(id==COAL_ORE || id==IRON_ORE || id==DIAMOND_ORE)++ore;
                if(id==LOG || id==BIRCH_LOG || id==SPRUCE_LOG || id==ACACIA_LOG || id==JUNGLE_LOG)++logs;
            }
        }
    }
    check(water>0 && cave>0 && ore>0,"terrain includes oceans, caves and ores");
    check(forest,"sample contains a birch forest");
    auto tree_a=Generation::generate(12345,forest_x,forest_z),tree_b=Generation::generate(12345,forest_x+1,forest_z);
    size_t leaves=0;for(size_t i=0;i<tree_a.size();i+=3){if(tree_a[i]==BIRCH_LOG)++logs;if(tree_a[i]==LEAVES)++leaves;}
    check(logs>0 && leaves>0,"forest trees map into usable logs and leaf crowns");
    Generation::clear_cache();
    check(tree_b==Generation::generate(12345,forest_x+1,forest_z) && tree_a==Generation::generate(12345,forest_x,forest_z),"forest crowns at chunk boundaries remain independent of loading order");
    auto high=Generation::generate(0x100003039ULL,0,0);
    check(high!=b,"full 64-bit seed must influence imported generation");
    int ocean_seeds=0;
    for(uint64_t seed=0;seed<64 && ocean_seeds<4;++seed){
        mc::TerrainGenerator probe(seed);auto origin=probe.column(8,8);
        if(!mc::isOceanBiome(origin.biome))continue;
        ++ocean_seeds;std::array<double,3> spawn;
        check(Generation::find_spawn(seed,spawn),"ocean-origin worlds must find dry surface before streaming");
        int wx=int(std::floor(spawn[0])),wz=int(std::floor(spawn[2]));int cx=mc::floorDiv(wx,16),cz=mc::floorDiv(wz,16),x=mc::floorMod(wx,16),z=mc::floorMod(wz,16),y=int(spawn[1]);
        auto data=Generation::generate(seed,cx,cz);
        check(y>0 && y<254 && cell(data,x,y-1,z)!=WATER && cell(data,x,y-1,z)!=AIR && (cell(data,x,y,z)==AIR || cell(data,x,y,z)==GRASS) && (cell(data,x,y+1,z)==AIR || cell(data,x,y+1,z)==GRASS),"spawn has dry ground and two free blocks above it");
        Generation::clear_cache();std::array<double,3> repeat;
        check(Generation::find_spawn(seed,repeat) && repeat==spawn,"spawn choice is deterministic after cache reset");
    }
    check(ocean_seeds>=3,"exercise several ocean-origin seeds");
    Generation::clear_cache();
    std::cout<<"Generation passed: "<<biomes.size()<<" biomes, "<<water<<" settled water, "<<cave<<" cave air, "<<ore<<" ores, "<<logs<<" logs; "
        <<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<" seconds\n";
}
