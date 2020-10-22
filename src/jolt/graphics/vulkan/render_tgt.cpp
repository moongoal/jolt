#include <jolt/graphics/vulkan.hpp>

using namespace jolt::text;

namespace jolt {
    namespace graphics {
        namespace vulkan {
            void RenderTarget::select_depth_stencil_image_format() {
                VkFormat allowed_fmts[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM};
                constexpr const size_t allowed_fmts_len = sizeof(allowed_fmts) / sizeof(VkFormat);

                for(size_t i = 0; i < allowed_fmts_len; ++i) {
                    VkFormatProperties props;

                    vkGetPhysicalDeviceFormatProperties(
                      m_renderer.get_phy_device(), allowed_fmts[i], &props);

                    if(
                      props.optimalTilingFeatures
                      & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                        m_ds_image_fmt = allowed_fmts[i];
                        return;
                    }
                }

                console.err("Unable to find a suitable format for depth/stencil buffer");
                abort();
            }

            void RenderTarget::initialize_depth_stencil_buffer() {
                console.debug("Initializing depth/stencil buffer");

                VkResult result;
                Window const &window = *m_renderer.get_window();

                select_depth_stencil_image_format();

                VkImageCreateInfo cinfo{
                  VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, // stype
                  nullptr,                             // pNext
                  0,                                   // flags
                  VK_IMAGE_TYPE_2D,                    // imageType
                  m_ds_image_fmt,                      // format
                  {
                    // extent
                    window.get_surface_capabilities().currentExtent.width,  // width
                    window.get_surface_capabilities().currentExtent.height, // height
                    1                                                       // depth
                  },
                  1,                                           // mipLevels
                  1,                                           // arrayLayers
                  VK_SAMPLE_COUNT_1_BIT,                       // samples
                  VK_IMAGE_TILING_OPTIMAL,                     // tiling
                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, // usage
                  VK_SHARING_MODE_EXCLUSIVE,                   // sharingMode
                  0,                                           // queueFamilyIndexCount
                  nullptr,                                     // pQueueFamilyIndices
                  VK_IMAGE_LAYOUT_UNDEFINED                    // initialLayout
                };

                result = vkCreateImage(
                  m_renderer.get_device(), &cinfo, get_vulkan_allocator(), &m_ds_image);
                jltassert2(result == VK_SUCCESS, "Unable to create image for depth/stencil buffer");

                { // Image storage
                    constexpr uint32_t mti_invalid = std::numeric_limits<uint32_t>::max();
                    uint32_t mti = mti_invalid; // Memory Type Index
                    VkMemoryRequirements reqs;

                    vkGetImageMemoryRequirements(m_renderer.get_device(), m_ds_image, &reqs);

                    // Check if requirements are satisfied both for device & image
                    for(uint32_t i = 0;
                        i < m_renderer.get_phy_device_memory_properties().memoryTypeCount;
                        ++i) {
                        if(
                          (m_renderer.get_phy_device_memory_properties()
                             .memoryTypes[i]
                             .propertyFlags
                           & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                          && (reqs.memoryTypeBits & (1 << i))) {
                            mti = i;
                            break;
                        }
                    }

                    jltassert2(
                      mti != mti_invalid,
                      "Required image memory type for depth/stencil buffer unavailable");

                    VkMemoryAllocateInfo cinfo{
                      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // sType
                      nullptr,                                // pNext
                      reqs.size,                              // allocationSize
                      mti                                     // memoryTypeIndex
                    };

                    result = vkAllocateMemory(
                      m_renderer.get_device(), &cinfo, get_vulkan_allocator(), &m_ds_image_memory);
                    jltassert2(
                      result == VK_SUCCESS, "Unable to allocate storage for depth/stencil buffer");
                }

                result =
                  vkBindImageMemory(m_renderer.get_device(), m_ds_image, m_ds_image_memory, 0);
                jltassert2(
                  result == VK_SUCCESS, "Unable to bind image memory for depth/stencil buffer");

                { // Image view
                    VkImageViewCreateInfo cinfo{
                      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
                      nullptr,                                  // pNext
                      0,                                        // flags
                      m_ds_image,                               // image
                      VK_IMAGE_VIEW_TYPE_2D,                    // viewType
                      m_ds_image_fmt,                           // format
                      {
                        // components
                        VK_COMPONENT_SWIZZLE_IDENTITY, // r
                        VK_COMPONENT_SWIZZLE_IDENTITY, // g
                        VK_COMPONENT_SWIZZLE_IDENTITY, // b
                        VK_COMPONENT_SWIZZLE_IDENTITY  // a
                      },
                      {
                        // subresourceRange
                        VK_IMAGE_ASPECT_DEPTH_BIT, // aspectMask
                        0,                         // baseMipLevel
                        1,                         // levelCount
                        0,                         // baseArrayLayer
                        1,                         // layerCount
                      }};

                    result = vkCreateImageView(
                      m_renderer.get_device(), &cinfo, get_vulkan_allocator(), &m_ds_image_view);
                    jltassert2(
                      result == VK_SUCCESS, "Unable to create view for depth/stencil buffer");
                }
            }

            void RenderTarget::initialize_render_pass() {
                console.debug("Creating render pass");

                VkResult result;
                Window const &window = *m_renderer.get_window();

                VkAttachmentDescription attachments[] = {
                  {
                    // Color
                    0,                                // flags
                    window.get_surface_format(),      // format
                    VK_SAMPLE_COUNT_1_BIT,            // samples
                    VK_ATTACHMENT_LOAD_OP_CLEAR,      // loadOp
                    VK_ATTACHMENT_STORE_OP_STORE,     // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // stencilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE, // stencilStoreOp
                    VK_IMAGE_LAYOUT_UNDEFINED,        // initialLayout
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR   // finalLayout
                  },
                  {
                    // Depth
                    0,                                       // flags
                    m_ds_image_fmt,                          // format
                    VK_SAMPLE_COUNT_1_BIT,                   // samples
                    VK_ATTACHMENT_LOAD_OP_CLEAR,             // loadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,        // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,         // stencilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,        // stencilStoreOp
                    VK_IMAGE_LAYOUT_UNDEFINED,               // initialLayout
                    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL // finalLayout
                  }};
                constexpr const uint32_t n_attachments =
                  sizeof(attachments) / sizeof(VkAttachmentDescription);

                VkAttachmentReference sp_color_att_ref = {
                  0,                                       // attachment
                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // layout
                };

                VkAttachmentReference sp_ds_att_ref = {
                  1,                                       // attachment
                  VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL // layout
                };

                VkSubpassDescription subpasses[] = {{
                  0,                               // flags
                  VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
                  0,                               // inputAttachmentCount
                  nullptr,                         // pInputAttachments
                  1,                               // colorAttachmentCount
                  &sp_color_att_ref,               // pColorAttachments
                  nullptr,                         // pResolveAttachments
                  &sp_ds_att_ref,                  // pDepthStencilAttachment
                  0,                               // preserveAttachmentCount
                  nullptr,                         // pPreserveAttachments
                }};
                constexpr const uint32_t n_subpasses =
                  sizeof(subpasses) / sizeof(VkSubpassDescription);

                VkRenderPassCreateInfo cinfo{
                  VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, // sType
                  nullptr,                                   // pNext
                  0,                                         // flags
                  n_attachments,                             // attachmentCount
                  attachments,                               // pAttachments
                  n_subpasses,                               // subpassCount
                  subpasses,                                 // pSubpasses
                  0,                                         // dependencyCount
                  nullptr                                    // pDependencies
                };

                result = vkCreateRenderPass(
                  m_renderer.get_device(), &cinfo, get_vulkan_allocator(), &m_render_pass);
                jltassert2(result == VK_SUCCESS, "Unable to create render pass");
            }

            void RenderTarget::initialize_framebuffer(view_array const &views) {
                console.debug("Creating framebuffer");

                VkResult result;
                Window const &window = *m_renderer.get_window();

                m_framebuffers = jltnew(fb_array, views.get_length());

                for(size_t i = 0; i < views.get_length(); ++i) {
                    VkImageView fb_views[] = {views[i], m_ds_image_view};
                    constexpr const size_t n_views = sizeof(fb_views) / sizeof(VkImageView);

                    VkFramebufferCreateInfo cinfo{
                      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,              // sType
                      nullptr,                                                // pNext
                      0,                                                      // flags
                      m_render_pass,                                          // renderPass
                      n_views,                                                // attachmentCount
                      fb_views,                                               // pAttachments
                      window.get_surface_capabilities().currentExtent.width,  // width
                      window.get_surface_capabilities().currentExtent.height, // height
                      1                                                       // layers
                    };

                    result = vkCreateFramebuffer(
                      m_renderer.get_device(),
                      &cinfo,
                      get_vulkan_allocator(),
                      &(*m_framebuffers)[i]);
                    jltassert2(result == VK_SUCCESS, "Unable to create framebuffer");
                }
            }

            void RenderTarget::initialize() {
                jltassert2(
                  m_renderer.get_presentation_target(),
                  "Renderer must have a presentation target for a renderer target to be "
                  "initialized");

                initialize_depth_stencil_buffer();
                initialize_render_pass();
                initialize_framebuffer(
                  m_renderer.get_presentation_target()->get_swapchain_image_views());
            }

            void RenderTarget::shutdown() {
                console.debug("Destroying framebuffer");

                for(auto fb : *m_framebuffers) {
                    vkDestroyFramebuffer(m_renderer.get_device(), fb, get_vulkan_allocator());
                }

                jltfree(m_framebuffers);

                console.debug("Destroying render pass");
                vkDestroyRenderPass(m_renderer.get_device(), m_render_pass, get_vulkan_allocator());

                console.debug("Destroying depth/stencil buffer");
                vkDestroyImageView(
                  m_renderer.get_device(), m_ds_image_view, get_vulkan_allocator());
                vkFreeMemory(m_renderer.get_device(), m_ds_image_memory, get_vulkan_allocator());
                vkDestroyImage(m_renderer.get_device(), m_ds_image, get_vulkan_allocator());
            }

            VkFramebuffer RenderTarget::get_active_framebuffer() const {
                uint32_t active_idx =
                  m_renderer.get_presentation_target()->get_active_swapchain_image_index();

                jltassert2(
                  active_idx != PresentationTarget::INVALID_SWAPCHAIN_IMAGE,
                  "Invalid swapchain image");

                return (*m_framebuffers)[active_idx];
            }
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt
