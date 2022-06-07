#pragma once

#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <string>

#include "VulkanRendererContext.hpp"

/**
 * \brief
 */
struct uniform_buffer_object
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

/**
 * \brief
 */
struct vertex
{
    glm::vec2 position;
    glm::vec3 color;
    glm::vec2 uv;

    static VkVertexInputBindingDescription get_vertex_input_binding_description();
    static std::array<VkVertexInputAttributeDescription, 3> get_vertex_input_attribute_descriptions();
};

/**
 * \brief
 */
class render_data
{
public:
    render_data(const renderer_context& renderer_context) : renderer_context_(renderer_context)
    {
    }

    void init(const std::string& vertex_shader_file, const std::string& fragment_shader_file, const std::string& texture_file);
    void shutdown();

    inline VkShaderModule get_vertex_shader() const { return vk_vertex_shader_; };
    inline VkShaderModule get_fragment_shader() const { return vk_fragment_shader_; };

    inline VkBuffer get_vertex_buffer() const { return vk_vertex_buffer_; };
    inline VkBuffer get_index_buffer() const { return vk_index_buffer_; };

    inline VkImageView get_texture_image_view() const { return vk_texture_image_view_; };
    inline VkSampler get_texture_image_sampler() const { return vk_texture_image_sampler_; };

    uint32_t get_number_of_indicies() const;
private:
    VkShaderModule create_shader(const std::string& path) const;

    void create_vertex_buffer();
    void create_index_buffer();

    void create_image(const std::string& path);
private:
    renderer_context renderer_context_;

    VkShaderModule vk_vertex_shader_{VK_NULL_HANDLE};
    VkShaderModule vk_fragment_shader_{VK_NULL_HANDLE};

    VkBuffer vk_vertex_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory vk_vertex_buffer_memory_{VK_NULL_HANDLE};

    VkBuffer vk_index_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory vk_index_buffer_memory_{VK_NULL_HANDLE};

    VkImage vk_texture_image_;
    VkDeviceMemory vk_texture_image_memory_;

    VkImageView vk_texture_image_view_;
    VkSampler vk_texture_image_sampler_;
};
