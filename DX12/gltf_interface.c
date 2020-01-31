#include "gltf_interface.h"

#define CGLTF_IMPLEMENTATION
#include "libs/cgltf/cgltf.h"

#include <assert.h>


void create_gltf(struct gltf_file_info *gltf_file_data, struct gltf_gpu_info *gltf_gpu_data)
{
        cgltf_options options = { 0 };

        cgltf_result result = cgltf_parse_file(&options, gltf_file_data->file_name,
                &gltf_file_data->data);
        assert(result == cgltf_result_success);

        process_gltf_scene(gltf_file_data->data->scene, gltf_gpu_data);
}

void release_gltf(struct gltf_file_info* gltf_file_data)
{
        cgltf_free(gltf_file_data->data);
}

void process_gltf_scene(struct cgltf_scene *scene, struct gltf_gpu_info *gltf_gpu_data)
{
        for (UINT i = 0; i < scene->nodes_count; ++i) {
                process_gltf_node(scene->nodes[i], gltf_gpu_data);
        }
}

void process_gltf_node(struct cgltf_node *node, struct gltf_gpu_info *gltf_gpu_data)
{
        // Every node has only one mesh
        if (node->mesh) {
                process_gltf_mesh(node->mesh, gltf_gpu_data);
        }

        // Recurse through child nodes
        for (UINT i = 0; i < node->children_count; ++i) {
                process_gltf_node(node->children[i], gltf_gpu_data);
        }
}

void process_gltf_mesh(struct cgltf_mesh *mesh, struct gltf_gpu_info *gltf_gpu_data)
{
        // Process through mesh primitives
        for (UINT i = 0; i < mesh->primitives_count; ++i) {
                process_gltf_primitive(&mesh->primitives[i], gltf_gpu_data);
        }
}

void process_gltf_primitive(struct cgltf_primitive *primitive, struct gltf_gpu_info *gltf_gpu_data)
{
        for (UINT i = 0; i < primitive->attributes_count; ++i) {
                gltf_gpu_data->attribute_names[i] = primitive->attributes[i].name;
                cgltf_accessor *accessor = primitive->attributes[i].data;
                gltf_gpu_data->attribute_formats[i] =
                        get_gltf_format(accessor->type, accessor->component_type);
        }
}

DXGI_FORMAT get_gltf_format(cgltf_type type, cgltf_component_type component_type)
{
        switch (type)
        {
                case cgltf_type_scalar:
                {
                        switch (component_type)
                        {
                                case cgltf_component_type_r_8:
                                        return DXGI_FORMAT_R8_SINT;
                                case cgltf_component_type_r_8u:
                                        return DXGI_FORMAT_R8_UINT;
                                case cgltf_component_type_r_16:
                                        return DXGI_FORMAT_R16_SINT;
                                case cgltf_component_type_r_16u:
                                        return DXGI_FORMAT_R16_UINT;
                                case cgltf_component_type_r_32:
                                        return DXGI_FORMAT_R32_SINT;
                                case cgltf_component_type_r_32u:
                                        return DXGI_FORMAT_R32_UINT;
                                case cgltf_component_type_r_32f:
                                        return DXGI_FORMAT_R32_FLOAT;
                        }
                }

                case cgltf_type_vec2:
                {
                        switch (component_type)
                        {
                                case cgltf_component_type_r_8:
                                        return DXGI_FORMAT_R8G8_SINT;
                                case cgltf_component_type_r_8u:
                                        return DXGI_FORMAT_R8G8_UINT;
                                case cgltf_component_type_r_16:
                                        return DXGI_FORMAT_R16G16_SINT;
                                case cgltf_component_type_r_16u:
                                        return DXGI_FORMAT_R16G16_UINT;
                                case cgltf_component_type_r_32:
                                        return DXGI_FORMAT_R32G32_SINT;
                                case cgltf_component_type_r_32u:
                                        return DXGI_FORMAT_R32G32_UINT;
                                case cgltf_component_type_r_32f:
                                        return DXGI_FORMAT_R32G32_FLOAT;
                        }
                }

                case cgltf_type_vec3:
                {
                        switch (component_type)
                        {
                                case cgltf_component_type_r_8:
                                        return DXGI_FORMAT_UNKNOWN;
                                case cgltf_component_type_r_8u:
                                        return DXGI_FORMAT_UNKNOWN;
                                case cgltf_component_type_r_16:
                                        return DXGI_FORMAT_UNKNOWN;
                                case cgltf_component_type_r_16u:
                                        return DXGI_FORMAT_UNKNOWN;
                                case cgltf_component_type_r_32:
                                        return DXGI_FORMAT_R32G32B32_SINT;
                                case cgltf_component_type_r_32u:
                                        return DXGI_FORMAT_R32G32B32_UINT;
                                case cgltf_component_type_r_32f:
                                        return DXGI_FORMAT_R32G32B32_FLOAT;
                        }
                }

                case cgltf_type_vec4:
                {
                        switch (component_type)
                        {
                                case cgltf_component_type_r_8:
                                        return DXGI_FORMAT_R8G8B8A8_SINT;
                                case cgltf_component_type_r_8u:
                                        return DXGI_FORMAT_R8G8B8A8_UINT;
                                case cgltf_component_type_r_16:
                                        return DXGI_FORMAT_R16G16B16A16_SINT;
                                case cgltf_component_type_r_16u:
                                        return DXGI_FORMAT_R16G16B16A16_UINT;
                                case cgltf_component_type_r_32:
                                        return DXGI_FORMAT_R32G32B32A32_SINT;
                                case cgltf_component_type_r_32u:
                                        return DXGI_FORMAT_R32G32B32A32_UINT;
                                case cgltf_component_type_r_32f:
                                        return DXGI_FORMAT_R32G32B32A32_FLOAT;
                        }
                }

                default:
                        return DXGI_FORMAT_UNKNOWN;
        }
}