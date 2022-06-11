#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

#include "VulkanRendererContext.hpp"
#include "RenderScene.hpp"

/**
 * \brief Renderer that the application will create and use.
 */
class renderer
{
public:
    renderer(const vulkan_renderer_context& context) : vk_renderer_context_(context), data_(context)
    {
    }

    void init(const std::string& vertex_shader_file, const std::string& fragment_shader_file, const std::string& texture_file, const std::string& model_file);
    VkCommandBuffer render(uint32_t image_index);
    void shutdown();

private:
    render_scene data_;
    vulkan_renderer_context vk_renderer_context_;

    VkRenderPass vk_render_pass_{VK_NULL_HANDLE};
    VkDescriptorSetLayout vk_descriptor_set_layout_{VK_NULL_HANDLE};
    VkPipelineLayout vk_pipeline_layout_{VK_NULL_HANDLE};
    VkPipeline vk_pipeline_{VK_NULL_HANDLE};

    std::vector<VkFramebuffer> vk_frame_buffers_;
    std::vector<VkCommandBuffer> vk_command_buffers_;

    std::vector<VkBuffer> vk_uniform_buffers_;
    std::vector<VkDeviceMemory> vk_uniform_buffers_memory_;

    std::vector<VkDescriptorSet> vk_descriptor_sets_;
};
