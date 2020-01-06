#ifndef GLTF_INTERFACE_H
#define GLTF_INTERFACE_H

#include "gpu_interface.h"


struct gltf_interface_info {
        const char *file_name;
        struct cgltf_data *data;
};

void create_gltf(struct gltf_interface_info *gltf_info);
void release_gltf(struct gltf_interface_info *gltf_info);
void process_gltf_scene(struct cgltf_scene *scene);
void process_gltf_node(struct cgltf_node *node);
void process_gltf_mesh(struct cgltf_mesh *mesh);
void process_gltf_primitive(struct cgltf_primitive* prmitive);

#endif
