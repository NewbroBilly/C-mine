#version 330 core
layout(location=0) in vec2 p;
uniform vec3 screen;
uniform vec4 rect;
uniform vec4 source;
out vec2 uv;
void main(){vec2 pixel=rect.xy+p*rect.zw;gl_Position=vec4(pixel.x/screen.x*2.0-1.0,1.0-pixel.y/screen.y*2.0,0,1);uv=source.xy+p*source.zw;}
