#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#else
    #error OS not supported.
#endif // _WIN32

#include <cstdlib>
#include <cstring>
#include <limits>
#include <Windows.h>
#include <vulkan/vulkan.h>
#include <jolt/version.hpp>
#include <jolt/debug.hpp>
#include <jolt/memory/allocator.hpp>
#include <jolt/collections/vector.hpp>
#include <jolt/collections/valueset.hpp>
#include <jolt/collections/array.hpp>
#include "vulkan.hpp"

#define N_SWAPCHAIN_IMAGES 3

#ifdef _WIN32
    #define OS_SPECIFIC_SURFACE_EXTENSION VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif // _WIN32

static VkAllocationCallbacks *g_allocator = nullptr;
static VkInstance g_instance;
static VkPhysicalDevice g_phy_device;
static VkDevice g_device;
static VkPhysicalDeviceProperties2 g_phy_props;
static VkPhysicalDeviceVulkan11Properties g_phy_props11;
static VkPhysicalDeviceFeatures2 g_phy_feats;
static VkPhysicalDeviceVulkan11Features g_phy_feats11;
static VkPhysicalDeviceVulkan12Features g_phy_feats12;
static VkPhysicalDeviceMemoryProperties g_phy_mem_props;

static uint32_t g_q_graphics_fam_index, g_q_transfer_fam_index;
static uint32_t g_q_graphics_index, g_q_transfer_index;
static VkQueue g_q_graphics, g_q_transfer;
static const float g_q_priorities[2] = {1.0f /* graphics */, 1.0f, /* transfer */};

static VkSurfaceKHR g_win_surface;
static VkSurfaceCapabilitiesKHR g_win_surface_caps;
static VkImageFormatProperties g_phy_dev_image_fmt_props;
static VkFormat g_win_surface_fmt;
static bool g_use_window;

static VkSwapchainKHR g_swapchain;
static jolt::collections::Array<VkImage> *g_swapchain_images = nullptr;
static jolt::collections::Array<VkImageView> *g_swapchain_image_views = nullptr;

static VkImage g_ds_image;
static VkImageView g_ds_image_view;
static VkDeviceMemory g_ds_image_memory;
static VkFormat g_ds_image_fmt;

static VkRenderPass g_render_pass;
static jolt::collections::Array<VkFramebuffer> *g_framebuffers;

using namespace jolt::text;

#ifdef _DEBUG
static VkDebugUtilsMessengerEXT g_debug_clbk;

static PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT;
static PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT;

VkBool32 VKAPI_PTR debug_logger_clbk(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageTypes,
  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
  void *pUserData) {
    switch(messageSeverity) {
    #define log(type)                                                                              \
        jolt::console.type(s(pCallbackData->pMessageIdName) + " - " + s(pCallbackData->pMessage))

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: log(info); break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: log(info); break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: log(info); break;

    default: log(debug); break;

    #undef log
    }

    return VK_FALSE;
}
#endif // _DEBUG

namespace jolt {
    namespace graphics {
        using layer_vector = collections::Vector<const char *>;
        using extension_vector = layer_vector;
        using queue_ci_vector = collections::Vector<VkDeviceQueueCreateInfo>;

        static layer_vector select_required_layers() {
            layer_vector layers; // Layers that will be returned
            uint32_t n_available_layers;
            const char *required_layers[] = {
#ifdef _DEBUG
              "VK_LAYER_KHRONOS_validation"
#endif // _DEBUG
            };
            const size_t required_layers_length = sizeof(required_layers) / sizeof(const char *);

            vkEnumerateInstanceLayerProperties(&n_available_layers, nullptr);

            collections::Array<VkLayerProperties> layer_props{n_available_layers};

            vkEnumerateInstanceLayerProperties(&n_available_layers, layer_props);

            for(size_t i = 0; i < required_layers_length; ++i) {
                bool found = false;

                for(auto it = layer_props.begin(), end = layer_props.end(); it != end; ++it) {
                    if(strcmp(required_layers[i], (*it).layerName) == 0) {
                        found = true;

                        layers.push(required_layers[i]);
                        break;
                    }
                }

                if(found) {
                    console.info("Found required layer " + s(required_layers[i]));
                } else {
                    console.err("Required layer " + s(required_layers[i]) + " not found");
                    abort();
                }
            }

            return layers;
        }

