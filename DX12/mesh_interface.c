#include "mesh_interface.h"

#include <stdlib.h>


void create_triangle(struct mesh_info *mi)
{
        mi->vertex_count = 3;
        mi->stride = sizeof (struct vertex);
        mi->vertex_pos_format = DXGI_FORMAT_R32G32B32_FLOAT;

        mi->verticies = malloc(mi->vertex_count * sizeof (struct vertex));

        // Bottom left vertex
        mi->verticies[0].position[0] = -1.0f;
        mi->verticies[0].position[1] = -1.0f;
        mi->verticies[0].position[2] = 0.0f;
        mi->verticies[0].uv[0] = 0.0f;
        mi->verticies[0].uv[1] = 0.0f;

        // Top
        mi->verticies[1].position[0] = 0.0f;
        mi->verticies[1].position[1] = 1.0f;
        mi->verticies[1].position[2] = 0.0f;
        mi->verticies[1].uv[0] = 0.5f;
        mi->verticies[1].uv[1] = 1.0f;

        // Bottom right vertex
        mi->verticies[2].position[0] = 1.0f;
        mi->verticies[2].position[1] = -1.0f;
        mi->verticies[2].position[2] = 0.0f;
        mi->verticies[2].uv[0] = 1.0f;
        mi->verticies[2].uv[1] = 0.0f;

        mi->index_count = 3;
        mi->index_format = DXGI_FORMAT_R32_UINT;

        mi->indices = malloc(mi->index_count * sizeof (unsigned int));
        mi->indices[0] = 0;
        mi->indices[1] = 1;
        mi->indices[2] = 2;
}

void release_triangle(struct mesh_info *mi)
{
        free(mi->verticies);
        free(mi->indices);
}