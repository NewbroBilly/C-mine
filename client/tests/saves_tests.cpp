#include <saves.h>
#include <creative_tabs.h>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>
static void check(bool c,const char* s){if(!c){std::cerr<<s<<'\n';std::exit(1);}}
int main(){
    auto root=Saves::unique_path(std::filesystem::temp_directory_path()/"cmine-save-tests");auto one=root/"one",two=root/"two";
    Saves::Data d;d.name="Quoted \"World\"";d.seed=-1726354;d.survival=false;d.spawned=true;d.position={-20.25,123,-.25};d.inventory.main[0]={Items::WATER_BUCKET,1};d.creative.main[8]={Items::IRON_PICKAXE,1,23};d.hotbar=8;d.air=120;d.flying=true;d.velocity={1.2,-.3,0};d.vitals.health=7;d.vitals.food=10;d.drops.push_back({{DIRT,3},{-1.5,23,5},{1,2,3},12});
    check(Saves::save(one,d),"write first world");Saves::Data loaded;check(Saves::load(one,loaded),"load first world");
    check(loaded.name==d.name && loaded.seed==d.seed && loaded.position==d.position && loaded.inventory.main==d.inventory.main && loaded.creative.main==d.creative.main && loaded.air==120 && loaded.flying && loaded.velocity==d.velocity && loaded.drops.size()==1 && loaded.drops[0].stack.count==3,"saved player data roundtrip");
    d.name="Second World";d.seed=99;check(Saves::save(two,d),"write independent world");check(Saves::list(root).size()==2,"world selection must list separate worlds");
    d.day_ticks=18345.5;d.spawn_point={-15.5,103,20.5};d.inventory.main[1]={Items::GOLD_HOE,1,17};
    check(Saves::save(two,d) && Saves::load(two,loaded) && loaded.day_ticks==d.day_ticks && loaded.spawn_point==d.spawn_point && loaded.inventory.main[1]==d.inventory.main[1],"new tools, clock and compass save roundtrip");
    auto legacy=root/"legacy";std::filesystem::create_directories(legacy);
    {std::ifstream f(one/"world.txt");std::string bytes((std::istreambuf_iterator<char>(f)),{});bytes[6]='1';for(int i=0;i<3;++i)bytes.erase(bytes.rfind('\n',bytes.size()-2)+1);std::ofstream out(legacy/"world.txt");out<<bytes;}
    check(Saves::load(legacy,loaded) && loaded.inventory.main[0].id==Items::WATER_BUCKET && loaded.day_ticks==6000,"legacy save accepted");
    {std::ifstream f(one/"world.txt");std::string bytes((std::istreambuf_iterator<char>(f)),{});bytes[6]='2';for(int i=0;i<2;++i)bytes.erase(bytes.rfind('\n',bytes.size()-2)+1);std::ofstream out(legacy/"world.txt");out<<bytes;}
    check(Saves::load(legacy,loaded) && loaded.generator_version==0,"version 2 worlds keep the old generator");
    auto localized=root/"localized";auto russian=d;russian.name="Мой мир \"Берёзы\"";russian.generator_version=1;
    check(Saves::save(localized,russian) && Saves::load(localized,loaded) && loaded.name==russian.name && loaded.generator_version==1,"Russian name and generation version roundtrip");
    auto entities=russian;Mobs::Entity sheep;sheep.kind=Mobs::Sheep;sheep.position={-22.5,117,54.25};sheep.health=5;sheep.sheared=true;sheep.yaw=1.3;sheep.random=0xabc123;
    entities.mobs.push_back(sheep);entities.initial_mobs=true;entities.inventory.armor[2]={Items::DIAMOND_LEGGINGS,1,101};
    auto mobworld=root/"entities";
    check(Saves::save(mobworld,entities) && Saves::load(mobworld,loaded) && loaded.initial_mobs && loaded.mobs.size()==1 && loaded.mobs[0].position==sheep.position && loaded.mobs[0].health==5 && loaded.mobs[0].sheared && loaded.mobs[0].random==sheep.random && loaded.inventory.armor==entities.inventory.armor,"living mobs, sheep shearing and worn armor roundtrip");
    std::vector<unsigned char> blocks(16*256*16*3,0);blocks[0]=BEDROCK;blocks[3]=WATER;blocks[4]=9;blocks[blocks.size()-3]=GLOWSTONE;
    check(Saves::save_chunk(one,-2,-1,blocks.data(),blocks.size()),"write negative chunk");auto copy=blocks;std::fill(copy.begin(),copy.end(),42);
    check(Saves::load_chunk(one,-2,-1,copy.data(),copy.size()) && copy==blocks,"chunk RLE roundtrip including water metadata");
    int revision=0;check(Saves::load_chunk(one,-2,-1,copy.data(),copy.size(),&revision) && revision==2,"new chunks carry the completed cactus migration revision");
    {std::fstream f(one/"chunks/-2_-1.cmc",std::ios::in|std::ios::out|std::ios::binary);f.seekp(3);f.put('1');}
    check(Saves::load_chunk(one,-2,-1,copy.data(),copy.size(),&revision) && revision==1 && copy==blocks,"legacy chunk format remains readable for migration");
    check(!Saves::load_chunk(two,-2,-1,copy.data(),copy.size()),"world chunk isolation");
    {std::ofstream f(one/"chunks/-3_0.cmc",std::ios::binary);f<<"CMC1garbage";}auto before=copy;
    check(!Saves::load_chunk(one,-3,0,copy.data(),copy.size()) && copy==before,"truncated chunk must leave destination intact");
    d.name="Updated";check(Saves::save(one,d),"update save with backup");{std::ofstream f(one/"world.txt");f<<"CMINE 1\ncorrupt";}
    check(Saves::load(one,loaded) && loaded.name=="Quoted \"World\"","corrupt metadata must recover previous atomic backup");
    for(const char* file:{"world.txt","world.txt.bak"}){std::ofstream f(two/file);f<<"unrecognized";}check(!Saves::load(two,loaded),"invalid world must be rejected");
    check(Saves::seed_from_text("-12345")==-12345 && Saves::seed_from_text("abc")==96354 && Saves::seed_from_text("\xf0\x9f\x98\x80")==1772899,"numeric and Java UTF-16 seed parsing");
    std::vector<int> supported;
    for(int t:{0,1,2,3,4,6,7,8,9,10}){auto entries=CreativeTabs::entries(t);supported.insert(supported.end(),entries.begin(),entries.end());}
    for(int id:CreativeTabs::entries(5))check(std::find(supported.begin(),supported.end(),id)!=supported.end(),"every item must have a creative category");
    auto search=CreativeTabs::entries(5,"WATER");check(search.size()==1 && search[0]==Items::WATER_BUCKET,"creative search must filter case insensitively");
    std::filesystem::remove_all(root);
    std::cout<<"Save roundtrip, isolated worlds, chunks, corruption recovery, seeds and creative categories passed\n";
}
