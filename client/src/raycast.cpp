#include <raycast.h>
#include <voxel_trace.h>
HitInfo raycast(const vec3& origin,const vec3& dir,float max_distance,Chunk* chunk,bool sources) {
    HitInfo result{};
    if(!chunk) return result;
    Chunk* current=chunk;
    ivec3 local(0);
    bool unloaded=false;
    auto hit=trace_voxels(origin,dir,max_distance,[&](ivec3 cell) {
        if(cell.y<0 || cell.y>=CHUNK_HEIGHT) return false;
        local=cell-ivec3(chunk_pos_to_world(current->pos));
        Chunk* next=get_neighbor_chunk(current,local);
        if(!next) { unloaded=true;return true; }
        current=next;
        auto b=*current->get(local);return b.id!=AIR && (b.id!=WATER || (sources && (b.data1&15)==0));
    });
    if(hit.hit && !unloaded) {
        result.hit=true;result.chunk=current;result.block_pos=local;
        result.face_normal=hit.normal;
        result.hit_point=origin+normalize(dir)*hit.distance;
    }
    return result;
}
