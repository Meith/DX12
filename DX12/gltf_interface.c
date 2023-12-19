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

        /*process_gltf_materials(&primitive_info->material_info,
                primitive_data->material);*/
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

void process_gltf_materials(struct gltf_material_info *material_info,
        struct cgltf_material *material_data)
{
        if (material_data->has_pbr_metallic_roughness) {

                cgltf_pbr_metallic_roughness *pbr_metallic_roughness =
                        &material_data->pbr_metallic_roughness;

                material_info->base_texture_file_name =
                        process_gltf_textures(&pbr_metallic_roughness->base_color_texture);

                material_info->metallic_roughness_texture_file_name =
                        process_gltf_textures(&pbr_metallic_roughness->metallic_roughness_texture);

                memcpy(material_info->base_color_factor,
                        pbr_metallic_roughness->base_color_factor,
                        sizeof (material_info->base_color_factor));
                material_info->metallic_factor =
                        pbr_metallic_roughness->metallic_factor;
                material_info->roughness_factor =
                        pbr_metallic_roughness->roughness_factor;
        }

        if (material_data->has_pbr_specular_glossiness) {

                cgltf_pbr_specular_glossiness *pbr_specular_glossiness =
                        &material_data->pbr_specular_glossiness;

                material_info->diffuse_texture_file_name =
                        process_gltf_textures(&pbr_specular_glossiness->diffuse_texture);

                material_info->specular_glossiness_texture_file_name =
                        process_gltf_textures(&pbr_specular_glossiness->specular_glossiness_texture);

                memcpy(material_info->diffuse_factor,
                        pbr_specular_glossiness->diffuse_factor,
                        sizeof (material_info->diffuse_factor));
                memcpy(material_info->specular_factor,
                        pbr_specular_glossiness->specular_factor,
                        sizeof (material_info->specular_factor));
                material_info->glossiness_factor =
                        pbr_specular_glossiness->glossiness_factor;
        }

        material_info->normal_texture_file_name =
                process_gltf_textures(&material_data->normal_texture);

        material_info->occlusion_texture_file_name =
                process_gltf_textures(&material_data->occlusion_texture);

        material_info->emissive_texture_file_name =
                process_gltf_textures(&material_data->emissive_texture);

        memcpy(material_info->emissive_factor, material_data->emissive_factor,
                sizeof (material_info->emissive_factor));
        material_info->alpha_blend_mode = material_data->alpha_mode;
        material_info->alpha_cutoff = material_data->alpha_cutoff;
        material_info->double_sided = material_data->double_sided;
        material_info->unlit = material_data->unlit;
 }

char* process_gltf_textures(cgltf_texture_view* texture_view)
{
        cgltf_texture* texture = texture_view->texture;

        cgltf_image* image = texture->image;

        size_t texture_file_name_len = strlen(image->uri);

        char* texture_file_name = malloc(texture_file_name_len + 1 * sizeof (char));
        strcpy(texture_file_name, image->uri);
        texture_file_name[texture_file_name_len] = '\0';

        return texture_file_name;
}

void release_gltf_nodes(size_t node_count, struct gltf_node_info *node_info,
        struct cgltf_node *node_data)
{
        for (UINT i = 0, j = 0; i < node_count; ++i) {

                if (node_data[i].mesh == NULL) continue;

                release_gltf_meshes(node_info[j].mesh_info);

                free(node_info[j].mesh_info);
                ++j;
        }
}

void release_gltf_meshes(struct gltf_mesh_info *mesh_info)
{
        release_gltf_primitives(&mesh_info->primitive_info);
}

void release_gltf_primitives(struct gltf_primitive_info *primitive_info)
{
        free(primitive_info->index_data);

        release_gltf_attributes(&primitive_info->attribute_info);

        //release_gltf_materials(&primitive_info->material_info);
}

void release_gltf_attributes(struct gltf_attribute_info *attribute_info)
{
        free(attribute_info->attribute_data);
        free(attribute_info->attribute_type_data_stride);
        free(attribute_info->attribute_type_data_format);
        free(attribute_info->attribute_type_indexes);
        free((CHAR *) attribute_info->attribute_type_names);
}

void release_gltf_materials(struct gltf_material_info *material_info)
{
        free(material_info->base_texture_file_name);
        free(material_info->metallic_roughness_texture_file_name);
        free(material_info->diffuse_texture_file_name);
        free(material_info->specular_glossiness_texture_file_name);
        free(material_info->normal_texture_file_name);
        free(material_info->occlusion_texture_file_name);
        free(material_info->emissive_texture_file_name);
}

void release_gltf(struct cgltf_data *gltf_data)
{
        cgltf_free(gltf_data);
}
