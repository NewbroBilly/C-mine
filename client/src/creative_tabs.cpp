#include <creative_tabs.h>
#include <algorithm>
#include <cctype>
namespace CreativeTabs {
const char* name(int tab){static const char* names[]={"Building Blocks","Decoration Blocks","Redstone","Transportation","Miscellaneous","Search Items","Foodstuffs","Tools","Combat","Brewing","Materials","Player Inventory"};return names[std::clamp(tab,0,11)];}
int icon(int tab){static const int ids[]={BRICKS,GRASS,Items::REDSTONE_DUST,Items::BUCKET,Items::WATER_BUCKET,Items::FLINT,Items::APPLE,Items::IRON_PICKAXE,Items::IRON_SWORD,GLASS,Items::IRON_INGOT,CRAFTING_TABLE};return ids[std::clamp(tab,0,11)];}
std::vector<int> entries(int tab,const std::string& search){
    std::vector<int> all;
    for(int id=1;id<BLOCK_COUNT;++id)if(Items::block(id))all.push_back(id);
    for(int id=Items::ITEM_START+1;id<Items::ITEM_END;++id)all.push_back(id);
    auto lower=[](std::string s){for(auto& c:s)c=char(std::tolower(static_cast<unsigned char>(c)));return s;};
    std::string query=lower(search);std::vector<int> result;
    for(int id:all){bool decor=id==LEAVES || id==GRASS || id==TORCH || id==CRAFTING_TABLE || id==BOOKSHELF;
        bool food=Items::food_points(id)>0;
        bool brewing=id==Items::NETHER_WART || id==Items::BLAZE_POWDER || id==Items::MAGMA_CREAM || id==Items::GHAST_TEAR;
        bool misc=id==Items::BUCKET || id==Items::WATER_BUCKET || id==Items::SNOWBALL;
        bool redstone=id==REDSTONE_BLOCK || id==Items::REDSTONE_DUST;
        bool combat=Items::tool_kind(id)==Items::Tool::Sword;
        bool tool=(Items::tool_kind(id)!=Items::Tool::None && !combat) || id==Items::FLINT_AND_STEEL || id==Items::FISHING_ROD || id==Items::CLOCK || id==Items::COMPASS;
        bool include=tab==5?lower(Items::name(id)).find(query)!=std::string::npos:
            (tab==0?Items::block(id) && !decor && id!=REDSTONE_BLOCK:
             tab==1?decor:tab==2?redstone:tab==4?misc:tab==6?food || id==Items::RAW_FISH:tab==7?tool:tab==8?combat:
             tab==9?brewing:tab==10?!Items::block(id) && !food && !misc && !tool && !combat && !redstone && !brewing:false);
        if(include)result.push_back(id);
    }return result;
}
}

