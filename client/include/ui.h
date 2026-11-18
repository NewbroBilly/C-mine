#pragma once
#include <global.h>
#include <items.h>
namespace Ui {
void init();void destroy();void clear_text();
void rect(float x,float y,float width,float height,vec4 color);
void image(Texture& texture,float x,float y,float width,float height,vec4 source);
void text(const std::string& key,const std::string& value,int x,int y,int size=2);
void item(const Items::Stack& stack,float x,float y,float size=48,bool count=true,const std::string& key="");
Texture& inventory_texture(bool creative,bool table=false);
Texture& icons_texture();
}
