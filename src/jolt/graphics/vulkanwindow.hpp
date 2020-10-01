#ifndef JLT_GRAPHICS_VULKANWINDOW_HPP
#define JLT_GRAPHICS_VULKANWINDOW_HPP

#include "defs.hpp"

#include <Windows.h>
#include <vulkan/vulkan.h>
#include <jolt/util.hpp>
#include <jolt/ui/window.hpp>

namespace jolt {
    namespace graphics {
        class VulkanRenderer;

        class JLTAPI VulkanWindow {
            VulkanRenderer const &m_renderer;
            ui::Window const &m_window;
            VkSurfaceKHR m_surface = VK_NULL_HANDLE;
            VkSurfaceCapabilitiesKHR m_surface_caps{};
            VkImageFormatProperties m_phy_dev_image_fmt_props{};
            VkFormat m_surface_fmt = VK_FORMAT_UNDEFINED;
            VkColorSpaceKHR m_surface_colorspace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

            void initialize();
            void populate_device_image_metadata();
            void initialize_surface();

            void shutdown();

          public:
            VulkanWindow(VulkanRenderer const &renderer, ui::Window const &window) :
              m_renderer{renderer}, m_window{window} {
                initialize();
            }

            ~VulkanWindow() { shutdown(); }

            VulkanRenderer const &get_renderer() const { return m_renderer; }

            ui::Window const &get_ui_window() const { return m_window; }

            VkSurfaceKHR get_surface() const { return m_surface; }

            VkSurfaceCapabilitiesKHR const &get_surface_capabilities() const {
                return m_surface_caps;
            }

            VkImageFormatProperties const &get_image_format_properties() const {
                return m_phy_dev_image_fmt_props;
            }

            VkFormat get_surface_format() const { return m_surface_fmt; }

            VkColorSpaceKHR get_surface_colorspace() const { return m_surface_colorspace; }
        };
    } // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKANWINDOW_HPP */
