#include "camera_interface.h"

void calc_proj_view_mat(struct camera_info *ci)
{
        ci->pv_mat[0][0] = 1.0f; 
        ci->pv_mat[0][1] = 0.0f; 
        ci->pv_mat[0][2] = 0.0f; 
        ci->pv_mat[0][3] = 0.0f;
        
        ci->pv_mat[1][0] = 0.0f; 
        ci->pv_mat[1][1] = 1.0f; 
        ci->pv_mat[1][2] = 0.0f; 
        ci->pv_mat[1][3] = 0.0f;
        
        ci->pv_mat[2][0] = 0.0f; 
        ci->pv_mat[2][1] = 0.0f; 
        ci->pv_mat[2][2] = 1.0f; 
        ci->pv_mat[2][3] = 0.0f;
        
        ci->pv_mat[3][0] = 0.0f; 
        ci->pv_mat[3][1] = 0.0f; 
        ci->pv_mat[3][2] = 0.0f; 
        ci->pv_mat[3][3] = 1.0f;
}