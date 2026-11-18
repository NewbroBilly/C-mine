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
    void build();
    void draw();
    void destroy();
};