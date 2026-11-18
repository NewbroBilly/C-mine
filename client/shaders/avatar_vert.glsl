#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 color;
uniform mat4 cam_mat;
out vec3 tint;
void main(){gl_Position=cam_mat*vec4(position,1);tint=color;}
