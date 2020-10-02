#ifndef JLT_GRAPHICS_VULKANCMD_HPP
#define JLT_GRAPHICS_VULKANCMD_HPP

#include "defs.hpp"

#include <limits>
#include <Windows.h>
#include <vulkan/vulkan.h>
#include <jolt/util.hpp>

namespace jolt {
    namespace graphics {
        class VulkanRenderer;
        class VulkanCommandBuffer;
        class VulkanFence;
        class VulkanSemaphore;
        struct VulkanActionSynchro;

        /**
         * Command buffer record params.
         *
         * @see
         * https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/chap6.html#VkCommandBufferBeginInfo
         */
        struct VulkanCommandBufferRecordParams {
            static constexpr const uint32_t INVALID_SUBPASS = std::numeric_limits<uint32_t>::max();

            uint32_t subpass;
            bool occlusion_query_enable = VK_FALSE;
            VkQueryControlFlags query_flags = 0;
            VkQueryPipelineStatisticFlags pipeline_statistics = 0;

            explicit VulkanCommandBufferRecordParams(uint32_t spass = INVALID_SUBPASS) :
              subpass{spass} {}
        };

        class JLTAPI VulkanCommandPool {
            VulkanRenderer const &m_renderer;
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
            VulkanCommandPool(
              VulkanRenderer const &renderer,
              bool transient,
              bool allow_reset,
              uint32_t queue_fam_index) :
              m_renderer{renderer} {
                initialize(transient, allow_reset, queue_fam_index);
            }

            VulkanCommandPool(VulkanCommandPool const &other) = delete;

            ~VulkanCommandPool() { dispose(); }

            VulkanRenderer const &get_renderer() const { return m_renderer; }
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
             * Resets the command pool, gets back all allocated resources from the command buffers
             * back to the command pool.
             *
             * @param release_resources If true, also release the resources from the command pool
             * back to the system.
             */
            void reset(bool release_resources);

            /**
             * Allocate new command buffers.
             *
             * @param out_buffers Memory region that will be populated by the newly allocated `n`
             * command buffers.
             * @param n The number of command buffers to allocate.
             * @param primary True if the allocated command buffers will be primary, false if
             * they'll be secondary.
             *
             * @remarks Primary command buffers are directly executable, secondary command buffers
             * can only be executed by a primary command buffer.
             */
            void allocate_command_buffers(
              VulkanCommandBuffer *out_buffers, uint32_t const n, bool const primary);

            /**
             * Allocate new command buffer.
             *
             * @param primary True if the allocated command buffer will be primary, false if
             * it'll be secondary.
             *
             * @return The newly allocated command buffer.
             *
             * @remarks Primary command buffers are directly executable, secondary command buffers
             * can only be executed by a primary command buffer.
             */
            VulkanCommandBuffer allocate_single_command_buffer(bool const primary);

            void free_command_buffers(VulkanCommandBuffer *buffers, uint32_t const n);

            void free_single_command_buffer(VulkanCommandBuffer &buffer);
        };

        class JLTAPI VulkanCommandBuffer {
            VulkanRenderer const &m_renderer;
            VkCommandBuffer m_buffer;
            bool m_primary;

          public:
            /**
             * Create a new command buffer.
             *
             * @param renderer The VulkanRenderer instance.
             * @param buffer The command buffer handle.
             * @param primary True if the command buffer is primary.
             *
             * @remarks To allocate a new command buffer use
             * `VulkanCommandPool::allocate_command_buffers()` instead of calling this constructor
             * manually.
             */
            VulkanCommandBuffer(
              VulkanRenderer const &renderer, VkCommandBuffer buffer, bool primary) :
              m_renderer{renderer},
              m_buffer{buffer}, m_primary{primary} {}

            VulkanCommandBuffer(VulkanCommandBuffer const &other) = delete;

            VkCommandBuffer get_buffer() const { return m_buffer; }

            /**
             * Reset the command buffer to its initial state.
             *
             * @param release_resources True to also release resources back to the command pool.
             */
            void reset(bool release_resources);

            bool is_primary() const { return m_primary; }

            void begin_record(
              VkCommandBufferUsageFlags flags = 0,
              VulkanCommandBufferRecordParams *const params = nullptr);
            void end_record();

            void cmd_begin_render_pass(bool const inline_commands);
            void cmd_end_render_pass();

            void submit(VkQueue const queue, VulkanActionSynchro const &synchro);
        };
    } // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKANCMD_HPP */
