#include "RenderScene.hpp"

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

    assert(file.is_open() && "Can't Open File");

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
    texture_.load_from_file(texture_file);
}

/**
 * \brief
 */
void render_scene::shutdown()
{
    vkDestroyShaderModule(vk_renderer_context_.vk_device_, vk_vertex_shader_, nullptr);
    vk_vertex_shader_ = VK_NULL_HANDLE;

    vkDestroyShaderModule(vk_renderer_context_.vk_device_, vk_fragment_shader_, nullptr);
    vk_fragment_shader_ = VK_NULL_HANDLE;

    texture_.clear_gpu_data();
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
