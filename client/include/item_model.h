#pragma once
#include <global.h>
namespace ItemModel {
void init();
void destroy();
bool cube(int id);
void draw(int id,Texture& item_texture,int frame,const mat4& matrix);
void gui(int id,float x,float y,float size);
void cracks(int stage,const mat4& matrix);
}
