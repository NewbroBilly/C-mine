#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec2 texture_uv;
layout(location=2) in float shade;
uniform mat4 cam_mat;
out vec2 uv;
out float lighting;
void main(){gl_Position=cam_mat*vec4(position,1);uv=texture_uv;lighting=shade;}
