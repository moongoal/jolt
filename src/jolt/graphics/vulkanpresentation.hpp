#ifndef JLT_GRAPHICS_VULKANPRESENTATION_HPP
#define JLT_GRAPHICS_VULKANPRESENTATION_HPP

#include "defs.hpp"

#include <Windows.h>
#include <vulkan/vulkan.h>
#include <jolt/util.hpp>
#include <jolt/collections/array.hpp>

namespace jolt {
    namespace graphics {
        class VulkanRenderer;
        class VulkanWindow;
        class VulkanFence;

        class JLTAPI VulkanPresentationTarget {
          public:
            using image_array = jolt::collections::Array<VkImage>;
            using view_array = jolt::collections::Array<VkImageView>;

            static constexpr const uint32_t INVALID_SWAPCHAIN_IMAGE =
              std::numeric_limits<uint32_t>::max();

          private:
            VulkanRenderer const &m_renderer;
            VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
            image_array *m_swapchain_images = nullptr;
            view_array *m_swapchain_image_views = nullptr;
            uint64_t m_acquire_timeout = 0;
            uint32_t m_active_swapchain_image = INVALID_SWAPCHAIN_IMAGE;

            void initialize();
            void shutdown();

          public:
            VulkanPresentationTarget(VulkanRenderer const &renderer) : m_renderer{renderer} {
                initialize();
            }

            ~VulkanPresentationTarget() { shutdown(); }

            VulkanRenderer const &get_renderer() const { return m_renderer; }

            VkSwapchainKHR get_swapchain() const { return m_swapchain; }

            image_array const &get_swapchain_images() const { return *m_swapchain_images; }
            view_array const &get_swapchain_image_views() const { return *m_swapchain_image_views; }
            uint64_t get_acquire_timeout() const { return m_acquire_timeout; }
            void set_acquire_timeout(uint64_t timeout) { m_acquire_timeout = timeout; }
            uint32_t get_active_swapchain_image_index() const { return m_active_swapchain_image; }

            void acquire_next_image(VulkanFence *fence = nullptr);
            void present_active_image();
        };
    } // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKANPRESENTATION_HPP */
