#include "VulkanTexture.hpp"
#include "VulkanUtils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>

vulkan_texture::~vulkan_texture()
{
    clear_gpu_data();
    clear_cpu_data();
}

bool vulkan_texture::load_from_file(const std::string& path)
{
    // TODO(dhaval): Support other image formats
    stbi_uc* stb_pixels = stbi_load(path.c_str(), &width_, &height_, &channels_, STBI_rgb_alpha);

    if (!stb_pixels)
    {
        std::cerr << "vulkan_texture::load_from_file(): " << stbi_failure_reason() << std::endl;
        return false;
    }

    size_t image_size = width_ * height_ * 4;
    if (pixels_ != nullptr)
    {
        delete[] pixels_;
    }

    pixels_ = new unsigned char[image_size];
    memcpy(pixels_, stb_pixels, image_size);

    stbi_image_free(stb_pixels);
    stb_pixels = nullptr;

    // NOTE(dhaval): Upload CPU data to GPU
    clear_gpu_data();
    upload_to_gpu();

    // TODO(dhaval): Should we clear CPU data after uploading it to the GPU?

    return true;
}

void vulkan_texture::upload_to_gpu()
{
    // TODO(dhaval): Support other image formats
    vk_format = VK_FORMAT_R8G8B8A8_UNORM;

    // NOTE(dhaval): Pixel data will have alpha channel even if the original image doesn't
    VkDeviceSize image_size = width_ * height_ * 4;

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

    vulkan_utils::create_buffer(vk_renderer_context_, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer,
                                staging_buffer_memory);

    // NOTE(dhaval): Fill staging buffer
    void* data = nullptr;
    vkMapMemory(vk_renderer_context_.vk_device_, staging_buffer_memory, 0, image_size, 0, &data);
    memcpy(data, pixels_, static_cast<size_t>(image_size));
    vkUnmapMemory(vk_renderer_context_.vk_device_, staging_buffer_memory);

    vulkan_utils::create_image_2d(vk_renderer_context_, width_, height_, vk_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_image_, vk_image_memory_);

    // NOTE(dhaval): Prepare the image for transfer
    vulkan_utils::transition_image_layout(vk_renderer_context_, vk_image_, vk_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // NOTE(dhaval): Copy to the image memory on the gpu
    vulkan_utils::copy_buffer_to_image(vk_renderer_context_, staging_buffer, vk_image_, width_, height_);

    // NOTE(dhaval): Prepare the image for shader access
    vulkan_utils::transition_image_layout(vk_renderer_context_, vk_image_, vk_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // NOTE(dhaval): destroy staging buffer
    vkDestroyBuffer(vk_renderer_context_.vk_device_, staging_buffer, nullptr);
    vkFreeMemory(vk_renderer_context_.vk_device_, staging_buffer_memory, nullptr);

    // NOTE(dhaval): create image view & sampler
    vk_image_view_ = vulkan_utils::create_image_2d_view(vk_renderer_context_, vk_image_, vk_format, VK_IMAGE_ASPECT_COLOR_BIT);
    vk_image_sampler_ = vulkan_utils::create_sampler(vk_renderer_context_);
}

void vulkan_texture::clear_gpu_data()
{
    vkDestroySampler(vk_renderer_context_.vk_device_, vk_image_sampler_, nullptr);
    vk_image_sampler_ = nullptr;

    vkDestroyImageView(vk_renderer_context_.vk_device_, vk_image_view_, nullptr);
    vk_image_view_ = nullptr;

    vkDestroyImage(vk_renderer_context_.vk_device_, vk_image_, nullptr);
    vk_image_ = nullptr;

    vkFreeMemory(vk_renderer_context_.vk_device_, vk_image_memory_, nullptr);
    vk_image_memory_ = nullptr;
}

void vulkan_texture::clear_cpu_data()
{
    delete[] pixels_;
    pixels_ = nullptr;

    width_ = 0;
    height_ = 0;
    channels_ = 0;
}



