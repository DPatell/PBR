#pragma once

#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>

#include <vector>

/**
 * \brief Macro that checks if a vulkan api function was successfull or not.
 * \param call Any vulkan api function that returns a VkResult.
 */
#define VK_CHECK(call) do { VkResult result_ = call; assert(result_ == VK_SUCCESS); } while (0)

/**
 * \brief Structure that stores the varaibles that the renderer may need.
 */
struct renderer_context
{
    VkDevice vk_device_{VK_NULL_HANDLE};
    VkPhysicalDevice vk_physical_device_{VK_NULL_HANDLE};
    VkCommandPool vk_command_pool_{VK_NULL_HANDLE};
    VkDescriptorPool vk_descriptor_pool;
    VkFormat vk_format_;
    VkExtent2D vk_extent_2d_;
    std::vector<VkImageView> vk_image_views_;

    VkQueue graphics_queue{VK_NULL_HANDLE};
    VkQueue present_queue{VK_NULL_HANDLE};
};
