#include "material_interface.h"

#include <stdlib.h>


void get_checkerboard_tex(UINT64 width, UINT64 height, struct material_info *mat_info)
{
        const UINT64 texture_pixel_size = 4;
        const UINT64 row_pitch = width * texture_pixel_size;
        const UINT64 cell_pitch = row_pitch >> 3;
        const UINT64 cell_height = width >> 3;
        const UINT64 texture_size = row_pitch * height;

        mat_info->tex_size = texture_size * sizeof(UINT8);
        mat_info->tex = malloc (mat_info->tex_size);

        for (UINT64 n = 0; n < texture_size; n += texture_pixel_size)
        {
                UINT64 x = n % row_pitch;
                UINT64 y = n / row_pitch;
                UINT64 i = x / cell_pitch;
                UINT64 j = y / cell_height;

                if (i % 2 == j % 2)
                {
                        mat_info->tex[n] = 0x00;        // R
                        mat_info->tex[n + 1] = 0x00;    // G
                        mat_info->tex[n + 2] = 0x00;    // B
                        mat_info->tex[n + 3] = 0xff;    // A
                }
                else
                {
                        mat_info->tex[n] = 0xff;        // R
                        mat_info->tex[n + 1] = 0xff;    // G
                        mat_info->tex[n + 2] = 0xff;    // B
                        mat_info->tex[n + 3] = 0xff;    // A
                }
        }
}

void release_material(struct material_info *mat_info)
{
        free(mat_info->tex);
}