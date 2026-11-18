#pragma once
#include <glm.hpp>
#include <cmath>
#include <limits>
struct VoxelHit { bool hit=false; glm::ivec3 cell{0},normal{0}; float distance=0; };
template<class Query>
VoxelHit trace_voxels(glm::vec3 origin,glm::vec3 direction,float reach,Query occupied) {
    VoxelHit hit;
    float magnitude=glm::length(direction);
    if (!(magnitude>0) || !std::isfinite(magnitude) || !(reach>=0)) return hit;
    direction/=magnitude;
    glm::ivec3 cell(glm::floor(origin)),step(0);
    glm::vec3 next(std::numeric_limits<float>::infinity()),stride=next;
    for(int axis=0;axis<3;++axis) if(direction[axis]!=0) {
        step[axis]=direction[axis]>0?1:-1;
        stride[axis]=std::abs(1.f/direction[axis]);
        float boundary=cell[axis]+(step[axis]>0?1.f:0.f);
        next[axis]=(boundary-origin[axis])/direction[axis];
    }
    for(;;) {
        int axis=next.x<=next.y && next.x<=next.z?0:(next.y<=next.z?1:2);
        float distance=next[axis];
        // Check reach before inspecting the next cell, including axis-aligned rays.
        if(distance>reach || !std::isfinite(distance)) return hit;
        cell[axis]+=step[axis];next[axis]+=stride[axis];
        if(occupied(cell)) {
            hit.hit=true;hit.cell=cell;hit.normal[axis]=-step[axis];hit.distance=distance;
            return hit;
        }
    }
}