        static extension_vector select_required_instance_extensions() {
            extension_vector extensions; // Layers that will be returned
            uint32_t n_available_ext;
            const char *required_ext[] = {
              VK_KHR_SURFACE_EXTENSION_NAME,
              OS_SPECIFIC_SURFACE_EXTENSION,

#ifdef _DEBUG
              VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif // _DEBUG
            };
            const size_t required_ext_length = sizeof(required_ext) / sizeof(const char *);

            vkEnumerateInstanceExtensionProperties(nullptr, &n_available_ext, nullptr);
            collections::Array<VkExtensionProperties> ext_props{n_available_ext};
            vkEnumerateInstanceExtensionProperties(nullptr, &n_available_ext, ext_props);

            for(size_t i = 0; i < required_ext_length; ++i) {
                bool found = false;

                for(auto it = ext_props.begin(), end = ext_props.end(); it != end; ++it) {
                    if(strcmp(required_ext[i], (*it).extensionName) == 0) {
                        found = true;

                        extensions.push(required_ext[i]);
                        break;
                    }
                }

                if(found) {
                    console.info("Found required extension " + s(required_ext[i]));
                } else {
                    console.err("Required extension " + s(required_ext[i]) + " not found");
                    abort();
                }
            }

            return extensions;
        }

        static extension_vector select_required_device_extensions() {
            extension_vector extensions; // Layers that will be returned
            uint32_t n_available_ext;
            const char *required_ext[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
            const size_t required_ext_length = sizeof(required_ext) / sizeof(const char *);

            vkEnumerateDeviceExtensionProperties(g_phy_device, nullptr, &n_available_ext, nullptr);
            collections::Array<VkExtensionProperties> ext_props{n_available_ext};
            vkEnumerateDeviceExtensionProperties(
              g_phy_device, nullptr, &n_available_ext, ext_props);

            for(size_t i = 0; i < required_ext_length; ++i) {
                bool found = false;

                for(auto it = ext_props.begin(), end = ext_props.end(); it != end; ++it) {
                    if(strcmp(required_ext[i], (*it).extensionName) == 0) {
                        found = true;

                        extensions.push(required_ext[i]);
                        break;
                    }
                }

                if(found) {
                    console.info("Found required device extension " + s(required_ext[i]));
                } else {
                    console.err("Required device extension " + s(required_ext[i]) + " not found");
                    abort();
                }
            }

            return extensions;
        }

        static void initialize_debug_logger() {
#ifdef _DEBUG
            console.debug("Initializing Vulkan debug logger");

            pfnCreateDebugUtilsMessengerEXT =
              reinterpret_cast<typeof(pfnCreateDebugUtilsMessengerEXT)>(
                vkGetDeviceProcAddr(g_device, "vkCreateDebugUtilsMessengerEXT"));
            pfnDestroyDebugUtilsMessengerEXT =
              reinterpret_cast<typeof(pfnDestroyDebugUtilsMessengerEXT)>(
                vkGetDeviceProcAddr(g_device, "vkDestroyDebugUtilsMessengerEXT"));

            if(!pfnCreateDebugUtilsMessengerEXT || !pfnDestroyDebugUtilsMessengerEXT) {
                console.warn("Unable to initialize debug logger API. No Vulkan-specific logging "
                             "will be provided");

                pfnCreateDebugUtilsMessengerEXT = nullptr;
                pfnDestroyDebugUtilsMessengerEXT = nullptr;
                return;
            }

            VkDebugUtilsMessengerCreateInfoEXT cinfo{
              VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT, // sType
              nullptr,                                                 // pNext
              0,                                                       // flags
              VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, // messageSeverity
              VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, // messageType
              &debug_logger_clbk,                                  // pfnUserCallback
              nullptr                                              // pUserDate
            };

            VkResult result =
              pfnCreateDebugUtilsMessengerEXT(g_instance, &cinfo, g_allocator, &g_debug_clbk);

            if(!result) {
                console.err("Unable to initialize Vulkan debug logger");
                abort();
            }
#endif // _DEBUG
        }

        static void shutdown_debug_logger() {
#ifdef _DEBUG
            if(pfnCreateDebugUtilsMessengerEXT && pfnDestroyDebugUtilsMessengerEXT) {
                console.debug("Destroying Vulkan debug logger");

                pfnDestroyDebugUtilsMessengerEXT(g_instance, g_debug_clbk, g_allocator);
            }
#endif // _DEBUG
        }

        static void initialize_instance(GraphicsEngineInitializationParams &params) {
            console.debug("Initializing Vulkan instance");

            VkApplicationInfo app_info{
              VK_STRUCTURE_TYPE_APPLICATION_INFO, // sType
              nullptr,                            // pNext
              "jolt",                             // pApplicationName
              VK_MAKE_VERSION(
                params.app_version_major,
                params.app_version_minor,
                params.app_version_revision), // applicationVersion
              params.app_name,                // pEngineName
              VK_MAKE_VERSION(
                JLT_VERSION_MAJOR, JLT_VERSION_MINOR, JLT_VERSION_PATCH), // engineVersion
              VK_API_VERSION_1_2                                          // apiVersion
            };
            layer_vector layers = select_required_layers();
            extension_vector extensions = select_required_instance_extensions();

            VkInstanceCreateInfo icf{
              VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // sType
              nullptr,                                // pNext
              0,                                      // flags
              &app_info,                              // pApplicationInfo
              layers.get_length(),                    // enabledLayerCount
              &layers[0],                             // ppEnabledLayerNames
              extensions.get_length(),                // enabledExtensionCount
              &extensions[0]                          // ppEnabledExtensionNames
            };

            VkResult result = vkCreateInstance(&icf, g_allocator, &g_instance);

            jltassert(result == VK_SUCCESS);
        }

        static void shutdown_instance() {
            console.debug("Destroying Vulkan instance");
            vkDestroyInstance(g_instance, g_allocator);
        }

        static void select_physical_device() {
            console.debug("Selecting Vulkan physical device");

            VkResult result;
            uint32_t n_devs;

            result = vkEnumeratePhysicalDevices(g_instance, &n_devs, nullptr);
            jltassert2(result == VK_SUCCESS, "Unable to enumerate physical devices");

            collections::Array<VkPhysicalDevice> devs{n_devs};

            result = vkEnumeratePhysicalDevices(g_instance, &n_devs, devs);
            jltassert2(result == VK_SUCCESS, "Unable to enumerate physical devices");

            g_phy_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            g_phy_props.pNext = &g_phy_props11;
            g_phy_props11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
            g_phy_props11.pNext = nullptr;

            g_phy_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            g_phy_feats.pNext = &g_phy_feats11;
            g_phy_feats11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
            g_phy_feats11.pNext = &g_phy_feats12;
            g_phy_feats12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            g_phy_feats12.pNext = nullptr;

            bool found = false; // Suitable device found?

            for(auto it = devs.begin(), end = devs.end(); it != end; ++it) {
                vkGetPhysicalDeviceProperties2(*it, &g_phy_props);
                vkGetPhysicalDeviceFeatures2(*it, &g_phy_feats);

                if(g_phy_props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                    g_phy_device = *it;
                    found = true;

                    console.info("Chosen physical device " + s(g_phy_props.properties.deviceName));
                    break;
                    // TODO: Log available physical devices and improve selection algorithm
                }
            }

            jltassert2(found, "No suitable physical device found");

            vkGetPhysicalDeviceMemoryProperties(g_phy_device, &g_phy_mem_props);
        }

        static queue_ci_vector select_device_queues() {
            queue_ci_vector result;
            uint32_t n_families;

            vkGetPhysicalDeviceQueueFamilyProperties(g_phy_device, &n_families, nullptr);
            collections::Array<VkQueueFamilyProperties> fam_props{n_families};
            vkGetPhysicalDeviceQueueFamilyProperties(g_phy_device, &n_families, fam_props);

            bool found_graphics = false, found_transfer = false;

            for(uint32_t i = 0; i < n_families; ++i) {
                VkQueueFamilyProperties *const p = fam_props + i;

                if(!found_graphics && p->queueCount > 0) {
                    if(p->queueFlags &= VK_QUEUE_GRAPHICS_BIT) {
                        --p->queueCount;
                        g_q_graphics_fam_index = i;
                        found_graphics = true;
                    }
                }

                if(!found_transfer && p->queueCount > 0) {
                    if(p->queueFlags &= VK_QUEUE_TRANSFER_BIT) {
                        --p->queueCount;
                        g_q_transfer_fam_index = i;
                        found_transfer = true;
                    }
                }
            }

            jltassert2(found_graphics, "Unable to find a suitable graphics queue");
            jltassert2(found_transfer, "Unable to find a suitable transfer queue");

            g_q_graphics_index = 0;
            g_q_transfer_index = g_q_graphics_fam_index == g_q_transfer_fam_index ? 1 : 0;

            result.push({
              // Graphics queue
              VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
              nullptr,                                    // pNext
              0,                                          // flags
              g_q_graphics_fam_index,                     // queueFamilyIndex
              static_cast<uint32_t>(
                g_q_graphics_fam_index == g_q_transfer_fam_index ? 2 : 1), // queueCount
              g_q_priorities                                               // pQueuePriorities
            });

            if(g_q_graphics_fam_index != g_q_transfer_fam_index) {
                result.push({
                  // Transfer queue
                  VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
                  nullptr,                                    // pNext
                  0,                                          // flags
                  g_q_transfer_fam_index,                     // queueFamilyIndex
                  1,                                          // queueCount
                  g_q_priorities + 1                          // pQueuePriorities
                });
            }

            return result;
        }

        static void initialize_device() {
            console.debug("Creating device");

            VkResult result;

            VkPhysicalDeviceVulkan12Features features12 = {};

            features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            features12.separateDepthStencilLayouts = VK_TRUE;

            VkPhysicalDeviceFeatures2 features{
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, // sType
              &features12,                                  // pNext
              {0}};

            // TODO: These features must be checked for in the physical device selection algorithm
            features.features.logicOp = VK_TRUE;
            features.features.fillModeNonSolid = VK_TRUE;
            features.features.wideLines = VK_TRUE;
            features.features.alphaToOne = VK_TRUE;
            features.features.multiViewport = VK_TRUE;

            queue_ci_vector q_cinfo = select_device_queues();
            extension_vector exts = select_required_device_extensions();

            VkDeviceCreateInfo cinfo{
              VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, // sType
              &features,                            // pNext
              0,                                    // flags
              q_cinfo.get_length(),                 // queueCreateInfoCount
              &q_cinfo[0],                          // pQueueCreateInfos
              0,                                    // enabledLayerCount
              nullptr,                              // ppEnabledLayerNames
              exts.get_length(),                    // enabledExtensionCount
              &exts[0],                             // ppEnabledExtensionNames
              nullptr                               // pEnabledFeatures
            };

            result = vkCreateDevice(g_phy_device, &cinfo, g_allocator, &g_device);

            switch(result) {
            case VK_ERROR_DEVICE_LOST: console.err("Physical device lost"); abort();

            case VK_ERROR_FEATURE_NOT_PRESENT:
                console.err("Unable to create device - required feature not present");
                abort();

            default: jltassert2(result == VK_SUCCESS, "Unable to create device"); break;
            }

            vkGetDeviceQueue(g_device, g_q_graphics_fam_index, g_q_graphics_index, &g_q_graphics);
            vkGetDeviceQueue(g_device, g_q_transfer_fam_index, g_q_transfer_index, &g_q_transfer);
        }

        static void shutdown_device() {
            console.debug("Destroying device");

            vkDestroyDevice(g_device, g_allocator);
        }

        static void populate_device_image_metadata() {
            VkResult result = vkGetPhysicalDeviceImageFormatProperties(
              g_phy_device,
              VK_FORMAT_B8G8R8A8_UNORM,
              VK_IMAGE_TYPE_2D,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
              VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT,
              &g_phy_dev_image_fmt_props);

            switch(result) {
            case VK_SUCCESS: break;

            case VK_ERROR_FORMAT_NOT_SUPPORTED: console.err("Image format not supported"); abort();

            default: console.err("Out of memory while querying for image format support"); abort();
            }

            result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
              g_phy_device, g_win_surface, &g_win_surface_caps);

            jltassert2(result == VK_SUCCESS, "Unable to get image capabilities");
        }

