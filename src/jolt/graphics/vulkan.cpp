#include <cstdlib>
#include <cstring>
#include <vulkan/vulkan.h>
#include <jolt/version.hpp>
#include <jolt/debug.hpp>
#include <jolt/memory/allocator.hpp>
#include <jolt/collections/vector.hpp>
#include <jolt/collections/valueset.hpp>
#include <jolt/collections/array.hpp>
#include "vulkan.hpp"

static VkAllocationCallbacks *g_allocator = nullptr;
static VkInstance g_instance;
static VkPhysicalDevice g_phy_device;
static VkDevice g_device;
static VkPhysicalDeviceProperties2 g_phy_props;
static VkPhysicalDeviceVulkan11Properties g_phy_props11;
static VkPhysicalDeviceFeatures2 g_phy_feats;
static VkPhysicalDeviceVulkan11Features g_phy_feats11;
static VkPhysicalDeviceVulkan12Features g_phy_feats12;

static uint32_t g_q_graphics_fam_index, g_q_transfer_fam_index;
static uint32_t g_q_graphics_index, g_q_transfer_index;
static VkQueue g_q_graphics, g_q_transfer;
static const float g_q_priorities[2] = {1.0f /* graphics */, 1.0f, /* transfer */};

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

        static extension_vector select_required_extensions() {
            extension_vector extensions; // Layers that will be returned
            uint32_t n_available_ext;
            const char *required_ext[] = {
              "VK_KHR_surface",
#ifdef _WIN32
              "VK_KHR_win32_surface",
#endif // _WIN32

#ifdef _DEBUG
              "VK_EXT_debug_utils",
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
            extension_vector extensions = select_required_extensions();

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

            VkPhysicalDeviceFeatures2 features{
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, // sType
              nullptr,                                      // pNext
              {0}};

            // TODO: These features must be checked for in the physical device selection algorithm
            features.features.logicOp = VK_TRUE;
            features.features.fillModeNonSolid = VK_TRUE;
            features.features.wideLines = VK_TRUE;
            features.features.alphaToOne = VK_TRUE;
            features.features.multiViewport = VK_TRUE;

            queue_ci_vector q_cinfo = select_device_queues();

            VkDeviceCreateInfo cinfo{
              VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, // sType
              &features,                            // pNext
              0,                                    // flags
              q_cinfo.get_length(),                 // queueCreateInfoCount
              &q_cinfo[0],                          // pQueueCreateInfos
              0,                                    // enabledLayerCount
              nullptr,                              // ppEnabledLayerNames
              0,                                    // enabledExtensionCount
              nullptr,                              // ppEnabledExtensionNames
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

        /**
         * To be called at initialization time or when the logical device is reported as lost.
         */
        static void reset_device() {
            initialize_device();
            initialize_debug_logger();
        }

        void initialize(GraphicsEngineInitializationParams &params) {
            initialize_instance(params);
            select_physical_device();
            reset_device();
        }

        void shutdown() {
            shutdown_debug_logger();
            shutdown_device();
            shutdown_instance();
        }
    } // namespace graphics
} // namespace jolt
