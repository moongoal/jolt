#ifndef JLT_GRAPHICS_VULKAN_RENDERER_HPP
#define JLT_GRAPHICS_VULKAN_RENDERER_HPP

#include "defs.hpp"

#include <jolt/ui/window.hpp>
#include <jolt/collections/vector.hpp>
#include <jolt/collections/array.hpp>
#include "window.hpp"
#include "render_tgt.hpp"
#include "presentation_tgt.hpp"
#include "cmd.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            struct GraphicsEngineInitializationParams {
                const char *app_name;
                unsigned short app_version_major, app_version_minor, app_version_revision;
                ui::Window *wnd;
            };

            class JLTAPI Renderer {
                using layer_vector = collections::Vector<const char *>;
                using extension_vector = layer_vector;
                using queue_ci_vector = collections::Vector<VkDeviceQueueCreateInfo>;

                static constexpr const float s_q_priorities[2] = {
                  1.0f, // graphics
                  1.0f, // transfer
                };

                VkInstance m_instance = VK_NULL_HANDLE;
                VkPhysicalDevice m_phy_device = VK_NULL_HANDLE;
                VkDevice m_device = VK_NULL_HANDLE;
                VkPhysicalDeviceProperties2 m_phy_props{};
                VkPhysicalDeviceVulkan11Properties m_phy_props11{};
                VkPhysicalDeviceMaintenance3Properties m_phy_maint_3_props{};
                VkPhysicalDeviceFeatures2 m_phy_feats{};
                VkPhysicalDeviceVulkan11Features m_phy_feats11{};
                VkPhysicalDeviceVulkan12Features m_phy_feats12{};
                VkPhysicalDeviceMemoryProperties m_phy_mem_props{};

                uint32_t m_q_graphics_fam_index =
                  std::numeric_limits<typeof(m_q_graphics_fam_index)>::max();
                uint32_t m_q_transfer_fam_index =
                  std::numeric_limits<typeof(m_q_transfer_fam_index)>::max();
                uint32_t m_q_graphics_index =
                  std::numeric_limits<typeof(m_q_graphics_index)>::max();
                uint32_t m_q_transfer_index =
                  std::numeric_limits<typeof(m_q_transfer_index)>::max();

                VkQueue m_q_graphics = VK_NULL_HANDLE;
                VkQueue m_q_transfer = VK_NULL_HANDLE;

                Window *m_window = nullptr;
                RenderTarget *m_render_target = nullptr;
                PresentationTarget *m_presentation_target = nullptr;

                mutable bool m_lost = false;

                void initialize_phase2(GraphicsEngineInitializationParams const &params);
                void initialize_instance(GraphicsEngineInitializationParams const &params);
                void select_physical_device();
                void initialize_device();
                void initialize_debug_logger();
                queue_ci_vector select_device_queues();
                layer_vector select_required_layers();
                extension_vector select_required_instance_extensions();
                extension_vector select_required_device_extensions();

                void shutdown_phase2();

              public:
                Renderer() = default;
                Renderer(Renderer const &other) = delete;

                VkInstance get_instance() const { return m_instance; }
                VkPhysicalDevice get_phy_device() const { return m_phy_device; }
                VkDevice get_device() const { return m_device; }
                
                const VkPhysicalDeviceFeatures2 &get_phy_device_features() const {
                    return m_phy_feats;
                }

                const VkPhysicalDeviceProperties2 &get_phy_device_properties() const {
                    return m_phy_props;
                }

                const VkPhysicalDeviceVulkan11Properties &get_phy_device_properties11() const {
                    return m_phy_props11;
                }

                const VkPhysicalDeviceVulkan11Features &get_phy_device_features11() const {
                    return m_phy_feats11;
                }

                const VkPhysicalDeviceVulkan12Features &get_phy_device_features12() const {
                    return m_phy_feats12;
                }

                const VkPhysicalDeviceMemoryProperties &get_phy_device_memory_properties() const {
                    return m_phy_mem_props;
                }

                VkDeviceSize get_max_alloc_size() const {
                    return m_phy_maint_3_props.maxMemoryAllocationSize;
                }

                uint32_t get_graphics_queue_family_index() const { return m_q_graphics_fam_index; }
                uint32_t get_transfer_queue_family_index() const { return m_q_transfer_fam_index; }
                uint32_t get_graphics_queue_index() const { return m_q_graphics_index; }
                uint32_t get_transfer_queue_index() const { return m_q_transfer_index; }

                VkQueue get_graphics_queue() const { return m_q_graphics; }
                VkQueue get_transfer_queue() const { return m_q_transfer; }

                Window *get_window() { return m_window; }
                Window const *get_window() const { return m_window; }
                RenderTarget *get_render_target() { return m_render_target; }
                RenderTarget const *get_render_target() const { return m_render_target; }
                PresentationTarget *get_presentation_target() { return m_presentation_target; }

                PresentationTarget const *get_presentation_target() const {
                    return m_presentation_target;
                }

                void initialize(GraphicsEngineInitializationParams const &params);
                void shutdown();

                /**
                 * Create a non-transient, resettable command pool suitable for operating the
                 * graphics queue.
                 */
                CommandPool create_graphics_command_pool(
                  bool const transient = false, bool const allow_reset = true);

                /**
                 * Create a non-transient, resettable command pool suitable for operating the
                 * transfer queue.
                 */
                CommandPool create_transfer_command_pool(
                  bool const transient = false, bool const allow_reset = true);

                void wait_graphics_queue_idle() const;
                void wait_transfer_queue_idle() const;

                /**
                 * Return a value stating whether the renderer is lost.
                 */
                bool is_lost() const { return m_lost; }

                /**
                 * Signal that the renderer is lost and needs to be reset.
                 */
                void signal_lost() const;

                /**
                 * To be called at initialization time or when the logical device is reported as
                 * lost.
                 */
                void reset(GraphicsEngineInitializationParams const &params);
            };

            VkAllocationCallbacks JLTAPI *get_vulkan_allocator();
            void JLTAPI check_vulkan_result(
              Renderer const &renderer, VkResult const result, text::String const &errmsg);
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_RENDERER_HPP */
