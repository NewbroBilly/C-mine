#pragma once
#include <cstdint>
#include <vector>
#include <array>
namespace Generation {
// x/y/z layout, three bytes per cell, matching the existing chunk save format.
std::vector<unsigned char> generate(uint64_t seed,int cx,int cz);
void clear_cache();
bool find_spawn(uint64_t seed,std::array<double,3>& position);
}
