#ifndef CAMERA_INTERFACE_H
#define CAMERA_INTERFACE_H

#include "linmath.h"

struct camera_info {
        mat4x4 view_mat;
        mat4x4 proj_mat;
        mat4x4 pv_mat;
};

void calc_proj_view_mat(struct camera_info *ci);

#endif