#pragma once

#include <volk.h>

#include <string>
#include <vector>

#include "VulkanRendererContext.hpp"

class render_scene;

/**
 * \brief Renderer that the application will create and use.
 */
class renderer
{
public:
    renderer(const vulkan_renderer_context& renderer_context, const vulkan_swapchain_context& swapchain_context) : vk_renderer_context_(renderer_context), vk_swapchain_context_(swapchain_context)
    {
    }

    void init(const render_scene* render_scene);
    VkCommandBuffer render(uint32_t image_index);
    void shutdown();

private:
    vulkan_renderer_context vk_renderer_context_;
    vulkan_swapchain_context vk_swapchain_context_;

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
