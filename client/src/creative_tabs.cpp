#include <language.h>
#include <creative_tabs.h>
#include <algorithm>
#include <cctype>
namespace CreativeTabs {
const char* name(int tab){static const char* names[]={"Building Blocks","Decoration Blocks","Redstone","Transportation","Miscellaneous","Search Items","Foodstuffs","Tools","Combat","Brewing","Materials","Player Inventory"};return Language::tr(names[std::clamp(tab,0,11)]);}
int icon(int tab){static const int ids[]={BRICKS,GRASS,Items::REDSTONE_DUST,Items::MINECART,Items::WATER_BUCKET,Items::FLINT,Items::APPLE,Items::IRON_PICKAXE,Items::IRON_SWORD,Items::POTION,Items::IRON_INGOT,CRAFTING_TABLE};return ids[std::clamp(tab,0,11)];}
std::vector<int> entries(int tab,const std::string& search){
    std::vector<int> all;
    for(int id=1;id<BLOCK_COUNT;++id)if(Items::block(id))all.push_back(id);
    for(int id=Items::ITEM_START+1;id<Items::ITEM_END;++id)all.push_back(id);
    auto lower=Language::lower;
    std::string query=lower(search);std::vector<int> result;
    for(int id:all){bool decor=id==LEAVES || id==GRASS || id==TORCH || id==CRAFTING_TABLE || id==BOOKSHELF;
        decor|=id==Items::PAINTING || id==Items::ITEM_FRAME || id==Items::SIGN || id==Items::BANNER || id==Items::FLOWER_POT || id==Items::ARMOR_STAND || id==Items::BED;
        bool transport=id==Items::MINECART || id==Items::CHEST_MINECART || id==Items::FURNACE_MINECART || id==Items::TNT_MINECART || id==Items::HOPPER_MINECART || id==Items::COMMAND_BLOCK_MINECART || id==Items::OAK_BOAT || (id>=Items::SPRUCE_BOAT && id<=Items::DARK_OAK_BOAT) || id==Items::SADDLE || id==Items::CARROT_ON_A_STICK || id==Items::ELYTRA;
        bool food=Items::food_points(id)>0;
        bool brewing=id==Items::NETHER_WART || id==Items::BLAZE_POWDER || id==Items::MAGMA_CREAM || id==Items::GHAST_TEAR;
        brewing|=id==Items::POTION || id==Items::SPLASH_POTION || id==Items::LINGERING_POTION || id==Items::GLASS_BOTTLE || id==Items::DRAGON_BREATH || id==Items::FERMENTED_SPIDER_EYE || id==Items::BREWING_STAND || id==Items::CAULDRON || id==Items::GLISTERING_MELON;
        bool misc=id==Items::BUCKET || id==Items::WATER_BUCKET || id==Items::SNOWBALL;
        misc|=id==Items::LAVA_BUCKET || id==Items::MILK_BUCKET || id==Items::SPAWN_EGG || Items::spawn_kind(id)>=0 || (id>=Items::RECORD_13 && id<=Items::RECORD_WAIT);
        bool redstone=id==REDSTONE_BLOCK || id==Items::REDSTONE_DUST;
        redstone|=id==Items::REPEATER || id==Items::COMPARATOR || id==Items::OAK_DOOR || id==Items::IRON_DOOR || (id>=Items::SPRUCE_DOOR && id<=Items::DARK_OAK_DOOR);
        bool combat=Items::tool_kind(id)==Items::Tool::Sword || Items::armor_slot(id)>=0;
        combat|=id==Items::BOW || Items::projectile(id)==Items::Projectile::Arrow || id==Items::SHIELD || id==Items::TOTEM;
        bool tool=(Items::tool_kind(id)!=Items::Tool::None && !combat) || id==Items::FLINT_AND_STEEL || id==Items::FISHING_ROD || id==Items::CLOCK || id==Items::COMPASS;
        bool include=tab==5?(lower(Items::name(id)).find(query)!=std::string::npos || lower(Items::english_name(id)).find(query)!=std::string::npos):
            (tab==0?Items::block(id) && !decor && id!=REDSTONE_BLOCK:
             tab==1?decor:tab==2?redstone:tab==3?transport:tab==4?misc:tab==6?food || id==Items::RAW_FISH:tab==7?tool:tab==8?combat:
             tab==9?brewing:tab==10?!Items::block(id) && !food && !misc && !tool && !combat && !redstone && !brewing && !transport && !decor:false);
        if(include)result.push_back(id);
    }return result;
}
}

