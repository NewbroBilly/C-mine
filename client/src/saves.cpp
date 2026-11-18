#include <saves.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <algorithm>
namespace Saves {
std::string root_override;
namespace {
bool commit(const std::filesystem::path& path,const std::string& bytes){
    std::error_code ec;std::filesystem::create_directories(path.parent_path(),ec);if(ec)return false;
    auto temp=path;temp+=".tmp";auto backup=path;backup+=".bak";
    {std::ofstream f(temp,std::ios::binary|std::ios::trunc);if(!f)return false;f.write(bytes.data(),bytes.size());f.flush();if(!f)return false;}
    if(std::filesystem::exists(path)){
        std::filesystem::copy_file(path,backup,std::filesystem::copy_options::overwrite_existing,ec);if(ec)return false;
        std::filesystem::remove(path,ec);if(ec)return false;
    }
    std::filesystem::rename(temp,path,ec);return !ec;
}
void stack(std::ostream& f,const Items::Stack& s){f<<s.id<<' '<<s.count<<' '<<s.damage<<' ';}
bool stack(std::istream& f,Items::Stack& s){
    if(!(f>>s.id>>s.count>>s.damage))return false;
    if(s.id==-1)return s.count==0 && s.damage==0;
    return Items::valid(s.id) && s.count>0 && s.count<=Items::limit(s.id) && s.damage>=0 && s.damage<=Items::durability(s.id);
}
void inventory(std::ostream& f,const Items::Inventory& d){for(auto s:d.main)stack(f,s);for(auto s:d.craft)stack(f,s);for(auto s:d.armor)stack(f,s);stack(f,d.offhand);stack(f,d.cursor);f<<'\n';}
bool inventory(std::istream& f,Items::Inventory& d){for(auto& s:d.main)if(!stack(f,s))return false;for(auto& s:d.craft)if(!stack(f,s))return false;for(auto& s:d.armor)if(!stack(f,s))return false;return stack(f,d.offhand) && stack(f,d.cursor);}
bool read(const std::filesystem::path& file,Data& result){
    std::ifstream f(file);std::string magic;int version;Data d;
    if(!(f>>magic>>version) || magic!="CMINE" || (version!=1 && version!=2))return false;
    if(!(f>>std::quoted(d.name)>>d.seed>>d.survival>>d.spawned) || d.name.empty() || d.name.size()>80)return false;
    for(auto& v:d.position)if(!(f>>v) || !std::isfinite(v) || std::abs(v)>30000000)return false;
    double length=0;for(auto& v:d.forward){if(!(f>>v) || !std::isfinite(v))return false;length+=v*v;}if(length<.99 || length>1.01 || d.forward[0]*d.forward[0]+d.forward[2]*d.forward[2]<1e-12)return false;
    for(auto& n:d.velocity)if(!(f>>n) || !std::isfinite(n) || std::abs(n)>10000)return false;
    if(!(f>>d.flying))return false;
    auto& v=d.vitals;
    if(!(f>>v.health>>v.food>>v.saturation>>v.exhaustion>>v.timer>>v.fallen>>d.hotbar>>d.air))return false;
    if(!std::isfinite(v.health) || !std::isfinite(v.saturation) || !std::isfinite(v.exhaustion) || !std::isfinite(v.fallen) || v.health<0 || v.health>20 || v.food<0 || v.food>20 || v.saturation<0 || v.saturation>20 || v.exhaustion<0 || v.exhaustion>40 || v.timer<0 || v.timer>80 || v.fallen<0 || d.hotbar<0 || d.hotbar>8 || d.air< -20 || d.air>300)return false;
    if(!inventory(f,d.inventory) || !inventory(f,d.creative))return false;
    int count;if(!(f>>count) || count<0 || count>100000)return false;
    for(int i=0;i<count;++i){Data::Drop a;if(!stack(f,a.stack))return false;
        for(auto& n:a.position)if(!(f>>n) || !std::isfinite(n) || std::abs(n)>30000000)return false;
        for(auto& n:a.velocity)if(!(f>>n) || !std::isfinite(n) || std::abs(n)>10000)return false;
        if(!(f>>a.age) || !std::isfinite(a.age) || a.age< -1 || a.age>301)return false;d.drops.push_back(a);
    }
    if(version>=2){
        if(!(f>>d.day_ticks) || !std::isfinite(d.day_ticks) || d.day_ticks<0 || d.day_ticks>=24000)return false;
        for(auto& n:d.spawn_point)if(!(f>>n) || !std::isfinite(n) || std::abs(n)>30000000)return false;
    }
    f>>std::ws;if(!f.eof())return false;result=d;return true;
}
std::filesystem::path chunk_path(const std::filesystem::path& root,int x,int z){return root/"chunks"/(std::to_string(x)+"_"+std::to_string(z)+".cmc");}
}
bool load(const std::filesystem::path& path,Data& data){return read(path/"world.txt",data) || read(path/"world.txt.bak",data);}
bool save(const std::filesystem::path& path,const Data& d){
    std::ostringstream f;f<<std::setprecision(17)<<"CMINE 2\n"<<std::quoted(d.name)<<'\n'<<d.seed<<' '<<d.survival<<' '<<d.spawned<<'\n';
    for(auto v:d.position)f<<v<<' ';for(auto v:d.forward)f<<v<<' ';for(auto v:d.velocity)f<<v<<' ';f<<d.flying;
    const auto& v=d.vitals;f<<'\n'<<v.health<<' '<<v.food<<' '<<v.saturation<<' '<<v.exhaustion<<' '<<v.timer<<' '<<v.fallen<<' '<<d.hotbar<<' '<<d.air<<'\n';
    inventory(f,d.inventory);inventory(f,d.creative);f<<d.drops.size()<<'\n';
    for(auto& a:d.drops){stack(f,a.stack);for(auto n:a.position)f<<n<<' ';for(auto n:a.velocity)f<<n<<' ';f<<a.age<<'\n';}
    f<<d.day_ticks<<' ';for(auto n:d.spawn_point)f<<n<<' ';f<<'\n';
    return commit(path/"world.txt",f.str());
}
std::vector<Entry> list(const std::filesystem::path& root){
    std::vector<Entry> entries;std::error_code ec;
    for(std::filesystem::directory_iterator i(root,ec),end;i!=end && !ec;i.increment(ec))if(i->is_directory(ec)){Data d;if(load(i->path(),d))entries.push_back({i->path(),d});}
    std::sort(entries.begin(),entries.end(),[](const Entry& a,const Entry& b){return a.path.filename().string()>b.path.filename().string();});return entries;
}
std::filesystem::path unique_path(const std::filesystem::path& root){
    auto id=std::chrono::system_clock::now().time_since_epoch().count();auto path=root/std::to_string(id);
    while(std::filesystem::exists(path))path=root/std::to_string(++id);return path;
}
int64_t seed_from_text(const std::string& text){
    if(text.empty())return std::chrono::high_resolution_clock::now().time_since_epoch().count();
    try{size_t n=0;auto value=std::stoll(text,&n);if(n==text.size())return value;}catch(...){}
    // Java String.hashCode operates on UTF-16 code units.
    uint32_t hash=0;for(size_t i=0;i<text.size();){uint32_t c=(unsigned char)text[i++];int n=0;if(c>=0xf0){c&=7;n=3;}else if(c>=0xe0){c&=15;n=2;}else if(c>=0xc0){c&=31;n=1;}while(n-- && i<text.size())c=(c<<6)|((unsigned char)text[i++]&63);
        if(c>0xffff){c-=0x10000;hash=hash*31+(0xd800+(c>>10));c=0xdc00+(c&1023);}hash=hash*31+c;}
    return int32_t(hash);
}
bool save_chunk(const std::filesystem::path& root,int x,int z,const unsigned char* data,size_t size){
    if(size%3)return false;std::ostringstream f(std::ios::binary);f.write("CMC1",4);
    for(size_t i=0;i<size;){uint32_t count=1;while(i+count*3<size && count<65535 && std::equal(data+i,data+i+3,data+i+count*3))++count;
        unsigned char length[]={static_cast<unsigned char>(count&255),static_cast<unsigned char>(count>>8)};f.write((char*)length,2);f.write((const char*)data+i,3);i+=count*3;}
    return commit(chunk_path(root,x,z),f.str());
}
bool load_chunk(const std::filesystem::path& root,int x,int z,unsigned char* data,size_t size){
    auto path=chunk_path(root,x,z);
    for(int attempt=0;attempt<2;++attempt){std::ifstream f(path,std::ios::binary);char magic[4];std::vector<unsigned char> out;out.reserve(size);
        if(f.read(magic,4) && std::string(magic,4)=="CMC1")while(out.size()<size){unsigned char b[5];if(!f.read((char*)b,5))break;size_t count=b[0]+(unsigned(b[1])<<8);if(!count || out.size()+count*3>size)break;for(size_t i=0;i<count;++i)out.insert(out.end(),b+2,b+5);}
        if(out.size()==size && f.peek()==EOF){std::copy(out.begin(),out.end(),data);return true;}path=chunk_path(root,x,z);path+=".bak";
    }return false;
}
}
