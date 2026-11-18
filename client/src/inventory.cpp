#include <inventory.h>
#include <player.h>
#include <ui.h>
#include <avatar.h>
#include <gtc/matrix_transform.hpp>
namespace {vec2 location(float x,float y){return vec2(x-win_size.x/2.f,win_size.y/2.f-y-48);}}
Inventory::Inventory(){
over=nullptr;int n=0;for(int id=1;id<BLOCK_COUNT;++id)if(id!=WATER)catalog[n++]={id,64};
for(int i=0;i<54;++i)slots[i/9][i%9].bind(catalog[i]);
int defaults[]={STONE,DIRT,GRASS_BLOCK,LOG,PLANKS,GLASS,TORCH,CRAFTING_TABLE,Items::APPLE};for(int i=0;i<9;++i)creative.main[i]={defaults[i],Items::limit(defaults[i])};}
Items::Inventory& Inventory::data(){return survival_mode?survival:creative;}
void Inventory::bind_hotbar(){for(int i=0;i<9;++i)player->hotbar.slots[i].bind(data().main[i]);layout();}
void Inventory::open(bool table){workbench=table && survival_mode;selected=0;keyboard_navigation=false;SDL_GetMouseState(&last_mouse.x,&last_mouse.y);layout();}
void Inventory::close(){for(auto s:data().close())player->drop_item(s);over=nullptr;workbench=false;}
void Inventory::set_item(int i,int j,int id){catalog[i*9+j]={id,Items::limit(id)};}
int Inventory::get_item(int i,int j){return catalog[i*9+j].id;}
void Inventory::layout(){
int x=(int(win_size.x)-528)/2,y=(int(win_size.y)-(survival_mode?498:450))/2;
auto bind=[&](Slot& slot,Items::Stack& s,int px,int py){slot.bind(s);slot.pos=location(float(x+px*3),float(y+py*3));};
for(int i=0;i<36;++i){int row=i<9?142:84+(i/9-1)*18;bind(storage[i],data().main[i],8+(i%9)*18,row);}
if(!survival_mode){
for(int i=0;i<54;++i){auto& s=slots[i/9][i%9];int id=-1;if(catalog_page==0){int n=0;for(int b=1;b<BLOCK_COUNT;++b)if(b!=WATER && n++==i){id=b;break;}}
else if(i<Items::ITEM_END-Items::ITEM_START-1)id=Items::ITEM_START+1+i;
catalog[i]={id,Items::limit(id)};Items::normalize(catalog[i]);s.pos=location(float(x+(8+i%9*18)*3),float(y+(18+i/9*18)*3));}
for(int i=0;i<9;++i)storage[i].pos=location(float(x+(8+i*18)*3),float(y+130*3));
}else{int width=workbench?3:2;for(int i=0;i<width*width;++i)bind(craft_slots[i],data().craft[i],(workbench?30:98)+(i%width)*18,(workbench?17:18)+(i/width)*18);
result=Items::recipe(data().craft,width).output;bind(result_slot,result,workbench?124:154,workbench?35:28);bind(offhand,data().offhand,77,62);for(int i=0;i<4;++i)bind(armor[i],data().armor[i],8,8+i*18);}}
Slot* Inventory::slot_at(int i){
if(!survival_mode)return i<54?&slots[i/9][i%9]:&storage[i-54];
if(i<36)return &storage[i];if(i<result_index())return &craft_slots[i-36];
if(i==result_index())return &result_slot;return &offhand;}
void Inventory::activate(int i,bool right,bool quick){
auto& d=data();if(!survival_mode){if(i<54){auto s=catalog[i];if(s.empty())return;if(quick){d.main[player->hotbar.index]=s;return;}
if(!d.cursor.empty() && d.cursor.id==s.id)d.cursor.count=right?std::min(Items::limit(s.id),d.cursor.count+1):Items::limit(s.id);
else{d.cursor=s;d.cursor.count=1;}}else Items::click(d.main[i-54],d.cursor,right);
}else if(i<36){if(quick)d.quick_move(i);else Items::click(d.main[i],d.cursor,right);}
else if(i<result_index()){int n=i-36;if(quick){auto& s=d.craft[n];s.count=d.add(s);Items::normalize(s);}else Items::click(d.craft[n],d.cursor,right);}
else if(i==result_index()){if(quick){while(d.take_result(workbench?3:2,true)){};}else d.take_result(workbench?3:2,false);}
else if(!workbench)Items::click(d.offhand,d.cursor,right);layout();}
void Inventory::update(){
layout();over=nullptr;ivec2 mouse;SDL_GetMouseState(&mouse.x,&mouse.y);if(mouse!=last_mouse){keyboard_navigation=false;last_mouse=mouse;}
int move=0;if(is_key_down(SDLK_LEFT))move=-1;if(is_key_down(SDLK_RIGHT))move=1;if(is_key_down(SDLK_UP))move=-9;if(is_key_down(SDLK_DOWN))move=9;
int count=survival_mode?(workbench?46:42):63;
if(move){
if(move==1 || move==-1)selected=(selected+move+count)%count;
else{auto p=slot_at(selected)->pos;float best=1e9;int found=-1;
for(int i=0;i<count;++i){auto d=slot_at(i)->pos-p;if((move<0?d.y>1:d.y< -1)){float score=abs(d.y)+abs(d.x)*4;if(score<best){best=score;found=i;}}}
if(found>=0)selected=found;
}keyboard_navigation=true;}
if(keyboard_navigation)over=slot_at(selected);else{vec2 p(mouse.x-win_size.x/2.f,win_size.y/2.f-mouse.y);for(int i=0;i<count;++i){auto* s=slot_at(i);if(s && s->is_point_over(p)){over=s;selected=i;break;}}}
bool quick=is_key_pressed(SDLK_LSHIFT) || is_key_pressed(SDLK_RSHIFT);
if(over && (is_button_down(0) || is_button_down(2) || is_key_down(SDLK_RETURN)))activate(selected,is_button_down(2),quick);
if(over && selected!=(survival_mode?result_index():-1))for(int i=0;i<9;++i)if(is_key_down(SDLK_1+i)){if(!survival_mode && selected<54)data().main[i]=catalog[selected];else if(over->stack!=&data().main[i])std::swap(*over->stack,data().main[i]);}
if(!over && is_button_down(0) && !data().cursor.empty()){player->drop_item(data().cursor);data().cursor.clear();}
if(!survival_mode && is_key_down(SDLK_p)){catalog_page=1-catalog_page;selected=0;layout();}}
void Inventory::draw(){
layout();glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);Ui::rect(0,0,float(win_size.x),float(win_size.y),vec4(0,0,0,.65));
int x=(int(win_size.x)-528)/2,y=(int(win_size.y)-(survival_mode?498:450))/2;
Ui::image(Ui::inventory_texture(!survival_mode,workbench),float(x),float(y),528,survival_mode?498.f:450.f,vec4(0,0,176,survival_mode?166:150));
if(survival_mode){if(!workbench){glViewport(x+75,int(win_size.y)-y-225,120,192);glEnable(GL_DEPTH_TEST);glClear(GL_DEPTH_BUFFER_BIT);mat4 matrix=ortho(-.65f,.65f,-.1f,1.95f,-5.f,5.f)*lookAt(vec3(0,0,3),vec3(0),vec3(0,1,0));Avatar::draw(dvec3(0),vec3(.2f,0,1),0,matrix,0);glViewport(0,0,win_size.x,win_size.y);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);}
Ui::rect(float(x+(workbench?28:97)*3-2),float(y+12),110,15,vec4(.25,.25,.25,1));Ui::text("inv-title",workbench?"Crafting Table":"Crafting",x+(workbench?28:97)*3,y+12,1);
for(auto& slot:storage)slot.draw();for(int i=0;i<(workbench?9:4);++i)craft_slots[i].draw();result_slot.draw();if(!workbench)offhand.draw();
}else{for(auto& row:slots)for(auto& slot:row)slot.draw();for(int i=0;i<9;++i)storage[i].draw();Ui::rect(float(x+22),float(y+5),190,18,vec4(.25,.25,.25,1));Ui::text("inv-title",catalog_page?"Items / Tools / Food":"Building Blocks",x+24,y+5,1);}
if(over){float sx=over->pos.x+win_size.x/2.f,sy=win_size.y/2.f-over->pos.y-48;Ui::rect(sx,sy,48,48,vec4(1,1,1,.35));}
int mx,my;SDL_GetMouseState(&mx,&my);if(keyboard_navigation && over){mx=int(over->pos.x+win_size.x/2.f+30);my=int(win_size.y/2.f-over->pos.y-20);}
if(!data().cursor.empty())Ui::item(data().cursor,float(mx-24),float(my-24),48,true,"cursor-count");else if(over && !over->is_empty()){std::string str=Items::name(over->get_item());int width=int(str.size())*12;int tx=std::min(mx+15,int(win_size.x)-width-10),ty=std::max(5,my-28);Ui::rect(float(tx-4),float(ty-4),float(width+8),26,vec4(.08,.02,.12,.95));Ui::text("inv-tooltip",str,tx,ty);}
Ui::text("inv-help",survival_mode?"LMB: move  RMB: split  Shift: quick move  E: close":"P: blocks/items  Shift+click: hotbar  E: close",x,int(win_size.y)-25,1);glEnable(GL_CULL_FACE);glEnable(GL_DEPTH_TEST);}
