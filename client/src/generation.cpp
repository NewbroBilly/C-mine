#include <generation.h>
#include <block_ids.h>
#include "../ported_mc/terrain/Decorator.h"
#include <map>
#include <memory>
#include <limits>
namespace Generation {
namespace {
struct Generator {
    uint64_t seed;
    mc::TerrainGenerator terrain;
    mc::Decorator decorator;
    struct Cached { std::unique_ptr<mc::Chunk> chunk; uint64_t used; };
    std::map<std::pair<int,int>,Cached> cache;
    uint64_t clock=0;
    std::vector<int> mapping;
    explicit Generator(uint64_t s):seed(s),terrain(s),decorator(s,terrain) {
        std::map<std::string,int> names={
            {"air",AIR},{"stone",STONE},{"dirt",DIRT},{"grass_block",GRASS_BLOCK},
            {"oak_log",LOG},{"oak_planks",PLANKS},{"water",WATER},{"glass",GLASS},
            {"grass",GRASS},{"torch",TORCH},{"bedrock",BEDROCK}};
#define BASE_BLOCK(id,label,...) names[#id]=id;
#define EXTRA_BLOCK BASE_BLOCK
#include <extra_blocks.def>
#undef BASE_BLOCK
#undef EXTRA_BLOCK
        // Catalog constants are uppercase; the imported registry uses lowercase.
        auto lower=[](std::string n){for(char& c:n)if(c>='A' && c<='Z')c+=32;return n;};
        auto copy=names;for(auto& n:copy)names[lower(n.first)]=n.second;
        names["terracotta"]=HARDENED_CLAY;names["melon"]=MELON_BLOCK;
        names["prismarine"]=PRISMARINE_ROUGH;
        mapping.resize(mc::Blocks::count(),STONE);
        for(int i=0;i<mc::Blocks::count();++i){
            auto& b=mc::Blocks::get(i);auto n=b.name;
            if(n.find("deepslate_")==0)n=n.substr(10);
            auto found=names.find(n);
            if(found!=names.end())mapping[i]=found->second;
            else if(n.find("leaves")!=std::string::npos)mapping[i]=LEAVES;
            else if(n.find("terracotta")!=std::string::npos)mapping[i]=HARDENED_CLAY;
            else if(b.plant || b.shape==mc::Shape::Cross)mapping[i]=AIR;
            else if(n=="snow")mapping[i]=AIR; // no unsupported full-cube snow layers
            else if(n=="lava")mapping[i]=OBSIDIAN;
            else if(n=="spawner" || n=="chest")mapping[i]=AIR;
            else if(n=="copper_ore")mapping[i]=STONE;
            else if(n=="moss_block")mapping[i]=GRASS_BLOCK;
            else if(n.find("mushroom")!=std::string::npos)mapping[i]=LOG;
        }
    }
    mc::Chunk* base(int x,int z){
        auto key=std::make_pair(x,z);auto it=cache.find(key);
        if(it==cache.end()){
            auto c=std::make_unique<mc::Chunk>(x,z);terrain.generate(*c);
            it=cache.emplace(key,Cached{std::move(c),0}).first;
        }
        it->second.used=++clock;return it->second.chunk.get();
    }
    void trim(){
        // Terrain-only cache: player edits and decorated trees never contaminate it.
        while(cache.size()>36){auto oldest=cache.begin();for(auto i=cache.begin();i!=cache.end();++i)if(i->second.used<oldest->second.used)oldest=i;cache.erase(oldest);}
    }
};
std::unique_ptr<Generator> generator;
}
void clear_cache(){generator.reset();}
size_t repair_legacy_cactus(unsigned char* saved,const unsigned char* natural){
    auto index=[](int x,int y,int z){return ((x*256+y)*16+z)*3;};size_t repaired=0;
    for(int x=0;x<16;++x)for(int z=0;z<16;++z)for(int y=1;y<253;++y){
        if(natural[index(x,y,z)]!=CACTUS || natural[index(x,y-1,z)]==CACTUS)continue;
        int top=y;while(top<255 && natural[index(x,top+1,z)]==CACTUS)++top;
        int ground=natural[index(x,y-1,z)];
        if(top-y>=3 || (ground!=SAND && ground!=RED_SAND) || saved[index(x,y-1,z)]!=ground || saved[index(x,top+1,z)]!=AIR)continue;
        bool untouched=true;for(int a=y;a<=top;++a){auto i=index(x,a,z);if(saved[i]!=STONE || saved[i+1] || saved[i+2])untouched=false;}
        if(untouched)for(int a=y;a<=top;++a){saved[index(x,a,z)]=CACTUS;++repaired;}
        y=top;
    }return repaired;
}
std::vector<unsigned char> generate(uint64_t seed,int cx,int cz){
    if(!generator || generator->seed!=seed)generator=std::make_unique<Generator>(seed);
    auto& g=*generator;mc::Chunk* nb[9];
    for(int dz=-1;dz<=1;++dz)for(int dx=-1;dx<=1;++dx)nb[(dz+1)*3+dx+1]=g.base(cx+dx,cz+dz);
    mc::Chunk target(cx,cz);auto& base=*nb[4];
    for(int i=0;i<256;++i){target.biome[i]=base.biome[i];target.terrainHeight[i]=base.terrainHeight[i];target.surfaceState[i]=base.surfaceState[i];}
    for(int x=0;x<16;++x)for(int z=0;z<16;++z)for(int y=mc::WORLD_MIN_Y;y<mc::WORLD_MAX_Y;++y)target.set(x,y,z,base.get(x,y,z));
    nb[4]=&target;g.decorator.decorate(target,nb);
    std::vector<unsigned char> out(16*256*16*3,0);
    for(int x=0;x<16;++x)for(int y=0;y<256;++y)for(int z=0;z<16;++z){
        // Preserve block scale from source Y=0..159 (including sea and most trees).
        // Compress the deep underground and highest mountains into our 256 layers.
        int sy=y<32?y*2-64:(y<192?y-32:160+(y-192)*5/2);
        auto source=target.get(x,sy,z);int id=g.mapping[mc::stateId(source)];
        if(y==0)id=BEDROCK;
        size_t i=((x*256+y)*16+z)*3;out[i]=static_cast<unsigned char>(id);
        if(id==LOG || id==SPRUCE_LOG || id==BIRCH_LOG || id==JUNGLE_LOG || id==ACACIA_LOG || id==DARK_OAK_LOG)out[i+1]=1;
        // Natural water stays settled, without any initial fluid queue.
    }
    g.trim();return out;
}
bool find_spawn(uint64_t seed,std::array<double,3>& position){
    mc::TerrainGenerator terrain(seed);
    // Follow the supplied game's dry-land search before starting chunk streaming.
    // Validate actual decorated blocks: a climate height alone can hide a cave.
    for(int r=0;r<160;++r)for(int k=0;k<(r?8:1);++k){
        double angle=k*.785398+r*.3;int wx=r?int(std::cos(angle)*r*24):8,wz=r?int(std::sin(angle)*r*24):8;
        auto c=terrain.column(wx,wz);
        if(mc::isOceanBiome(c.biome) || c.biome==mc::Biome::River || c.biome==mc::Biome::FrozenRiver || c.height<65 || c.height>=100)continue;
        int cx=mc::floorDiv(wx,16),cz=mc::floorDiv(wz,16);auto data=generate(seed,cx,cz);
        auto at=[&](int x,int y,int z){return data[((x*256+y)*16+z)*3];};
        for(int i=0;i<256;++i){int x=(mc::floorMod(wx,16)+i%16)%16,z=(mc::floorMod(wz,16)+i/16)%16;
            for(int y=253;y>0;--y){int id=at(x,y,z);
                if(id==AIR || id==GRASS)continue;
                bool ground=id==GRASS_BLOCK || id==DIRT || id==SAND || id==RED_SAND || id==STONE || id==GRAVEL || id==PODZOL || id==MYCELIUM || id==SNOW_BLOCK;
                if(ground && (at(x,y+1,z)==AIR || at(x,y+1,z)==GRASS) && (at(x,y+2,z)==AIR || at(x,y+2,z)==GRASS)){
                    position={cx*16+x+.5,double(y+1),cz*16+z+.5};return true;
                }
                break; // underwater columns and tree crowns are not spawn ground
            }
        }
    }
    return false;
}
}
