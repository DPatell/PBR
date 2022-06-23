#pragma once

#include <volk.h>

#include <string>

#include "VulkanRendererContext.hpp"

class vulkan_texture
{
public:
    vulkan_texture(const vulkan_renderer_context& vk_renderer_context) : vk_renderer_context_(vk_renderer_context)
    {
    }

    ~vulkan_texture();

    inline VkImage get_image() const { return vk_image_; }
    inline VkImageView get_image_view() const { return vk_image_view_; }
    inline VkSampler get_sampler() const { return vk_image_sampler_; }

    bool load_from_file(const std::string& path);

    void upload_to_gpu();
    void clear_gpu_data();
    void clear_cpu_data();
private:
    vulkan_renderer_context vk_renderer_context_;

    unsigned char* pixels_{nullptr};

    int width_{0};
    int height_{0};
    int channels_{0};
    int mip_levels_{0};

    VkFormat vk_format_{VK_FORMAT_R8G8B8A8_UNORM};

    VkImage vk_image_{VK_NULL_HANDLE};
    VkDeviceMemory vk_image_memory_{VK_NULL_HANDLE};
    VkImageView vk_image_view_{VK_NULL_HANDLE};
    VkSampler vk_image_sampler_{VK_NULL_HANDLE};
};
