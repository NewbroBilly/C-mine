#version 330 core
in vec2 uv;
out vec4 frag_color;
uniform sampler2D tex;
uniform vec4 color;
uniform int textured;
void main(){frag_color=color;if(textured==1)frag_color*=texture(tex,uv);if(frag_color.a<0.01)discard;}