        static void initialize_window_surface(ui::Window &window) {
            console.debug("Creating window surface");

            VkResult result;
            VkBool32 surface_support;
            VkWin32SurfaceCreateInfoKHR cinfo{
              VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, // sType
              nullptr,                                         // pNext
              0,                                               // flags
              ui::get_hinstance(),                             // hinstance
              window.get_handle()                              // hwnd
            };

            result = vkCreateWin32SurfaceKHR(g_instance, &cinfo, g_allocator, &g_win_surface);

            jltassert2(result == VK_SUCCESS, "Unable to create window surface");

            vkGetPhysicalDeviceSurfaceSupportKHR(
              g_phy_device, g_q_graphics_fam_index, g_win_surface, &surface_support);

            jltassert2(result == VK_SUCCESS, "Unable to query for surface support");
            jltassert2(surface_support == VK_TRUE, "Current device doesn't support window");
        }

        static void shutdown_window_surface() {
            console.debug("Destroying window surface");

            vkDestroySurfaceKHR(g_instance, g_win_surface, g_allocator);
        }

        static void initialize_swapchain() {
            console.debug("Creating swapchain");

            VkResult result;
            uint32_t n_fmts;
            uint32_t n_present_modes;

            result =
              vkGetPhysicalDeviceSurfaceFormatsKHR(g_phy_device, g_win_surface, &n_fmts, nullptr);
            jltassert2(result == VK_SUCCESS, "Unable to get available device surface formats");

            collections::Array<VkSurfaceFormatKHR> fmts{n_fmts};
            result =
              vkGetPhysicalDeviceSurfaceFormatsKHR(g_phy_device, g_win_surface, &n_fmts, fmts);
            jltassert2(result == VK_SUCCESS, "Unable to get available device surface formats");

            result = vkGetPhysicalDeviceSurfacePresentModesKHR(
              g_phy_device, g_win_surface, &n_present_modes, nullptr);
            jltassert2(
              result == VK_SUCCESS, "Unable to get available device surface presentation formats");

            collections::Array<VkPresentModeKHR> present_modes{n_present_modes};
            result = vkGetPhysicalDeviceSurfacePresentModesKHR(
              g_phy_device, g_win_surface, &n_present_modes, present_modes);
            jltassert2(
              result == VK_SUCCESS, "Unable to get available device surface presentation formats");

            bool present_mode_fifo_supported = false, present_mode_mailbox_supported = false;
            VkPresentModeKHR chosen_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

            for(VkPresentModeKHR pm : present_modes) {
                if(pm == VK_PRESENT_MODE_MAILBOX_KHR) {
                    present_mode_mailbox_supported = true;
                }

                if(pm == VK_PRESENT_MODE_FIFO_KHR) {
                    present_mode_fifo_supported = true;
                }
            }

            if(present_mode_mailbox_supported) {
                chosen_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            } else if(present_mode_fifo_supported) {
                chosen_present_mode = VK_PRESENT_MODE_FIFO_KHR;
            }

            g_win_surface_fmt = fmts[0].format;

            VkSwapchainCreateInfoKHR cinfo{
              VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, // sType
              nullptr,                                     // pNext
              0,                                           // flags
              g_win_surface,                               // surface
              N_SWAPCHAIN_IMAGES,                          // minImageCount
              fmts[0].format,                              // imageFormat
              fmts[0].colorSpace,                          // imageColorSpace
              g_win_surface_caps.currentExtent,            // imageExtent
              1,                                           // imageArrayLayers
              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,         // imageUsage
              VK_SHARING_MODE_EXCLUSIVE,                   // imageSharingMode
              1,                                           // queueFamilyIndexCount
              &g_q_graphics_fam_index,                     // pQueueFamilyIndices
              g_win_surface_caps.currentTransform,         // preTransform
              VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,           // compositeAlpha
              chosen_present_mode,                         // presentMode
              VK_TRUE,                                     // clipped
              VK_NULL_HANDLE,                              // oldSwapchain
            };

            result = vkCreateSwapchainKHR(g_device, &cinfo, g_allocator, &g_swapchain);
            jltassert2(result == VK_SUCCESS, "Unable to create swapchain");

            // Get images
            uint32_t n_images;

            result = vkGetSwapchainImagesKHR(g_device, g_swapchain, &n_images, nullptr);
            jltassert2(result == VK_SUCCESS, "Unable to get swapchain images");

            g_swapchain_images =
              memory::allocate<typeof(*g_swapchain_images)>(1, memory::ALLOC_PERSIST);
            g_swapchain_image_views =
              memory::allocate<typeof(*g_swapchain_image_views)>(1, memory::ALLOC_PERSIST);

            jltassert(g_swapchain_images);
            memory::construct(g_swapchain_images, n_images);
            memory::construct(g_swapchain_image_views, n_images);

            result = vkGetSwapchainImagesKHR(g_device, g_swapchain, &n_images, *g_swapchain_images);
            jltassert2(result == VK_SUCCESS, "Unable to get swapchain images");

            { // Create image
                for(size_t i = 0; i < g_swapchain_images->get_length(); ++i) {
                    VkImageViewCreateInfo cinfo{
                      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
                      nullptr,                                  // pNext
                      0,                                        // flags
                      (*g_swapchain_images)[i],                 // image
                      VK_IMAGE_VIEW_TYPE_2D,                    // viewType
                      fmts[0].format,                           // format
                      {
                        // components
                        VK_COMPONENT_SWIZZLE_IDENTITY, // r
                        VK_COMPONENT_SWIZZLE_IDENTITY, // g
                        VK_COMPONENT_SWIZZLE_IDENTITY, // b
                        VK_COMPONENT_SWIZZLE_IDENTITY  // a
                      },
                      {
                        // subresourceRange
                        VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
                        0,                         // baseMipLevel
                        1,                         // levelCount
                        0,                         // baseArrayLayer
                        1,                         // layerCount
                      }};

                    result = vkCreateImageView(
                      g_device, &cinfo, g_allocator, *g_swapchain_image_views + i);

                    jltassert2(result == VK_SUCCESS, "Unable to create swapchain image views");
                }
            }
        }

