#include <jolt/collections/array.hpp>
#include <jolt/graphics/vulkan.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            void CommandPool::dispose() {
                if(m_pool != VK_NULL_HANDLE) {
                    vkDestroyCommandPool(m_renderer.get_device(), m_pool, get_vulkan_allocator());
                    m_pool = VK_NULL_HANDLE;
                }
            }

            void CommandPool::initialize(bool transient, bool allow_reset, uint32_t queue_fam_index) {
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

            void CommandPool::trim() { vkTrimCommandPool(m_renderer.get_device(), m_pool, 0); }

            void CommandPool::reset(bool release_resources) {
                VkResult result = vkResetCommandPool(
                  m_renderer.get_device(),
                  m_pool,
                  release_resources ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0);

                jltassert2(result == VK_SUCCESS, "Unable to reset command pool");
            }

            void CommandPool::allocate_command_buffers(
              CommandBuffer *out_buffers, uint32_t const n, bool const primary) {
                VkCommandBufferAllocateInfo ainfo{
                  VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // sType
                  nullptr,                                        // pNext
                  m_pool,                                         // commandPool
                  primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY, // level
                  n // commandBufferCount
                };
                collections::Array<VkCommandBuffer> raw_buffers{n};

                VkResult result = vkAllocateCommandBuffers(m_renderer.get_device(), &ainfo, raw_buffers);

                for(size_t i = 0; i < n; ++i) {
                    jltconstruct(out_buffers + i, m_renderer, raw_buffers[i], primary);
                }

                jltassert2(result == VK_SUCCESS, "Unable to allocate command buffers");
            }

            CommandBuffer CommandPool::allocate_single_command_buffer(bool const primary) {
                VkCommandBufferAllocateInfo ainfo{
                  VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // sType
                  nullptr,                                        // pNext
                  m_pool,                                         // commandPool
                  primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY, // level
                  1 // commandBufferCount
                };
                VkCommandBuffer raw_cmd_buffer;

                VkResult result = vkAllocateCommandBuffers(m_renderer.get_device(), &ainfo, &raw_cmd_buffer);

                jltassert2(result == VK_SUCCESS, "Unable to allocate command buffers");

                return CommandBuffer{m_renderer, raw_cmd_buffer, primary};
            }

            void CommandPool::free_command_buffers(CommandBuffer *buffers, uint32_t const n) {
                collections::Array<VkCommandBuffer> raw_buffers{n};

                for(size_t i = 0; i < n; ++i) { raw_buffers[i] = buffers[i].get_buffer(); }

                vkFreeCommandBuffers(m_renderer.get_device(), m_pool, n, raw_buffers);
            }

            void CommandPool::free_single_command_buffer(CommandBuffer &buffer) {
                VkCommandBuffer raw_buffer = buffer.get_buffer();

                vkFreeCommandBuffers(m_renderer.get_device(), m_pool, 1, &raw_buffer);
            }

            void CommandBuffer::reset(bool release_resources) {
                VkResult result = vkResetCommandBuffer(
                  m_buffer, release_resources ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0);
                jltassert2(result == VK_SUCCESS, "Unable to reset command buffer");
            }

            void CommandBuffer::begin_record(
              VkCommandBufferUsageFlags flags, CommandBufferRecordParams *const params) {
                VkResult result;
                VkCommandBufferInheritanceInfo inheritance_info;

                if(!is_primary()) {
                    RenderTarget const &rtgt = *m_renderer.get_render_target();

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
            }

            void CommandBuffer::end_record() {
                VkResult result = vkEndCommandBuffer(m_buffer);
                jltassert2(result == VK_SUCCESS, "Unable to end recording of the command buffer");
            }

            void CommandBuffer::cmd_begin_render_pass(bool const inline_commands) {
                RenderTarget const &tgt = *m_renderer.get_render_target();
                VkExtent2D const &win_extent =
                  m_renderer.get_window()->get_surface_capabilities().currentExtent;

                VkClearValue clear_values[2] = {};

                clear_values[0].color.float32[0] = 0.0f;
                clear_values[0].color.float32[1] = 1.0f;
                clear_values[0].color.float32[2] = 0.0f;
                clear_values[0].color.float32[3] = 0.0f;

                VkRenderPassBeginInfo binfo{
                  VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, // sType
                  nullptr,                                  // pNext
                  tgt.get_render_pass(),                    // renderPass
                  tgt.get_active_framebuffer(),             // framebuffer
                  {                                         // renderArea
                   {
                     // offset
                     0, // x
                     0, // y
                   },
                   {
                     // extent
                     win_extent.width, // width
                     win_extent.height // height
                   }},
                  2,           // clearValueCount
                  clear_values // pClearValues
                };

                vkCmdBeginRenderPass(
                  m_buffer,
                  &binfo,
                  inline_commands ? VK_SUBPASS_CONTENTS_INLINE
                                  : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
            }

            void CommandBuffer::cmd_end_render_pass() { vkCmdEndRenderPass(m_buffer); }

            void CommandBuffer::submit(VkQueue const queue, ActionSynchro const &synchro) {
                VkSubmitInfo sinfo{
                  VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
                  nullptr,                        // pNext
                  synchro.wait_semaphore_count,   // waitSemaphoreCount
                  synchro.wait_semaphores,        // pWaitSemaphores
                  synchro.wait_semaphores_stages, // pWaitDstStageMask
                  1,                              // commandBufferCount
                  &m_buffer,                      // pCommandBuffers
                  synchro.signal_semaphore_count, // signalSemaphoreCount
                  synchro.signal_semaphores,      // pSignalSemaphores
                };

                VkResult result = vkQueueSubmit(queue, 1, &sinfo, synchro.fence);
                jltvkcheck(result, "Error while submitting queue");
            }
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt
