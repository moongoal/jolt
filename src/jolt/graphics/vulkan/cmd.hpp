#ifndef JLT_GRAPHICS_VULKAN_CMD_HPP
#define JLT_GRAPHICS_VULKAN_CMD_HPP

#include "defs.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            class Renderer;
            class CommandBuffer;
            class Fence;
            class Semaphore;
            struct ActionSynchro;

            /**
             * Command buffer record params.
             *
             * @see
             * https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/chap6.html#VkCommandBufferBeginInfo
             */
            struct CommandBufferRecordParams {
                static constexpr const uint32_t INVALID_SUBPASS = std::numeric_limits<uint32_t>::max();

                uint32_t subpass;
                bool occlusion_query_enable = VK_FALSE;
                VkQueryControlFlags query_flags = 0;
                VkQueryPipelineStatisticFlags pipeline_statistics = 0;

                explicit CommandBufferRecordParams(uint32_t spass = INVALID_SUBPASS) : subpass{spass} {}
            };

            class JLTAPI CommandPool {
                Renderer const &m_renderer;
                VkCommandPool m_pool = VK_NULL_HANDLE;

                void initialize(bool transient, bool allow_reset, uint32_t queue_fam_index);

              public:
                /**
                 * Create a command pool.
                 *
                 * @param renderer The renderer.
                 * @param transient True if the queue will be used to allocate short-lived command
                 * buffers.
                 * @param allow_reset True if the queue will be allowed to be reset.
                 */
                CommandPool(
                  Renderer const &renderer, bool transient, bool allow_reset, uint32_t queue_fam_index) :
                  m_renderer{renderer} {
                    initialize(transient, allow_reset, queue_fam_index);
                }

                CommandPool(CommandPool const &) = delete;
                CommandPool(CommandPool &&other) : m_renderer{other.m_renderer}, m_pool{other.m_pool} {
                    other.m_pool = VK_NULL_HANDLE;
                }

                ~CommandPool() { dispose(); }

                Renderer const &get_renderer() const { return m_renderer; }
                VkCommandPool get_pool() const { return m_pool; }

                /**
                 * Delete the command queue and free any allocated resource.
                 */
                void dispose();

                /**
                 * Releases all non-used resources from the command pool back to the system.
                 */
                void trim();

                /**
                 * Resets the command pool, gets back all allocated resources from the command
                 * buffers back to the command pool.
                 *
                 * @param release_resources If true, also release the resources from the command
                 * pool back to the system.
                 */
                void reset(bool release_resources);

                /**
                 * Allocate new command buffers.
                 *
                 * @param out_buffers Memory region that will be populated by the newly allocated
                 * `n` command buffers.
                 * @param n The number of command buffers to allocate.
                 * @param primary True if the allocated command buffers will be primary, false if
                 * they'll be secondary.
                 *
                 * @remarks Primary command buffers are directly executable, secondary command
                 * buffers can only be executed by a primary command buffer.
                 */
                void
                allocate_command_buffers(CommandBuffer *out_buffers, uint32_t const n, bool const primary);

                /**
                 * Allocate new command buffer.
                 *
                 * @param primary True if the allocated command buffer will be primary, false if
                 * it'll be secondary.
                 *
                 * @return The newly allocated command buffer.
                 *
                 * @remarks Primary command buffers are directly executable, secondary command
                 * buffers can only be executed by a primary command buffer.
                 */
                CommandBuffer allocate_single_command_buffer(bool const primary);

                void free_command_buffers(CommandBuffer *buffers, uint32_t const n);
                void free_raw_command_buffers(VkCommandBuffer *buffers, uint32_t const n);

                void free_single_command_buffer(CommandBuffer &buffer);
            };

            class JLTAPI CommandBuffer {
                Renderer const &m_renderer;
                VkCommandBuffer m_buffer;
                bool m_primary;

              public:
                /**
                 * Create a new command buffer.
                 *
                 * @param renderer The Renderer instance.
                 * @param buffer The command buffer handle.
                 * @param primary True if the command buffer is primary.
                 *
                 * @remarks To allocate a new command buffer use
                 * `CommandPool::allocate_command_buffers()` instead of calling this
                 * constructor manually.
                 */
                CommandBuffer(Renderer const &renderer, VkCommandBuffer buffer, bool primary) :
                  m_renderer{renderer}, m_buffer{buffer}, m_primary{primary} {}

                CommandBuffer(CommandBuffer const &other) = delete;
                CommandBuffer(CommandBuffer &&) = default;

                Renderer const &get_renderer() const { return m_renderer; }
                VkCommandBuffer get_buffer() const { return m_buffer; }

                /**
                 * Reset the command buffer to its initial state.
                 *
                 * @param release_resources True to also release resources back to the command pool.
                 */
                void reset(bool release_resources);

                bool is_primary() const { return m_primary; }

                void begin_record(
                  VkCommandBufferUsageFlags flags = 0, CommandBufferRecordParams *const params = nullptr);
                void end_record();

                void cmd_begin_render_pass(
                  bool const inline_commands, VkClearValue const *const clearColor = nullptr);
                void cmd_end_render_pass();

                void submit(VkQueue const queue, ActionSynchro const &synchro);

                operator VkCommandBuffer() const { return m_buffer; }
            };
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_CMD_HPP */
