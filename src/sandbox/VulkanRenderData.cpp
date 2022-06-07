#include "VulkanRenderData.hpp"
#include "VulkanUtils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fstream>
#include <iostream>
#include <vector>

const std::vector<vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2,
    2, 3, 0,
};

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

VkVertexInputBindingDescription vertex::get_vertex_input_binding_description()
{
    VkVertexInputBindingDescription vertex_input_binding_description;
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = sizeof(vertex);
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return vertex_input_binding_description;
}

std::array<VkVertexInputAttributeDescription, 3> vertex::get_vertex_input_attribute_descriptions()
{
    std::array<VkVertexInputAttributeDescription, 3> vertex_input_attribute_descriptions = {};

    vertex_input_attribute_descriptions[0].binding = 0;
    vertex_input_attribute_descriptions[0].location = 0;
    vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_input_attribute_descriptions[0].offset = offsetof(vertex, position);

    vertex_input_attribute_descriptions[1].binding = 0;
    vertex_input_attribute_descriptions[1].location = 1;
    vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_descriptions[1].offset = offsetof(vertex, color);

    vertex_input_attribute_descriptions[2].binding = 0;
    vertex_input_attribute_descriptions[2].location = 2;
    vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_input_attribute_descriptions[2].offset = offsetof(vertex, uv);

    return vertex_input_attribute_descriptions;
}

/**
 * \brief
 * \param vertex_shader_file
 * \param fragment_shader_file
 */
void render_data::init(const std::string& vertex_shader_file, const std::string& fragment_shader_file, const std::string& texture_file)
{
    vk_vertex_shader_ = create_shader(vertex_shader_file);
    vk_fragment_shader_ = create_shader(fragment_shader_file);

    create_image(texture_file);
    create_vertex_buffer();
    create_index_buffer();
}

/**
 * \brief
 */
void render_data::shutdown()
{
    vkDestroySampler(renderer_context_.vk_device_, vk_texture_image_sampler_, nullptr);
    vk_texture_image_sampler_ = VK_NULL_HANDLE;

    vkDestroyImageView(renderer_context_.vk_device_, vk_texture_image_view_, nullptr);
    vk_texture_image_view_ = VK_NULL_HANDLE;

    vkDestroyImage(renderer_context_.vk_device_, vk_texture_image_, nullptr);
    vk_texture_image_ = VK_NULL_HANDLE;

    vkFreeMemory(renderer_context_.vk_device_, vk_texture_image_memory_, nullptr);
    vk_texture_image_memory_ = VK_NULL_HANDLE;

    vkDestroyBuffer(renderer_context_.vk_device_, vk_vertex_buffer_, nullptr);
    vk_vertex_buffer_ = VK_NULL_HANDLE;

    vkFreeMemory(renderer_context_.vk_device_, vk_vertex_buffer_memory_, nullptr);
    vk_vertex_buffer_memory_ = VK_NULL_HANDLE;

    vkDestroyBuffer(renderer_context_.vk_device_, vk_index_buffer_, nullptr);
    vk_index_buffer_ = VK_NULL_HANDLE;

    vkFreeMemory(renderer_context_.vk_device_, vk_index_buffer_memory_, nullptr);
    vk_index_buffer_memory_ = VK_NULL_HANDLE;

    vkDestroyShaderModule(renderer_context_.vk_device_, vk_vertex_shader_, nullptr);
    vk_vertex_shader_ = VK_NULL_HANDLE;

    vkDestroyShaderModule(renderer_context_.vk_device_, vk_fragment_shader_, nullptr);
    vk_fragment_shader_ = VK_NULL_HANDLE;
}

uint32_t render_data::get_number_of_indicies() const
{
    return static_cast<uint32_t>(indices.size());
}

/**
 * \brief
 * \param path
 * \return
 */
VkShaderModule render_data::create_shader(const std::string& path) const
{
    std::vector<char> code = read_file(path);

    VkShaderModuleCreateInfo shader_module_create_info{};
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = code.size();
    shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shader_module;
    VK_CHECK(vkCreateShaderModule(renderer_context_.vk_device_, &shader_module_create_info, nullptr, &shader_module));

    return shader_module;
}

/**
 * \brief
 */
void render_data::create_vertex_buffer()
{
    VkDeviceSize buffer_size = sizeof(vertex) * vertices.size();

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

    vulkan_utils::create_buffer(renderer_context_, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_vertex_buffer_,
                                vk_vertex_buffer_memory_);

    // NOTE(dhaval): Create staging buffer.
    vulkan_utils::create_buffer(renderer_context_, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer,
                                staging_buffer_memory);

    // NOTE(dhaval): Fill staging buffer.
    void* data = nullptr;
    VK_CHECK(vkMapMemory(renderer_context_.vk_device_, staging_buffer_memory, 0, buffer_size, 0, &data));
    memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
    vkUnmapMemory(renderer_context_.vk_device_, staging_buffer_memory);

    // NOTE(dhaval): Transfer to GPU local memory.
    vulkan_utils::copy_buffer(renderer_context_, staging_buffer, vk_vertex_buffer_, buffer_size);

    // NOTE(dhaval): Destroy staging buffer.
    vkDestroyBuffer(renderer_context_.vk_device_, staging_buffer, nullptr);
    vkFreeMemory(renderer_context_.vk_device_, staging_buffer_memory, nullptr);
}

/**
 * \brief
 */
void render_data::create_index_buffer()
{
    VkDeviceSize buffer_size = sizeof(uint16_t) * indices.size();

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

    vulkan_utils::create_buffer(renderer_context_, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_index_buffer_,
                                vk_index_buffer_memory_);

    // NOTE(dhaval): Create staging buffer.
    vulkan_utils::create_buffer(renderer_context_, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer,
                                staging_buffer_memory);

    // NOTE(dhaval): Fill staging buffer.
    void* data = nullptr;
    VK_CHECK(vkMapMemory(renderer_context_.vk_device_, staging_buffer_memory, 0, buffer_size, 0, &data));
    memcpy(data, indices.data(), static_cast<size_t>(buffer_size));
    vkUnmapMemory(renderer_context_.vk_device_, staging_buffer_memory);

    // NOTE(dhaval): Transfer to GPU local memory.
    vulkan_utils::copy_buffer(renderer_context_, staging_buffer, vk_index_buffer_, buffer_size);

    // NOTE(dhaval): Destroy staging buffer.
    vkDestroyBuffer(renderer_context_.vk_device_, staging_buffer, nullptr);
    vkFreeMemory(renderer_context_.vk_device_, staging_buffer_memory, nullptr);
}

void render_data::create_image(const std::string& path)
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

    vulkan_utils::create_buffer(renderer_context_, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer,
                                staging_buffer_memory);

    // NOTE(dhaval): Fill staging buffer
    void* data = nullptr;
    vkMapMemory(renderer_context_.vk_device_, staging_buffer_memory, 0, image_size, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(image_size));
    vkUnmapMemory(renderer_context_.vk_device_, staging_buffer_memory);

    stbi_image_free(pixels);
    pixels = nullptr;

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    vulkan_utils::create_image_2d(renderer_context_, texture_width, texture_height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_texture_image_, vk_texture_image_memory_);

    // NOTE(dhaval): Prepare image for transfer
    vulkan_utils::transition_image_layout(renderer_context_, vk_texture_image_, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // NOTE(dhaval): Copy to the image memory on GPU
    vulkan_utils::copy_buffer_to_image(renderer_context_, staging_buffer, vk_texture_image_, texture_width, texture_height);

    // NOTE(dhaval): Prepare the image for shader access
    vulkan_utils::transition_image_layout(renderer_context_, vk_texture_image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // NOTE(dhaval): Destroy staging buffer
    vkDestroyBuffer(renderer_context_.vk_device_, staging_buffer, nullptr);
    vkFreeMemory(renderer_context_.vk_device_, staging_buffer_memory, nullptr);

    // NOTE(dhaval): Create image view & sampler
    vk_texture_image_view_ = vulkan_utils::create_image_2d_view(renderer_context_, vk_texture_image_, format);
    vk_texture_image_sampler_ = vulkan_utils::create_sampler(renderer_context_);
}
