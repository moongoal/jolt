#include <jolt/collections/array.hpp>
#include "vulkan.hpp"

namespace jolt {
    namespace graphics {
        void VulkanCommandPool::dispose() {
            if(m_pool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(m_renderer.get_device(), m_pool, get_vulkan_allocator());
                m_pool = VK_NULL_HANDLE;
            }
        }

        void
        VulkanCommandPool::initialize(bool transient, bool allow_reset, uint32_t queue_fam_index) {
            VkResult result;
            VkCommandPoolCreateFlags flags = 0;

            if(allow_reset) {
                flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            }

            if(transient) {
                flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            }

            VkCommandPoolCreateInfo cinfo{
              VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, // sType
              nullptr,                                    // pNext
              flags,                                      // flags
              queue_fam_index                             // queueFamilyIndex
            };

            result =
              vkCreateCommandPool(m_renderer.get_device(), &cinfo, get_vulkan_allocator(), &m_pool);

            jltassert2(result == VK_SUCCESS, "Unable to create command pool");
        }

        void VulkanCommandPool::trim() { vkTrimCommandPool(m_renderer.get_device(), m_pool, 0); }

        void VulkanCommandPool::reset(bool release_resources) {
            VkResult result = vkResetCommandPool(
              m_renderer.get_device(),
              m_pool,
              release_resources ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0);

            jltassert2(result == VK_SUCCESS, "Unable to reset command pool");
        }

        void VulkanCommandPool::allocate_command_buffers(
          VulkanCommandBuffer *out_buffers, uint32_t const n, bool const primary) {
            VkCommandBufferAllocateInfo ainfo{
              VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // sType
              nullptr,                                        // pNext
              m_pool,                                         // commandPool
              primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY
                      : VK_COMMAND_BUFFER_LEVEL_SECONDARY, // level
              n                                            // commandBufferCount
            };
            collections::Array<VkCommandBuffer> raw_buffers{n};

            VkResult result =
              vkAllocateCommandBuffers(m_renderer.get_device(), &ainfo, raw_buffers);

            for(size_t i = 0; i < n; ++i) {
                jltconstruct(out_buffers + i, m_renderer, raw_buffers[i], primary);
            }

            jltassert2(result == VK_SUCCESS, "Unable to allocate command buffers");
        }

        void
        VulkanCommandPool::free_command_buffers(VulkanCommandBuffer *buffers, uint32_t const n) {
            collections::Array<VkCommandBuffer> raw_buffers{n};

            for(size_t i = 0; i < n; ++i) { raw_buffers[i] = buffers[i].get_buffer(); }

            vkFreeCommandBuffers(m_renderer.get_device(), m_pool, n, raw_buffers);
        }

        void VulkanCommandBuffer::reset(bool release_resources) {
            VkResult result = vkResetCommandBuffer(
              m_buffer, release_resources ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0);
            jltassert2(result == VK_SUCCESS, "Unable to reset command buffer");
        }

        void VulkanCommandBuffer::begin_record(
          VkCommandBufferUsageFlags flags, VulkanCommandBufferRecordParams *const params) {
            VkResult result;
            VkCommandBufferInheritanceInfo inheritance_info;

            if(!is_primary()) {
                VulkanRenderTarget const &rtgt = *m_renderer.get_render_target();

                inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
                inheritance_info.pNext = nullptr;
                inheritance_info.renderPass = rtgt.get_render_pass();
                inheritance_info.subpass = params->subpass;
                inheritance_info.framebuffer = rtgt.get_active_framebuffer();
                inheritance_info.occlusionQueryEnable = params->occlusion_query_enable;
                inheritance_info.queryFlags = params->query_flags;
                inheritance_info.pipelineStatistics = params->pipeline_statistics;
            }

            VkCommandBufferBeginInfo binfo{
              VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, // sType
              nullptr,                                     // pNext
              flags,                                       // flags
              is_primary() ? nullptr : &inheritance_info   // pInheritanceInfo
            };

            result = vkBeginCommandBuffer(m_buffer, &binfo);
            jltassert2(result == VK_SUCCESS, "Unable to begin recording of the command buffer");
        } // namespace graphics

        void VulkanCommandBuffer::end_record() {
            VkResult result = vkEndCommandBuffer(m_buffer);
            jltassert2(result == VK_SUCCESS, "Unable to end recording of the command buffer");
        }

        void VulkanCommandBuffer::cmd_begin_render_pass(bool inline_commands) {
            VulkanRenderTarget const &tgt = *m_renderer.get_render_target();
            VkExtent2D const &win_extent =
              m_renderer.get_window()->get_surface_capabilities().currentExtent;
            VkClearValue clear_values[2] = {};

            clear_values[0].color.float32[0] = 1.0f;
            clear_values[0].color.float32[1] = 0.0f;
            clear_values[0].color.float32[2] = 0.0f;
            clear_values[0].color.float32[3] = 1.0f;

            VkRenderPassBeginInfo binfo{
              VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, // sType
              nullptr,                                  // pNext
              tgt.get_render_pass(),                    // renderPass
              tgt.get_active_framebuffer(),             // framebuffer
              {
                // renderArea
                0,                // x
                0,                // y
                win_extent.width, // width
                win_extent.height // height
              },
              2,           // clearValueCount
              clear_values // pClearValues
            };

            vkCmdBeginRenderPass(
              m_buffer,
              &binfo,
              inline_commands ? VK_SUBPASS_CONTENTS_INLINE
                              : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        }

        void VulkanCommandBuffer::cmd_end_render_pass() { vkCmdEndRenderPass(m_buffer); }

        void VulkanCommandBuffer::submit(VulkanFence *const fence) {
            VkSubmitInfo sinfo{
              VK_STRUCTURE_TYPE_SUBMIT_INFO, // sType
              nullptr,                       // pNext
              0,                             // waitSemaphoreCount TODO
              nullptr,                       // pSemaphores
              nullptr,                       // pWaitDstStageMask TODO
              1,                             // commandBufferCount
              &m_buffer,                     // pCommandBuffers
              0,                             // signalSemaphoreCount TODO
              nullptr,                       // pSignalSemaphores
            };

            // TODO graphics queue not always
            VkResult result = vkQueueSubmit(
              m_renderer.get_graphics_queue(),
              1,
              &sinfo,
              fence ? fence->get_fence() : VK_NULL_HANDLE);
        }
    } // namespace graphics
} // namespace jolt
