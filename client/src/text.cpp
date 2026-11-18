#include <text.h>
#include <language.h>

struct FontVertex
{
    vec2 p;
    vec2 uv;
};

vec2 font_vertex_offset[4] =
{
    vec2(0, 0),
    vec2(1, 0),
    vec2(1, 1),
    vec2(0, 1)
};

vec2 font_uv_offset[4] =
{
    vec2(0, 1),
    vec2(1, 1),
    vec2(1, 0),
    vec2(0, 0)
};

int font_width[] = { 4, 2, 5, 6, 6, 6, 6, 3, 5, 5, 5, 6, 2, 6, 2, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 2, 2, 5, 6, 5, 6, 7, 6, 6, 6, 6, 6, 6, 6, 6, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 6, 4, 6, 6, 6, 6, 6, 6, 6, 6, 5, 6, 6, 2, 6, 5, 3, 6, 6, 6, 6, 6, 6, 6, 4, 6, 6, 6, 6, 6, 6, 5, 2, 5, 7 };

Text::Text(const ivec2& _pos)
{
    pos = _pos;
    str = "";

    vao = vbo = ebo = 0;
    indices_count = 0;
}

int Text::measure(const std::string& text,int size){
    int line=0,width=0;
    for(size_t i=0;i<text.size();){auto cp=Language::next(text,i);
        if(cp=='\n'){width=std::max(width,line);line=0;continue;}
        int glyph=Language::glyph(cp);line+=(glyph>=32 && glyph<=126?font_width[glyph-32]:7)*size;
    }return std::max(width,line);
}

void Text::set_str(const std::string& _str)
{
    if (str == _str) return;
    str = _str;

    destroy();
    build();
}

void Text::build()
{
    std::vector<FontVertex> vertices;
    std::vector<int> indices;



    int index = 0;
    vec2 start_pos = vec2(pos.x, pos.y - 12 * font_size);

    for (size_t i = 0; i < str.size();)
    {
        auto cp=Language::next(str,i);
        int c=Language::glyph(cp);

        int width = 7;

        if (c >= ' ' && c <= '~')
            width = font_width[c - ' '];

        if (c != ' ' && c != '\n')
        {
            vec2 uv = vec2(c % 16, c / 16) / 16.0f;

            for (int j = 0; j < 4; j++)
            {
                FontVertex v;

                vec2 p_off = vec2(font_vertex_offset[j].x * width, font_vertex_offset[j].y * 12);
                vec2 uv_off = vec2(font_uv_offset[j].x * width / 8.0f, font_uv_offset[j].y) / 16.0f;

                v.p = start_pos + p_off * (float)font_size;
                v.uv = uv + uv_off;

                vertices.push_back(v);
            }

            indices.push_back(index + 0);
            indices.push_back(index + 1);
            indices.push_back(index + 2);

            indices.push_back(index + 0);
            indices.push_back(index + 2);
            indices.push_back(index + 3);

            index += 4;
        }

        if (c == '\n')
            start_pos = vec2(pos.x, start_pos.y - 14 * font_size);
        else
            start_pos += vec2(width * font_size, 0);
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(FontVertex), vertices.data(), GL_STREAM_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STREAM_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(FontVertex), (void*)offsetof(FontVertex, p));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(FontVertex), (void*)offsetof(FontVertex, uv));
    glEnableVertexAttribArray(1);

    indices_count = indices.size();
}

void Text::draw(vec3 color)
{
    if (str.empty())
        return;

    font_shader.bind();
    font_shader.upload_vec3("tint",color);

    glBindVertexArray(vao);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawElements(GL_TRIANGLES, indices_count, GL_UNSIGNED_INT, 0);
}

void Text::destroy()
{
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
    vao = vbo = ebo = 0;
}
