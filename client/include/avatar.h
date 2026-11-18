#pragma once
#include <global.h>
#include <items.h>
#include <mobs.h>
namespace Avatar {
void init();
void draw(dvec3 position,vec3 forward,double phase,const mat4& camera,float amount,float attack=1,const std::array<Items::Stack,4>* armor=nullptr);
void mob(const Mobs::Entity& entity,const mat4& camera,double partial);
void hand(const mat4& matrix);
void destroy();
}
