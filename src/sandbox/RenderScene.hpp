#pragma once

#include <vulkan/vulkan.h>

#include <string>

#include "VulkanRendererContext.hpp"
#include "VulkanMesh.hpp"
#include "VulkanTexture.hpp"

/**
 * \brief
 */
class render_scene
{
public:
    render_scene(const vulkan_renderer_context& vk_renderer_context) : vk_renderer_context_(vk_renderer_context), mesh_(vk_renderer_context), texture_(vk_renderer_context)
    {
    }

    void init(const std::string& vertex_shader_file, const std::string& fragment_shader_file, const std::string& texture_file, const std::string& model_file);
    void shutdown();

    inline VkShaderModule get_vertex_shader() const { return vk_vertex_shader_; };
    inline VkShaderModule get_fragment_shader() const { return vk_fragment_shader_; };

    inline const vulkan_texture& get_texture() const { return texture_; }
    inline const vulkan_mesh& get_mesh() const { return mesh_; }

private:
    VkShaderModule create_shader(const std::string& path) const;

    void create_vertex_buffer();
    void create_index_buffer();

private:
    vulkan_renderer_context vk_renderer_context_;
    vulkan_mesh mesh_;
    vulkan_texture texture_;

    VkShaderModule vk_vertex_shader_{VK_NULL_HANDLE};
    VkShaderModule vk_fragment_shader_{VK_NULL_HANDLE};
};
