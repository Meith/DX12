#ifndef MATERIAL_INTERFACE_H
#define MATERIAL_INTERFACE_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


struct material_info {
        UINT8 *tex;
        UINT64 tex_size;
};

void get_checkerboard_tex(UINT64 width, UINT64 height, struct material_info *mat_info);
void release_material(struct material_info *mat_info);

#endif