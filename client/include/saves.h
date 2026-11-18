#pragma once
#include <items.h>
#include <array>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>
namespace Saves {
struct Data {
    std::string name="New World";
    int64_t seed=0;
    bool survival=true,spawned=false;
    std::array<double,3> position{8.5,122,8.5},forward{0,0,1};
    std::array<double,3> velocity{};
    bool flying=false;
    double day_ticks=6000;
    std::array<double,3> spawn_point{8.5,122,8.5};
    Items::Vitals vitals;
    Items::Inventory inventory,creative;
    int hotbar=0,air=300;
    struct Drop {Items::Stack stack;std::array<double,3> position,velocity;double age;};
    std::vector<Drop> drops;
};
struct Entry {std::filesystem::path path;Data data;};
extern std::string root_override;
bool load(const std::filesystem::path& path,Data& data);
bool save(const std::filesystem::path& path,const Data& data);
std::vector<Entry> list(const std::filesystem::path& root);
std::filesystem::path unique_path(const std::filesystem::path& root);
int64_t seed_from_text(const std::string& text);
// RLE block snapshots, separate from lighting/meshes. A failed read leaves data intact.
bool save_chunk(const std::filesystem::path& root,int x,int z,const unsigned char* data,size_t size);
bool load_chunk(const std::filesystem::path& root,int x,int z,unsigned char* data,size_t size);
}
