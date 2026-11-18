#pragma once
#include <global.h>
namespace Avatar {
void init();
void draw(dvec3 position,vec3 forward,double phase,const mat4& camera,float amount);
void destroy();
}
