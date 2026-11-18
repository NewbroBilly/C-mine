#pragma once
#include <cstdint>
#include <functional>
namespace Ores {
using Query=std::function<int(int,int,int)>;
using Write=std::function<void(int,int,int,int)>;
// Coordinates passed to callbacks are world coordinates, clipped to this chunk.
void generate(int chunk_x,int chunk_z,uint64_t seed,const Query& get,const Write& set);
}
