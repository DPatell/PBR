#pragma once

#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <vector>
#include <string>

#include "VulkanRendererContext.hpp"

class vulkan_mesh
{
public:
    vulkan_mesh(const renderer_context& renderer_context) : vk_renderer_context_(renderer_context)
    {
    }

    ~vulkan_mesh();

    inline VkBuffer get_vertex_buffer() const { return vk_vertex_buffer_; }
    inline VkBuffer get_index_buffer() const { return vk_index_buffer_; }
    inline uint32_t get_num_indices() const { return static_cast<uint32_t>(indices.size()); }

    static VkVertexInputBindingDescription get_vertex_input_binding_description();
    static std::array<VkVertexInputAttributeDescription, 3> get_vertex_input_attribute_descriptions();

    bool load_from_file(const std::string& path);

    void upload_to_gpu();
    void clear_gpu_data();
    void clear_cpu_data();

private:
    void create_vertex_buffer();
    void create_index_buffer();

private:
    renderer_context vk_renderer_context_;

    struct vertex
    {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 uv;
    };

    std::vector<vertex> vertices;
    std::vector<uint32_t> indices;

    VkBuffer vk_vertex_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory vk_vertex_buffer_memory_{VK_NULL_HANDLE};

    VkBuffer vk_index_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory vk_index_buffer_memory_{VK_NULL_HANDLE};
};
