#include <jolt/util.hpp>
#include <jolt/graphics/vulkan.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            void PresentationTarget::initialize() {
                console.debug("Creating swapchain");

                VkResult result;
                uint32_t n_present_modes;
                Window const &window = *m_renderer.get_window();

                result = vkGetPhysicalDeviceSurfacePresentModesKHR(
                  m_renderer.get_phy_device(), window.get_surface(), &n_present_modes, nullptr);
                jltassert2(
                  result == VK_SUCCESS,
                  "Unable to get available device surface presentation formats");

                collections::Array<VkPresentModeKHR> present_modes{n_present_modes};
                result = vkGetPhysicalDeviceSurfacePresentModesKHR(
                  m_renderer.get_phy_device(),
                  window.get_surface(),
                  &n_present_modes,
                  present_modes);
                jltassert2(
                  result == VK_SUCCESS,
                  "Unable to get available device surface presentation formats");

                bool present_mode_fifo_supported = false, present_mode_mailbox_supported = false;
                VkPresentModeKHR chosen_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

                for(VkPresentModeKHR pm : present_modes) {
                    if(pm == VK_PRESENT_MODE_MAILBOX_KHR) {
                        present_mode_mailbox_supported = true;
                    }

                    if(pm == VK_PRESENT_MODE_FIFO_KHR) {
                        present_mode_fifo_supported = true;
                    }
                }

                if(present_mode_mailbox_supported) {
                    chosen_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                } else if(present_mode_fifo_supported) {
                    chosen_present_mode = VK_PRESENT_MODE_FIFO_KHR;
                }

                uint32_t fam_index = m_renderer.get_graphics_queue_family_index();
                uint32_t max_imgs = min(
                  static_cast<uint32_t>(JLT_OPTIMAL_SWAPCHAIN_IMAGE_COUNT),
                  m_renderer.get_window()->get_surface_capabilities().maxImageCount);

                VkSwapchainCreateInfoKHR cinfo{
                  VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,        // sType
                  nullptr,                                            // pNext
                  0,                                                  // flags
                  window.get_surface(),                               // surface
                  max_imgs + 1,                                       // minImageCount
                  window.get_surface_format(),                        // imageFormat
                  window.get_surface_colorspace(),                    // imageColorSpace
                  window.get_surface_capabilities().currentExtent,    // imageExtent
                  1,                                                  // imageArrayLayers
                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,                // imageUsage
                  VK_SHARING_MODE_EXCLUSIVE,                          // imageSharingMode
                  1,                                                  // queueFamilyIndexCount
                  &fam_index,                                         // pQueueFamilyIndices
                  window.get_surface_capabilities().currentTransform, // preTransform
                  VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,                  // compositeAlpha
                  chosen_present_mode,                                // presentMode
                  VK_TRUE,                                            // clipped
                  VK_NULL_HANDLE,                                     // oldSwapchain
                };

                do {
                    cinfo.minImageCount--;

                    result = vkCreateSwapchainKHR(
                      m_renderer.get_device(), &cinfo, get_vulkan_allocator(), &m_swapchain);
                } while(result == VK_ERROR_INITIALIZATION_FAILED && cinfo.minImageCount > 1);

                jltvkcheck(result, "Unable to create swapchain");

                // Get images
                uint32_t n_images;

                result =
                  vkGetSwapchainImagesKHR(m_renderer.get_device(), m_swapchain, &n_images, nullptr);
                jltassert2(result == VK_SUCCESS, "Unable to get swapchain images");

                m_swapchain_images = jltalloc(typeof(*m_swapchain_images), 1);
                m_swapchain_image_views = jltalloc(typeof(*m_swapchain_image_views), 1);

                jltassert(m_swapchain_images);

                jltconstruct(m_swapchain_images, n_images);
                jltconstruct(m_swapchain_image_views, n_images);

                result = vkGetSwapchainImagesKHR(
                  m_renderer.get_device(), m_swapchain, &n_images, *m_swapchain_images);
                jltassert2(result == VK_SUCCESS, "Unable to get swapchain images");

                { // Create image
                    for(size_t i = 0; i < m_swapchain_images->get_length(); ++i) {
                        VkImageViewCreateInfo cinfo{
                          VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
                          nullptr,                                  // pNext
                          0,                                        // flags
                          (*m_swapchain_images)[i],                 // image
                          VK_IMAGE_VIEW_TYPE_2D,                    // viewType
                          window.get_surface_format(),              // format
                          {
                            // components
                            VK_COMPONENT_SWIZZLE_IDENTITY, // r
                            VK_COMPONENT_SWIZZLE_IDENTITY, // g
                            VK_COMPONENT_SWIZZLE_IDENTITY, // b
                            VK_COMPONENT_SWIZZLE_IDENTITY  // a
                          },
                          {
                            // subresourceRange
                            VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
                            0,                         // baseMipLevel
                            1,                         // levelCount
                            0,                         // baseArrayLayer
                            1,                         // layerCount
                          }};

                        result = vkCreateImageView(
                          m_renderer.get_device(),
                          &cinfo,
                          get_vulkan_allocator(),
                          *m_swapchain_image_views + i);

                        jltassert2(result == VK_SUCCESS, "Unable to create swapchain image views");
                    }
                }
            }

            void PresentationTarget::shutdown() {
                console.debug("Destroying swapchain");

                for(VkImageView vw : *m_swapchain_image_views) {
                    vkDestroyImageView(m_renderer.get_device(), vw, get_vulkan_allocator());
                }

                jltfree(m_swapchain_image_views);
                jltfree(m_swapchain_images);

                vkDestroySwapchainKHR(m_renderer.get_device(), m_swapchain, get_vulkan_allocator());
            }

            void PresentationTarget::acquire_next_image(
              Semaphore const *const semaphore, Fence const *const fence) {
                VkResult result;

                result = vkAcquireNextImageKHR(
                  m_renderer.get_device(),
                  m_swapchain,
                  m_acquire_timeout,
                  semaphore ? semaphore->get_semaphore() : VK_NULL_HANDLE,
                  fence ? fence->get_fence() : VK_NULL_HANDLE,
                  &m_active_swapchain_image);

                jltvkcheck(result, "Unable to acquire next swapchain image");
            }

            void PresentationTarget::present_active_image(
              WaitSemaphoreActionSynchro const &synchro) {
                VkPresentInfoKHR pinfo{
                  VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, // sType
                  nullptr,                            // pNext
                  synchro.wait_semaphore_count,       // semaphoreCount
                  synchro.wait_semaphores,            // pWaitSemaphores
                  1,                                  // swapchainCount
                  &m_swapchain,                       // pSwapchains
                  &m_active_swapchain_image,          // pImageIndices
                  nullptr                             // pResults
                };

                VkResult result = vkQueuePresentKHR(m_renderer.get_graphics_queue(), &pinfo);
                jltvkcheck(result, "Unable to present active swapchain image");
            }
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt
