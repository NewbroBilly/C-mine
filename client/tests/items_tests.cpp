#include <items.h>
#include <iostream>
#include <stdexcept>
using namespace Items;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int total(const Inventory& inv,int id){int n=0;for(auto& s:inv.main)if(s.id==id)n+=s.count;return n;}
int main(){try{
    Stack a{DIRT,5},cursor;click(a,cursor,true);
    require(a.count==2 && cursor.count==3,"Right click takes the larger half");
    click(a,cursor,true);require(a.count==3 && cursor.count==2,"Right click deposits one");
    a={DIRT,63};cursor={DIRT,5};click(a,cursor,false);
    require(a.count==64 && cursor.count==4,"Stack cap retains cursor remainder");
    a={STONE,4};click(a,cursor,false);require(a.id==DIRT && cursor.id==STONE,"Left click swaps different items");
    Stack tool{WOOD_PICKAXE,1,4},other{WOOD_PICKAXE,1,8};click(tool,other,false);
    require(tool.damage==8 && other.damage==4,"Tool durability survives swaps");
    Inventory inv;inv.main[0]={DIRT,63};inv.main[1]={DIRT,64};
    require(inv.add({DIRT,70})==0 && inv.main[0].count==64 && inv.main[2].count==64 && inv.main[3].count==5,"Insertion merges and splits");
    for(auto& s:inv.main)s={STONE,64};require(inv.add({DIRT,5})==5,"Full inventory returns all leftovers");
    inv.main[9]={DIRT,10};inv.main[0]={DIRT,60};inv.quick_move(9);
    require(inv.main[0].count==64 && inv.main[9].count==6,"Quick move conserves items with partial capacity");
    inv={};inv.craft[3]={LOG,2};require(inv.take_result(2,false),"Offset one-log recipe");
    require(inv.cursor.id==PLANKS && inv.cursor.count==4 && inv.craft[3].count==1,"Craft consumes one ingredient");
    inv.cursor={DIRT,1};require(!inv.take_result(2,false) && inv.craft[3].count==1,"Blocked result never consumes inputs");
    inv.cursor.clear();inv.close();require(total(inv,LOG)==1,"Closing returns crafting ingredients");
    inv={};inv.craft[0]={PLANKS,2};inv.craft[2]={BIRCH_PLANKS,3};
    require(recipe(inv.craft,2).output.id==STICK,"Mixed planks make sticks vertically");
    inv.craft[1]={DIRT,1};require(recipe(inv.craft,2).output.empty(),"Extra ingredients invalidate recipe");
    inv={};for(int i=0;i<4;++i)inv.craft[i]={PLANKS,1};
    require(inv.take_result(2,true) && total(inv,CRAFTING_TABLE)==1,"2x2 crafting table");
    inv={};for(int i=0;i<3;++i)inv.craft[i]={PLANKS,1};inv.craft[4]={STICK,1};inv.craft[7]={STICK,1};
    require(recipe(inv.craft,3).output.id==WOOD_PICKAXE,"3x3 wooden pickaxe");
    require(inv.take_result(3,true) && total(inv,WOOD_PICKAXE)==1,"Crafting tool into inventory");
    inv={};for(auto& s:inv.main)s={STONE,64};inv.cursor={DIRT,3};inv.craft[0]={LOG,1};
    auto leftovers=inv.close();require(leftovers.size()==2 && leftovers[0].count==3 && inv.cursor.empty() && inv.craft[0].empty(),"Full inventory closure returns drops");
    require(!harvestable(STONE,-1) && harvestable(STONE,WOOD_PICKAXE),"Stone requires pickaxe");
    require(!harvestable(DIAMOND_ORE,STONE_PICKAXE) && harvestable(DIAMOND_ORE,IRON_PICKAXE),"Diamond requires iron tier");
    require(break_seconds(DIRT,-1,true)==.75,"Bare hand dirt reference duration");
    require(break_seconds(STONE,WOOD_PICKAXE,true)<break_seconds(STONE,-1,true),"Pickaxe speeds stone mining");
    require(drop(STONE,-1,0).empty() && drop(STONE,WOOD_PICKAXE,0).id==COBBLESTONE,"Correct stone loot");
    require(drop(BEDROCK,DIAMOND_PICKAXE,0).empty(),"No survival bedrock loot");
    Vitals v;v.landing(4.5,false);v.landing(.5,true);require(v.health==18,"Five-block fall deals two damage");
    v={};v.landing(3,true);require(v.health==20,"Three-block fall is safe");
    v.food=10;v.saturation=0;require(v.eat(APPLE) && v.food==14 && v.saturation>2.3,"Apple nutrition");
    v={};require(!v.eat(APPLE),"Cannot eat when full");v.saturation=0;v.exhaust(4.1f);v.tick();require(v.food==19,"Exhaustion depletes food after saturation");
    v={};v.health=18;for(int i=0;i<10;++i)v.tick();require(v.health>18 && v.exhaustion>0,"Saturated natural regeneration");
    v={};v.food=0;v.saturation=0;for(int i=0;i<160;++i)v.tick();require(v.health==18,"Normal starvation every 80 ticks");
    std::cout<<"Inventory conservation, slot clicks, recipes, harvesting, fall damage and food passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
