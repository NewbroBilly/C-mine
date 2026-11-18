#pragma once
#include <global.h>
namespace Avatar {
void init();
void draw(dvec3 position,vec3 forward,double phase,const mat4& camera,float amount,float attack=1);
void hand(const mat4& matrix);
void destroy();
}
