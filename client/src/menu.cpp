#include <language.h>
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
Texture widgets,logo;
GLuint vao=0, cube=0;
int selected=0, last_x=-1,last_y=-1;
int count(bool paused){
if(Game::screen==Game::Screen::Worlds)return 3;
if(Game::screen==Game::Screen::CreateWorld)return 5;
return Game::screen==Game::Screen::Options?9:(Game::screen==Game::Screen::Dead?2:(paused?5:3));}
int button_y(int index){
if(Game::screen==Game::Screen::Worlds)return int(win_size.y)-174+index*48;
if(Game::screen==Game::Screen::CreateWorld)return int(win_size.y)/2-125+index*58;
bool options=Game::screen==Game::Screen::Options;
return (options?int(win_size.y)/2-195:int(win_size.y)/2-65)+index*(options?46:54);}
int first_world(){return std::max(0,Game::world_index-3);}
int edit_field=-1;
double panorama_angle=0;
std::map<std::string,Text> labels;
void rectangle(float x,float y,float w,float h,vec4 color) {
    flat.bind(); flat.upload_vec3("screen",vec3(win_size,0));
    flat.upload_vec4("rect",vec4(x,y,w,h)); flat.upload_vec4("color",color);
    glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES,0,6);
}
void text(const std::string& str,int y,int size=3,vec3 color=vec3(1)) {
    int width=0;
    auto& t=labels[std::to_string(y)];
    width=Text::measure(str,size);
    ivec2 pos(-width/2,(int)win_size.y/2-y);
    if (t.pos!=pos || t.font_size!=size) { t.destroy(); t.str.clear(); t.pos=pos; t.font_size=size; }
    t.set_str(str); t.draw(color);
}
}
void Menu::init() {
    flat=Shader("shaders/screen_vert.glsl","shaders/screen_frag.glsl");
    std::string path="resource_pack/widgets.png";
#ifdef __SWITCH__
    path="sdmc:/switch/C-mine/resource_pack/widgets.png";
#endif
    if(!std::filesystem::exists(path))path="textures/widgets.png";
    widgets=Texture(path.c_str(),13);logo=Texture("textures/logo.png",14);
    // The supplied logo is artwork rather than pixel art; filter its outline
    // smoothly when fitting it to the menu, without changing widget filtering.
    logo.bind();
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
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
    Language::append(value,input,32);
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
    if(create){edit_field=selected<2?selected:-1;if(edit_field>=0){SDL_StartTextInput();auto& value=edit_field==0?Game::world_name:Game::world_seed;if(is_key_down(SDLK_BACKSPACE) && !value.empty())Language::pop(value);}else SDL_StopTextInput();}
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
        if(R_SUCCEEDED(swkbdCreate(&keyboard,0))){char buffer[128]{};swkbdConfigMakePresetDefault(&keyboard);swkbdConfigSetInitialText(&keyboard,value.c_str());swkbdConfigSetStringLenMax(&keyboard,32);swkbdConfigSetHeaderText(&keyboard,selected==0?Language::tr("World name"):Language::tr("World seed"));
            if(R_SUCCEEDED(swkbdShow(&keyboard,buffer,sizeof(buffer)))){value.clear();Language::append(value,buffer,32);}swkbdClose(&keyboard);}
        return -1;
    }
