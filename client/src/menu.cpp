#include <menu.h>
#include <game.h>
#include <stb_image.h>
#include <map>
#include <settings.h>
#include <ui.h>
#ifdef __SWITCH__
#include <switch.h>
#endif
extern int font_width[];
namespace {
Shader flat, panorama;
GLuint vao=0, cube=0;
int selected=0, last_x=-1,last_y=-1;
int count(bool paused){
if(Game::screen==Game::Screen::Worlds)return 3;
if(Game::screen==Game::Screen::CreateWorld)return 5;
return Game::screen==Game::Screen::Options?8:(Game::screen==Game::Screen::Dead?2:(paused?5:3));}
int button_y(int index){
if(Game::screen==Game::Screen::Worlds)return int(win_size.y)-174+index*48;
if(Game::screen==Game::Screen::CreateWorld)return int(win_size.y)/2-125+index*58;
bool options=Game::screen==Game::Screen::Options;
return (options?int(win_size.y)/2-170:int(win_size.y)/2-65)+index*(options?46:54);}
int first_world(){return std::max(0,Game::world_index-3);}
int edit_field=-1;
double panorama_angle=0;
std::map<std::string,Text> labels;
void rectangle(float x,float y,float w,float h,vec4 color) {
    flat.bind(); flat.upload_vec3("screen",vec3(win_size,0));
    flat.upload_vec4("rect",vec4(x,y,w,h)); flat.upload_vec4("color",color);
    glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES,0,6);
}
void text(const std::string& str,int y,int size=3) {
    int width=0;
    auto& t=labels[std::to_string(y)];
    for (char c:str) width += (c>=' ' && c<='~' ? ::font_width[c-' '] : 8)*size;
    ivec2 pos(-width/2,(int)win_size.y/2-y);
    if (t.pos!=pos || t.font_size!=size) { t.destroy(); t.str.clear(); t.pos=pos; t.font_size=size; }
    t.set_str(str); t.draw();
}
}
void Menu::init() {
    flat=Shader("shaders/screen_vert.glsl","shaders/screen_frag.glsl");
    panorama=Shader("shaders/screen_vert.glsl","shaders/panorama_frag.glsl");
    glGenVertexArrays(1,&vao); glGenTextures(1,&cube);
    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_CUBE_MAP,cube);
    const int faces[]={1,3,4,5,0,2};
    for(int i=0;i<6;++i) {
        std::string path="textures/panorama/panorama_"+std::to_string(faces[i])+".png";
        int w,h,n; auto* data=stbi_load(path.c_str(),&w,&h,&n,4);
        if(!data) { SDL_Log("Missing panorama: %s",path.c_str()); std::exit(7); }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
        stbi_image_free(data);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
}
void Menu::reset() { selected=Game::screen==Game::Screen::Worlds && Game::saved_worlds.empty()?1:0;edit_field=-1;SDL_StopTextInput();SDL_GetMouseState(&last_x,&last_y); }
void Menu::text_input(const char* input){
    if(Game::screen!=Game::Screen::CreateWorld || edit_field<0)return;
    auto& value=edit_field==0?Game::world_name:Game::world_seed;
    // The bundled bitmap font supports printable ASCII.
    for(const char* c=input;*c && value.size()<32;++c)if(*c>=' ' && *c<='~')value+=*c;
}
void Menu::wheel(int amount){if(Game::screen==Game::Screen::Worlds){Game::world_index=std::clamp(Game::world_index-amount,0,std::max(0,int(Game::saved_worlds.size())-1));}}
int Menu::update(bool paused) {
    panorama_angle+=std::min(delta_time,.05)*.035;
    const int count=::count(paused);
    bool worlds=Game::screen==Game::Screen::Worlds,create=Game::screen==Game::Screen::CreateWorld;
    const bool options=Game::screen==Game::Screen::Options;
    if(worlds){
        if(is_key_down(SDLK_UP))wheel(1);if(is_key_down(SDLK_DOWN))wheel(-1);
        if(is_key_down(SDLK_LEFT))selected=(selected+count-1)%count;
        if(is_key_down(SDLK_RIGHT) || is_key_down(SDLK_TAB))selected=(selected+1)%count;
    }else{
        if(is_key_down(SDLK_UP))selected=(selected+count-1)%count;
        if(is_key_down(SDLK_DOWN) || (create && is_key_down(SDLK_TAB)))selected=(selected+1)%count;
    }
    if(create){edit_field=selected<2?selected:-1;if(edit_field>=0){SDL_StartTextInput();auto& value=edit_field==0?Game::world_name:Game::world_seed;if(is_key_down(SDLK_BACKSPACE) && !value.empty())value.pop_back();}else SDL_StopTextInput();}
    int x,y;SDL_GetMouseState(&x,&y);
    if(worlds && is_button_down(0) && x>=int(win_size.x)/2-260 && x<int(win_size.x)/2+260 && y>=95 && y<int(win_size.y)-190){
        int row=(y-95)/58;int index=first_world()+row;
        if(index<int(Game::saved_worlds.size())){Game::world_index=index;selected=0;}
    }
    for(int i=0;i<count;++i){int top=button_y(i);
        if(x>=int(win_size.x)/2-210 && x<int(win_size.x)/2+210 && y>=top && y<top+42){
            // Text fields keep their focus while typing.
            if((!create || i>=2) && (x!=last_x || y!=last_y))selected=i;
            if(is_button_down(0)){selected=i;if(create && i<2){edit_field=i;SDL_StartTextInput();}return i;}
        }
    }
    last_x=x;last_y=y;
    bool confirm=is_key_down(SDLK_RETURN);
#ifdef __SWITCH__
    if(create && selected<2 && confirm){
        auto& value=selected==0?Game::world_name:Game::world_seed;SwkbdConfig keyboard;
        if(R_SUCCEEDED(swkbdCreate(&keyboard,0))){char buffer[128]{};swkbdConfigMakePresetDefault(&keyboard);swkbdConfigSetInitialText(&keyboard,value.c_str());swkbdConfigSetStringLenMax(&keyboard,32);swkbdConfigSetHeaderText(&keyboard,selected==0?"World name":"World seed");
            if(R_SUCCEEDED(swkbdShow(&keyboard,buffer,sizeof(buffer)))){value.clear();for(const char* p=buffer;*p && value.size()<32;++p)if(*p>=' ' && *p<='~')value+=*p;}swkbdClose(&keyboard);}
        return -1;
    }
#endif
    return (confirm || (options && selected<7 && (is_key_down(SDLK_LEFT) || is_key_down(SDLK_RIGHT))))?selected:-1;
}
void Menu::draw(bool paused) {
    glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    if(!paused) {
        glActiveTexture(GL_TEXTURE5);glBindTexture(GL_TEXTURE_CUBE_MAP,cube);
        panorama.bind();panorama.upload_int("panorama",5);
        panorama.upload_float("angle",(float)panorama_angle);panorama.upload_float("aspect",win_size.x/(float)win_size.y);
        panorama.upload_vec3("screen",vec3(win_size,0));panorama.upload_vec4("rect",vec4(0,0,win_size.x,win_size.y));
        glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,6);
    } else rectangle(0,0,win_size.x,win_size.y,vec4(0,0,0,.65));
    bool options=Game::screen==Game::Screen::Options;
    bool worlds=Game::screen==Game::Screen::Worlds,create=Game::screen==Game::Screen::CreateWorld;
    text(worlds?"SELECT WORLD":(create?"CREATE NEW WORLD":(options?"OPTIONS":(Game::screen==Game::Screen::Dead?"YOU DIED":(paused?"GAME PAUSED":"C-MINE")))),(options || worlds || create)?30:int(win_size.y)/5,(options || worlds || create)?4:(paused?4:7));
    std::vector<std::string> labels;
    auto& o=Settings::options;
    if(options)labels={"FOV: "+std::to_string(o.fov),"Sensitivity: "+std::to_string(o.sensitivity)+"%",
        "Render distance: "+std::to_string(o.distance),std::string("VSync: ")+(o.vsync?"ON":"OFF"),
        std::string("Ambient occlusion: ")+(o.ao?"ON":"OFF"),std::string("Fullscreen: ")+(o.fullscreen?"ON":"OFF"),
        std::string("Invert mouse Y: ")+(o.invert_y?"ON":"OFF"),"Done"};
    else if(worlds)labels={"Play Selected World","Create New World","Cancel"};
    else if(create)labels={"World name: "+Game::world_name.substr(Game::world_name.size()>24?Game::world_name.size()-24:0)+(edit_field==0?"_":""),"Seed: "+Game::world_seed.substr(Game::world_seed.size()>28?Game::world_seed.size()-28:0)+(edit_field==1?"_":""),Game::new_survival?"Game Mode: Survival":"Game Mode: Creative","Create New World","Cancel"};
    else if(Game::screen==Game::Screen::Dead)labels={"Respawn","Back to Title"};
    else if(paused)labels={"Resume Game",survival_mode?"Mode: Survival":"Mode: Creative","Options...","Back to Title","Quit Game"};
    else labels={"Singleplayer","Options...","Quit Game"};
    if(worlds){
        rectangle(win_size.x/2.f-270,85,540,float(int(win_size.y)-275),vec4(0,0,0,.55));
        if(Game::saved_worlds.empty())text("No saved worlds. Create your first world.",120,2);
        int first=first_world();
        for(int i=first;i<int(Game::saved_worlds.size());++i){int y=95+(i-first)*58;if(y+54>int(win_size.y)-190)break;
            const auto& d=Game::saved_worlds[i].data;float x=win_size.x/2.f-260;
            rectangle(x,float(y),520,54,i==Game::world_index?vec4(.36,.4,.54,.95):vec4(.13,.13,.13,.9));
            Ui::text("save-name"+std::to_string(i-first),d.name,int(x+10),y+4,2);
            Ui::text("save-mode"+std::to_string(i-first),std::string(d.survival?"Survival":"Creative")+" / Seed: "+std::to_string(d.seed),int(x+10),y+31,1);
        }
    }
    for(int i=0;i<(int)labels.size();++i) {
        float x=win_size.x/2.f-210,y=float(button_y(i));
        rectangle(x,y,420,42,i==selected?vec4(.72,.76,.92,1):vec4(.18,.18,.18,1));
        rectangle(x+2,y+2,416,38,i==selected?vec4(.37,.42,.61,1):vec4(.34,.34,.34,1));
        text(labels[i],(int)y+4,(options || create)?2:3);
    }
    if(!Game::message.empty())text(Game::message,70,1);
    text(create?"Enter name and seed / Up-Down: field / Empty seed: random":(worlds?"Up-Down: world / Left-Right: action / Enter: confirm":(options?"Click / Left-Right to change  /  Esc: back":(paused?"World is paused":"E: inventory  /  Tab: game mode"))),(int)win_size.y-25,1);
    glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
}
void Menu::destroy() {
    for(auto& entry:labels) entry.second.destroy(); labels.clear();
    glDeleteTextures(1,&cube);glDeleteVertexArrays(1,&vao);flat.destroy();panorama.destroy();
}
