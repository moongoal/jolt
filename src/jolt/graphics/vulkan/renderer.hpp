#ifndef JLT_GRAPHICS_VULKAN_RENDERER_HPP
#define JLT_GRAPHICS_VULKAN_RENDERER_HPP

#include "defs.hpp"

#include <jolt/ui/window.hpp>
#include <jolt/collections/vector.hpp>
#include <jolt/collections/array.hpp>
#include <jolt/threading/mutex.hpp>
#include "window.hpp"
#include "render_tgt.hpp"
#include "presentation_tgt.hpp"
#include "cmd.hpp"
#include "shader-mgr.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            struct GraphicsEngineInitializationParams {
                const char *app_name;
                unsigned short app_version_major, app_version_minor, app_version_revision;
                ui::Window *wnd;
                uint32_t n_queues_graphics; //< Preferred number of graphics queues to request.
                uint32_t n_queues_transfer; //< Preferred number of transfer queues to request.
                uint32_t n_queues_compute;  //< Preferred number of compute queues to request.
            };

            /**
             * Represents the lost state of a Vulkan renderer.
             */
            enum RendererLostState {
                RENDERER_NOT_LOST,     //< The renderer is not lost.
                RENDERER_LOST_PRESENT, /*< The presentation target is lost. Both the render and
                                         presentation targets need to be reset. */
                RENDERER_LOST_DEVICE   //< The device is lost, the renderer has to be reset.
            };

            class JLTAPI Renderer {
                using layer_vector = collections::Vector<const char *>;
                using extension_vector = layer_vector;
                using queue_ci_vector = collections::Vector<VkDeviceQueueCreateInfo>;
                using queue_fam_prop_array = collections::Array<VkQueueFamilyProperties>;

                struct QueueInfo {
                    VkQueue queue;
                    VkQueueFlags flags;
                    uint32_t queue_family_index;
                };

                using queue_info_mutex = threading::Mutex<QueueInfo>;
                using queue_info_array = collections::Array<queue_info_mutex>;

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
                VkPhysicalDeviceVulkan12Features m_phy_req_feats12;
                VkPhysicalDeviceFeatures2 m_phy_req_feats;

                mutable queue_info_array *m_queues = nullptr;
                Window *m_window = nullptr;
                RenderTarget *m_render_target = nullptr;
                PresentationTarget *m_presentation_target = nullptr;
                ShaderManager *m_shader_manager = nullptr;

                mutable RendererLostState m_lost = RENDERER_NOT_LOST;

                void initialize_phase2(GraphicsEngineInitializationParams const &params);
                void initialize_instance(GraphicsEngineInitializationParams const &params);
                void select_physical_device();
                void initialize_device(GraphicsEngineInitializationParams const &params);
                void initialize_debug_logger();

                queue_ci_vector select_device_queues(
                  queue_fam_prop_array &fam_props,
                  uint32_t const n_graph,
                  uint32_t const n_trans,
                  uint32_t const n_compute);

                int select_single_queue(
                  queue_fam_prop_array &fam_props, VkQueueFlags const requirements, bool const exact);

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

                const VkPhysicalDeviceFeatures2 &get_phy_device_features() const { return m_phy_feats; }

                const VkPhysicalDeviceFeatures2 &get_enabled_phy_device_features() const {
                    return m_phy_req_feats;
                }

                const VkPhysicalDeviceProperties2 &get_phy_device_properties() const { return m_phy_props; }

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

                ShaderManager &get_shader_manager() const { return *m_shader_manager; }
                void set_shader_manager(ShaderManager *const manager) { m_shader_manager = manager; }

                /**
                 * Return the family index for a queue.
                 *
                 * @param queue The queue.
                 *
                 * @return The family index for the given queue or JLT_VULKAN_INVALID32 if the queue
                 * is not from this renderer instance.
                 */
                uint32_t get_queue_family_index(VkQueue const queue) const;

                /**
                 * Return a free queue satisfying the requirements.
                 *
                 * @param flags The queue requirements.
                 *
                 * @return A queue handle or VK_NULL_HANDLE if no queue is available for the given
                 * set of requirements.
                 */
                VkQueue acquire_queue(VkQueueFlags const flags) const;

                /**
                 * Release an acquired queue.
                 *
                 * @param queue A queue handle returned by `acquire_queue()`.
                 */
                void release_queue(VkQueue const queue) const;

                VkQueue acquire_graphics_queue() const;
                VkQueue acquire_transfer_queue() const;
                VkQueue acquire_compute_queue() const;

                Window *get_window() { return m_window; }
                Window const *get_window() const { return m_window; }
                void set_window(Window *const wnd) { m_window = wnd; }
                RenderTarget *get_render_target() { return m_render_target; }
                RenderTarget const *get_render_target() const { return m_render_target; }
                PresentationTarget *get_presentation_target() { return m_presentation_target; }

                /**
                 * Set the render target.
                 *
                 * @param render_target The render target to use with this renderer.
                 *
                 * @remarks Target shut down is responsibility of the application. The renderer will
                 * never shut down or deallocate the render target.
                 */
                void set_render_target(RenderTarget *const render_target) { m_render_target = render_target; }

                /**
                 * Set the presentation target.
                 *
                 * @param presentation_target The presentation target to use with this renderer.
                 *
                 * @remarks Target shut down is responsibility of the application. The renderer will
                 * never shut down or deallocate the presentation target.
                 */
                void set_presentation_target(PresentationTarget *const presentation_target) {
                    m_presentation_target = presentation_target;
                }

                PresentationTarget const *get_presentation_target() const { return m_presentation_target; }

                void initialize(GraphicsEngineInitializationParams const &params);
                void shutdown();

                /**
                 * Wait for all the queues to be idle.
                 *
                 * Before calling this function, all the queues must be released. Usage of this
                 * function is intended for shutdown purposes only and only in a single-threaded
                 * situation.
                 */
                void wait_queues_idle() const;

                /**
                 * Return a value stating whether the renderer is lost.
                 */
                bool is_lost() const { return m_lost != RENDERER_NOT_LOST; }

                /**
                 * Return the lost state of the renderer.
                 */
                RendererLostState get_lost_state() const { return m_lost; }

                /**
                 * Signal that the renderer is lost and needs to be reset.
                 *
                 * @param state The new lost state to set.
                 *
                 * @remarks This function only changes the lost state if the new state represents a
                 * worse lost state (i.e. more will need to reset). To reset the lost state to
                 * RENDERER_NOT_LOST, use `reset_lost_state()`.
                 *
                 * @see reset_lost_state().
                 */
                void signal_lost(RendererLostState const state) const;

                /**
                 * Reset the lost state to RENDERER_NOT_LOST.
                 */
                void reset_lost_state();

                /**
                 * To be called at initialization time or when the logical device is reported as
                 * lost.
                 */
                void reset(GraphicsEngineInitializationParams const &params);

                /**
                 * Get the memory type index for the given requirements.
                 *
                 * @param requirements A bitmask of device memory requirements.
                 *
                 * @return The memory type index that only has the exact provided requirements or
                 * JLT_VULKAN_INVALID32 if no such memory type is available.
                 */
                uint32_t get_memory_type_index(VkMemoryPropertyFlags const requirements) const;

                /**
                 * Get the memory type index for the given requirements.
                 *
                 * @param requirements A bitmask of device memory requirements.
                 * @param exclusions Requirements to exclude. Any returned result will not have
                 * these flags included.
                 * @param mem_bits A bitmask specifying the valid memory type indices.
                 *
                 * @return The memory type index for the requirements or JLT_VULKAN_INVALID32 if no
                 * available memory type meets the requirements.
                 */
                uint32_t get_memory_type_index(
                  VkMemoryPropertyFlags const requirements,
                  VkMemoryPropertyFlags const exclusions,
                  uint32_t const mem_bits = UINT32_MAX) const;
            };

            VkAllocationCallbacks JLTAPI *get_vulkan_allocator();
            void JLTAPI
            check_vulkan_result(Renderer const &renderer, VkResult const result, text::String const &errmsg);
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_RENDERER_HPP */
