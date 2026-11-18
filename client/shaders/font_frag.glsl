#version 330 core

in vec2 uv;

out vec4 FragColor;

uniform sampler2D tex3;
uniform vec3 tint;

void main()
{
    vec4 color = texture(tex3, uv);

    if(color.w < 0.1f)
        discard;

    FragColor = vec4(color.rgb*tint,color.a);
}
