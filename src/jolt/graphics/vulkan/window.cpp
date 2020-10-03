#include <jolt/text/string.hpp>
#include <jolt/debug/console.hpp>
#include <jolt/graphics/vulkan.hpp>

using namespace jolt::text;

namespace jolt {
    namespace graphics {
        namespace vulkan {
            void Window::populate_device_image_metadata() {
                VkResult result = vkGetPhysicalDeviceImageFormatProperties(
                  m_renderer.get_phy_device(),
                  VK_FORMAT_B8G8R8A8_UNORM,
                  VK_IMAGE_TYPE_2D,
                  VK_IMAGE_TILING_OPTIMAL,
                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                  VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT,
                  &m_phy_dev_image_fmt_props);

                switch(result) {
                    case VK_SUCCESS:
                        break;

                    case VK_ERROR_FORMAT_NOT_SUPPORTED:
                        console.err("Image format not supported");
                        abort();

                    default:
                        console.err("Out of memory while querying for image format support");
                        abort();
                }

                result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                  m_renderer.get_phy_device(), m_surface, &m_surface_caps);

                jltvkcheck(result, "Unable to get image capabilities");
            }

            void Window::initialize() {
                initialize_surface();
                populate_device_image_metadata();
            }

            void Window::initialize_surface() {
                console.debug("Creating window surface");

                VkResult result;
                VkBool32 surface_support;
                uint32_t n_fmts;
                VkWin32SurfaceCreateInfoKHR cinfo{
                  VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, // sType
                  nullptr,                                         // pNext
                  0,                                               // flags
                  ui::get_hinstance(),                             // hinstance
                  m_window.get_handle()                            // hwnd
                };

                result = vkCreateWin32SurfaceKHR(
                  m_renderer.get_instance(), &cinfo, get_vulkan_allocator(), &m_surface);

                jltassert2(result == VK_SUCCESS, "Unable to create window surface");

                vkGetPhysicalDeviceSurfaceSupportKHR(
                  m_renderer.get_phy_device(),
                  m_renderer.get_graphics_queue_family_index(),
                  m_surface,
                  &surface_support);

                jltvkcheck(result, "Unable to query for surface support");
                jltassert2(surface_support == VK_TRUE, "Current device doesn't support window");

                result = vkGetPhysicalDeviceSurfaceFormatsKHR(
                  m_renderer.get_phy_device(), m_surface, &n_fmts, nullptr);
                jltvkcheck(result, "Unable to get available device surface formats");

                collections::Array<VkSurfaceFormatKHR> fmts{n_fmts};
                result = vkGetPhysicalDeviceSurfaceFormatsKHR(
                  m_renderer.get_phy_device(), m_surface, &n_fmts, fmts);
                jltvkcheck(result, "Unable to get available device surface formats");

                m_surface_fmt = fmts[0].format;
                m_surface_colorspace = fmts[0].colorSpace;
            }

            void Window::shutdown() {
                console.debug("Destroying Vulkan window");

                vkDestroySurfaceKHR(m_renderer.get_instance(), m_surface, get_vulkan_allocator());
            }
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt
