#include "gltf_interface.h"

#define CGLTF_IMPLEMENTATION
#include "libs/cgltf/cgltf.h"

#include <assert.h>


void create_gltf(struct gltf_interface_info *gltf_info)
{
        cgltf_options options = { 0 };

        cgltf_result result = cgltf_parse_file(&options, gltf_info->file_name,
                &gltf_info->data);
        assert(result == cgltf_result_success);

        process_gltf_scene(gltf_info->data->scene);
}

void release_gltf(struct gltf_interface_info *gltf_info)
{
        cgltf_free(gltf_info->data);
}

void process_gltf_scene(struct cgltf_scene *scene)
{
        for (UINT i = 0; i < scene->nodes_count; ++i) {
                process_gltf_node(scene->nodes[i]);
        }
}

void process_gltf_node(struct cgltf_node *node)
{
        // Every node has only one mesh
        if (node->mesh) {
                process_gltf_mesh(node->mesh);
        }

        // Recurse through child nodes
        for (UINT i = 0; i < node->children_count; ++i) {
                process_gltf_node(node->children[i]);
        }
}

void process_gltf_mesh(struct cgltf_mesh *mesh)
{
        // Process through mesh primitives
        for (UINT i = 0; i < mesh->primitives_count; ++i) {
                process_gltf_primitive(&mesh->primitives[i]);
        }
}

void process_gltf_primitive(struct cgltf_primitive *prmitive)
{
}