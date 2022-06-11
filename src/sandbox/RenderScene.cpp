#include "RenderScene.hpp"
#include "VulkanUtils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fstream>
#include <iostream>
#include <vector>

/**
 * \brief Reads text or binary files.
 * \param filename Name of the file that we want to read.
 * \return std::vector<char>
 */
static std::vector<char> read_file(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    assert(file.is_open(), "Can't Open File");

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    return buffer;
}

/**
 * \brief
 * \param vertex_shader_file 
 * \param fragment_shader_file 
 * \param texture_file 
 * \param model_file 
 */
void render_scene::init(const std::string& vertex_shader_file, const std::string& fragment_shader_file, const std::string& texture_file, const std::string& model_file)
{
    vk_vertex_shader_ = create_shader(vertex_shader_file);
    vk_fragment_shader_ = create_shader(fragment_shader_file);
    mesh_.load_from_file(model_file);
    
    create_image(texture_file);
}

/**
 * \brief
 */
void render_scene::shutdown()
{
    vkDestroySampler(vk_renderer_context_.vk_device_, vk_texture_image_sampler_, nullptr);
    vk_texture_image_sampler_ = VK_NULL_HANDLE;

    vkDestroyImageView(vk_renderer_context_.vk_device_, vk_texture_image_view_, nullptr);
    vk_texture_image_view_ = VK_NULL_HANDLE;

    vkDestroyImage(vk_renderer_context_.vk_device_, vk_texture_image_, nullptr);
    vk_texture_image_ = VK_NULL_HANDLE;

    vkFreeMemory(vk_renderer_context_.vk_device_, vk_texture_image_memory_, nullptr);
    vk_texture_image_memory_ = VK_NULL_HANDLE;

    vkDestroyShaderModule(vk_renderer_context_.vk_device_, vk_vertex_shader_, nullptr);
    vk_vertex_shader_ = VK_NULL_HANDLE;

    vkDestroyShaderModule(vk_renderer_context_.vk_device_, vk_fragment_shader_, nullptr);
    vk_fragment_shader_ = VK_NULL_HANDLE;

    mesh_.clear_gpu_data();
}

/**
 * \brief
 * \param path
 * \return
 */
VkShaderModule render_scene::create_shader(const std::string& path) const
{
    std::vector<char> code = read_file(path);

    VkShaderModuleCreateInfo shader_module_create_info{};
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = code.size();
    shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shader_module;
    VK_CHECK(vkCreateShaderModule(vk_renderer_context_.vk_device_, &shader_module_create_info, nullptr, &shader_module));

    return shader_module;
}

/**
 * \brief 
 * \param path 
 */
void render_scene::create_image(const std::string& path)
{
    int texture_width;
    int texture_height;
    int texture_channels;

    stbi_uc* pixels = stbi_load(path.c_str(), &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);

    // NOTE(dhaval): Pixel data will have alpha channel even if the original image doesn't
    VkDeviceSize image_size = texture_width * texture_height * 4;

    assert(pixels, "Failed to load texture image!");

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

    vulkan_utils::create_buffer(vk_renderer_context_, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer,
                                staging_buffer_memory);

    // NOTE(dhaval): Fill staging buffer
    void* data = nullptr;
    vkMapMemory(vk_renderer_context_.vk_device_, staging_buffer_memory, 0, image_size, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(image_size));
    vkUnmapMemory(vk_renderer_context_.vk_device_, staging_buffer_memory);

    stbi_image_free(pixels);
    pixels = nullptr;

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    vulkan_utils::create_image_2d(vk_renderer_context_, texture_width, texture_height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_texture_image_, vk_texture_image_memory_);

    // NOTE(dhaval): Prepare image for transfer
    vulkan_utils::transition_image_layout(vk_renderer_context_, vk_texture_image_, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // NOTE(dhaval): Copy to the image memory on GPU
    vulkan_utils::copy_buffer_to_image(vk_renderer_context_, staging_buffer, vk_texture_image_, texture_width, texture_height);

    // NOTE(dhaval): Prepare the image for shader access
    vulkan_utils::transition_image_layout(vk_renderer_context_, vk_texture_image_, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // NOTE(dhaval): Destroy staging buffer
    vkDestroyBuffer(vk_renderer_context_.vk_device_, staging_buffer, nullptr);
    vkFreeMemory(vk_renderer_context_.vk_device_, staging_buffer_memory, nullptr);

    // NOTE(dhaval): Create image view & sampler
    vk_texture_image_view_ = vulkan_utils::create_image_2d_view(vk_renderer_context_, vk_texture_image_, format, VK_IMAGE_ASPECT_COLOR_BIT);
    vk_texture_image_sampler_ = vulkan_utils::create_sampler(vk_renderer_context_);
}
