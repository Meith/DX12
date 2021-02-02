#ifndef GLTF_INTERFACE_H
#define GLTF_INTERFACE_H

#include "linmath.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <dxgiformat.h>


// SOA
struct gltf_attribute_info
{
        size_t attribute_type_count;

        LPCSTR *attribute_type_names;
        int *attribute_type_indexes;

        DXGI_FORMAT *attribute_type_data_format;
        UINT64 *attribute_type_data_stride;

        size_t attribute_data_count;
        size_t attribute_data_size;
        UINT64 attribute_data_stride;
        void *attribute_data;
};

// AOS
struct gltf_primitive_info
{
        struct gltf_attribute_info attribute_info;

        size_t indices_count;
        DXGI_FORMAT index_format;
        UINT64 index_stride;
        size_t index_size;
        void *index_data;
};

// AOS
struct gltf_mesh_info
{
        // Seems like all meshes have only one primitive
        struct gltf_primitive_info primitive_info;
};

// AOS
struct gltf_node_info
{
        mat4x4 world_transform;
        struct gltf_mesh_info *mesh_info;
};

void create_gltf(const char *file_name, struct cgltf_data **gltf_pdata);
void process_gltf_nodes(size_t node_count, struct gltf_node_info *node_info,
        struct cgltf_node *node_data);
void process_gltf_meshes(struct gltf_mesh_info *mesh_info,
        struct cgltf_mesh *mesh_data);
void process_gltf_primitives(struct gltf_primitive_info *primitive_info,
        struct cgltf_primitive *primitive_data);
void process_gltf_attributes(struct gltf_attribute_info *attribute_info,
        struct cgltf_attribute *attribute_data);
DXGI_FORMAT process_gltf_format(enum cgltf_type type,
        enum cgltf_component_type component_type);
void release_gltf_nodes(size_t node_count, struct gltf_node_info *node_info,
        struct cgltf_node *node_data);
void release_gltf_meshes(struct gltf_mesh_info *mesh_info,
        struct cgltf_mesh *mesh_data);
void release_gltf_primitives(struct gltf_primitive_info *primitive_info,
        struct cgltf_primitive *primitive_data);
void release_gltf_attributes(struct gltf_attribute_info *attribute_info,
        struct cgltf_attribute *attribute_data);
void release_gltf(struct cgltf_data *gltf_data);


#endif
