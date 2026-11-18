#pragma once
#include <cstdint>
#include <cmath>
namespace Trees {
// java.util.Random's 48-bit LCG, including bounded nextInt rejection.
struct Random {
    uint64_t state;
    explicit Random(uint64_t seed):state((seed^0x5DEECE66DULL)&((1ULL<<48)-1)){}
    uint32_t bits(int count){state=(state*0x5DEECE66DULL+11)&((1ULL<<48)-1);return uint32_t(state>>(48-count));}
    int next(int bound){if((bound&-bound)==bound)return int((uint64_t(bound)*bits(31))>>31);uint32_t b,v;do{b=bits(31);v=b%bound;}while(uint64_t(b-v)+bound-1>=0x80000000ULL);return int(v);}
};
enum Cell {Air,Grass,Dirt,Log,Leaves,Other};
template<class Get,class Set>
bool oak(int x,int y,int z,Random& random,Get get,Set set) {
    int height=random.next(3)+4;
    if(y<1 || y+height+1>256) return false;
    for(int level=y;level<=y+height+1;++level) {
        int radius=level==y?0:1;
        if(level>=y+height-1) radius=2;
        for(int xx=x-radius;xx<=x+radius;++xx)for(int zz=z-radius;zz<=z+radius;++zz){
            auto c=get(xx,level,zz);if(c!=Air && c!=Leaves && c!=Grass && c!=Dirt && c!=Log)return false;
        }
    }
    auto ground=get(x,y-1,z);
    if((ground!=Grass && ground!=Dirt) || y>=256-height-1)return false;
    set(x,y-1,z,Dirt);
    for(int level=y-3+height;level<=y+height;++level){
        int relative=level-(y+height),radius=1-relative/2;
        for(int xx=x-radius;xx<=x+radius;++xx)for(int zz=z-radius;zz<=z+radius;++zz){
            if(std::abs(xx-x)==radius && std::abs(zz-z)==radius && (random.next(2)==0 || relative==0))continue;
            auto c=get(xx,level,zz);if(c==Air || c==Leaves)set(xx,level,zz,Leaves);
        }
    }
    for(int level=0;level<height;++level){auto c=get(x,y+level,z);if(c==Air || c==Leaves)set(x,y+level,z,Log);}
    return true;
}
}
