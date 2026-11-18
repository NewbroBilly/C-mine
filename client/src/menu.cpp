#include <menu.h>
#include <game.h>
#include <stb_image.h>
#include <map>
extern int font_width[];
namespace {
Shader flat, panorama;
GLuint vao=0, cube=0;
int selected=0, last_x=-1,last_y=-1;
double panorama_angle=0;
std::map<std::string,Text> labels;
void rectangle(float x,float y,float w,float h,vec4 color) {
    flat.bind(); flat.upload_vec3("screen",vec3(win_size,0));
    flat.upload_vec4("rect",vec4(x,y,w,h)); flat.upload_vec4("color",color);
    glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES,0,6);
}
void text(const std::string& str,int y,int size=3) {
    int width=0;
    auto& t=labels[str];
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
void Menu::reset() { selected=0; SDL_GetMouseState(&last_x,&last_y); }
int Menu::update(bool paused) {
    panorama_angle+=std::min(delta_time,.05)*.035;
    const int count=paused?3:2;
    if(is_key_down(SDLK_UP)) selected=(selected+count-1)%count;
    if(is_key_down(SDLK_DOWN)) selected=(selected+1)%count;
    int x,y; SDL_GetMouseState(&x,&y);
    for(int i=0;i<count;++i) {
        int top=(int)win_size.y/2+i*54;
        if(x>=(int)win_size.x/2-180 && x<(int)win_size.x/2+180 && y>=top && y<top+42) {
            if(x!=last_x || y!=last_y) selected=i;
            if(is_button_down(0)) return i;
        }
    }
    last_x=x;last_y=y;
    return is_key_down(SDLK_RETURN)?selected:-1;
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
    text(paused?"GAME PAUSED":"C-MINE",(int)win_size.y/5,paused?4:7);
    const char* title[]={"Singleplayer","Quit Game"};
    const char* pause[]={"Resume Game","Back to Title","Quit Game"};
    for(int i=0;i<(paused?3:2);++i) {
        float x=win_size.x/2.f-180,y=win_size.y/2.f+i*54;
        rectangle(x,y,360,42,i==selected?vec4(.72,.76,.92,1):vec4(.18,.18,.18,1));
        rectangle(x+2,y+2,356,38,i==selected?vec4(.37,.42,.61,1):vec4(.34,.34,.34,1));
        text(paused?pause[i]:title[i],(int)y+4,3);
    }
    text(paused?"World is paused":"WASD  /  Space: jump  /  F5: camera",(int)win_size.y-40,2);
    glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
}
void Menu::destroy() {
    for(auto& entry:labels) entry.second.destroy(); labels.clear();
    glDeleteTextures(1,&cube);glDeleteVertexArrays(1,&vao);flat.destroy();panorama.destroy();
}

