#ifndef JLT_GRAPHICS_VULKAN_WINDOW_HPP
#define JLT_GRAPHICS_VULKAN_WINDOW_HPP

#include "defs.hpp"

#include <jolt/ui/window.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            class Renderer;

            class JLTAPI Window {
                Renderer const &m_renderer;
                ui::Window const &m_window;
                VkQueue const m_queue;
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
                Window(Renderer const &renderer, ui::Window const &window, VkQueue const queue) :
                  m_renderer{renderer}, m_window{window}, m_queue{queue} {
                    initialize();
                }

                ~Window() { shutdown(); }

                Renderer const &get_renderer() const { return m_renderer; }

                VkQueue get_queue() const { return m_queue; }

                ui::Window const &get_ui_window() const { return m_window; }

                VkSurfaceKHR get_surface() const { return m_surface; }

                VkSurfaceCapabilitiesKHR const &get_surface_capabilities() const { return m_surface_caps; }

                VkImageFormatProperties const &get_image_format_properties() const {
                    return m_phy_dev_image_fmt_props;
                }

                VkFormat get_surface_format() const { return m_surface_fmt; }

                VkColorSpaceKHR get_surface_colorspace() const { return m_surface_colorspace; }
            };
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_WINDOW_HPP */
