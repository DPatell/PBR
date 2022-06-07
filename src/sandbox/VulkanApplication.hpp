#pragma once

#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>

struct GLFWwindow;
class renderer;

/**
 * \brief Helper Struct that is used to determine whether the physical device chosen supports a certain queue family.
 */
struct queue_family_indicies
{
    std::optional<uint32_t> graphics_family{std::nullopt};
    std::optional<uint32_t> present_family{std::nullopt};

    bool is_complete()
    {
        return graphics_family.has_value() && present_family.has_value();
    }
};

/**
 * \brief Saves swapchain capabilities that may be needed later on.
 */
struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR surface_capabilities_khr;
    std::vector<VkSurfaceFormatKHR> surface_formats;
    std::vector<VkPresentModeKHR> present_modes;
};

/**
 * \brief Saves swapchain settings that may be needed later on.
 */
struct SwapchainSettings
{
    VkSurfaceFormatKHR surface_format_khr;
    VkPresentModeKHR present_mode_khr;
    VkExtent2D extent_2d;
};

/**
 * \brief This class is used to initialize and manage our applications state.
 */
class application
{
public:
    void run();

private:
    void init_window();
    void shutdown_window();

    bool check_required_extensions(std::vector<const char*>& extensions) const;
    bool check_required_layers(std::vector<const char*>& layers) const;
    bool check_required_physical_device_extensions(VkPhysicalDevice physical_device, std::vector<const char*>& physical_device_extensions) const;

    void init_vulkan_debug_pfn();

    SwapchainSupportDetails fetch_swapchain_support_details(VkPhysicalDevice physical_device, VkSurfaceKHR surface_khr) const;
    SwapchainSettings select_optimal_swapchain_settings(const SwapchainSupportDetails& swapchain_support_details) const;
    queue_family_indicies fetch_queue_family_indicies(VkPhysicalDevice physical_device) const;
    VkPhysicalDevice pick_physical_device(const std::vector<VkPhysicalDevice>& physical_devices, VkSurfaceKHR surface_khr) const;

    void init_vulkan();
    void shutdown_vulkan();

    void init_renderer();
    void shutdown_renderer();

    void render();

    void main_loop();

private:
    GLFWwindow* window_{nullptr};

    renderer* renderer_{nullptr};

    VkInstance vk_instance_{VK_NULL_HANDLE};
    VkPhysicalDevice vk_physical_device_{VK_NULL_HANDLE};
    VkDevice vk_device_{VK_NULL_HANDLE};
    VkSurfaceKHR vk_surface_khr_{VK_NULL_HANDLE};

    VkQueue vk_graphics_queue_{VK_NULL_HANDLE};
    VkQueue vk_present_queue_{VK_NULL_HANDLE};

    VkSwapchainKHR vk_swapchain_khr_{VK_NULL_HANDLE};
    std::vector<VkImage> vk_swapchain_images_;
    std::vector<VkImageView> vk_swapchain_image_views_;

    VkFormat vk_swapchain_image_format_;
    VkExtent2D vk_swapchain_extent_2d_;

    VkDescriptorPool vk_descriptor_pool_;
    VkCommandPool vk_command_pool_{VK_NULL_HANDLE};

    std::vector<VkSemaphore> vk_available_image_semaphores_;
    std::vector<VkSemaphore> vk_finished_render_semaphores_;
    std::vector<VkFence> vk_in_flight_fences_;
    size_t current_frame_{0};

    VkDebugUtilsMessengerEXT vk_debug_utils_messenger_{VK_NULL_HANDLE};

    static std::vector<const char*> vk_required_physical_device_extensions_;
    static std::vector<const char*> vk_required_validation_layers_;

    static PFN_vkCreateDebugUtilsMessengerEXT vk_create_debug_utils_messenger_;
    static PFN_vkDestroyDebugUtilsMessengerEXT vk_destroy_debug_utils_messenger_;

    const uint32_t window_width_ = 640;
    const uint32_t window_height_ = 480;

    const uint32_t max_frames_in_flight_ = 2;
};