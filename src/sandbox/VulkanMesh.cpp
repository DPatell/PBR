#include "VulkanMesh.hpp"
#include "VulkanUtils.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>

vulkan_mesh::~vulkan_mesh()
{
    clear_gpu_data();
    clear_cpu_data();
}

VkVertexInputBindingDescription vulkan_mesh::get_vertex_input_binding_description()
{
    VkVertexInputBindingDescription vertex_input_binding_description;
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = sizeof(vertex);
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return vertex_input_binding_description;
}

std::array<VkVertexInputAttributeDescription, 3> vulkan_mesh::get_vertex_input_attribute_descriptions()
{
    std::array<VkVertexInputAttributeDescription, 3> vertex_input_attribute_descriptions = {};

    vertex_input_attribute_descriptions[0].binding = 0;
    vertex_input_attribute_descriptions[0].location = 0;
    vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_descriptions[0].offset = offsetof(vertex, position);

    vertex_input_attribute_descriptions[1].binding = 0;
    vertex_input_attribute_descriptions[1].location = 1;
    vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_descriptions[1].offset = offsetof(vertex, color);

    vertex_input_attribute_descriptions[2].binding = 0;
    vertex_input_attribute_descriptions[2].location = 2;
    vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_input_attribute_descriptions[2].offset = offsetof(vertex, uv);

    return vertex_input_attribute_descriptions;
}

bool vulkan_mesh::load_from_file(const std::string& path)
{
    Assimp::Importer importer;

    unsigned int flags = aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType;
    const aiScene* scene = importer.ReadFile(path, flags);

    if (!scene)
    {
        std::cerr << importer.GetErrorString() << std::endl;
        return false;
    }

    if (!scene->HasMeshes())
    {
        std::cerr << "vulkan_mesh::load_from_file(): model has no meshs" << std::endl;
    }

    aiMesh* mesh = scene->mMeshes[0];
    assert(mesh != nullptr, "Mesh is null");

    vertices.resize(mesh->mNumVertices);
    indices.resize(mesh->mNumFaces * 3);

    aiVector3D* mesh_vertices = mesh->mVertices;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        vertices[i].position = glm::vec3(mesh_vertices[i].x, mesh_vertices[i].y, mesh_vertices[i].z);
    }

    aiVector3D* mesh_uvs = mesh->mTextureCoords[0];
    if (mesh_uvs)
    {
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            vertices[i].uv = glm::vec2(mesh_uvs[i].x, 1.0f - mesh_uvs[i].y);
        }
    }
    else
    {
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            vertices[i].uv = glm::vec2(0.0f, 0.0f);
        }
    }

    aiColor4D* mesh_colors = mesh->mColors[0];
    if (mesh_colors)
    {
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            vertices[i].color = glm::vec3(mesh_colors[i].r, mesh_colors[i].g, mesh_colors[i].b);
        }
    }
    else
    {
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            vertices[i].color = glm::vec3(1.0f, 1.0f, 1.0f);
        }
    }

    aiFace* mesh_faces = mesh->mFaces;
    unsigned int index = 0;
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        for (unsigned int face_index = 0; face_index < mesh_faces[i].mNumIndices; face_index++)
        {
            indices[index++] = mesh_faces[i].mIndices[face_index];
        }
    }

    // NOTE(dhaval): Upload cpu data to gpu
    upload_to_gpu();

    // TODO(dhaval): clear cpu data?

    return true;
}

/**
 * \brief
 */
void vulkan_mesh::create_vertex_buffer()
{
    VkDeviceSize buffer_size = sizeof(vertex) * vertices.size();

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

    vulkan_utils::create_buffer(vk_renderer_context_, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_vertex_buffer_,
                                vk_vertex_buffer_memory_);

    // NOTE(dhaval): Create staging buffer.
    vulkan_utils::create_buffer(vk_renderer_context_, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer,
                                staging_buffer_memory);

    // NOTE(dhaval): Fill staging buffer.
    void* data = nullptr;
    VK_CHECK(vkMapMemory(vk_renderer_context_.vk_device_, staging_buffer_memory, 0, buffer_size, 0, &data));
    memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
    vkUnmapMemory(vk_renderer_context_.vk_device_, staging_buffer_memory);

    // NOTE(dhaval): Transfer to GPU local memory.
    vulkan_utils::copy_buffer(vk_renderer_context_, staging_buffer, vk_vertex_buffer_, buffer_size);

    // NOTE(dhaval): Destroy staging buffer.
    vkDestroyBuffer(vk_renderer_context_.vk_device_, staging_buffer, nullptr);
    vkFreeMemory(vk_renderer_context_.vk_device_, staging_buffer_memory, nullptr);
}

/**
 * \brief
 */
void vulkan_mesh::create_index_buffer()
{
    VkDeviceSize buffer_size = sizeof(uint32_t) * indices.size();

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

    vulkan_utils::create_buffer(vk_renderer_context_, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_index_buffer_,
                                vk_index_buffer_memory_);

    // NOTE(dhaval): Create staging buffer.
    vulkan_utils::create_buffer(vk_renderer_context_, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer,
                                staging_buffer_memory);

    // NOTE(dhaval): Fill staging buffer.
    void* data = nullptr;
    VK_CHECK(vkMapMemory(vk_renderer_context_.vk_device_, staging_buffer_memory, 0, buffer_size, 0, &data));
    memcpy(data, indices.data(), static_cast<size_t>(buffer_size));
    vkUnmapMemory(vk_renderer_context_.vk_device_, staging_buffer_memory);

    // NOTE(dhaval): Transfer to GPU local memory.
    vulkan_utils::copy_buffer(vk_renderer_context_, staging_buffer, vk_index_buffer_, buffer_size);

    // NOTE(dhaval): Destroy staging buffer.
    vkDestroyBuffer(vk_renderer_context_.vk_device_, staging_buffer, nullptr);
    vkFreeMemory(vk_renderer_context_.vk_device_, staging_buffer_memory, nullptr);
}

void vulkan_mesh::upload_to_gpu()
{
    create_vertex_buffer();
    create_index_buffer();
}

void vulkan_mesh::clear_gpu_data()
{
    vkDestroyBuffer(vk_renderer_context_.vk_device_, vk_vertex_buffer_, nullptr);
    vk_vertex_buffer_ = VK_NULL_HANDLE;

    vkFreeMemory(vk_renderer_context_.vk_device_, vk_vertex_buffer_memory_, nullptr);
    vk_vertex_buffer_memory_ = VK_NULL_HANDLE;

    vkDestroyBuffer(vk_renderer_context_.vk_device_, vk_index_buffer_, nullptr);
    vk_index_buffer_ = VK_NULL_HANDLE;

    vkFreeMemory(vk_renderer_context_.vk_device_, vk_index_buffer_memory_, nullptr);
    vk_index_buffer_memory_ = VK_NULL_HANDLE;
}

void vulkan_mesh::clear_cpu_data()
{
    vertices.clear();
    indices.clear();
}

