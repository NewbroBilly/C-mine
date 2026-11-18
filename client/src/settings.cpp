#include <settings.h>
#include <algorithm>
#include <fstream>
#include <filesystem>
namespace Settings {
Options options;
std::string storage_override;
void validate(Options& o){o.fov=std::clamp(o.fov,50,110);o.sensitivity=std::clamp(o.sensitivity,10,200);o.distance=std::clamp(o.distance,2,8);}
bool load(const std::string& path,Options& o){
    std::ifstream in(path);if(!in)return false;
    Options result;std::string line;
    while(std::getline(in,line)){
        auto equal=line.find('=');if(equal==std::string::npos)continue;
        std::string key=line.substr(0,equal);int value;
        try{size_t count=0;value=std::stoi(line.substr(equal+1),&count);if(count!=line.size()-equal-1)continue;}catch(...){continue;}
        if(key=="fov")result.fov=value;else if(key=="sensitivity")result.sensitivity=value;
        else if(key=="distance")result.distance=value;else if(key=="vsync")result.vsync=value!=0;
        else if(key=="ao")result.ao=value!=0;else if(key=="fullscreen")result.fullscreen=value!=0;
        else if(key=="invert_y")result.invert_y=value!=0;
    }
    validate(result);o=result;return true;
}
bool save(const std::string& path,const Options& input){
    Options o=input;validate(o);std::error_code ec;
    auto parent=std::filesystem::path(path).parent_path();if(!parent.empty())std::filesystem::create_directories(parent,ec);
    std::ofstream out(path,std::ios::trunc);if(!out)return false;
    out<<"fov="<<o.fov<<"\nsensitivity="<<o.sensitivity<<"\ndistance="<<o.distance<<"\nvsync="<<o.vsync<<"\nao="<<o.ao<<"\nfullscreen="<<o.fullscreen<<"\ninvert_y="<<o.invert_y<<'\n';
    out.close();return bool(out);
}
}
