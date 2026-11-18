#include <water.h>
#include <algorithm>
#include <cmath>
namespace Water {
namespace {
const Pos directions[]={{-1,0,0},{1,0,0},{0,0,-1},{0,0,1}};
Pos add(Pos a,Pos b){for(int i=0;i<3;++i)a[i]+=b[i];return a;}
bool open(int v){return v==Air || (liquid(v) && v!=0);}
int cost(Pos p,int depth,int back,const Query& get){
    int best=1000;
    for(int i=0;i<4;++i){if(i==back)continue;Pos n=add(p,directions[i]);if(!open(get(n)))continue;
        int below=get(add(n,{0,-1,0}));if(below!=Solid && below!=Unloaded)return depth;
        if(depth<4)best=std::min(best,cost(n,depth+1,i^1,get));
    }return best;
}
}
bool liquid(int v){return v>=0 && v<16;}
double height(int v){return liquid(v)?1.-((v>=8?0:v)+1)/9.:0.;}
bool contains(std::array<double,3> point,const Query& get){
    Pos p{int(std::floor(point[0])),int(std::floor(point[1])),int(std::floor(point[2]))};
    int level=get(p);
    if(!liquid(level))return false;
    // Only the exposed surface has a lowered liquid height. Applying that
    // height inside a column creates an air gap at every block boundary.
    return liquid(get(add(p,{0,1,0}))) || point[1]<p[1]+height(level);
}
std::array<double,3> flow(Pos p,const Query& get){
    int raw=get(p);if(!liquid(raw))return {0,0,0};int level=raw>=8?0:raw;
    std::array<double,3> v{};
    for(auto d:directions){auto n=add(p,d);int l=get(n),delta=0;
        if(liquid(l))delta=(l>=8?0:l)-level;
        else if(l==Air){int below=get(add(n,{0,-1,0}));if(liquid(below))delta=(below>=8?0:below)-(level-8);}
        for(int i=0;i<3;++i)v[i]+=d[i]*delta;
    }
    auto normalize=[&](){double n=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(n>1e-9)for(auto& a:v)a/=n;};
    if(raw>=8)for(auto d:directions)if(get(add(p,d))==Solid || get(add(add(p,d),{0,1,0}))==Solid){normalize();v[1]-=6;break;}
    normalize();return v;
}
void Simulator::schedule(Pos p){if(p[1]<0 || p[1]>=256 || scheduled.count(p))return;scheduled[p]=clock+5;queue.push({clock+5,p});}
void Simulator::changed(Pos p){schedule(p);for(auto d:directions)schedule(add(p,d));schedule(add(p,{0,1,0}));schedule(add(p,{0,-1,0}));}
void Simulator::tick(const Query& get,const Write& set,int budget){
    ++clock;while(budget-->0 && !queue.empty() && queue.top().time<=clock){auto e=queue.top();queue.pop();scheduled.erase(e.p);if(liquid(get(e.p)))update(e.p,get,set);}
}
void Simulator::update(Pos p,const Query& get,const Write& set){
    int level=get(p);auto write=[&](Pos n,int l){if(get(n)==l)return;set(n,l);changed(n);};
    if(level>0){
        int minimum=1000,sources=0;bool unknown=false;
        for(auto d:directions){int l=get(add(p,d));if(l==Unloaded)unknown=true;if(!liquid(l))continue;if(l==0)++sources;minimum=std::min(minimum,l>=8?0:l);}
        int next=minimum+1;if(next>=8)next=Air;
        int above=get(add(p,{0,1,0}));if(above==Unloaded)unknown=true;
        if(liquid(above))next=above>=8?above:above+8;
        int below=get(add(p,{0,-1,0}));if(sources>=2 && (below==Solid || below==0))next=0;
        // Freeze the loaded frontier until the neighboring chunk arrives.
        if(unknown){schedule(p);return;}
        if(next!=level){write(p,next);level=next;if(level==Air){auto down=add(p,{0,-1,0});if(get(down)==Air)write(down,7);return;}}
    }
    Pos down=add(p,{0,-1,0});int below=get(down);
    if(below==Unloaded){schedule(p);return;}
    // canFlowInto rejects existing water, including non-source water. Its own
    // scheduled update derives metadata from the block above instead.
    if(below==Air){write(down,level>=8?level:level+8);return;}
    // A non-source flowing block on water does not fan out in midair.
    if(level!=0 && below!=Solid)return;
    int next=level>=8?1:level+1;if(next>=8)return;
    int costs[4],best=1000;
    for(int i=0;i<4;++i){auto n=add(p,directions[i]);costs[i]=1000;if(!open(get(n)))continue;
        int down_level=get(add(n,{0,-1,0}));costs[i]=down_level!=Solid && down_level!=Unloaded?0:cost(n,1,i^1,get);best=std::min(best,costs[i]);}
    for(int i=0;i<4;++i){auto n=add(p,directions[i]);if(costs[i]==best && get(n)==Air)write(n,next);}
}
}
