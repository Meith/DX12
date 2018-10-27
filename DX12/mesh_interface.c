#include "mesh_interface.h"

#include <stdlib.h>


void create_triangle(struct mesh_info *mi)
{
        mi->vertex_count = 3;

        mi->verticies = malloc(mi->vertex_count * sizeof (struct vertex));

        // Bottom left vertex
        mi->verticies[0].position[0] = -1.0f;
        mi->verticies[0].position[1] = -1.0f;
        mi->verticies[0].position[2] = 0.0f;
        mi->verticies[0].position[3] = 1.0f;
        mi->verticies[0].colour[0] = 1.0f;
        mi->verticies[0].colour[1] = 0.0f;
        mi->verticies[0].colour[2] = 0.0f;
        mi->verticies[0].colour[3] = 1.0f;

        // Top vertex
        mi->verticies[1].position[0] = 0.0f;
        mi->verticies[1].position[1] = 1.0f;
        mi->verticies[1].position[2] = 0.0f;
        mi->verticies[1].position[3] = 1.0f;
        mi->verticies[1].colour[0] = 0.0f;
        mi->verticies[1].colour[1] = 1.0f;
        mi->verticies[1].colour[2] = 0.0f;
        mi->verticies[1].colour[3] = 1.0f;

        // Bottom right vertex
        mi->verticies[2].position[0] = 1.0f;
        mi->verticies[2].position[1] = -1.0f;
        mi->verticies[2].position[2] = 0.0f;
        mi->verticies[2].position[3] = 1.0f;
        mi->verticies[2].colour[0] = 0.0f;
        mi->verticies[2].colour[1] = 0.0f;
        mi->verticies[2].colour[2] = 1.0f;
        mi->verticies[2].colour[3] = 1.0f;

        mi->index_count = 3;
        mi->indices = malloc(mi->index_count * sizeof(unsigned int));
        mi->indices[0] = 0;
        mi->indices[1] = 1;
        mi->indices[2] = 2;
}

void release_triangle(struct mesh_info *mi)
{
        free(mi->verticies);
        free(mi->indices);
}