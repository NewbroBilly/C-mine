#pragma once
#include <string>
namespace Settings {
struct Options {
    int fov=80, sensitivity=100, distance=4;
    bool vsync=true, ao=true, fullscreen=false, invert_y=false;
    bool russian=true;
    bool operator==(const Options& b) const {return fov==b.fov && sensitivity==b.sensitivity && distance==b.distance && vsync==b.vsync && ao==b.ao && fullscreen==b.fullscreen && invert_y==b.invert_y && russian==b.russian;}
};
extern Options options;
extern std::string storage_override;
void validate(Options& value);
bool load(const std::string& path,Options& value);
bool save(const std::string& path,const Options& value);
}
