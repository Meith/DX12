#ifndef GLTF_INTERFACE_H
#define GLTF_INTERFACE_H

#include "gpu_interface.h"

struct gltf_file_info {
        const char *file_name;
        struct cgltf_data *data;
};

#define MAX_ATTRIBUTE_COUNT 10

struct gltf_gpu_info {
        LPCSTR attribute_names[MAX_ATTRIBUTE_COUNT];
        DXGI_FORMAT attribute_formats[MAX_ATTRIBUTE_COUNT];
};

void create_gltf(struct gltf_file_info *gltf_file_data, struct gltf_gpu_info *gltf_gpu_data);
void release_gltf(struct gltf_file_info *gltf_file_data);
void process_gltf_scene(struct cgltf_scene *scene, struct gltf_gpu_info *gltf_gpu_data);
void process_gltf_node(struct cgltf_node *node, struct gltf_gpu_info *gltf_gpu_data);
void process_gltf_mesh(struct cgltf_mesh *mesh, struct gltf_gpu_info *gltf_gpu_data);
void process_gltf_primitive(struct cgltf_primitive *primitive, struct gltf_gpu_info *gltf_gpu_data);
DXGI_FORMAT get_gltf_format(enum cgltf_type type, enum cgltf_component_type component_type);

#endif
