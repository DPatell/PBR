#include "VulkanRenderer.hpp"
#include "VulkanUtils.hpp"

#include <chrono>

/**
 * \brief Initializes the Renderer.
 * \param vertex_shader_file Path to the vertex shader file.
 * \param fragment_shader_file Path to the fragment shader file.
 * \param texture_file
 * \param model_file
 */
void renderer::init(const std::string& vertex_shader_file, const std::string& fragment_shader_file, const std::string& texture_file, const std::string& model_file)
{
    data_.init(vertex_shader_file, fragment_shader_file, texture_file, model_file);

    // NOTE(dhaval): Create Uniform buffers
    VkDeviceSize uniform_buffer_object_size = sizeof(uniform_buffer_object);

    uint32_t image_count = static_cast<uint32_t>(vk_renderer_context_.vk_swapchain_image_views_.size());
    vk_uniform_buffers_.resize(image_count);
    vk_uniform_buffers_memory_.resize(image_count);

    for (uint32_t i = 0; i < image_count; i++)
    {
        vulkan_utils::create_buffer(vk_renderer_context_, uniform_buffer_object_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    vk_uniform_buffers_[i], vk_uniform_buffers_memory_[i]);
    }

    // NOTE(dhaval): Creating Shader Stages.
    VkPipelineShaderStageCreateInfo vertex_shader_stage_create_info{};
    vertex_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_create_info.module = data_.get_vertex_shader();
    vertex_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_create_info{};
    fragment_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_create_info.module = data_.get_fragment_shader();
    fragment_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vertex_shader_stage_create_info, fragment_shader_stage_create_info};

    // NOTE(dhaval): Creating Vertex Input.
    VkVertexInputBindingDescription vertex_input_binding_description = vertex::get_vertex_input_binding_description();
    auto vertex_input_attribute_descriptions = vertex::get_vertex_input_attribute_descriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attribute_descriptions.size());
    vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_input_attribute_descriptions.data();

    // NOTE(dhaval): Creating Input Assembly.
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    // NOTE(dhaval): Creating Viewport State.
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(vk_renderer_context_.vk_extent_2d_.width);
    viewport.height = static_cast<float>(vk_renderer_context_.vk_extent_2d_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = vk_renderer_context_.vk_extent_2d_;

    VkPipelineViewportStateCreateInfo viewport_state_create_info{};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissor;

    // NOTE(dhaval): Create Rasterizer State.
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.lineWidth = 1.0f;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
    rasterization_state_create_info.depthBiasClamp = 0.0f;
    rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;

    // NOTE(dhaval): Create MultiSampling State.
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.minSampleShading = 1.0f;
    multisample_state_create_info.pSampleMask = nullptr;
    multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    // NOTE(dhaval): Create Depth Stencil State.
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.minDepthBounds = 0.0f;
    depth_stencil_state_create_info.maxDepthBounds = 1.0f;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_create_info.front = {};
    depth_stencil_state_create_info.back = {};


    // NOTE(dhaval): Create Color Blend State.
    VkPipelineColorBlendAttachmentState color_blend_attachment_state{};
    color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state.blendEnable = VK_FALSE;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info{};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment_state;
    color_blend_state_create_info.blendConstants[0] = 0.0f;
    color_blend_state_create_info.blendConstants[1] = 0.0f;
    color_blend_state_create_info.blendConstants[2] = 0.0f;
    color_blend_state_create_info.blendConstants[3] = 0.0f;

    // NOTE(dhaval): Create Pipeline Dynamic State. (For Later Use)
    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH};

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = 2;
    dynamic_state_create_info.pDynamicStates = dynamic_states;

    // NOTE(dhaval): Create descriptor set layout
    VkDescriptorSetLayoutBinding uniform_buffer_layout_binding{};
    uniform_buffer_layout_binding.binding = 0;
    uniform_buffer_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_buffer_layout_binding.descriptorCount = 1;
    uniform_buffer_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uniform_buffer_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding sampler_layout_binding{};
    sampler_layout_binding.binding = 1;
    sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.descriptorCount = 1;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    sampler_layout_binding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> descriptor_set_layout_bindings = {uniform_buffer_layout_binding, sampler_layout_binding};

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(descriptor_set_layout_bindings.size());
    descriptor_set_layout_create_info.pBindings = descriptor_set_layout_bindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(vk_renderer_context_.vk_device_, &descriptor_set_layout_create_info, nullptr, &vk_descriptor_set_layout_));

    // NOTE(dhaval): Create descriptor sets
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts(image_count, vk_descriptor_set_layout_);

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info{};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.descriptorPool = vk_renderer_context_.vk_descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = image_count;
    descriptor_set_allocate_info.pSetLayouts = descriptor_set_layouts.data();

    vk_descriptor_sets_.resize(image_count);
    VK_CHECK(vkAllocateDescriptorSets(vk_renderer_context_.vk_device_, &descriptor_set_allocate_info, vk_descriptor_sets_.data()));

    for (size_t i = 0; i < image_count; i++)
    {
        VkDescriptorBufferInfo descriptor_buffer_info{};
        descriptor_buffer_info.buffer = vk_uniform_buffers_[i];
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = sizeof(uniform_buffer_object);

        VkDescriptorImageInfo descriptor_image_info{};
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptor_image_info.imageView = data_.get_texture_image_view();
        descriptor_image_info.sampler = data_.get_texture_image_sampler();

        std::array<VkWriteDescriptorSet, 2> write_descriptor_sets{};

        write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_sets[0].dstSet = vk_descriptor_sets_[i];
        write_descriptor_sets[0].dstBinding = 0;
        write_descriptor_sets[0].dstArrayElement = 0;
        write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_descriptor_sets[0].descriptorCount = 1;
        write_descriptor_sets[0].pBufferInfo = &descriptor_buffer_info;

        write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_sets[1].dstSet = vk_descriptor_sets_[i];
        write_descriptor_sets[1].dstBinding = 1;
        write_descriptor_sets[1].dstArrayElement = 0;
        write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_descriptor_sets[1].descriptorCount = 1;
        write_descriptor_sets[1].pImageInfo = &descriptor_image_info;

        vkUpdateDescriptorSets(vk_renderer_context_.vk_device_, static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
    }

    // NOTE(dhaval): Create Pipeline Layout.
    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &vk_descriptor_set_layout_;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;

    VK_CHECK(vkCreatePipelineLayout(vk_renderer_context_.vk_device_, &pipeline_layout_create_info, nullptr, &vk_pipeline_layout_));

    // NOTE(dhaval): Create RenderPass.
    VkAttachmentDescription color_attachment_description{};
    color_attachment_description.format = vk_renderer_context_.vk_color_format_;
    color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_reference{};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth_attachment_description{};
    depth_attachment_description.format = vk_renderer_context_.vk_depth_format_;
    depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference{};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description{};
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &color_attachment_reference;
    subpass_description.pDepthStencilAttachment = &depth_attachment_reference;

    VkSubpassDependency subpass_dependency{};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachment_descriptions = {color_attachment_description, depth_attachment_description};
    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachment_descriptions.size());
    render_pass_create_info.pAttachments = attachment_descriptions.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &subpass_dependency;

    VK_CHECK(vkCreateRenderPass(vk_renderer_context_.vk_device_, &render_pass_create_info, nullptr, &vk_render_pass_));

    // NOTE(dhaval): Create Graphics Pipeline;
    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info{};
    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.stageCount = 2;
    graphics_pipeline_create_info.pStages = shader_stages;
    graphics_pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    graphics_pipeline_create_info.pMultisampleState = &multisample_state_create_info;
    graphics_pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
    graphics_pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    graphics_pipeline_create_info.pDynamicState = nullptr;
    graphics_pipeline_create_info.layout = vk_pipeline_layout_;
    graphics_pipeline_create_info.renderPass = vk_render_pass_;
    graphics_pipeline_create_info.subpass = 0;
    graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_create_info.basePipelineIndex = -1;

    VK_CHECK(vkCreateGraphicsPipelines(vk_renderer_context_.vk_device_, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &vk_pipeline_));

    // NOTE(dhaval): Create Frambuffers
    vk_frame_buffers_.resize(image_count);
    for (size_t i = 0; i < image_count; i++)
    {
        std::array<VkImageView, 2> attachment_image_views = {vk_renderer_context_.vk_swapchain_image_views_[i], vk_renderer_context_.vk_depth_image_view_};

        VkFramebufferCreateInfo framebuffer_create_info{};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = vk_render_pass_;
        framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachment_image_views.size());
        framebuffer_create_info.pAttachments = attachment_image_views.data();
        framebuffer_create_info.width = vk_renderer_context_.vk_extent_2d_.width;
        framebuffer_create_info.height = vk_renderer_context_.vk_extent_2d_.height;
        framebuffer_create_info.layers = 1;

        VK_CHECK(vkCreateFramebuffer(vk_renderer_context_.vk_device_, &framebuffer_create_info, nullptr, &vk_frame_buffers_[i]));
    }

    // NOTE(dhaval): Create Command Buffers
    vk_command_buffers_.resize(image_count);

    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = vk_renderer_context_.vk_command_pool_;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = static_cast<uint32_t>(vk_command_buffers_.size());

    VK_CHECK(vkAllocateCommandBuffers(vk_renderer_context_.vk_device_, &command_buffer_allocate_info, vk_command_buffers_.data()));

    // NOTE(dhaval): Record Command Buffers
    for (size_t i = 0; i < vk_command_buffers_.size(); i++)
    {
        VkCommandBufferBeginInfo command_buffer_begin_info{};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        VK_CHECK(vkBeginCommandBuffer(vk_command_buffers_[i], &command_buffer_begin_info));

        VkRenderPassBeginInfo render_pass_begin_info{};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.renderPass = vk_render_pass_;
        render_pass_begin_info.framebuffer = vk_frame_buffers_[i];
        render_pass_begin_info.renderArea.offset = {0, 0};
        render_pass_begin_info.renderArea.extent = vk_renderer_context_.vk_extent_2d_;

        std::array<VkClearValue, 2> clear_values = {};
        clear_values[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
        clear_values[1].depthStencil = {1.0f, 0};
        render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        render_pass_begin_info.pClearValues = clear_values.data();

        vkCmdBeginRenderPass(vk_command_buffers_[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(vk_command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_);
        vkCmdBindDescriptorSets(vk_command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_layout_, 0, 1, &vk_descriptor_sets_[i], 0, nullptr);

        VkBuffer vertex_buffers[] = {data_.get_vertex_buffer()};
        VkBuffer index_buffer = data_.get_index_buffer();
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(vk_command_buffers_[i], 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(vk_command_buffers_[i], index_buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(vk_command_buffers_[i], data_.get_number_of_indicies(), 1, 0, 0, 0);

        vkCmdEndRenderPass(vk_command_buffers_[i]);

        VK_CHECK(vkEndCommandBuffer(vk_command_buffers_[i]));
    }
}

/**
 * \brief
 * \param image_index
 * \return  
 */
VkCommandBuffer renderer::render(uint32_t image_index)
{
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    VkBuffer uniform_buffer = vk_uniform_buffers_[image_index];
    VkDeviceMemory uniform_buffer_memory = vk_uniform_buffers_memory_[image_index];

    uniform_buffer_object uniform_buffer_object{};
    uniform_buffer_object.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    uniform_buffer_object.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    uniform_buffer_object.proj = glm::perspective(glm::radians(45.0f), vk_renderer_context_.vk_extent_2d_.width / (float)vk_renderer_context_.vk_extent_2d_.height, 0.1f, 10.0f);
    uniform_buffer_object.proj[1][1] *= -1;

    void* data;
    vkMapMemory(vk_renderer_context_.vk_device_, uniform_buffer_memory, 0, sizeof(uniform_buffer_object), 0, &data);
    memcpy(data, &uniform_buffer_object, sizeof(uniform_buffer_object));
    vkUnmapMemory(vk_renderer_context_.vk_device_, uniform_buffer_memory);

    return vk_command_buffers_[image_index];
}

/**
 * \brief Destorys all resources created in renderer::init().
 */
void renderer::shutdown()
{
    data_.shutdown();

    for (auto uniform_buffer : vk_uniform_buffers_)
    {
        vkDestroyBuffer(vk_renderer_context_.vk_device_, uniform_buffer, nullptr);
    }

    vk_uniform_buffers_.clear();

    for (auto uniform_buffer_memory : vk_uniform_buffers_memory_)
    {
        vkFreeMemory(vk_renderer_context_.vk_device_, uniform_buffer_memory, nullptr);
    }

    vk_uniform_buffers_memory_.clear();

    for (auto frame_buffer : vk_frame_buffers_)
    {
        vkDestroyFramebuffer(vk_renderer_context_.vk_device_, frame_buffer, nullptr);
    }

    vk_frame_buffers_.clear();

    vkDestroyPipeline(vk_renderer_context_.vk_device_, vk_pipeline_, nullptr);
    vk_pipeline_ = VK_NULL_HANDLE;

    vkDestroyPipelineLayout(vk_renderer_context_.vk_device_, vk_pipeline_layout_, nullptr);
    vk_pipeline_layout_ = VK_NULL_HANDLE;

    vkDestroyDescriptorSetLayout(vk_renderer_context_.vk_device_, vk_descriptor_set_layout_, nullptr);
    vk_descriptor_set_layout_ = nullptr;

    vkDestroyRenderPass(vk_renderer_context_.vk_device_, vk_render_pass_, nullptr);
    vk_render_pass_ = VK_NULL_HANDLE;
}
