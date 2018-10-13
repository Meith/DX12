#ifndef MESH_INTERFACE_H
#define MESH_INTERFACE_H

#include "linmath.h"


struct vertex {
        vec4 position;
        vec4 colour;
};

struct mesh_info {
        unsigned int vertex_count;
        struct vertex *verticies;
        unsigned int index_count;
        unsigned int *indices;
};

void create_triangle(struct mesh_info *mi);
void release_triangle(struct mesh_info *mi);

#endif