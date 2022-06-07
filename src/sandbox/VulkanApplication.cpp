#include "VulkanApplication.hpp"
#include "VulkanRenderer.hpp"
#include "VulkanRendererContext.hpp"
#include "VulkanRenderData.hpp"
#include "VulkanUtils.hpp"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <iostream>
#include <set>
#include <array>

/**
 * \brief Callback Function for our debug messenger that the validation layers use.
 * \param message_severity A bitmask of VkDebugUtilsMessageSeverityFlagBitsEXT specifying which type of event(s) will cause this callback to be called.
 * \param message_type A bitmask of VkDebugUtilsMessageTypeFlagBitsEXT specifying which type of event(s) will cause this callback to be called.
 * \param p_callback_data Contains all the callback related data in the VkDebugUtilsMessengerCallbackDataEXT structure.
 * \param p_user_data User data provided when the VkDebugUtilsMessengerEXT was created.
 * \return VkBool32
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                     VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
                                                     void* p_user_data)
{
    std::cerr << "[Validation Layer]: " << p_callback_data->pMessage << std::endl;

    return VK_FALSE;
}


std::vector<const char*> application::vk_required_physical_device_extensions_ = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,};
std::vector<const char*> application::vk_required_validation_layers_ = {"VK_LAYER_KHRONOS_validation",};

/**
 * \brief Function Pointers for our debug messenger functions
 */
PFN_vkCreateDebugUtilsMessengerEXT application::vk_create_debug_utils_messenger_{VK_NULL_HANDLE};
PFN_vkDestroyDebugUtilsMessengerEXT application::vk_destroy_debug_utils_messenger_{VK_NULL_HANDLE};

/**
 * \brief Does what the name says. It runs our application. (Manages the creation and destruction of our applications state)
 */
void application::run()
{
    init_window();
    init_vulkan();
    init_renderer();
    main_loop();
    shutdown_renderer();
    shutdown_vulkan();
    shutdown_window();
}

void application::render()
{
    vkWaitForFences(vk_device_, 1, &vk_in_flight_fences_[current_frame_], VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(vk_device_, 1, &vk_in_flight_fences_[current_frame_]);
    uint32_t image_index;
    vkAcquireNextImageKHR(vk_device_, vk_swapchain_khr_, std::numeric_limits<uint64_t>::max(), vk_available_image_semaphores_[current_frame_], VK_NULL_HANDLE, &image_index);

    VkCommandBuffer command_buffer = renderer_->render(image_index);

    VkSemaphore wait_semaphores[] = {vk_available_image_semaphores_[current_frame_]};
    VkPipelineStageFlags pipeline_wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSemaphore signal_semaphores[] = {vk_finished_render_semaphores_[current_frame_]};

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = pipeline_wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    VK_CHECK(vkQueueSubmit(vk_graphics_queue_, 1, &submit_info, vk_in_flight_fences_[current_frame_]));

    VkSwapchainKHR swapchains[] = {vk_swapchain_khr_};

    VkPresentInfoKHR present_info_khr{};
    present_info_khr.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info_khr.waitSemaphoreCount = 1;
    present_info_khr.pWaitSemaphores = signal_semaphores;
    present_info_khr.swapchainCount = 1;
    present_info_khr.pSwapchains = swapchains;
    present_info_khr.pImageIndices = &image_index;
    present_info_khr.pResults = nullptr;

    VK_CHECK(vkQueuePresentKHR(vk_present_queue_, &present_info_khr));

    current_frame_ = (current_frame_ + 1) % max_frames_in_flight_;
}

/**
 * \brief Creates a GLFW windows.
 */
void application::init_window()
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window_ = glfwCreateWindow(window_width_, window_height_, "Physically Based Rendering", nullptr, nullptr);
    if (!window_)
    {
        return;
    }
}

/**
 * \brief Destroys GLFW window.
 */
void application::shutdown_window()
{
    glfwDestroyWindow(window_);
    window_ = nullptr;
}

/**
 * \brief
 */
void application::init_renderer()
{
    renderer_context context = {};
    context.vk_device_ = vk_device_;
    context.vk_physical_device_ = vk_physical_device_;
    context.vk_command_pool_ = vk_command_pool_;
    context.vk_descriptor_pool = vk_descriptor_pool_;
    context.vk_format_ = vk_swapchain_image_format_;
    context.vk_extent_2d_ = vk_swapchain_extent_2d_;
    context.vk_image_views_ = vk_swapchain_image_views_;
    context.graphics_queue = vk_graphics_queue_;
    context.present_queue = vk_present_queue_;

    renderer_ = new renderer(context);
    renderer_->init("D:/PBR/shaders/vertex_shader.spv", "D:/PBR/shaders/fragment_shader.spv", "D:/PBR/textures/texture.jpg");
}

