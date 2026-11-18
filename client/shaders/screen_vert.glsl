#version 330 core
uniform vec4 rect;
uniform vec3 screen;
out vec2 uv;
void main() {
    vec2 corners[6] = vec2[](vec2(0,0),vec2(0,1),vec2(1,1),vec2(0,0),vec2(1,1),vec2(1,0));
    uv = corners[gl_VertexID];
    vec2 pixel = rect.xy + uv * rect.zw;
    gl_Position = vec4(pixel.x / screen.x * 2.0 - 1.0, 1.0 - pixel.y / screen.y * 2.0, 0, 1);
}
