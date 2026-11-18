#include <settings.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstdlib>
void check(bool ok,const char* reason){if(!ok){std::cerr<<reason<<'\n';std::exit(1);}}
int main(){
    const std::string path="settings-test/options.txt";
    Settings::Options value;check(value.russian,"Russian default language");value.russian=false;value.fov=100;value.sensitivity=170;value.distance=8;value.vsync=false;value.ao=false;value.invert_y=true;value.fullscreen=true;
    check(Settings::save(path,value),"save preferences");Settings::Options loaded;
    check(Settings::load(path,loaded) && loaded==value,"all options must survive restart");
    {std::ofstream file(path);file<<"fov=900\nsensitivity=-7\ndistance=200\nunknown=8\nao=0\nvsync=bad\nfov=60junk\n";}
    check(Settings::load(path,loaded),"load partially malformed options");
    check(loaded.fov==110 && loaded.sensitivity==10 && loaded.distance==8 && !loaded.ao && loaded.vsync,"clamp ranges and ignore malformed values");
    value=loaded;check(!Settings::load("settings-test/missing.txt",loaded) && loaded==value,"missing file must preserve current options");
    std::filesystem::remove(path);std::filesystem::remove("settings-test");
    std::cout<<"Settings: persistence, all fields, bounds, malformed/missing files passed\n";
}
