#ifndef JLT_GRAPHICS_VULKAN_PRESENTATION_TGT_HPP
#define JLT_GRAPHICS_VULKAN_PRESENTATION_TGT_HPP

#include "defs.hpp"

#include <jolt/collections/array.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            class Renderer;
            class Window;
            class Fence;
            class Semaphore;
            struct WaitSemaphoreActionSynchro;

            class JLTAPI PresentationTarget {
              public:
                using image_array = jolt::collections::Array<VkImage>;
                using view_array = jolt::collections::Array<VkImageView>;

                static constexpr const uint32_t INVALID_SWAPCHAIN_IMAGE =
                  std::numeric_limits<uint32_t>::max();

              private:
                Renderer const &m_renderer;
                VkQueue const m_queue;
                VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
                image_array *m_swapchain_images = nullptr;
                view_array *m_swapchain_image_views = nullptr;
                uint64_t m_acquire_timeout = 0;
                uint32_t m_active_swapchain_image = INVALID_SWAPCHAIN_IMAGE;

                void initialize();
                void shutdown();

              public:
                /**
                 * Create a new presentation target.
                 *
                 * @param renderer The renderer.
                 * @param queue A handle to a graphics queue obtained from `renderer`.
                 */
                PresentationTarget(Renderer const &renderer, VkQueue const queue) :
                  m_renderer{renderer}, m_queue{queue} {
                    initialize();
                }

                PresentationTarget(PresentationTarget const &other) = delete;

                ~PresentationTarget() { shutdown(); }

                Renderer const &get_renderer() const { return m_renderer; }
                VkQueue acquire_queue() const { return m_queue; }
                VkSwapchainKHR get_swapchain() const { return m_swapchain; }

                image_array const &get_swapchain_images() const { return *m_swapchain_images; }

                view_array const &get_swapchain_image_views() const {
                    return *m_swapchain_image_views;
                }

                uint64_t get_acquire_timeout() const { return m_acquire_timeout; }
                void set_acquire_timeout(uint64_t const timeout) { m_acquire_timeout = timeout; }

                uint32_t get_active_swapchain_image_index() const {
                    return m_active_swapchain_image;
                }

                void acquire_next_image(
                  Semaphore const *const semaphore = nullptr, Fence const *const fence = nullptr);
                void present_active_image(WaitSemaphoreActionSynchro const &synchro);
            };
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_PRESENTATION_TGT_HPP */
