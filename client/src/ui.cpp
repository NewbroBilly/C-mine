#include <ui.h>
#include <blocks.h>
#include <text.h>
#include <map>
#include <player.h>
#include <item_model.h>
namespace {
Shader shader;GLuint vao,vbo;
Texture inventory,creative,table,items,icons,tabs;
std::map<std::string,Text> texts;
Texture load(const char* name,int unit){std::string root="resource_pack/";
#ifdef __SWITCH__
root="sdmc:/switch/C-mine/resource_pack/";
#endif
std::string path=root+name;if(!std::filesystem::exists(path))path=std::string("textures/")+name;return Texture(path.c_str(),unit);}
void quad(float x,float y,float w,float h,vec4 source,vec4 color,Texture* tex){
shader.bind();shader.upload_vec3("screen",vec3(win_size,0));shader.upload_vec4("rect",vec4(x,y,w,h));shader.upload_vec4("source",source);shader.upload_vec4("color",color);shader.upload_int("textured",tex?1:0);
if(tex){tex->bind();shader.upload_texture("tex",*tex);}glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,6);}
}
void Ui::init(){
shader=Shader("shaders/ui_vert.glsl","shaders/ui_frag.glsl");float vertices[]={0,0,1,0,1,1,0,0,1,1,0,1};
glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,nullptr);glEnableVertexAttribArray(0);
inventory=load("inventory.png",7);creative=load("creative.png",8);table=load("crafting.png",9);items=load("items.png",10);icons=load("icons.png",11);tabs=load("creative_tabs.png",12);ItemModel::init();}
void Ui::clear_text(){for(auto& entry:texts)entry.second.destroy();texts.clear();}
void Ui::destroy(){ItemModel::destroy();clear_text();inventory.destroy();creative.destroy();table.destroy();items.destroy();icons.destroy();tabs.destroy();shader.destroy();glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
void Ui::rect(float x,float y,float w,float h,vec4 color){quad(x,y,w,h,vec4(0),color,nullptr);}
void Ui::image(Texture& tex,float x,float y,float w,float h,vec4 source){source/=vec4(tex.width,tex.height,tex.width,tex.height);quad(x,y,w,h,source,vec4(1),&tex);}
void Ui::text(const std::string& key,const std::string& value,int x,int y,int size){auto& t=texts[key];ivec2 p(x-int(win_size.x)/2,int(win_size.y)/2-y);if(t.pos!=p || t.font_size!=size){t.destroy();t.str.clear();t.pos=p;t.font_size=size;}t.set_str(value);t.draw();}
int Ui::item_frame(int id){
    int n=id-Items::ITEM_START-1;
    if(id==Items::CLOCK && world)n=160+Items::clock_frame(world->day_ticks);
    if(id==Items::COMPASS && world && player){
        auto d=world->spawn_point-player->pos;n=192+Items::compass_frame(d.x,d.z,player->cam.forward.x,player->cam.forward.z);
    }
    if(id==Items::FISHING_ROD && player && player->fishing)n=224;
    return n;
}
void Ui::item(const Items::Stack& s,float x,float y,float size,bool count,const std::string& key){
if(s.empty())return;
if(ItemModel::cube(s.id))ItemModel::gui(s.id,x,y,size);
else if(Items::block(s.id)){Sprite& icon=get_block_by_id(s.id)->icon;image(*icon.tex,x,y,size,size,vec4(icon.uv.x,icon.uv.y-icon.size.y,icon.size.x,icon.size.y));}
else{int n=item_frame(s.id);
    image(items,x,y,size,size,vec4((n%16)*16,(n/16)*16,16,16));}
if(count && s.count>1){std::string str=std::to_string(s.count);text(key.empty()?"count"+std::to_string(int(x))+":"+std::to_string(int(y)):key,str,int(x+size-str.size()*12),int(y+size-18),2);}
int durability=Items::durability(s.id);if(durability && s.damage){float f=std::max(0.f,1.f-s.damage/float(durability));rect(x,y+size-5,size,5,vec4(0,0,0,1));rect(x+1,y+size-4,(size-2)*f,3,vec4(1-f,f,0,1));}}
Texture& Ui::inventory_texture(bool c,bool t){return t?table:(c?creative:inventory);}
Texture& Ui::icons_texture(){return icons;}
Texture& Ui::tabs_texture(){return tabs;}
Texture& Ui::items_texture(){return items;}