/**
 * \brief
 */
void application::shutdown_renderer()
{
    renderer_->shutdown();

    delete renderer_;
    renderer_ = nullptr;
}

/**
 * \brief Checks to make sure that our device has the neccessary vulkan extensions in order to run this application.
 * \param extensions Vector that contains all extensions used by the application
 * \return bool
 */
bool application::check_required_extensions(std::vector<const char*>& extensions) const
{
    uint32_t vk_extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &vk_extension_count, nullptr);
    std::vector<VkExtensionProperties> vk_extensions(vk_extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &vk_extension_count, vk_extensions.data());

    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char*> required_extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    extensions.clear();
    for (const auto& required_extension : required_extensions)
    {
        bool supported = false;
        for (const auto& vk_extension : vk_extensions)
        {
            if (strcmp(required_extension, vk_extension.extensionName) == 0)
            {
                supported = true;
                break;
            }
        }

        if (!supported)
        {
            std::cerr << required_extension << " is not supported on this device!" << std::endl;
            return false;
        }

        std::cout << required_extension << " is enabled!" << std::endl;
        extensions.push_back(required_extension);
    }

    return true;
}

/**
 * \brief Checks to make sure that our device has the neccessary vulkan layers in order to run this application.
 * \param layers Vector that contains all layers used by the application.
 * \return bool
 */
bool application::check_required_layers(std::vector<const char*>& layers) const
{
    uint32_t vk_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&vk_layer_count, nullptr);
    std::vector<VkLayerProperties> vk_layers(vk_layer_count);
    vkEnumerateInstanceLayerProperties(&vk_layer_count, vk_layers.data());

    layers.clear();
    for (const auto required_layer : vk_required_validation_layers_)
    {
        bool supported = false;
        for (const auto& vk_layer : vk_layers)
        {
            if (strcmp(required_layer, vk_layer.layerName) == 0)
            {
                supported = true;
                break;
            }
        }

        if (!supported)
        {
            std::cerr << required_layer << "is not supported on this device!" << std::endl;
            return false;
        }

        std::cout << required_layer << " is enabled" << std::endl;
        layers.push_back(required_layer);
    }

    return true;
}

/**
 * \brief
 * \param physical_device
 * \param physical_device_extensions
 * \return
 */
bool application::check_required_physical_device_extensions(VkPhysicalDevice physical_device, std::vector<const char*>& physical_device_extensions) const
{
    uint32_t physical_device_extension_count = 0;
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &physical_device_extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(physical_device_extension_count);
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &physical_device_extension_count, extensions.data());

    physical_device_extensions.clear();
    for (const char* required_extension : vk_required_physical_device_extensions_)
    {
        bool supported = false;
        for (const auto extension : extensions)
        {
            if (strcmp(required_extension, extension.extensionName) == 0)
            {
                supported = true;
                break;
            }
        }

        if (!supported)
        {
            std::cerr << required_extension << " is not supported on this physical device!" << std::endl;
            return false;
        }

        std::cout << required_extension << " is enabled on this physical device" << std::endl;
        physical_device_extensions.push_back(required_extension);
    }

    return true;
}


/**
 * \brief Retrieves function pointers for the vkCreateDebugUtilsMessengerEXT & vkDestroyDebugUtilsMessengerEXT functions.
 */
void application::init_vulkan_debug_pfn()
{
    vk_create_debug_utils_messenger_ = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(vk_instance_, "vkCreateDebugUtilsMessengerEXT"));
    assert(vk_create_debug_utils_messenger_ != VK_NULL_HANDLE, "Cannot find vkCreateDebugUtilsMessengerEXT function!");

    vk_destroy_debug_utils_messenger_ = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(vk_instance_, "vkDestroyDebugUtilsMessengerEXT"));
    assert(vk_destroy_debug_utils_messenger_ != VK_NULL_HANDLE, "Cannot find vkCreateDebugUtilsMessengerEXT function!");
}

/**
 * \brief Gets the index values for all the queue families that the application needs.
 * \param physical_device The physical device that is being checked for the queue families.
 * \return queue_family_indicies
 */
