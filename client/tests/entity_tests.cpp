#include <mobs.h>
#include <items.h>
#include <iostream>
#include <stdexcept>
#include <cmath>
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){try{
    using namespace Items;
    std::array<Stack,4> armor{};
    for(int m=0;m<5;++m)for(int part=0;part<4;++part){int id=LEATHER_HELMET+m*4+part;
        check(armor_slot(id)==part && armor_material(id)==m && limit(id)==1 && durability(id)>50,"armor catalog, slots and durability");}
    Stack helmet{DIAMOND_HELMET,1};check(equip(armor,helmet) && helmet.empty(),"right click and quick equip consume the source");
    Stack spare{IRON_HELMET,1};check(!equip(armor,spare) && spare.id==IRON_HELMET,"equip cannot overwrite occupied slot");
    armor_click(armor[0],spare,0,false);check(armor[0].id==IRON_HELMET && spare.id==DIAMOND_HELMET,"armor swaps preserve both items");
    Stack dirt{DIRT,64};armor_click(armor[0],dirt,0,false);check(armor[0].id==IRON_HELMET && dirt.count==64,"armor slot rejects blocks");
    Stack boots{IRON_BOOTS,1};armor_click(armor[0],boots,0,false);check(boots.id==IRON_BOOTS && armor[0].id==IRON_HELMET,"armor slot rejects wrong part");
    for(int i=0;i<4;++i)armor[i]={DIAMOND_HELMET+i,1};
    check(std::abs(absorb_damage(armor,10)-3.f)<.001,"1.12 toughness formula: 20 armor, 8 toughness, 10 raw damage");
    for(auto& s:armor)check(s.damage==2,"raw damage determines armor wear before protection");
    armor={};check(absorb_damage(armor,10)==10,"empty armor has no protection");
    armor[1]={LEATHER_CHESTPLATE,1,durability(LEATHER_CHESTPLATE)};absorb_damage(armor,4);check(armor[1].empty(),"worn armor breaks");
    Inventory inventory;inventory.main[12]={IRON_BOOTS,1};inventory.quick_move(12);check(inventory.armor[3].id==IRON_BOOTS && inventory.main[12].empty(),"shift equips matching equipment");
    const char* patterns[]={"RRRR R","R RRRRRRR","RRRR RR R","R RR R"};
    for(int m:{0,2,3,4})for(int p=0;p<4;++p){std::array<Stack,9> grid{};int material=m==0?LEATHER:m==2?IRON_INGOT:m==3?GOLD_INGOT:DIAMOND;
        for(int i=0;patterns[p][i];++i)if(patterns[p][i]!=' ')grid[i]={material,1};
        check(recipe(grid,3).output.id==LEATHER_HELMET+m*4+p,"all sixteen craftable armor recipes");}
    auto flat=[](int,int y,int){return y<0?1:0;};
    for(int kind=0;kind<Mobs::Count;++kind){auto e=Mobs::create(kind,{0,5,0},42);
        for(int i=0;i<100;++i)Mobs::tick(e,{30,0,30},false,false,flat);
        check(e.grounded && e.position[1]>=-1e-6 && e.position[1]<.001,"all mob sizes fall and land on solid ground");
        check(Mobs::ray_distance(e,{e.position[0],.4,e.position[2]-4},{0,0,1},5)>0,"melee ray intersects mob hitbox");
        check(Mobs::ray_distance(e,{e.position[0]+3,.4,e.position[2]-4},{0,0,1},5)<0,"melee ray misses off-axis mob");
    }
    auto creeper=Mobs::create(Mobs::Creeper,{0,0,0},1);creeper.grounded=true;
    for(int i=0;i<40;++i)check(Mobs::tick(creeper,{1,0,0},false,true,flat)==0,"creative player never triggers creeper");
    creeper=Mobs::create(Mobs::Creeper,{0,0,0},1);creeper.grounded=true;
    int explosion=0;for(int i=0;i<32 && explosion!=2;++i)explosion=Mobs::tick(creeper,{1,0,0},true,true,flat);
    check(explosion==2,"creeper completes a thirty-tick fuse in survival");
    auto retreat=Mobs::create(Mobs::Creeper,{0,0,0},3);retreat.fuse=.7;Mobs::tick(retreat,{20,0,0},true,false,flat);check(retreat.fuse<.7,"breaking contact defuses creeper");
    auto zombie=Mobs::create(Mobs::Zombie,{0,0,0},2);zombie.grounded=true;
    check(Mobs::tick(zombie,{1,0,0},true,false,flat)==0,"walls block hostile attacks");
    check(Mobs::tick(zombie,{1,0,0},true,true,flat)==1,"zombie hits player in reach");
    check(Mobs::tick(zombie,{1,0,0},true,true,flat)==0,"zombie melee has a cooldown");
    auto wall=[](int x,int y,int){return y<0 || (x>=2 && y<4)?1:0;};
    auto pig=Mobs::create(Mobs::Pig,{1.4,0,0},3);pig.velocity={20,0,0};Mobs::tick(pig,{0,0,0},false,false,wall);
    check(pig.position[0]+Mobs::traits(Mobs::Pig).width/2<=2.00001,"mob swept collision stops knockback at walls");
    auto unknown=[](int,int,int){return 3;};check(!Mobs::fits(pig,{0,0,0},unknown),"mobs cannot walk into unloaded chunks");
    std::cout<<"Armor equipment, protection, toughness, wear and crafting; mob gravity, rays, collision and hostile AI passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
