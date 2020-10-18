#define JLT_GRAPHICS_VULKAN_RENDERER_CPP
#include <cstdlib>
#include <cstring>
#include <jolt/version.hpp>
#include <jolt/debug.hpp>
#include <jolt/memory/allocator.hpp>
#include <jolt/text/stringbuilder.hpp>
#include <jolt/graphics/vulkan.hpp>
#include <jolt/collections/hashmap.hpp>

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
  JLT_MAYBE_UNUSED VkDebugUtilsMessageTypeFlagsEXT messageTypes,
  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
  JLT_MAYBE_UNUSED void *pUserData) {
    switch(messageSeverity) {
    #define log(type)                                                                                        \
        jolt::console.type(s(pCallbackData->pMessageIdName) + " - " + s(pCallbackData->pMessage))

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            log(info);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            log(info);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            log(info);
            break;

        default:
            log(debug);
            break;

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
        VkPhysicalDeviceProperties2 props{};

        props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

        vkGetPhysicalDeviceProperties2(*it, &props);

        sb.add(" - ");
        sb.add(props.properties.deviceName);
        sb.add(" (");

        switch(props.properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                sb.add("discrete GPU");
                break;

            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                sb.add("integrated GPU");
                break;

            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                sb.add("virtual GPU");
                break;

            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                sb.add("CPU");
                break;

            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                sb.add("other");
                break;

            default:
                sb.add("unknown");
                break;
        }

        sb.add(")");

        jolt::console.info(sb);
        sb.reset();
    }
}

