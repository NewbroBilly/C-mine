#include <chunk.h>

ChunkMesh::ChunkMesh()
{
    this->chunk = NULL;

    vao = vbo = ebo = 0;
    indices_count = 0;
}

ChunkMesh::ChunkMesh(Chunk* chunk)
{
    this->chunk = chunk;

    vao = vbo = ebo = 0;
    indices_count = 0;
}

void ChunkMesh::build()
{
    const GLuint old_vao = vao, old_vbo = vbo, old_ebo = ebo;
    std::vector<Vertex> vertices;
    std::vector<int> indices;

    int index = 0;

    for(int pass=0;pass<2;++pass){
    for (int x = 0; x < CHUNK_WIDTH; x++)
        for (int y = 0; y < CHUNK_HEIGHT; y++)
            for (int z = 0; z < CHUNK_WIDTH; z++)
            {
                ivec3 pos = ivec3(x, y, z);

                BlockState* bs = chunk->get(pos);
                if((bs->id==WATER)!=(pass==1))continue;

                Block* block = get_block_by_id(bs->id);

                block->build(chunk, pos, vertices, indices, index);
            }
    if(pass==0)opaque_count=int(indices.size());
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STREAM_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribIPointer(1, 1, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, data));
    glEnableVertexAttribArray(1);

    indices_count = indices.size();
    // Keep the old mesh alive until its replacement is fully uploaded.
    glBindVertexArray(0);
    glDeleteBuffers(1, &old_vbo);
    glDeleteBuffers(1, &old_ebo);
    glDeleteVertexArrays(1, &old_vao);
}

void ChunkMesh::draw(bool water)
{
    if (!vao || !indices_count) return;
    glBindVertexArray(vao);

    glPolygonMode(GL_FRONT_AND_BACK, show_wireframe ? GL_LINE : GL_FILL);
    glDrawElements(GL_TRIANGLES, water?indices_count-opaque_count:opaque_count, GL_UNSIGNED_INT, reinterpret_cast<void*>(water?size_t(opaque_count)*sizeof(int):0));
}

void ChunkMesh::destroy()
{
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
    vao = vbo = ebo = 0;
    indices_count = 0;
}
