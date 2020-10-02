#include <cstdlib>
#include <cstring>
#include <jolt/version.hpp>
#include <jolt/debug.hpp>
#include <jolt/memory/allocator.hpp>
#include <jolt/text/stringbuilder.hpp>
#include "vulkan.hpp"

#ifdef _WIN32
    #define OS_SPECIFIC_SURFACE_EXTENSION VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif // _WIN32

static VkAllocationCallbacks *g_allocator = nullptr;

bool m_use_window;

using namespace jolt::text;

#ifdef _DEBUG
static VkDebugUtilsMessengerEXT m_debug_clbk;

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

template<typename It>
static void log_phy_devs(It const &begin, It const &end) {
    // Output available devices to log
    jolt::console.info("Available physical devices:");
    StringBuilder sb{5};

    for(auto it = begin; it != end; ++it) {
        VkPhysicalDeviceProperties2 props{
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, // sType
          nullptr,                                        // pNext
          {0}                                             // properties
        };

        vkGetPhysicalDeviceProperties2(*it, &props);

        sb.add(" - ");
        sb.add(props.properties.deviceName);
        sb.add(" (");

        switch(props.properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: sb.add("discrete GPU"); break;

        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: sb.add("integrated GPU"); break;

        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: sb.add("virtual GPU"); break;

        case VK_PHYSICAL_DEVICE_TYPE_CPU: sb.add("CPU"); break;

        case VK_PHYSICAL_DEVICE_TYPE_OTHER: sb.add("other"); break;

        default: sb.add("unknown"); break;
        }

        sb.add(")");

        jolt::console.info(sb);
        sb.reset();
    }
}

namespace jolt {
    namespace graphics {
        VulkanRenderer::layer_vector VulkanRenderer::select_required_layers() {
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
                    console.debug("Found required layer " + s(required_layers[i]));
                } else {
                    console.err("Required layer " + s(required_layers[i]) + " not found");
                    abort();
                }
            }

            return layers;
        }

        VulkanRenderer::extension_vector VulkanRenderer::select_required_instance_extensions() {
            extension_vector extensions; // Extensions that will be returned
            uint32_t n_available_ext;
            const char *required_ext[] = {
              VK_KHR_SURFACE_EXTENSION_NAME,
              OS_SPECIFIC_SURFACE_EXTENSION,
#ifdef _DEBUG
              VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
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
                    console.debug("Found required extension " + s(required_ext[i]));
                } else {
                    console.err("Required extension " + s(required_ext[i]) + " not found");
                    abort();
                }
            }

            return extensions;
        }

        VulkanRenderer::extension_vector VulkanRenderer::select_required_device_extensions() {
            extension_vector extensions; // Device extensions that will be returned
            uint32_t n_available_ext;
            const char *required_ext[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
            const size_t required_ext_length = sizeof(required_ext) / sizeof(const char *);

            vkEnumerateDeviceExtensionProperties(m_phy_device, nullptr, &n_available_ext, nullptr);
            collections::Array<VkExtensionProperties> ext_props{n_available_ext};
            vkEnumerateDeviceExtensionProperties(
              m_phy_device, nullptr, &n_available_ext, ext_props);

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
                    console.debug("Found required device extension " + s(required_ext[i]));
                } else {
                    console.err("Required device extension " + s(required_ext[i]) + " not found");
                    abort();
                }
            }

            return extensions;
        }

        void VulkanRenderer::initialize_instance(GraphicsEngineInitializationParams const &params) {
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

            VkResult result = vkCreateInstance(&icf, g_allocator, &m_instance);

            jltassert(result == VK_SUCCESS);
        }

        void VulkanRenderer::select_physical_device() {
            console.debug("Selecting Vulkan physical device");

            VkResult result;
            uint32_t n_devs;

            result = vkEnumeratePhysicalDevices(m_instance, &n_devs, nullptr);
            jltassert2(result == VK_SUCCESS, "Unable to enumerate physical devices");

            collections::Array<VkPhysicalDevice> devs{n_devs};

            result = vkEnumeratePhysicalDevices(m_instance, &n_devs, devs);
            jltassert2(result == VK_SUCCESS, "Unable to enumerate physical devices");

            m_phy_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            m_phy_props.pNext = &m_phy_props11;
            m_phy_props11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
            m_phy_props11.pNext = nullptr;

            m_phy_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            m_phy_feats.pNext = &m_phy_feats11;
            m_phy_feats11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
            m_phy_feats11.pNext = &m_phy_feats12;
            m_phy_feats12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            m_phy_feats12.pNext = nullptr;

            log_phy_devs(devs.begin(), devs.end());

            bool found = false; // Suitable device found?

            for(auto it = devs.begin(), end = devs.end(); it != end; ++it) {
                vkGetPhysicalDeviceProperties2(*it, &m_phy_props);
                vkGetPhysicalDeviceFeatures2(*it, &m_phy_feats);

                if(m_phy_props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                    m_phy_device = *it;
                    found = true;

                    console.info("Chosen physical device " + s(m_phy_props.properties.deviceName));
                    break;
                }
            }

            jltassert2(found, "No suitable physical device found");

            vkGetPhysicalDeviceMemoryProperties(m_phy_device, &m_phy_mem_props);
        }

        void VulkanRenderer::initialize_device() {
            console.debug("Creating device");

            VkResult result;

            VkPhysicalDeviceVulkan12Features features12 = {};
            features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

            VkPhysicalDeviceFeatures2 features{
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, // sType
              &features12,                                  // pNext
              {0}};

            features.features.logicOp = VK_TRUE;
            features.features.fillModeNonSolid = VK_TRUE;
            features.features.wideLines = VK_TRUE;
            features.features.alphaToOne = VK_TRUE;

            features12.separateDepthStencilLayouts = VK_TRUE;

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

            result = vkCreateDevice(m_phy_device, &cinfo, g_allocator, &m_device);

            switch(result) {
            case VK_ERROR_DEVICE_LOST: console.err("Physical device lost"); abort();

            case VK_ERROR_FEATURE_NOT_PRESENT:
                console.err("Unable to create device - required feature not present");
                abort();

            default: jltassert2(result == VK_SUCCESS, "Unable to create device"); break;
            }

            vkGetDeviceQueue(m_device, m_q_graphics_fam_index, m_q_graphics_index, &m_q_graphics);
            vkGetDeviceQueue(m_device, m_q_transfer_fam_index, m_q_transfer_index, &m_q_transfer);
        }

        void VulkanRenderer::initialize_debug_logger() {
#ifdef _DEBUG
            console.debug("Initializing Vulkan debug logger");

            pfnCreateDebugUtilsMessengerEXT =
              reinterpret_cast<typeof(pfnCreateDebugUtilsMessengerEXT)>(
                vkGetDeviceProcAddr(m_device, "vkCreateDebugUtilsMessengerEXT"));
            pfnDestroyDebugUtilsMessengerEXT =
              reinterpret_cast<typeof(pfnDestroyDebugUtilsMessengerEXT)>(
                vkGetDeviceProcAddr(m_device, "vkDestroyDebugUtilsMessengerEXT"));

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
              pfnCreateDebugUtilsMessengerEXT(m_instance, &cinfo, g_allocator, &m_debug_clbk);

            if(!result) {
                console.err("Unable to initialize Vulkan debug logger");
                abort();
            }
#endif // _DEBUG
        }

        VulkanRenderer::queue_ci_vector VulkanRenderer::select_device_queues() {
            queue_ci_vector result;
            uint32_t n_families;

            vkGetPhysicalDeviceQueueFamilyProperties(m_phy_device, &n_families, nullptr);
            collections::Array<VkQueueFamilyProperties> fam_props{n_families};
            vkGetPhysicalDeviceQueueFamilyProperties(m_phy_device, &n_families, fam_props);

            bool found_graphics = false, found_transfer = false;

            for(uint32_t i = 0; i < n_families; ++i) {
                VkQueueFamilyProperties *const p = fam_props + i;

                if(!found_graphics && p->queueCount > 0) {
                    if((p->queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                        --p->queueCount;
                        m_q_graphics_fam_index = i;
                        found_graphics = true;
                    }
                }

                if(!found_transfer && p->queueCount > 0) {
                    if(
                      (p->queueFlags & VK_QUEUE_TRANSFER_BIT)
                      || (p->queueFlags & VK_QUEUE_GRAPHICS_BIT)
                      || (p->queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                        --p->queueCount;
                        m_q_transfer_fam_index = i;
                        found_transfer = true;
                    }
                }
            }

            jltassert2(found_graphics, "Unable to find a suitable graphics queue");
            jltassert2(found_transfer, "Unable to find a suitable transfer queue");

            m_q_graphics_index = 0;
            m_q_transfer_index = m_q_graphics_fam_index == m_q_transfer_fam_index ? 1 : 0;

            result.push({
              // Graphics queue
              VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
              nullptr,                                    // pNext
              0,                                          // flags
              m_q_graphics_fam_index,                     // queueFamilyIndex
              static_cast<uint32_t>(
                m_q_graphics_fam_index == m_q_transfer_fam_index ? 2 : 1), // queueCount
              s_q_priorities                                               // pQueuePriorities
            });

            if(m_q_graphics_fam_index != m_q_transfer_fam_index) {
                result.push({
                  // Transfer queue
                  VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
                  nullptr,                                    // pNext
                  0,                                          // flags
                  m_q_transfer_fam_index,                     // queueFamilyIndex
                  1,                                          // queueCount
                  s_q_priorities + 1                          // pQueuePriorities
                });
            }

            return result;
        }

        void VulkanRenderer::reset_device(GraphicsEngineInitializationParams const &params) {
            initialize_device();
            initialize_debug_logger();

            m_window = jltnew(VulkanWindow, *this, *params.wnd);
            m_presentation_target = jltnew(VulkanPresentationTarget, *this);
            m_render_target = jltnew(VulkanRenderTarget, *this);
        }

        void VulkanRenderer::initialize(GraphicsEngineInitializationParams const &params) {
            jltassert2(m_instance == VK_NULL_HANDLE, "Vulkan renderer already initialized");

            console.info("Initializing Vulkan renderer");

            initialize_instance(params);
            select_physical_device();
            reset_device(params);
        }

        void VulkanRenderer::shutdown() {
            console.info("Shutting down Vulkan renderer");

            wait_graphics_queue_idle();
            wait_transfer_queue_idle();

            if(m_render_target) {
                jltfree(m_render_target);
                m_render_target = nullptr;
            }

            if(m_presentation_target) {
                jltfree(m_presentation_target);
                m_presentation_target = nullptr;
            }

            if(m_window) {
                jltfree(m_window);
                m_window = nullptr;
            }

#ifdef _DEBUG
            if(pfnCreateDebugUtilsMessengerEXT && pfnDestroyDebugUtilsMessengerEXT) {
                console.debug("Destroying Vulkan debug logger");

                pfnDestroyDebugUtilsMessengerEXT(m_instance, m_debug_clbk, g_allocator);
            }
#endif // _DEBUG

            console.debug("Destroying Vulkan device");
            vkDestroyDevice(m_device, g_allocator);

            console.debug("Destroying Vulkan instance");
            vkDestroyInstance(m_instance, g_allocator);
        }

        VkAllocationCallbacks JLTAPI *get_vulkan_allocator() { return g_allocator; }

        VulkanCommandPool
        VulkanRenderer::create_graphics_command_pool(bool const transient, bool const allow_reset) {
            return VulkanCommandPool{*this, transient, allow_reset, m_q_graphics_fam_index};
        }

        VulkanCommandPool
        VulkanRenderer::create_transfer_command_pool(bool const transient, bool const allow_reset) {
            return VulkanCommandPool{*this, transient, allow_reset, m_q_transfer_fam_index};
        }

        void VulkanRenderer::wait_graphics_queue_idle() const {
            VkResult result = vkQueueWaitIdle(m_q_graphics);
            jltassert2(result == VK_SUCCESS, "Error while waiting for graphics queue to be idle");
        }

        void VulkanRenderer::wait_transfer_queue_idle() const {
            VkResult result = vkQueueWaitIdle(m_q_transfer);
            jltassert2(result == VK_SUCCESS, "Error while waiting for transfer queue to be idle");
        }
    } // namespace graphics
} // namespace jolt