queue_family_indicies application::fetch_queue_family_indicies(VkPhysicalDevice physical_device) const
{
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties.data());

    int i = 0;
    queue_family_indicies indicies{};

    for (const auto& queue_family_property : queue_family_properties)
    {
        if (queue_family_property.queueCount > 0 && queue_family_property.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indicies.graphics_family = std::make_optional(i);
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, vk_surface_khr_, &present_support);

        if (queue_family_property.queueCount > 0 && present_support)
        {
            indicies.present_family = std::make_optional(i);
        }

        if (indicies.is_complete())
        {
            break;
        }

        i++;
    }

    return indicies;
}

/**
 * \brief Retreives the physical device that supports all the features that the application requires.
 * \param physical_devices List of all physical device present in the current system.
 * \param surface_khr The program surface.
 * \return VkPhysicalDevice
 */
VkPhysicalDevice application::pick_physical_device(const std::vector<VkPhysicalDevice>& physical_devices, VkSurfaceKHR surface_khr) const
{
    for (const auto& physical_device : physical_devices)
    {
        VkPhysicalDeviceProperties physical_device_properties;
        vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

        queue_family_indicies indicies = fetch_queue_family_indicies(physical_device);
        if (!indicies.is_complete())
        {
            std::cerr << "Could not fetch physical device queue family indicies!!!" << std::endl;
            return VK_NULL_HANDLE;
        }

        std::vector<const char*> physical_device_extensions;
        if (!check_required_physical_device_extensions(physical_device, physical_device_extensions))
        {
            return VK_NULL_HANDLE;
        }

        SwapchainSupportDetails swapchain_detais = fetch_swapchain_support_details(physical_device, surface_khr);
        if (swapchain_detais.surface_formats.empty() || swapchain_detais.present_modes.empty())
        {
            return VK_NULL_HANDLE;
        }

        VkPhysicalDeviceFeatures physical_device_features;
        vkGetPhysicalDeviceFeatures(physical_device, &physical_device_features);

        if (!physical_device_features.geometryShader)
        {
            return VK_NULL_HANDLE;
        }

        if (!physical_device_features.samplerAnisotropy)
        {
            return VK_NULL_HANDLE;
        }

        if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            std::cout << "Using Discrete GPU: " << physical_device_properties.deviceName << std::endl;
            return physical_device;
        }
    }

    if (!physical_devices.empty())
    {
        VkPhysicalDeviceProperties physical_device_properties;
        vkGetPhysicalDeviceProperties(physical_devices[0], &physical_device_properties);

        queue_family_indicies indicies = fetch_queue_family_indicies(physical_devices[0]);
        if (!indicies.is_complete())
        {
            std::cerr << "Could not fetch physical device queue family indicies!!!" << std::endl;
            return VK_NULL_HANDLE;
        }

        std::vector<const char*> physical_device_extensions;
        if (!check_required_physical_device_extensions(physical_devices[0], physical_device_extensions))
        {
            return VK_NULL_HANDLE;
        }

        SwapchainSupportDetails swapchain_detais = fetch_swapchain_support_details(physical_devices[0], surface_khr);
        if (swapchain_detais.surface_formats.empty() || swapchain_detais.present_modes.empty())
        {
            return VK_NULL_HANDLE;
        }

        VkPhysicalDeviceFeatures physical_device_features;
        vkGetPhysicalDeviceFeatures(physical_devices[0], &physical_device_features);

        if (!physical_device_features.geometryShader)
        {
            return VK_NULL_HANDLE;
        }

        if (!physical_device_features.samplerAnisotropy)
        {
            return VK_NULL_HANDLE;
        }

        std::cout << "Using Fallback GPU: " << physical_device_properties.deviceName << std::endl;
        return physical_devices[0];
    }

    std::cerr << "No Physical device available" << std::endl;

    return VK_NULL_HANDLE;
}

/**
 * \brief
 * \param physical_device
 * \param surface_khr
 * \return
 */
SwapchainSupportDetails application::fetch_swapchain_support_details(VkPhysicalDevice physical_device, VkSurfaceKHR surface_khr) const
{
    SwapchainSupportDetails swapchain_support_details;

    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface_khr, &swapchain_support_details.surface_capabilities_khr));

    uint32_t format_count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface_khr, &format_count, nullptr));

    if (format_count > 0)
    {
        swapchain_support_details.surface_formats.resize(format_count);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface_khr, &format_count, swapchain_support_details.surface_formats.data()));
    }

    uint32_t present_mode_count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface_khr, &present_mode_count, nullptr));

    if (present_mode_count > 0)
    {
        swapchain_support_details.present_modes.resize(present_mode_count);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface_khr, &present_mode_count, swapchain_support_details.present_modes.data()));
    }

    return swapchain_support_details;
}

