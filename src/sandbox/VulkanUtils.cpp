#include "VulkanUtils.hpp"
#include "VulkanRendererContext.hpp"

/**
 * \brief
 * \param renderer_context
 * \param type_filter
 * \param memory_property_flags
 * \return
 */
uint32_t vulkan_utils::find_memory_type(const renderer_context& renderer_context, uint32_t type_filter, VkMemoryPropertyFlags memory_property_flags)
{
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(renderer_context.vk_physical_device_, &physical_device_memory_properties);

    for (uint32_t i = 0; i < physical_device_memory_properties.memoryTypeCount; i++)
    {
        uint32_t memory_type_properties = physical_device_memory_properties.memoryTypes[i].propertyFlags;
        if ((type_filter & (1 << i)) && (memory_type_properties & memory_property_flags) == memory_property_flags)
        {
            return i;
        }
    }

    assert(false, "Can't find suitable memory type");
}

void vulkan_utils::create_buffer(const renderer_context& renderer_context,
                                 VkDeviceSize device_size,
                                 VkBufferUsageFlags buffer_usage_flags,
                                 VkMemoryPropertyFlags memory_property_flags,
                                 VkBuffer& buffer,
                                 VkDeviceMemory& memory)
{
    // NOTE(dhaval): Create buffer.
    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = device_size;
    buffer_create_info.usage = buffer_usage_flags;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateBuffer(renderer_context.vk_device_, &buffer_create_info, nullptr, &buffer));

    // NOTE(dhaval): Allocate memory for the buffer.
    VkMemoryRequirements memory_requirements{};
    vkGetBufferMemoryRequirements(renderer_context.vk_device_, buffer, &memory_requirements);

    VkMemoryAllocateInfo memory_allocate_info{};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = find_memory_type(renderer_context, memory_requirements.memoryTypeBits, memory_property_flags);

    VK_CHECK(vkAllocateMemory(renderer_context.vk_device_, &memory_allocate_info, nullptr, &memory));

    // NOTE(dhaval): Bind the buffer
    VK_CHECK(vkBindBufferMemory(renderer_context.vk_device_, buffer, memory, 0));
}

void vulkan_utils::create_image_2d(const renderer_context& renderer_context,
                                   uint32_t width,
                                   uint32_t height,
                                   VkFormat format,
                                   VkImageTiling image_tiling,
                                   VkImageUsageFlags image_usage_flags,
                                   VkMemoryPropertyFlags memory_property_flags,
                                   VkImage& image,
                                   VkDeviceMemory& device_memory)
{
    // NOTE(dhaval): Create buffer
    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.format = format;
    image_create_info.tiling = image_tiling;
    image_create_info.usage = image_usage_flags;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.flags = 0;

    VK_CHECK(vkCreateImage(renderer_context.vk_device_, &image_create_info, nullptr, &image));

    // NOTE(dhaval): Allocate memory for buffer
    VkMemoryRequirements memory_requirements{};
    vkGetImageMemoryRequirements(renderer_context.vk_device_, image, &memory_requirements);

    VkMemoryAllocateInfo memory_allocate_info{};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = find_memory_type(renderer_context, memory_requirements.memoryTypeBits, memory_property_flags);

    VK_CHECK(vkAllocateMemory(renderer_context.vk_device_, &memory_allocate_info, nullptr, &device_memory));

    // NOTE(dhaval): Bind buffer
    VK_CHECK(vkBindImageMemory(renderer_context.vk_device_, image, device_memory, 0));
}

VkImageView vulkan_utils::create_image_2d_view(const renderer_context& renderer_context, VkImage image, VkFormat format)
{
    VkImageViewCreateInfo image_view_create_info{};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = format;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    VK_CHECK(vkCreateImageView(renderer_context.vk_device_, &image_view_create_info, nullptr, &image_view));

    return image_view;
}

VkSampler vulkan_utils::create_sampler(const renderer_context& renderer_context)
{
    VkSamplerCreateInfo sampler_create_info{};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.anisotropyEnable = VK_TRUE;
    sampler_create_info.maxAnisotropy = 16;
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 0.0f;

    VkSampler sampler = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSampler(renderer_context.vk_device_, &sampler_create_info, nullptr, &sampler));

    return sampler;
}

void vulkan_utils::copy_buffer(const renderer_context& renderer_context, VkBuffer source, VkBuffer destination, VkDeviceSize device_size)
{
    VkCommandBuffer command_buffer = begin_single_time_commands(renderer_context);

    VkBufferCopy buffer_copy{};
    buffer_copy.size = device_size;
    vkCmdCopyBuffer(command_buffer, source, destination, 1, &buffer_copy);

    end_single_time_commands(renderer_context, command_buffer);
}

void vulkan_utils::copy_buffer_to_image(const renderer_context& renderer_context, VkBuffer source, VkImage destination, uint32_t width, uint32_t height)
{
    VkCommandBuffer command_buffer = begin_single_time_commands(renderer_context);

    VkBufferImageCopy buffer_image_copy{};
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;

    buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;

    buffer_image_copy.imageOffset = {0, 0, 0};
    buffer_image_copy.imageExtent.width = width;
    buffer_image_copy.imageExtent.height = height;
    buffer_image_copy.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(command_buffer, source, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);

    end_single_time_commands(renderer_context, command_buffer);
}

void vulkan_utils::transition_image_layout(const renderer_context& renderer_context, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
    VkCommandBuffer command_buffer = begin_single_time_commands(renderer_context);

    VkImageMemoryBarrier image_memory_barrier{};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.oldLayout = old_layout;
    image_memory_barrier.newLayout = new_layout;

    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    image_memory_barrier.image = image;
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_pipeline_stage_flags;
    VkPipelineStageFlags dst_pipeline_stage_flags;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_pipeline_stage_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_pipeline_stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_pipeline_stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_pipeline_stage_flags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        assert(false, "Unsupported layout transition");
    }

    vkCmdPipelineBarrier(command_buffer, src_pipeline_stage_flags, dst_pipeline_stage_flags, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);

    end_single_time_commands(renderer_context, command_buffer);
}

VkCommandBuffer vulkan_utils::begin_single_time_commands(const renderer_context& renderer_context)
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandPool = renderer_context.vk_command_pool_;
    command_buffer_allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    VK_CHECK(vkAllocateCommandBuffers(renderer_context.vk_device_, &command_buffer_allocate_info, &command_buffer));

    VkCommandBufferBeginInfo command_buffer_begin_info{};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

    return command_buffer;
}

void vulkan_utils::end_single_time_commands(const renderer_context& renderer_context, VkCommandBuffer command_buffer)
{
    VK_CHECK(vkEndCommandBuffer(command_buffer));

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    VK_CHECK(vkQueueSubmit(renderer_context.graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(renderer_context.graphics_queue));

    vkFreeCommandBuffers(renderer_context.vk_device_, renderer_context.vk_command_pool_, 1, &command_buffer);
}
