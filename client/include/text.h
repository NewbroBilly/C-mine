#pragma once

#include <global.h>
#include <string>
#include <vector>

struct Text
{
    ivec2 pos{0};
    std::string str;

    int font_size = 3;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    int indices_count;

    Text() {}
    Text(const ivec2& _pos);

    void set_str(const std::string& _str);
    static int measure(const std::string& str,int size=1);
    void build();
    void draw(vec3 color=vec3(1));
    void destroy();
};