        static void shutdown_swapchain() {
            console.debug("Destroying swapchain");

            for(VkImageView vw : *g_swapchain_image_views) {
                vkDestroyImageView(g_device, vw, g_allocator);
            }

            memory::free(g_swapchain_image_views);
            memory::free(g_swapchain_images);

            vkDestroySwapchainKHR(g_device, g_swapchain, g_allocator);
        }

        static void select_depth_stencil_image_format() {
            VkFormat allowed_fmts[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM};
            constexpr const size_t allowed_fmts_len = sizeof(allowed_fmts) / sizeof(VkFormat);

            for(size_t i = 0; i < allowed_fmts_len; ++i) {
                VkFormatProperties props;

                vkGetPhysicalDeviceFormatProperties(g_phy_device, allowed_fmts[i], &props);

                if(props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                    g_ds_image_fmt = allowed_fmts[i];
                    return;
                }
            }

            console.err("Unable to find a suitable format for depth/stencil buffer");
            abort();
        }

        static void initialize_depth_stencil_buffer() {
            console.debug("Initializing depth/stencil buffer");

            VkResult result;

            select_depth_stencil_image_format();

            VkImageCreateInfo cinfo{
              VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, // stype
              nullptr,                             // pNext
              0,                                   // flags
              VK_IMAGE_TYPE_2D,                    // imageType
              g_ds_image_fmt,                      // format
              {
                // extent
                g_win_surface_caps.currentExtent.width,  // width
                g_win_surface_caps.currentExtent.height, // height
                1                                        // depth
              },
              1,                                           // mipLevels
              1,                                           // arrayLayers
              VK_SAMPLE_COUNT_1_BIT,                       // samples
              VK_IMAGE_TILING_OPTIMAL,                     // tiling
              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, // usage
              VK_SHARING_MODE_EXCLUSIVE,                   // sharingMode
              0,                                           // queueFamilyIndexCount
              nullptr,                                     // pQueueFamilyIndices
              VK_IMAGE_LAYOUT_UNDEFINED                    // initialLayout
            };

            result = vkCreateImage(g_device, &cinfo, g_allocator, &g_ds_image);
            jltassert2(result == VK_SUCCESS, "Unable to create image for depth/stencil buffer");

            { // Image storage
                constexpr uint32_t mti_invalid = std::numeric_limits<uint32_t>::max();
                uint32_t mti = mti_invalid; // Memory Type Index
                VkMemoryRequirements reqs;

                vkGetImageMemoryRequirements(g_device, g_ds_image, &reqs);

                // Check if requirements are satisfied both for device & image
                for(uint32_t i = 0; i < g_phy_mem_props.memoryTypeCount; ++i) {
                    if(
                      (g_phy_mem_props.memoryTypes[i].propertyFlags
                       & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                      && (reqs.memoryTypeBits & (1 << i))) {
                        mti = i;
                        break;
                    }
                }

                jltassert2(
                  mti != mti_invalid,
                  "Required image memory type for depth/stencil buffer unavailable");

                VkMemoryAllocateInfo cinfo{
                  VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // sType
                  nullptr,                                // pNext
                  reqs.size,                              // allocationSize
                  mti                                     // memoryTypeIndex
                };

                result = vkAllocateMemory(g_device, &cinfo, g_allocator, &g_ds_image_memory);
                jltassert2(
                  result == VK_SUCCESS, "Unable to allocate storage for depth/stencil buffer");
            }

            result = vkBindImageMemory(g_device, g_ds_image, g_ds_image_memory, 0);
            jltassert2(
              result == VK_SUCCESS, "Unable to bind image memory for depth/stencil buffer");

            { // Image view
                VkImageViewCreateInfo cinfo{
                  VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
                  nullptr,                                  // pNext
                  0,                                        // flags
                  g_ds_image,                               // image
                  VK_IMAGE_VIEW_TYPE_2D,                    // viewType
                  g_ds_image_fmt,                           // format
                  {
                    // components
                    VK_COMPONENT_SWIZZLE_IDENTITY, // r
                    VK_COMPONENT_SWIZZLE_IDENTITY, // g
                    VK_COMPONENT_SWIZZLE_IDENTITY, // b
                    VK_COMPONENT_SWIZZLE_IDENTITY  // a
                  },
                  {
                    // subresourceRange
                    VK_IMAGE_ASPECT_DEPTH_BIT, // aspectMask
                    0,                         // baseMipLevel
                    1,                         // levelCount
                    0,                         // baseArrayLayer
                    1,                         // layerCount
                  }};

                result = vkCreateImageView(g_device, &cinfo, g_allocator, &g_ds_image_view);
                jltassert2(result == VK_SUCCESS, "Unable to create view for depth/stencil buffer");
            }
        }

        static void shutdown_depth_stencil_buffer() {
            console.debug("Destroying depth/stencil buffer");

            vkDestroyImageView(g_device, g_ds_image_view, g_allocator);
            vkFreeMemory(g_device, g_ds_image_memory, g_allocator);
            vkDestroyImage(g_device, g_ds_image, g_allocator);
        }

        static void initialize_render_pass() {
            console.debug("Creating render pass");

            VkResult result;

            VkAttachmentDescription attachments[] = {
              {
                // Color
                0,                                // flags
                g_win_surface_fmt,                // format
                VK_SAMPLE_COUNT_1_BIT,            // samples
                VK_ATTACHMENT_LOAD_OP_CLEAR,      // loadOp
                VK_ATTACHMENT_STORE_OP_DONT_CARE, // storeOp
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // stencilLoadOp
                VK_ATTACHMENT_STORE_OP_DONT_CARE, // stencilStoreOp
                VK_IMAGE_LAYOUT_UNDEFINED,        // initialLayout
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR   // finalLayout
              },
              {
                // Depth
                0,                                       // flags
                g_ds_image_fmt,                          // format
                VK_SAMPLE_COUNT_1_BIT,                   // samples
                VK_ATTACHMENT_LOAD_OP_CLEAR,             // loadOp
                VK_ATTACHMENT_STORE_OP_DONT_CARE,        // storeOp
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,         // stencilLoadOp
                VK_ATTACHMENT_STORE_OP_DONT_CARE,        // stencilStoreOp
                VK_IMAGE_LAYOUT_UNDEFINED,               // initialLayout
                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL // finalLayout
              }};
            constexpr const uint32_t n_attachments =
              sizeof(attachments) / sizeof(VkAttachmentDescription);

            VkAttachmentReference sp_color_att_ref = {
              0,                                       // attachment
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // layout
            };

            VkAttachmentReference sp_ds_att_ref = {
              1,                                       // attachment
              VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL // layout
            };

            VkSubpassDescription subpasses[] = {{
              0,                               // flags
              VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
              0,                               // inputAttachmentCount
              nullptr,                         // pInputAttachments
              1,                               // colorAttachmentCount
              &sp_color_att_ref,               // pColorAttachments
              nullptr,                         // pResolveAttachments
              &sp_ds_att_ref,                  // pDepthStencilAttachment
              0,                               // preserveAttachmentCount
              nullptr,                         // pPreserveAttachments
            }};
            constexpr const uint32_t n_subpasses = sizeof(subpasses) / sizeof(VkSubpassDescription);

            VkRenderPassCreateInfo cinfo{
              VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, // sType
              nullptr,                                   // pNext
              0,                                         // flags
              n_attachments,                             // attachmentCount
              attachments,                               // pAttachments
              n_subpasses,                               // subpassCount
              subpasses,                                 // pSubpasses
              0,                                         // dependencyCount
              nullptr                                    // pDependencies
            };

            result = vkCreateRenderPass(g_device, &cinfo, g_allocator, &g_render_pass);
            jltassert2(result == VK_SUCCESS, "Unable to create render pass");

            console.debug("Creating framebuffer");

            g_framebuffers = memory::allocate_and_construct<typeof(*g_framebuffers)>(
              g_swapchain_image_views->get_length());

            for(size_t i = 0; i < g_swapchain_image_views->get_length(); ++i) {
                VkImageView views[] = {(*g_swapchain_image_views)[i], g_ds_image_view};
                constexpr const size_t n_views = sizeof(views) / sizeof(VkImageView);

                VkFramebufferCreateInfo cinfo{
                  VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // sType
                  nullptr,                                   // pNext
                  0,                                         // flags
                  g_render_pass,                             // renderPass
                  n_views,                                   // attachmentCount
                  views,                                     // pAttachments
                  g_win_surface_caps.currentExtent.width,    // width
                  g_win_surface_caps.currentExtent.height,   // height
                  1                                          // layers
                };

                result = vkCreateFramebuffer(g_device, &cinfo, g_allocator, &(*g_framebuffers)[i]);
                jltassert2(result == VK_SUCCESS, "Unable to create framebuffer");
            }
        }

        static void shutdown_render_pass() {
            console.debug("Destroying framebuffer");

            for(auto fb : *g_framebuffers) { vkDestroyFramebuffer(g_device, fb, g_allocator); }

            memory::free(g_framebuffers);

            console.debug("Destroying render pass");
            vkDestroyRenderPass(g_device, g_render_pass, g_allocator);
        }

        /**
         * To be called at initialization time or when the logical device is reported as lost.
         */
        static void reset_device() {
            initialize_device();
            initialize_debug_logger();
            initialize_swapchain();
            initialize_depth_stencil_buffer();
            initialize_render_pass();
        }

        void initialize(GraphicsEngineInitializationParams &params) {
            initialize_instance(params);
            select_physical_device();

            if(params.wnd) {
                initialize_window_surface(*params.wnd);
                g_use_window = true;
            } else {
                g_use_window = false;
            }

            populate_device_image_metadata();
            reset_device();
        }

        void shutdown() {
            shutdown_render_pass();
            shutdown_depth_stencil_buffer();
            shutdown_swapchain();
            shutdown_debug_logger();
            shutdown_device();

            if(g_use_window) {
                shutdown_window_surface();
            }

            shutdown_instance();
        }
    } // namespace graphics
} // namespace jolt