/**
 * \brief
 * \param swapchain_support_details
 * \return
 */
SwapchainSettings application::select_optimal_swapchain_settings(const SwapchainSupportDetails& swapchain_support_details) const
{
    assert(!swapchain_support_details.surface_formats.empty(), "Swapchain surface formats were not reteived correctly");
    assert(!swapchain_support_details.present_modes.empty(), "Swapchain present modes were not reteived correctly");

    SwapchainSettings swapchain_settings;

    // NOTE(dhaval): Select the best format if the surface has no preferred format.
    if (swapchain_support_details.surface_formats.size() == 1 && swapchain_support_details.surface_formats[0].format == VK_FORMAT_UNDEFINED)
    {
        swapchain_settings.surface_format_khr = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }
    // NOTE(dhaval): Otherwise, select one of the available formats
    else
    {
        swapchain_settings.surface_format_khr = swapchain_support_details.surface_formats[0];
        for (const auto& format : swapchain_support_details.surface_formats)
        {
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                swapchain_settings.surface_format_khr = format;
                break;
            }
        }
    }

    // NOTE(dhaval): Select the best present mode
    swapchain_settings.present_mode_khr = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& present_mode : swapchain_support_details.present_modes)
    {
        if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            swapchain_settings.present_mode_khr = present_mode;
        }

        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchain_settings.present_mode_khr = present_mode;
            break;
        }
    }

    // NOTE(dhaval): Select the current swap extent if the window manager doesn't allow to set custom extent
    if (swapchain_support_details.surface_capabilities_khr.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        swapchain_settings.extent_2d = swapchain_support_details.surface_capabilities_khr.currentExtent;
    }
    // NOTE(dhaval): Otherwise, manually set the extent to match the min/max extent bounds
    else
    {
        const VkSurfaceCapabilitiesKHR& capabilities = swapchain_support_details.surface_capabilities_khr;

        swapchain_settings.extent_2d = {window_width_, window_height_};
        swapchain_settings.extent_2d.width = std::max(capabilities.minImageExtent.width, std::min(swapchain_settings.extent_2d.width, capabilities.maxImageExtent.width));
        swapchain_settings.extent_2d.height = std::max(capabilities.minImageExtent.height, std::min(swapchain_settings.extent_2d.height, capabilities.maxImageExtent.height));
    }

    return swapchain_settings;
}

/**
 * \brief Initializes our vulkan application.
 */
