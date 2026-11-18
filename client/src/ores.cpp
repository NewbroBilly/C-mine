#include <ores.h>
#include <oak_tree.h>
#include <block_ids.h>
#include <algorithm>
#include <cmath>
namespace Ores {
namespace {
struct Deposit {int id,count,size,min,max;bool triangular;};
const Deposit deposits[]={
    {COAL_ORE,20,17,0,128,false},{IRON_ORE,20,9,0,64,false},
    {GOLD_ORE,2,9,0,32,false},{REDSTONE_ORE,8,8,0,16,false},
    {DIAMOND_ORE,1,8,0,16,false},{LAPIS_ORE,1,7,0,16,true}
};
double unit(Trees::Random& random){return random.bits(24)/16777216.;}
template<class Put>
void vein(int x,int y,int z,int id,int size,Trees::Random& random,Put put){
    // WorldGenMinable's overlapping ellipsoids, including its +8 offset.
    double angle=unit(random)*3.141592653589793;
    double x0=x+8+std::sin(angle)*size/8.,x1=x+8-std::sin(angle)*size/8.;
    double z0=z+8+std::cos(angle)*size/8.,z1=z+8-std::cos(angle)*size/8.;
    double y0=y+random.next(3)-2,y1=y+random.next(3)-2;
    for(int i=0;i<size;++i){
        double t=double(i)/size,xx=x0+(x1-x0)*t,yy=y0+(y1-y0)*t,zz=z0+(z1-z0)*t;
        double radius=((std::sin(t*3.141592653589793)+1)*unit(random)*size/16.+1)/2.;
        for(int bx=int(std::floor(xx-radius));bx<=int(std::floor(xx+radius));++bx){
            double dx=(bx+.5-xx)/radius;if(dx*dx>=1)continue;
            for(int by=std::max(1,int(std::floor(yy-radius)));by<=std::min(255,int(std::floor(yy+radius)));++by){
                double dy=(by+.5-yy)/radius;if(dx*dx+dy*dy>=1)continue;
                for(int bz=int(std::floor(zz-radius));bz<=int(std::floor(zz+radius));++bz){
                    double dz=(bz+.5-zz)/radius;if(dx*dx+dy*dy+dz*dz<1)put(bx,by,bz,id);
                }
            }
        }
    }
}
}
void generate(int cx,int cz,uint64_t seed,const Query& get,const Write& set){
    auto put=[&](int x,int y,int z,int id){
        if(x>=cx*16 && x<cx*16+16 && z>=cz*16 && z<cz*16+16 && get(x,y,z)==STONE)set(x,y,z,id);
    };
    // Replay owner chunks in a fixed order. Features reach only towards the
    // positive neighbors; clipping keeps veins independent of load order.
    for(int gx=cx-1;gx<=cx;++gx)for(int gz=cz-1;gz<=cz;++gz){
        Trees::Random random(seed ^ (uint64_t(int64_t(gx))*341873128712ULL) ^ (uint64_t(int64_t(gz))*132897987541ULL) ^ 0x6f726573ULL);
        for(auto d:deposits)for(int i=0;i<d.count;++i){
            int x=gx*16+random.next(16),y=d.min+random.next(d.max-d.min),z=gz*16+random.next(16);
            if(d.triangular)y+=random.next(d.max);
            vein(x,y,z,d.id,d.size,random,put);
        }
        // There are no biome types yet: rare emerald pockets use all terrain.
        int emeralds=3+random.next(6);
        for(int i=0;i<emeralds;++i){int x=gx*16+8+random.next(16),y=4+random.next(28),z=gz*16+8+random.next(16);put(x,y,z,EMERALD_ORE);}
    }
}
}
