#pragma once

#include <vulkan/vulkan.h>

#include <cassert>

struct vulkan_renderer_context;

/**
 * \brief Helper class tha conatains useful functions to create various vulkan structures such as images, samplers, etc.
 */
class vulkan_utils
{
public:
    static uint32_t find_memory_type(const vulkan_renderer_context& vk_renderer_context, uint32_t type_filter, VkMemoryPropertyFlags memory_property_flags);

    static void create_buffer(const vulkan_renderer_context& vk_renderer_context,
                              VkDeviceSize device_size,
                              VkBufferUsageFlags buffer_usage_flags,
                              VkMemoryPropertyFlags memory_property_flags,
                              VkBuffer& buffer,
                              VkDeviceMemory& memory);

    static void create_image_2d(const vulkan_renderer_context& vk_renderer_context,
                                uint32_t width,
                                uint32_t height,
                                uint32_t mip_levels,
                                VkFormat format,
                                VkImageTiling image_tiling,
                                VkImageUsageFlags image_usage_flags,
                                VkMemoryPropertyFlags memory_property_flags,
                                VkImage& image,
                                VkDeviceMemory& device_memory);

    static VkImageView create_image_2d_view(const vulkan_renderer_context& vk_renderer_context, VkImage image, uint32_t mip_levels, VkFormat format, VkImageAspectFlags aspect_flags);

    static VkSampler create_sampler(const vulkan_renderer_context& vk_renderer_context, uint32_t mip_levels);

    static void copy_buffer(const vulkan_renderer_context& vk_renderer_context, VkBuffer source, VkBuffer destination, VkDeviceSize device_size);

    static void copy_buffer_to_image(const vulkan_renderer_context& vk_renderer_context, VkBuffer source, VkImage destination, uint32_t width, uint32_t height);

    static void transition_image_layout(const vulkan_renderer_context& vk_renderer_context, VkImage image, uint32_t mip_levels, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);

    static void generate_image_2d_mipmaps(const vulkan_renderer_context& vk_renderer_context, VkImage image, uint32_t width, uint32_t height, uint32_t mip_levels, VkFormat format, VkFilter filter);

private:
    static bool has_stencil_component(VkFormat format);
    static VkCommandBuffer begin_single_time_commands(const vulkan_renderer_context& vk_renderer_context);
    static void end_single_time_commands(const vulkan_renderer_context& vk_renderer_context, VkCommandBuffer command_buffer);
};