void application::init_vulkan()
{
    // NOTE(dhaval): Checking required extensions and layers.
    std::vector<const char*> extensions;
    assert(check_required_extensions(extensions) != false, "This device does not have the supported extensions");

    std::vector<const char*> layers;
    assert(check_required_layers(layers) != false, "This device does not have the supported layers");

    // NOTE(dhaval): Create Vulkan Instance.
    VkApplicationInfo application_info{};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = "Physically Based Rendering";
    application_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    application_info.pEngineName = "No Engine";
    application_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    application_info.apiVersion = VK_API_VERSION_1_0;

    VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info{};
    debug_utils_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_utils_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_utils_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_utils_messenger_create_info.pfnUserCallback = debug_callback;
    debug_utils_messenger_create_info.pUserData = nullptr;

    VkInstanceCreateInfo instance_create_info{};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &application_info;
    instance_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instance_create_info.ppEnabledExtensionNames = extensions.data();
    instance_create_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
    instance_create_info.ppEnabledLayerNames = layers.data();
    instance_create_info.pNext = &debug_utils_messenger_create_info;

    VK_CHECK(vkCreateInstance(&instance_create_info, nullptr, &vk_instance_));

    init_vulkan_debug_pfn();

    // NOTE(dhaval): Create Vulkan Debug Messenger.
    VK_CHECK(vk_create_debug_utils_messenger_(vk_instance_, &debug_utils_messenger_create_info, nullptr, &vk_debug_utils_messenger_));

    // NOTE(dhaval): Create Vulkan Win32 Surface.
    VkWin32SurfaceCreateInfoKHR win32_surface_create_info{};
    win32_surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32_surface_create_info.hwnd = glfwGetWin32Window(window_);
    win32_surface_create_info.hinstance = GetModuleHandleA(nullptr);

    VK_CHECK(vkCreateWin32SurfaceKHR(vk_instance_, &win32_surface_create_info, nullptr, &vk_surface_khr_));

    // NOTE(dhaval): Enumerate Physical Devices
    uint32_t physical_device_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(vk_instance_, &physical_device_count, nullptr));
    assert(physical_device_count != 0, "Failed to find GPUs that support vulkan!!!");

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    VK_CHECK(vkEnumeratePhysicalDevices(vk_instance_, &physical_device_count, physical_devices.data()));

    vk_physical_device_ = pick_physical_device(physical_devices, vk_surface_khr_);
    assert(vk_physical_device_ != VK_NULL_HANDLE, "Failed to find a suitable GPU!!!");

    // NOTE(dhaval): Create Logical Device.
    queue_family_indicies indicies = fetch_queue_family_indicies(vk_physical_device_);

    const float queue_priority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = {indicies.graphics_family.value(), indicies.present_family.value()};

    for (uint32_t queue_family_index : unique_queue_families)
    {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family_index;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;

        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures physical_device_features{};
    physical_device_features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pEnabledFeatures = &physical_device_features;
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(vk_required_physical_device_extensions_.size());
    device_create_info.ppEnabledExtensionNames = vk_required_physical_device_extensions_.data();
    device_create_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
    device_create_info.ppEnabledLayerNames = layers.data();

    VK_CHECK(vkCreateDevice(vk_physical_device_, &device_create_info, nullptr, &vk_device_));

    // NOTE(dhaval): Get Logical Device Queues.
    vkGetDeviceQueue(vk_device_, indicies.graphics_family.value(), 0, &vk_graphics_queue_);
    assert(vk_graphics_queue_ != VK_NULL_HANDLE, "Graphics Queue could not be retreived");

    vkGetDeviceQueue(vk_device_, indicies.present_family.value(), 0, &vk_present_queue_);
    assert(vk_present_queue_ != VK_NULL_HANDLE, "Present Queue could not be retreived");

    // Create Swapchain
    SwapchainSupportDetails swapchain_support_details = fetch_swapchain_support_details(vk_physical_device_, vk_surface_khr_);
    SwapchainSettings swapchain_settings = select_optimal_swapchain_settings(swapchain_support_details);

    // NOTE(dhaval): Simply sticking to this minimum means that we may sometimes have to wait
    // on the driver to complete internal operations before we can acquire another image to render to.
    // Therefore it is recommended to request at least one more image than the minimum.
    uint32_t image_count = swapchain_support_details.surface_capabilities_khr.minImageCount + 1;

    // NOTE(dhaval): We should also make sure to not exceed the maximum number of images while doing this,
    // where 0 is a special value that means that there is no maximum
    if (swapchain_support_details.surface_capabilities_khr.maxImageCount > 0)
    {
        image_count = std::min(image_count, swapchain_support_details.surface_capabilities_khr.maxImageCount);
    }

    VkSwapchainCreateInfoKHR swapchain_create_info{};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = vk_surface_khr_;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = swapchain_settings.surface_format_khr.format;
    swapchain_create_info.imageColorSpace = swapchain_settings.surface_format_khr.colorSpace;
    swapchain_create_info.imageExtent = swapchain_settings.extent_2d;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (indicies.graphics_family.value() != indicies.present_family.value())
    {
        uint32_t queue_families[] = {indicies.graphics_family.value(), indicies.present_family.value()};
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = queue_families;
    }
    else
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = nullptr;
    }

    swapchain_create_info.preTransform = swapchain_support_details.surface_capabilities_khr.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = swapchain_settings.present_mode_khr;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(vk_device_, &swapchain_create_info, nullptr, &vk_swapchain_khr_));

    uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_khr_, &swapchain_image_count, nullptr);
    assert(swapchain_image_count != 0, "Zero swapchain images");

    vk_swapchain_images_.resize(swapchain_image_count);
    vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_khr_, &swapchain_image_count, vk_swapchain_images_.data());

    vk_swapchain_image_format_ = swapchain_settings.surface_format_khr.format;
    vk_swapchain_extent_2d_ = swapchain_settings.extent_2d;

    // NOTE(dhaval): Create Command Pool
    VkCommandPoolCreateInfo command_pool_create_info{};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.queueFamilyIndex = indicies.graphics_family.value();
    command_pool_create_info.flags = 0;

    VK_CHECK(vkCreateCommandPool(vk_device_, &command_pool_create_info, nullptr, &vk_command_pool_));

    // NOTE(dhaval): Create descriptor pools
    std::array<VkDescriptorPoolSize, 2> descriptor_pool_sizes{};
    descriptor_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_sizes[0].descriptorCount = swapchain_image_count;
    descriptor_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_pool_sizes[1].descriptorCount = swapchain_image_count;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info{};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size());
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes.data();
    descriptor_pool_create_info.maxSets = swapchain_image_count;
    descriptor_pool_create_info.flags = 0;

    VK_CHECK(vkCreateDescriptorPool(vk_device_, &descriptor_pool_create_info, nullptr, &vk_descriptor_pool_));

    // NOTE(dhaval): Create Sync Objects
    vk_available_image_semaphores_.resize(max_frames_in_flight_);
    vk_finished_render_semaphores_.resize(max_frames_in_flight_);
    vk_in_flight_fences_.resize(max_frames_in_flight_);

    for (size_t i = 0; i < max_frames_in_flight_; i++)
    {
        VkSemaphoreCreateInfo semaphore_create_info{};
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VK_CHECK(vkCreateSemaphore(vk_device_, &semaphore_create_info, nullptr, &vk_available_image_semaphores_[i]));
        VK_CHECK(vkCreateSemaphore(vk_device_, &semaphore_create_info, nullptr, &vk_finished_render_semaphores_[i]));

        VkFenceCreateInfo fence_create_info{};
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VK_CHECK(vkCreateFence(vk_device_, &fence_create_info, nullptr, &vk_in_flight_fences_[i]));
    }

    renderer_context renderer_context{};
    renderer_context.vk_device_ = vk_device_;

    // NOTE(dhaval): Create swapchain image views
    vk_swapchain_image_views_.resize(swapchain_image_count);
    for (size_t i = 0; i < vk_swapchain_image_views_.size(); i++)
    {
        vk_swapchain_image_views_[i] = vulkan_utils::create_image_2d_view(renderer_context, vk_swapchain_images_[i], vk_swapchain_image_format_);
    }
}

