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

        class JLTAPI VulkanPresentationTarget {
          public:
            using image_array = jolt::collections::Array<VkImage>;
            using view_array = jolt::collections::Array<VkImageView>;

          private:
            VulkanRenderer const &m_renderer;
            VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
            image_array *m_swapchain_images = nullptr;
            view_array *m_swapchain_image_views = nullptr;

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
        };
    } // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKANPRESENTATION_HPP */
