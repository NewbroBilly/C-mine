#version 330 core
in vec2 uv;
in float lighting;
out vec4 frag_color;
uniform sampler2D tex;
void main(){frag_color=texture(tex,uv);if(frag_color.a<0.1)discard;frag_color.rgb*=lighting;}