/**
 * \brief Destroys all the vulkan resources that were initialized in init_vulkan().
 */
void application::shutdown_vulkan()
{
    vkDestroyCommandPool(vk_device_, vk_command_pool_, nullptr);
    vk_command_pool_ = VK_NULL_HANDLE;

    vkDestroyDescriptorPool(vk_device_, vk_descriptor_pool_, nullptr);
    vk_descriptor_pool_ = VK_NULL_HANDLE;

    for (size_t i = 0; i < max_frames_in_flight_; i++)
    {
        vkDestroySemaphore(vk_device_, vk_finished_render_semaphores_[i], nullptr);
        vkDestroySemaphore(vk_device_, vk_available_image_semaphores_[i], nullptr);
        vkDestroyFence(vk_device_, vk_in_flight_fences_[i], nullptr);
    }

    vk_finished_render_semaphores_.clear();
    vk_available_image_semaphores_.clear();
    vk_in_flight_fences_.clear();

    for (auto image_view : vk_swapchain_image_views_)
    {
        vkDestroyImageView(vk_device_, image_view, nullptr);
    }

    vk_swapchain_image_views_.clear();
    vk_swapchain_images_.clear();

    vkDestroySwapchainKHR(vk_device_, vk_swapchain_khr_, nullptr);
    vk_swapchain_khr_ = VK_NULL_HANDLE;

    vkDestroyDevice(vk_device_, nullptr);
    vk_device_ = VK_NULL_HANDLE;

    vkDestroySurfaceKHR(vk_instance_, vk_surface_khr_, nullptr);
    vk_surface_khr_ = VK_NULL_HANDLE;

    vk_destroy_debug_utils_messenger_(vk_instance_, vk_debug_utils_messenger_, nullptr);
    vk_debug_utils_messenger_ = VK_NULL_HANDLE;

    vkDestroyInstance(vk_instance_, nullptr);
    vk_instance_ = VK_NULL_HANDLE;
}

/**
 * \brief Main application loop.
 */
void application::main_loop()
{
    if (!window_)
    {
        return;
    }

    while (!glfwWindowShouldClose(window_))
    {
        glfwPollEvents();
        render();
    }

    vkDeviceWaitIdle(vk_device_);
}
