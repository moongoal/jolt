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

using namespace jolt::text;

namespace jolt {
    namespace graphics {
        using layer_vector = collections::Vector<const char *>;
        using extension_vector = layer_vector;

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

        void initialize(GraphicsEngineInitializationParams &params) {
            console.debug("Initializing Vulkan");
            initialize_instance(params);
        }

        void shutdown() {
            console.debug("Destroying Vulkan instance");
            vkDestroyInstance(g_instance, g_allocator);
        }
    } // namespace graphics
} // namespace jolt
