#include <language.h>
#include <inventory.h>
#include <player.h>
#include <ui.h>
#include <avatar.h>
#include <creative_tabs.h>
#include <gtc/matrix_transform.hpp>
namespace {vec2 location(float x,float y){return vec2(x-win_size.x/2.f,win_size.y/2.f-y-48);}}
Inventory::Inventory(){
over=nullptr;
for(int i=0;i<54;++i)slots[i/9][i%9].bind(catalog[i]);
int defaults[]={STONE,DIRT,GRASS_BLOCK,LOG,PLANKS,GLASS,TORCH,CRAFTING_TABLE,Items::APPLE};for(int i=0;i<9;++i)creative.main[i]={defaults[i],Items::limit(defaults[i])};}
Items::Inventory& Inventory::data(){return survival_mode?survival:creative;}
void Inventory::bind_hotbar(){for(int i=0;i<9;++i)player->hotbar.slots[i].bind(data().main[i]);layout();}
void Inventory::open(bool table){if(!survival_mode && catalog_page==5)SDL_StartTextInput();workbench=table && survival_mode;selected=0;keyboard_navigation=false;SDL_GetMouseState(&last_mouse.x,&last_mouse.y);layout();}
void Inventory::close(){SDL_StopTextInput();for(auto s:data().close())player->drop_item(s);over=nullptr;workbench=false;}
void Inventory::set_item(int i,int j,int id){catalog[i*9+j]={id,Items::limit(id)};}
int Inventory::get_item(int i,int j){return catalog[i*9+j].id;}
void Inventory::layout(){
int x=(int(win_size.x)-(survival_mode?528:585))/2,y=(int(win_size.y)-(survival_mode?498:408))/2;
auto bind=[&](Slot& slot,Items::Stack& s,int px,int py){slot.bind(s);slot.pos=location(float(x+px*3),float(y+py*3));};
for(int i=0;i<36;++i){int row=i<9?142:84+(i/9-1)*18;bind(storage[i],data().main[i],8+(i%9)*18,row);}
if(!survival_mode){
auto entries=CreativeTabs::entries(catalog_page,search);
int maximum=std::max(0,(int(entries.size())+8)/9-5);scroll_row=std::clamp(scroll_row,0,maximum);
for(int i=0;i<45;++i){auto& s=slots[i/9][i%9];int n=scroll_row*9+i,id=n<int(entries.size())?entries[n]:-1;
catalog[i]={id,1};Items::normalize(catalog[i]);s.pos=location(float(x+(9+i%9*18)*3),float(y+(18+i/9*18)*3));}
for(int i=0;i<9;++i)storage[i].pos=location(float(x+(9+i*18)*3),float(y+112*3));
if(catalog_page==11)for(int i=9;i<36;++i)storage[i].pos=location(float(x+(9+i%9*18)*3),float(y+(54+(i/9-1)*18)*3));
if(catalog_page==11){for(int i=0;i<4;++i)bind(armor[i],data().armor[i],9+i*18,18);bind(offhand,data().offhand,77,62);}
}else{int width=workbench?3:2;for(int i=0;i<width*width;++i)bind(craft_slots[i],data().craft[i],(workbench?30:98)+(i%width)*18,(workbench?17:18)+(i/width)*18);
result=Items::recipe(data().craft,width).output;bind(result_slot,result,workbench?124:154,workbench?35:28);bind(offhand,data().offhand,77,62);for(int i=0;i<4;++i)bind(armor[i],data().armor[i],8,8+i*18);}}
Slot* Inventory::slot_at(int i){
if(!survival_mode){if(catalog_page==11){if(i==40)return &offhand;return i>=36?&armor[i-36]:&storage[i<27?i+9:i-27];}return i<45?&slots[i/9][i%9]:&storage[i-45];}
if(i<36)return &storage[i];if(i<result_index())return &craft_slots[i-36];
if(i==result_index())return &result_slot;if(!workbench && i>=42)return &armor[i-42];return &offhand;}
void Inventory::activate(int i,bool right,bool quick){
if(!survival_mode && catalog_page==11 && i==40){auto& d=data();if(quick){d.offhand.count=d.add(d.offhand);Items::normalize(d.offhand);}else Items::click(d.offhand,d.cursor,right);layout();return;}
auto& d=data();if(!survival_mode){if(catalog_page==11){if(i>=36){auto& s=d.armor[i-36];if(quick){s.count=d.add(s);Items::normalize(s);}else Items::armor_click(s,d.cursor,i-36,right);layout();return;}int n=i<27?i+9:i-27;if(quick)d.quick_move(n);else Items::click(d.main[n],d.cursor,right);layout();return;}if(i<45){auto s=catalog[i];if(s.empty())return;if(quick){s.count=Items::limit(s.id);d.main[player->hotbar.index]=s;return;}
if(!d.cursor.empty() && d.cursor.id==s.id)d.cursor.count=right?std::min(Items::limit(s.id),d.cursor.count+1):Items::limit(s.id);
else{d.cursor=s;d.cursor.count=1;}}else Items::click(d.main[i-45],d.cursor,right);
}else if(i<36){if(quick)d.quick_move(i);else Items::click(d.main[i],d.cursor,right);}
else if(i<result_index()){int n=i-36;if(quick){auto& s=d.craft[n];s.count=d.add(s);Items::normalize(s);}else Items::click(d.craft[n],d.cursor,right);}
else if(i==result_index()){if(quick){while(d.take_result(workbench?3:2,true)){};}else d.take_result(workbench?3:2,false);}
else if(!workbench && i>=42){auto& s=d.armor[i-42];if(quick){s.count=d.add(s);Items::normalize(s);}else Items::armor_click(s,d.cursor,i-42,right);}
else if(!workbench)Items::click(d.offhand,d.cursor,right);layout();}
void Inventory::update(){
layout();over=nullptr;ivec2 mouse;SDL_GetMouseState(&mouse.x,&mouse.y);if(mouse!=last_mouse){keyboard_navigation=false;last_mouse=mouse;}
int move=0;if(is_key_down(SDLK_LEFT))move=-1;if(is_key_down(SDLK_RIGHT))move=1;if(is_key_down(SDLK_UP))move=-9;if(is_key_down(SDLK_DOWN))move=9;
int count=survival_mode?46:(catalog_page==11?41:54);
selected=std::clamp(selected,0,count-1);
if(move){
if(move==1 || move==-1)selected=(selected+move+count)%count;
else{auto p=slot_at(selected)->pos;float best=1e9;int found=-1;
for(int i=0;i<count;++i){auto d=slot_at(i)->pos-p;if((move<0?d.y>1:d.y< -1)){float score=abs(d.y)+abs(d.x)*4;if(score<best){best=score;found=i;}}}
if(found>=0)selected=found;
}keyboard_navigation=true;}
if(keyboard_navigation)over=slot_at(selected);else{vec2 p(mouse.x-win_size.x/2.f,win_size.y/2.f-mouse.y);for(int i=0;i<count;++i){auto* s=slot_at(i);if(s && s->is_point_over(p)){over=s;selected=i;break;}}}
bool quick=is_key_pressed(SDLK_LSHIFT) || is_key_pressed(SDLK_RSHIFT);
if(over && (is_button_down(0) || is_button_down(2) || is_key_down(SDLK_RETURN)))activate(selected,is_button_down(2),quick);
if(!(catalog_page==5 && !survival_mode) && over && selected!=(survival_mode?result_index():-1))for(int i=0;i<9;++i)if(is_key_down(SDLK_1+i)){if(!survival_mode && catalog_page!=11 && selected<45){if(!catalog[selected].empty()){data().main[i]=catalog[selected];data().main[i].count=Items::limit(catalog[selected].id);}}else if(over->stack!=&data().main[i]){int part=(!workbench && survival_mode && selected>=42)?selected-42:(!survival_mode && catalog_page==11 && selected>=36?selected-36:-1);if(part<0 || data().main[i].empty() || Items::armor_slot(data().main[i].id)==part)std::swap(*over->stack,data().main[i]);}}
if(!over && !(!survival_mode && mouse.x>=(int(win_size.x)-585)/2 && mouse.x<(int(win_size.x)+585)/2 && mouse.y>=(int(win_size.y)-408)/2-96 && mouse.y<(int(win_size.y)+408)/2+96) && is_button_down(0) && !data().cursor.empty()){player->drop_item(data().cursor);data().cursor.clear();}
if(!survival_mode){
if((catalog_page!=5 && is_key_down(SDLK_p)) || is_key_down(SDLK_RIGHTBRACKET) || is_key_down(SDLK_PAGEDOWN))tab((catalog_page+1)%12);
if(is_key_down(SDLK_LEFTBRACKET) || is_key_down(SDLK_PAGEUP))tab((catalog_page+11)%12);
if(catalog_page==5 && is_key_down(SDLK_BACKSPACE) && !search.empty()){Language::pop(search);scroll_row=0;}
int x=(int(win_size.x)-585)/2,y=(int(win_size.y)-408)/2;
if(is_button_down(0))for(int t=0;t<12;++t){int col=t%6,tx=x+(col==5?167:col*29)*3,ty=y+(t<6?-28:134)*3;if(mouse.x>=tx && mouse.x<tx+84 && mouse.y>=ty && mouse.y<ty+84){tab(t);break;}}
if(catalog_page!=11 && (is_button_down(0) || is_button_pressed(0)) && mouse.x>=x+175*3 && mouse.x<x+187*3 && mouse.y>=y+18*3 && mouse.y<y+112*3){auto entries=CreativeTabs::entries(catalog_page,search);int maximum=std::max(0,(int(entries.size())+8)/9-5);scroll_row=int(std::clamp((mouse.y-y-54-22)/237.f,0.f,1.f)*maximum+.5f);}
}}
void Inventory::tab(int index){catalog_page=std::clamp(index,0,11);scroll_row=selected=0;over=nullptr;if(catalog_page==5)SDL_StartTextInput();else SDL_StopTextInput();layout();}
void Inventory::scroll(int amount){if(!survival_mode && catalog_page!=11){scroll_row=std::max(0,scroll_row+amount);layout();}}
void Inventory::text_input(const char* text){if(survival_mode || catalog_page!=5)return;Language::append(search,text,24);scroll_row=0;layout();}
void Inventory::draw(){
layout();glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);Ui::rect(0,0,float(win_size.x),float(win_size.y),vec4(0,0,0,.65));
int x=(int(win_size.x)-(survival_mode?528:585))/2,y=(int(win_size.y)-(survival_mode?498:408))/2;
auto draw_tab=[&](int t){int col=t%6,tx=x+(col==5?167:col*29)*3,ty=y+(t<6?-28:134)*3;
Ui::image(Ui::tabs_texture(),float(tx),float(ty),84,96,vec4(col*28,(t>=6?64:0)+(catalog_page==t?32:0),28,32));
Ui::item({CreativeTabs::icon(t),1},float(tx+18),float(ty+18),48,false);};
if(!survival_mode)for(int t=0;t<12;++t)if(t!=catalog_page)draw_tab(t);
Ui::image(Ui::inventory_texture(!survival_mode,workbench),float(x),float(y),survival_mode?528.f:585.f,survival_mode?498.f:408.f,vec4(0,0,survival_mode?176:195,survival_mode?166:136));
if(!survival_mode)draw_tab(catalog_page);
if(survival_mode){if(!workbench){glViewport(x+75,int(win_size.y)-y-225,120,192);glEnable(GL_DEPTH_TEST);glClear(GL_DEPTH_BUFFER_BIT);mat4 matrix=ortho(-.65f,.65f,-.1f,1.95f,-5.f,5.f)*lookAt(vec3(0,0,3),vec3(0),vec3(0,1,0));Avatar::draw(dvec3(0),vec3(.2f,0,1),0,matrix,0,1,&data().armor);glViewport(0,0,win_size.x,win_size.y);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);}
Ui::rect(float(x+(workbench?28:97)*3-2),float(y+12),110,15,vec4(.25,.25,.25,1));Ui::text("inv-title",workbench?Language::tr("Crafting Table"):Language::tr("Crafting"),x+(workbench?28:97)*3,y+12,1);
for(auto& slot:storage)slot.draw();for(int i=0;i<(workbench?9:4);++i)craft_slots[i].draw();result_slot.draw();if(!workbench){offhand.draw();for(auto& s:armor)s.draw();}
}else{
Ui::text("inv-title",catalog_page==5?(std::string(Language::tr("Search: "))+search+"_"):CreativeTabs::name(catalog_page),x+27,y+13,2);
if(catalog_page==11){Ui::rect(float(x+21),float(y+48),543,276,vec4(.78,.78,.78,1));for(int i=0;i<4;++i){auto& slot=armor[i];float sx=slot.pos.x+win_size.x/2.f,sy=win_size.y/2.f-slot.pos.y-48;Ui::image(Ui::inventory_texture(false),sx-3,sy-3,54,54,vec4(7,7+i*18,18,18));slot.draw();}for(auto& slot:storage){float sx=slot.pos.x+win_size.x/2.f,sy=win_size.y/2.f-slot.pos.y-48;Ui::rect(sx-3,sy-3,54,54,vec4(.35,.35,.35,1));Ui::rect(sx,sy,48,48,vec4(.55,.55,.55,1));slot.draw();}}
else{if(catalog_page==11)offhand.draw();for(int i=0;i<45;++i)slots[i/9][i%9].draw();for(int i=0;i<9;++i)storage[i].draw();
auto entries=CreativeTabs::entries(catalog_page,search);int maximum=std::max(0,(int(entries.size())+8)/9-5);
Ui::image(Ui::tabs_texture(),float(x+525),float(y+54+(maximum?int(scroll_row/float(maximum)*237):0)),36,45,vec4(maximum?232:244,0,12,15));
if(entries.empty())Ui::text("inv-empty",Language::tr("No supported items in this section"),x+35,y+160,2);}
}
if(over){float sx=over->pos.x+win_size.x/2.f,sy=win_size.y/2.f-over->pos.y-48;Ui::rect(sx,sy,48,48,vec4(1,1,1,.35));}
int mx,my;SDL_GetMouseState(&mx,&my);if(keyboard_navigation && over){mx=int(over->pos.x+win_size.x/2.f+30);my=int(win_size.y/2.f-over->pos.y-20);}
if(!data().cursor.empty())Ui::item(data().cursor,float(mx-24),float(my-24),48,true,"cursor-count");else if(over && !over->is_empty()){std::string str=Items::name(over->get_item());int width=Text::measure(str,2);int tx=std::min(mx+15,int(win_size.x)-width-10),ty=std::max(5,my-28);Ui::rect(float(tx-4),float(ty-4),float(width+8),26,vec4(.08,.02,.12,.95));Ui::text("inv-tooltip",str,tx,ty);}
if(!survival_mode)for(int t=0;t<12;++t){int col=t%6,tx=x+(col==5?167:col*29)*3,ty=y+(t<6?-28:134)*3;if(mx>=tx && mx<tx+84 && my>=ty && my<ty+84){std::string label=CreativeTabs::name(t);int w=Text::measure(label,2),px=std::clamp(mx+15,5,int(win_size.x)-w-10),py=std::clamp(my+20,5,int(win_size.y)-30);Ui::rect(float(px-4),float(py-4),float(w+8),26,vec4(.08,.02,.12,.95));Ui::text("tab-tooltip",label,px,py,2);break;}}
if(survival_mode)Ui::text("inv-help",Language::tr("LMB: move  RMB: split  Shift: quick move  E: close"),x,int(win_size.y)-25,1);glEnable(GL_CULL_FACE);glEnable(GL_DEPTH_TEST);}
