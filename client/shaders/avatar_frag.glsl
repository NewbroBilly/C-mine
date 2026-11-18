#version 330 core
in vec2 uv;
in float lighting;
uniform sampler2D skin;
out vec4 FragColor;
void main(){vec4 color=texture(skin,uv);if(color.a<.1)discard;FragColor=vec4(color.rgb*lighting,color.a);}
