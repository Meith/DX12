#include "gltf_interface.h"
#include "misc.h"

#include <assert.h>

#define CGLTF_IMPLEMENTATION
#include "libs/cgltf/cgltf.h"


void create_gltf(const char *file_name, struct cgltf_data **gltf_pdata)
{
        cgltf_options options = { 0 };

        cgltf_result result = cgltf_parse_file(&options, file_name, gltf_pdata);
        assert(result == cgltf_result_success);

        result = cgltf_load_buffers(&options, *gltf_pdata, file_name);
        assert(result == cgltf_result_success);
}

void process_gltf_nodes(size_t node_count, struct gltf_node_info *node_info,
        struct cgltf_node *node_data)
{
        for (UINT i = 0, j = 0; i < node_count; ++i) {

                // We have all nodes in a flat list thanks to cgltf
                // I think its safe to ingnore all nodes that don't have meshes
                if (node_data[i].mesh == NULL) continue;

                cgltf_node_transform_world(&node_data[i],
                        node_info[j].world_transform[0]);

                node_info[j].mesh_info = malloc(sizeof (struct gltf_mesh_info));
                process_gltf_meshes(node_info[j].mesh_info, node_data[i].mesh);
                ++j;
        }
}

void process_gltf_meshes(struct gltf_mesh_info *mesh_info,
        struct cgltf_mesh *mesh_data)
{
        // Just for safety make sure we have only one primitive
        assert(mesh_data->primitives_count == 1);

        // process primitive for each mesh
        process_gltf_primitives(&mesh_info->primitive_info,
                mesh_data->primitives);
}

void process_gltf_primitives(struct gltf_primitive_info *primitive_info,
        struct cgltf_primitive *primitive_data)
{
        // Process attributes
        primitive_info->attribute_info.attribute_type_count =
                primitive_data->attributes_count;
        process_gltf_attributes(&primitive_info->attribute_info,
                primitive_data->attributes);

        cgltf_accessor *index_accessor = primitive_data->indices;
        primitive_info->indices_count = index_accessor->count;
        primitive_info->index_format =
                process_gltf_format(index_accessor->type,
                index_accessor->component_type);
        primitive_info->index_stride = index_accessor->stride;
        primitive_info->index_size = index_accessor->count *
                index_accessor->stride;
        primitive_info->index_data =
                malloc(primitive_info->index_size);

        cgltf_buffer_view *index_buffer_view =
                index_accessor->buffer_view;
        cgltf_buffer *index_buffer = index_buffer_view->buffer;
        BYTE *index_buffer_data = index_buffer->data;

        memcpy(primitive_info->index_data,
                &index_buffer_data[index_buffer_view->offset +
                index_accessor->offset],
                primitive_info->index_size);
}

void process_gltf_attributes(struct gltf_attribute_info *attribute_info,
        struct cgltf_attribute *attribute_data)
{
        attribute_info->attribute_type_names =
                malloc(attribute_info->attribute_type_count * sizeof (LPCSTR));
        attribute_info->attribute_type_indexes =
                malloc(attribute_info->attribute_type_count * sizeof (int));
        attribute_info->attribute_type_data_format =
                malloc(attribute_info->attribute_type_count *
                        sizeof (DXGI_FORMAT));
        attribute_info->attribute_type_data_stride =
                malloc(attribute_info->attribute_type_count * sizeof (UINT64));

        attribute_info->attribute_data_stride = 0;

        for (UINT i = 0; i < attribute_info->attribute_type_count; ++i) {

                // Semantic names are in format  NAME_[index]
                // So need to get rid of _[index]
                // For attribute setup in gpu_interface
                truncate_string(attribute_data[i].name, '_');
                attribute_info->attribute_type_names[i] =
                        attribute_data[i].name;

                attribute_info->attribute_type_indexes[i] =
                        attribute_data[i].index;

                cgltf_accessor *attribute_data_accessor =
                        attribute_data[i].data;
                attribute_info->attribute_type_data_format[i] =
                        process_gltf_format(attribute_data_accessor->type,
                        attribute_data_accessor->component_type);
                attribute_info->attribute_type_data_stride[i] =
                        attribute_data_accessor->stride;

                attribute_info->attribute_data_stride +=
                        attribute_data_accessor->stride;
        }

        // Lets just use first attribute accessor to get count
        cgltf_accessor *attribute_data_accessor = attribute_data[0].data;
        attribute_info->attribute_data_count = attribute_data_accessor->count;

        // Size should be count * stride
        attribute_info->attribute_data_size =
                attribute_info->attribute_data_stride *
                attribute_info->attribute_data_count;
        attribute_info->attribute_data =
                malloc(attribute_info->attribute_data_size);

        BYTE *attribute_type_data = attribute_info->attribute_data;
        for (UINT i = 0; i < attribute_info->attribute_data_count; ++i) {

                for (UINT j = 0; j < attribute_info->attribute_type_count; ++j) {

                        attribute_data_accessor = attribute_data[j].data;

                        cgltf_buffer_view *attribute_buffer_view =
                                attribute_data_accessor->buffer_view;
                        cgltf_buffer *attribute_buffer =
                                attribute_buffer_view->buffer;
                        BYTE *attribute_buffer_data = attribute_buffer->data;

                        size_t offset = attribute_buffer_view->offset + // start of whole buffer data
                                attribute_data_accessor->offset + // offset to the current attribute
                                i * attribute_data_accessor->stride; // offset to the index of current attribute

                        // This is probably not the best way to store data given how its laid out.
                        // I'm forcing interleaving and significantly slowing down copy process.
                        memcpy(attribute_type_data,
                        &attribute_buffer_data[offset],
                        attribute_data_accessor->stride);

                        // Increment total data stride size in bytes to copy next chunk
                        attribute_type_data += attribute_data_accessor->stride;
                }
        }
}

DXGI_FORMAT process_gltf_format(enum cgltf_type type,
        enum cgltf_component_type component_type)
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

void release_gltf_nodes(size_t node_count, struct gltf_node_info *node_info,
        struct cgltf_node *node_data)
{
        for (UINT i = 0, j = 0; i < node_count; ++i) {

                if (node_data[i].mesh == NULL) continue;

                release_gltf_meshes(node_info[j].mesh_info, node_data[i].mesh);

                free(node_info[j].mesh_info);
                ++j;
        }
}

void release_gltf_meshes(struct gltf_mesh_info *mesh_info,
        struct cgltf_mesh *mesh_data)
{
        release_gltf_primitives(&mesh_info->primitive_info,
                mesh_data->primitives);
}

void release_gltf_primitives(struct gltf_primitive_info *primitive_info,
        struct cgltf_primitive *primitive_data)
{
        free(primitive_info->index_data);

        release_gltf_attributes(&primitive_info->attribute_info,
                primitive_data->attributes);
}

void release_gltf_attributes(struct gltf_attribute_info *attribute_info,
        struct cgltf_attribute *attribute_data)
{
        free(attribute_info->attribute_data);
        free(attribute_info->attribute_type_data_stride);
        free(attribute_info->attribute_type_data_format);
        free(attribute_info->attribute_type_indexes);
        free((CHAR *) attribute_info->attribute_type_names);
}

void release_gltf(struct cgltf_data *gltf_data)
{
        cgltf_free(gltf_data);
}