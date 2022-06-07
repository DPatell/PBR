#pragma once

#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <cassert>

struct renderer_context;

/**
 * \brief
 */
class vulkan_utils
{
public:
    static uint32_t find_memory_type(const renderer_context& renderer_context, uint32_t type_filter, VkMemoryPropertyFlags memory_property_flags);

    static void create_buffer(const renderer_context& renderer_context,
                              VkDeviceSize device_size,
                              VkBufferUsageFlags buffer_usage_flags,
                              VkMemoryPropertyFlags memory_property_flags,
                              VkBuffer& buffer,
                              VkDeviceMemory& memory);

    static void create_image_2d(const renderer_context& renderer_context,
                                uint32_t width,
                                uint32_t height,
                                VkFormat format,
                                VkImageTiling image_tiling,
                                VkImageUsageFlags image_usage_flags,
                                VkMemoryPropertyFlags memory_property_flags,
                                VkImage& image,
                                VkDeviceMemory& device_memory);

    static VkImageView create_image_2d_view(const renderer_context& renderer_context, VkImage image, VkFormat format);

    static VkSampler create_sampler(const renderer_context& renderer_context);

    static void copy_buffer(const renderer_context& renderer_context, VkBuffer source, VkBuffer destination, VkDeviceSize device_size);

    static void copy_buffer_to_image(const renderer_context& renderer_context, VkBuffer source, VkImage destination, uint32_t width, uint32_t height);

    static void transition_image_layout(const renderer_context& renderer_context, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);

private:
    static VkCommandBuffer begin_single_time_commands(const renderer_context& renderer_context);
    static void end_single_time_commands(const renderer_context& renderer_context, VkCommandBuffer command_buffer);
};
