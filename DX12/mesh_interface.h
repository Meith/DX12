#ifndef MESH_INTERFACE_H
#define MESH_INTERFACE_H

#include "linmath.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <dxgiformat.h>


struct vertex {
        vec3 position;
        vec2 uv;
};

struct mesh_info {
        unsigned int vertex_count;
        UINT64 stride;
        DXGI_FORMAT vertex_pos_format;
        struct vertex *verticies;
        unsigned int index_count;
        DXGI_FORMAT index_format;
        unsigned int *indices;
};

void create_triangle(struct mesh_info *mi);
void release_triangle(struct mesh_info *mi);

#endif