#endif
    return (confirm || (options && selected<8 && (is_key_down(SDLK_LEFT) || is_key_down(SDLK_RIGHT))))?selected:-1;
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
    if(Game::screen==Game::Screen::Title){float w=std::min(float(win_size.x)-50.f,660.f),h=w*logo.height/logo.width;float top=std::max(8.f,(win_size.y/2.f-85-h)*.5f);Ui::image(logo,(win_size.x-w)/2,top,w,h,vec4(0,0,logo.width,logo.height));}
    else text(worlds?Language::tr("SELECT WORLD"):(create?Language::tr("CREATE NEW WORLD"):(options?Language::tr("OPTIONS"):(Game::screen==Game::Screen::Dead?Language::tr("YOU DIED"):(paused?Language::tr("GAME PAUSED"):"C-MINE")))),(options || worlds || create)?30:int(win_size.y)/5,(options || worlds || create)?4:(paused?4:7));
    std::vector<std::string> labels;
    auto& o=Settings::options;
    if(options)labels={Language::tr("FOV: ")+std::to_string(o.fov),Language::tr("Sensitivity: ")+std::to_string(o.sensitivity)+"%",
        Language::tr("Render distance: ")+std::to_string(o.distance),std::string(Language::tr("VSync: "))+(o.vsync?Language::tr("ON"):Language::tr("OFF")),
        std::string(Language::tr("Ambient occlusion: "))+(o.ao?Language::tr("ON"):Language::tr("OFF")),std::string(Language::tr("Fullscreen: "))+(o.fullscreen?Language::tr("ON"):Language::tr("OFF")),
        std::string(Language::tr("Invert mouse Y: "))+(o.invert_y?Language::tr("ON"):Language::tr("OFF")),Language::tr("Language: English"),Language::tr("Done")};
    else if(worlds)labels={Language::tr("Play Selected World"),Language::tr("Create New World"),Language::tr("Cancel")};
    else if(create)labels={Language::tr("World name: ")+Language::tail(Game::world_name,24)+(edit_field==0?"_":""),Language::tr("Seed: ")+Language::tail(Game::world_seed,28)+(edit_field==1?"_":""),Game::new_survival?Language::tr("Game Mode: Survival"):Language::tr("Game Mode: Creative"),Language::tr("Create New World"),Language::tr("Cancel")};
    else if(Game::screen==Game::Screen::Dead)labels={Language::tr("Respawn"),Language::tr("Back to Title")};
    else if(paused)labels={Language::tr("Resume Game"),survival_mode?Language::tr("Mode: Survival"):Language::tr("Mode: Creative"),Language::tr("Options..."),Language::tr("Back to Title"),Language::tr("Quit Game")};
    else labels={Language::tr("Singleplayer"),Language::tr("Options..."),Language::tr("Quit Game")};
    if(worlds){
        rectangle(win_size.x/2.f-270,85,540,float(int(win_size.y)-275),vec4(0,0,0,.55));
        if(Game::saved_worlds.empty())text(Language::tr("No saved worlds. Create your first world."),120,2);
        int first=first_world();
        for(int i=first;i<int(Game::saved_worlds.size());++i){int y=95+(i-first)*58;if(y+54>int(win_size.y)-190)break;
            const auto& d=Game::saved_worlds[i].data;float x=win_size.x/2.f-260;
            rectangle(x,float(y),520,54,i==Game::world_index?vec4(.36,.4,.54,.95):vec4(.13,.13,.13,.9));
            Ui::text("save-name"+std::to_string(i-first),d.name,int(x+10),y+4,2);
            Ui::text("save-mode"+std::to_string(i-first),std::string(d.survival?Language::tr("Survival"):Language::tr("Creative"))+Language::tr(" / Seed: ")+std::to_string(d.seed),int(x+10),y+31,1);
        }
    }
    for(int i=0;i<(int)labels.size();++i) {
        float x=win_size.x/2.f-210,y=float(button_y(i));
        // Two original 100px halves preserve the classic border at any width.
        int sprite_y=i==selected?86:66;
        Ui::image(widgets,x,y,210,42,vec4(0,sprite_y,100,20));
        Ui::image(widgets,x+210,y,210,42,vec4(100,sprite_y,100,20));
        int size=(options || create)?2:3;while(size>1 && Text::measure(labels[i],size)>400)--size;
        text(labels[i],(int)y+(42-size*12)/2,size,i==selected?vec3(1,1,.63):vec3(.88));
    }
    if(!Game::message.empty())text(Game::message,70,1);
    text(create?Language::tr("Enter name and seed / Up-Down: field / Empty seed: random"):(worlds?Language::tr("Up-Down: world / Left-Right: action / Enter: confirm"):(options?Language::tr("Click / Left-Right to change  /  Esc: back"):(paused?Language::tr("World is paused"):Language::tr("E: inventory  /  Tab: game mode")))),(int)win_size.y-25,1);
    glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
}
void Menu::destroy() {
    for(auto& entry:labels) entry.second.destroy(); labels.clear();
    widgets.destroy();logo.destroy();glDeleteTextures(1,&cube);glDeleteVertexArrays(1,&vao);flat.destroy();panorama.destroy();
}
