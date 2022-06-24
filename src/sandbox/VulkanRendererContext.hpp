#pragma once

#include <volk.h>

#include <vector>

/**
 * \brief Macro that checks if a vulkan api function was successfull or not.
 * \param call Any vulkan api function that returns a VkResult.
 */
#define VK_CHECK(call) do { VkResult result_ = call; assert(result_ == VK_SUCCESS); } while (0)

/**
 * \brief Structure that stores the varaibles that the renderer may need.
 */
struct vulkan_renderer_context
{
    VkDevice vk_device_{VK_NULL_HANDLE};
    VkPhysicalDevice vk_physical_device_{VK_NULL_HANDLE};
    VkCommandPool vk_command_pool_{VK_NULL_HANDLE};

    VkQueue graphics_queue{VK_NULL_HANDLE};
    VkQueue present_queue{VK_NULL_HANDLE};
};

/**
 * \brief Structure that stores the varaiables that the swapchain may need.
 */
struct vulkan_swapchain_context
{
    VkDescriptorPool vk_descriptor_pool{VK_NULL_HANDLE};
    VkFormat vk_color_format_;
    VkFormat vk_depth_format_;
    VkExtent2D vk_extent_2d_;
    std::vector<VkImageView> vk_swapchain_image_views_;
    VkImageView vk_depth_image_view_{VK_NULL_HANDLE};
};
