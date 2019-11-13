#ifndef CAMERA_INTERFACE_H
#define CAMERA_INTERFACE_H

#include "linmath.h"

struct camera_info {
        mat4x4 view_mat;
        mat4x4 projection_mat;
        mat4x4 pv_mat;
};

void calc_pv_mat(struct camera_info *ci);

#endif