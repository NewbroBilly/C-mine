#include <ores.h>
#include <block_ids.h>
#include <map>
#include <tuple>
#include <vector>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using Pos=std::tuple<int,int,int>;
using Cells=std::map<Pos,int>;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
    std::vector<std::pair<int,int>> chunks;for(int x=-2;x<2;++x)for(int z=-2;z<2;++z)chunks.push_back({x,z});
    auto generate=[&](uint64_t seed){Cells cells;
        for(auto [cx,cz]:chunks)Ores::generate(cx,cz,seed,
            [&](int x,int y,int z){
                check(x>=cx*16 && x<cx*16+16 && z>=cz*16 && z<cz*16+16,"ore query outside target chunk");
                if(y==0)return int(BEDROCK);
                if(x==-3)return int(WATER);if(x==-2)return int(AIR);if(x==-1)return int(DIRT);
                auto found=cells.find({x,y,z});return found==cells.end()?int(STONE):found->second;
            },[&](int x,int y,int z,int id){check(y>0 && y<256 && x!=-3 && x!=-2 && x!=-1,"ore overwrote protected material");cells[{x,y,z}]=id;});
        return cells;
    };
    auto cells=generate(12345);std::reverse(chunks.begin(),chunks.end());
    check(cells==generate(12345),"ore layout depends on chunk load order");
    check(cells!=generate(54321),"ore layout must depend on world seed");
    std::map<int,int> counts;int seam=0;
    for(auto [p,id]:cells){auto [x,y,z]=p;++counts[id];
        if(id==COAL_ORE)check(y<=130,"coal above configured depth");
        else if(id==IRON_ORE)check(y<=66,"iron above configured depth");
        else if(id==GOLD_ORE || id==LAPIS_ORE)check(y<=34,"gold/lapis above configured depth");
        else if(id==EMERALD_ORE)check(y>=4 && y<32,"emerald depth");
        else check(y<=18,"diamond/redstone above configured depth");
        if((x+1)%16==0){auto it=cells.find({x+1,y,z});if(it!=cells.end() && it->second==id)++seam;}
    }
    for(int id:{COAL_ORE,IRON_ORE,GOLD_ORE,REDSTONE_ORE,DIAMOND_ORE,LAPIS_ORE,EMERALD_ORE})check(counts[id]>0,"missing ore kind");
    check(counts[COAL_ORE]>counts[DIAMOND_ORE]*10,"diamond should be rarer than coal");
    check(seam>0,"ore veins must cross chunk seams");
    std::cout<<"Seeded ore veins, all seven kinds, depths, protected blocks, negative coordinates and chunk seams passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
