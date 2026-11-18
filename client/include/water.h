#pragma once
#include <array>
#include <functional>
#include <map>
#include <queue>
#include <cstdint>
namespace Water {
using Pos=std::array<int,3>;
constexpr int Solid=-1, Unloaded=-2, Air=16;
using Query=std::function<int(Pos)>;
using Write=std::function<void(Pos,int)>;
bool liquid(int level);
double height(int level);
bool contains(std::array<double,3> point,const Query& get);
std::array<double,3> flow(Pos p,const Query& get);
// BlockDynamicLiquid water rules, scheduled every five 20 Hz game ticks.
class Simulator {
    struct Event {uint64_t time;Pos p;bool operator<(const Event& b)const{return time>b.time;}};
    uint64_t clock=0;
    std::priority_queue<Event> queue;
    std::map<Pos,uint64_t> scheduled;
    void update(Pos p,const Query& get,const Write& set);
public:
    void schedule(Pos p);
    void changed(Pos p);
    void tick(const Query& get,const Write& set,int budget=1024);
    size_t pending()const{return scheduled.size();}
};
}