namespace jolt {
    namespace graphics {
        namespace vulkan {
            Renderer::layer_vector Renderer::select_required_layers() {
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

            Renderer::extension_vector Renderer::select_required_instance_extensions() {
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

            Renderer::extension_vector Renderer::select_required_device_extensions() {
                extension_vector extensions; // Device extensions that will be returned
                uint32_t n_available_ext;
                const char *required_ext[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
                const size_t required_ext_length = sizeof(required_ext) / sizeof(const char *);

                vkEnumerateDeviceExtensionProperties(m_phy_device, nullptr, &n_available_ext, nullptr);
                collections::Array<VkExtensionProperties> ext_props{n_available_ext};
                vkEnumerateDeviceExtensionProperties(m_phy_device, nullptr, &n_available_ext, ext_props);

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

            void Renderer::initialize_instance(GraphicsEngineInitializationParams const &params) {
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
                  VK_MAKE_VERSION(JLT_VERSION_MAJOR, JLT_VERSION_MINOR, JLT_VERSION_PATCH), // engineVersion
                  VK_API_VERSION_1_2                                                        // apiVersion
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

                jltassert2(result == VK_SUCCESS, "Unable to create Vulkan instance");
            }

            void Renderer::select_physical_device() {
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
                m_phy_props11.pNext = &m_phy_maint_3_props;
                m_phy_maint_3_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
                m_phy_maint_3_props.pNext = nullptr;

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

            void Renderer::initialize_device(GraphicsEngineInitializationParams const &params) {
                console.debug("Creating device");

                VkResult result;

                uint32_t n_families;

                vkGetPhysicalDeviceQueueFamilyProperties(m_phy_device, &n_families, nullptr);
                queue_fam_prop_array q_fam_props{n_families};
                vkGetPhysicalDeviceQueueFamilyProperties(m_phy_device, &n_families, q_fam_props);

                VkPhysicalDeviceVulkan12Features features12 = {};
                features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

                VkPhysicalDeviceFeatures2 features{};

                features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                features.pNext = &features12;
                features.features.logicOp = VK_TRUE;
                features.features.fillModeNonSolid = VK_TRUE;
                features.features.wideLines = VK_TRUE;
                features.features.alphaToOne = VK_TRUE;

                features12.separateDepthStencilLayouts = VK_TRUE;

                queue_ci_vector q_cinfo = select_device_queues(
                  q_fam_props, params.n_queues_graphics, params.n_queues_transfer, params.n_queues_compute);

                extension_vector exts = select_required_device_extensions();

                // Fill-in the priority values
                uint32_t max_queues = 0;

                for(auto &info : q_cinfo) {
                    if(info.queueCount > max_queues) {
                        max_queues = info.queueCount;
                    }
                }

                collections::Array<float> q_priorities{max_queues};
                q_priorities.fill(1.0f);

                for(auto &info : q_cinfo) { info.pQueuePriorities = q_priorities; }

                // Create the device & queues
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
                    case VK_ERROR_DEVICE_LOST:
                        console.err("Physical device lost");
                        abort();

                    case VK_ERROR_FEATURE_NOT_PRESENT:
                        console.err("Unable to create device - required feature not present");
                        abort();

                    default:
                        jltassert2(result == VK_SUCCESS, "Unable to create device");
                        break;
                }

                // Get the created queues
                uint32_t n_total_queues = 0;
                uint32_t q = 0;

                for(auto &info : q_cinfo) { n_total_queues += info.queueCount; }

                m_queues = jltnew(queue_info_array, n_total_queues);

                for(auto &q : *m_queues) { jltconstruct(&q.lock, 0); }

                for(auto &info : q_cinfo) {
                    for(uint32_t i = 0; i < info.queueCount; ++i) {
                        VkQueue queue;

                        vkGetDeviceQueue(m_device, info.queueFamilyIndex, i, &queue);

                        (*m_queues)[q].value = {
                          queue,                                         // queue
                          q_fam_props[info.queueFamilyIndex].queueFlags, // flags
                          info.queueFamilyIndex                          // queue_family_index
                        };
                    }
                }
            }

            uint32_t Renderer::get_queue_family_index(VkQueue const queue) const {
                for(auto const &qinfo : *m_queues) {
                    if(qinfo.value.queue == queue) {
                        return qinfo.value.queue_family_index;
                    }
                }

                return JLT_VULKAN_INVALID32;
            }

            void Renderer::initialize_debug_logger() {
#ifdef _DEBUG
                console.debug("Initializing Vulkan debug logger");

                pfnCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                  vkGetDeviceProcAddr(m_device, "vkCreateDebugUtilsMessengerEXT"));
                pfnDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
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
                  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
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

            int Renderer::select_single_queue(
              Renderer::queue_fam_prop_array &fam_props, VkQueueFlags const requirements, bool const exact) {
                for(size_t i = 0; i < fam_props.get_length(); ++i) {
                    VkQueueFamilyProperties *const p = fam_props + i;

                    if(p->queueCount > 0) {
                        if(
                          (exact && (p->queueFlags == requirements))
                          || (!exact && (p->queueFlags & requirements)) == requirements) {
                            --p->queueCount;
                            return i;
                        }
                    }
                }

                return -1;
            }

            Renderer::queue_ci_vector Renderer::select_device_queues(
              queue_fam_prop_array &fam_props,
              uint32_t const n_graph,
              uint32_t const n_trans,
              uint32_t const n_compute) {
                queue_ci_vector result;

                using fam_count_map = collections::HashMap<uint32_t, uint32_t, hash::Identity<uint32_t>>;

                fam_count_map fam_counts;
                uint32_t n_graph2 = n_graph;
                uint32_t n_trans2 = n_trans;
                uint32_t n_compute2 = n_compute;

#define selqueue(n, req, exact)                                                                              \
    do {                                                                                                     \
        int fam_idx = 0;                                                                                     \
        do {                                                                                                 \
            fam_idx = select_single_queue(fam_props, req, exact);                                            \
            if(fam_idx != -1) {                                                                              \
                fam_counts.set_value(                                                                        \
                  static_cast<uint32_t>(fam_idx),                                                            \
                  fam_counts.get_value_with_default(static_cast<uint32_t>(fam_idx), 0) + 1);                 \
                n--;                                                                                         \
            }                                                                                                \
        } while(n > 0 && fam_idx != -1);                                                                     \
    } while(false)

                jltassert2(n_graph, "At least one graphics queue is required");

                // Try selecting queues from families that have the exact requirements.
                selqueue(n_graph2, VK_QUEUE_GRAPHICS_BIT, true);

                if(n_trans) {
                    selqueue(n_trans2, VK_QUEUE_TRANSFER_BIT, true);
                }

                if(n_compute) {
                    selqueue(n_compute2, VK_QUEUE_COMPUTE_BIT, true);
                }

                // If any one of such families exist or if not enough queues were provided by those,
                // accept queues from any family satisfying the requirements.
                //
                // Ordering here is different. In the first scenario ordering doesn't matter, while
                // here we need to prioritise requirements. Transfer queues can be last as any
                // graphics or compute queue can transfer data, while compute queues need to be
                // first as their availability may be more limited than that of graphics queue.
                if(n_compute2) {
                    selqueue(n_compute2, VK_QUEUE_COMPUTE_BIT, false);
                }

                if(n_graph2) {
                    selqueue(n_graph2, VK_QUEUE_GRAPHICS_BIT, false);
                }

                if(n_trans2) {
                    selqueue(n_trans2, VK_QUEUE_TRANSFER_BIT, false);
                }
#undef selqueue

                bool const found_graphics = n_graph != n_graph2;

                jltassert2(found_graphics, "Unable to find a suitable graphics queue");

                for(auto const &[fam_index, queue_count] : fam_counts) {
                    result.push({
                      // Graphics queue
                      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
                      nullptr,                                    // pNext
                      0,                                          // flags
                      fam_index,                                  // queueFamilyIndex
                      queue_count,                                // queueCount
                      nullptr                                     // pQueuePriorities
                    });
                }

                return result;
            }

            void Renderer::reset(GraphicsEngineInitializationParams const &params) {
                shutdown_phase2();
                initialize_phase2(params);
            }

            void Renderer::initialize_phase2(GraphicsEngineInitializationParams const &params) {
                initialize_device(params);
                reset_lost_state();

                initialize_debug_logger();
            }

            void Renderer::initialize(GraphicsEngineInitializationParams const &params) {
                jltassert2(m_instance == VK_NULL_HANDLE, "Vulkan renderer already initialized");

                console.info("Initializing Vulkan renderer");

                initialize_instance(params);
                select_physical_device();
                initialize_phase2(params);
            }

            void Renderer::shutdown_phase2() {
                wait_queues_idle();

                m_presentation_target = nullptr;
                m_render_target = nullptr;
                m_window = nullptr;

                if(m_queues) {
                    jltfreearray(m_queues, m_queues->get_length());
                    m_queues = nullptr;
                }

#ifdef _DEBUG
                if(pfnCreateDebugUtilsMessengerEXT && pfnDestroyDebugUtilsMessengerEXT) {
                    console.debug("Destroying Vulkan debug logger");

                    pfnDestroyDebugUtilsMessengerEXT(m_instance, m_debug_clbk, g_allocator);
                }
#endif // _DEBUG

                console.debug("Destroying Vulkan device");
                vkDestroyDevice(m_device, g_allocator);
            }

            void Renderer::shutdown() {
                console.info("Shutting down Vulkan renderer");

                shutdown_phase2();

                console.debug("Destroying Vulkan instance");
                vkDestroyInstance(m_instance, g_allocator);
            }

            VkAllocationCallbacks JLTAPI *get_vulkan_allocator() { return g_allocator; }

            void Renderer::wait_queues_idle() const {
                VkResult result = vkDeviceWaitIdle(m_device);
                jltvkcheck(result, "Error while waiting for the device to be idle");
            }

            VkQueue Renderer::acquire_queue(VkQueueFlags const flags) const {
                for(auto &qinfo : *m_queues) {
                    if((qinfo.value.flags & flags) == flags) {
                        if(qinfo.lock.try_acquire()) {
                            return qinfo.value.queue;
                        }
                    }
                }

                return VK_NULL_HANDLE;
            }

            void Renderer::release_queue(VkQueue const queue) const {
                for(auto &qinfo : *m_queues) {
                    if(qinfo.value.queue == queue) {
                        qinfo.lock.release();
                        break;
                    }
                }
            }

            VkQueue Renderer::acquire_graphics_queue() const { return acquire_queue(VK_QUEUE_GRAPHICS_BIT); }

            VkQueue Renderer::acquire_transfer_queue() const {
                VkQueue queue = acquire_queue(VK_QUEUE_TRANSFER_BIT);

                if(queue == VK_NULL_HANDLE) {
                    queue = acquire_graphics_queue();
                }

                if(queue == VK_NULL_HANDLE) {
                    queue = acquire_compute_queue();
                }

                return queue;
            }

            VkQueue Renderer::acquire_compute_queue() const { return acquire_queue(VK_QUEUE_COMPUTE_BIT); }

            void Renderer::signal_lost(RendererLostState const state) const {
                if(state > m_lost) {
                    m_lost = state;
                }
            }

            void Renderer::reset_lost_state() { m_lost = RENDERER_NOT_LOST; }

            void
            check_vulkan_result(Renderer const &renderer, VkResult const result, text::String const &errmsg) {
                RendererLostState state = RENDERER_NOT_LOST;

                switch(result) {
                    case VK_SUCCESS:
                        return;

                    case VK_SUBOPTIMAL_KHR:
                        state = RENDERER_LOST_PRESENT;

                    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
                        console.warn("Exclusive mode lost");
                        break;

                    case VK_ERROR_OUT_OF_DATE_KHR:
                        console.warn("Extent out of date");
                        state = RENDERER_LOST_PRESENT;
                        break;

                    case VK_ERROR_SURFACE_LOST_KHR:
                        console.warn("Surface lost");
                        state = RENDERER_LOST_PRESENT;
                        break;

                    case VK_ERROR_DEVICE_LOST:
                        console.warn("Device lost");
                        state = RENDERER_LOST_DEVICE;
                        break;

                    default:
                        jolt::console.err(errmsg);
                        abort();
                }

                renderer.signal_lost(state);
            }

            uint32_t Renderer::get_memory_type_index(VkMemoryPropertyFlags const requirements) const {
                for(uint32_t mem_type_idx = 0; mem_type_idx < m_phy_mem_props.memoryTypeCount;
                    ++mem_type_idx) {
                    VkMemoryType const &mem_type = m_phy_mem_props.memoryTypes[mem_type_idx];

                    if(mem_type.propertyFlags == requirements) {
                        return mem_type_idx;
                    }
                }

                return JLT_VULKAN_INVALID32;
            }

            uint32_t Renderer::get_memory_type_index(
              VkMemoryPropertyFlags const requirements, VkMemoryPropertyFlags const exclusions) const {
                for(uint32_t mem_type_idx = 0; mem_type_idx < m_phy_mem_props.memoryTypeCount;
                    ++mem_type_idx) {
                    VkMemoryType const &mem_type = m_phy_mem_props.memoryTypes[mem_type_idx];

                    if(
                      (mem_type.propertyFlags & requirements) == requirements
                      && (mem_type.propertyFlags & exclusions) == 0) {
                        return mem_type_idx;
                    }
                }

                return JLT_VULKAN_INVALID32;
            }
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt
