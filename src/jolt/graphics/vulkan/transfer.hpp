#ifndef JLT_GRAPHICS_VULKAN_TRANSFER_HPP
#define JLT_GRAPHICS_VULKAN_TRANSFER_HPP

#include "defs.hpp"
#include "synchro.hpp"
#include "cmd.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            class Renderer;
            class UploadOp;
            class BufferUploadOp;

            /**
             * A data transfer facility that leverages a staging buffer.
             */
            class JLTAPI StagingBuffer {
                static constexpr const uint32_t COHERENT_BIT = 0x00010000;
                Renderer const &m_renderer;
                VkQueue const m_queue;
                VkDeviceSize const m_buffer_size;
                VkBuffer m_buffer;
                VkDeviceMemory m_memory;
                void *m_map_ptr;
                bool m_memory_is_coherent;
                Fence m_fence;
                CommandPool m_cmdpool;

                void initialize();

                /**
                 * Choose and return a proper memory type.
                 *
                 * @return The best memory type index available for the task, masked with
                 * `COHERENT_BIT` if the memory type has coherency.
                 */
                uint32_t choose_memory_type() const;

              public:
                /**
                 * Create a new transfer object.
                 *
                 * @param renderer The renderer.
                 * @param queue The queue that will be used to transfer the data.
                 * @param size The size of the staging buffer.
                 */
                StagingBuffer(
                  Renderer const &renderer, VkQueue const queue, VkDeviceSize const size);
                StagingBuffer(StagingBuffer const &) = delete;

                ~StagingBuffer() { dispose(); }

                BufferUploadOp transfer_to_buffer(
                  const void *const data,
                  VkDeviceSize const size,
                  VkBuffer const buffer,
                  VkDeviceSize const offset,
                  VkQueue const queue,
                  VkPipelineStageFlags const dst_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

                /**
                 * Release any resources associated with the object.
                 *
                 * @remarks This function is automatically called upon object destruction.
                 */
                void dispose();

                void *get_buffer_ptr() { return m_map_ptr; }
                VkDeviceSize get_buffer_size() const { return m_buffer_size; }
                bool is_coherent() const { return m_memory_is_coherent; }
                Renderer const &get_renderer() const { return m_renderer; }
                VkDeviceMemory const get_memory() const { return m_memory; }
                CommandPool &get_command_pool() { return m_cmdpool; }
                VkBuffer get_buffer() const { return m_buffer; }
                VkQueue get_queue() const { return m_queue; }
                Fence &get_fence() { return m_fence; }
            };

            class JLTAPI UploadOp {
                uint8_t const *m_ptr; //< The pointer to the host source memory to transfer.
                bool m_finished;      //< True if the transfer is complete, false if it is ongoing.

              protected:
                VkDeviceSize const m_total_size; //< The total size of the transfer operation.
                StagingBuffer &m_transfer; //< The transfer objet owner of this transfer operation.
                CommandBuffer m_cmdbuf;    //< The command buffer used to transfer the data.
                uint32_t m_tgt_q_fam_idx; /*< Family index for the queue that will take ownership of
                                             the target buffer after the transfer. */
                VkDeviceSize m_offset; //< The current offset from `m_ptr` for the next chunk copy.
                VkPipelineStageFlags const
                  m_dst_stage_mask; //< Desination stage mask for the last execution barrier.

                VkDeviceSize host_begin_next_transfer();

                virtual void device_copy_to_destination(
                  bool const last_chunk, uint32_t const q_fam_idx, VkDeviceSize const bufsz) = 0;

              public:
                /**
                 * Create a new transfer operation.
                 *
                 * @param transfer The transfer object that creates this operation.
                 * @param ptr Pointer to the host data to transfer.
                 * @param size Size of the data to transfer.
                 * @param tgt_queue Target queue for ownership transfer.
                 * @param dst_stage_mask Desination stage mask for the last execution barrier.
                 */
                UploadOp(
                  StagingBuffer &transfer,
                  void const *const ptr,
                  VkDeviceSize const size,
                  VkQueue const tgt_queue,
                  VkPipelineStageFlags const dst_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
                UploadOp(UploadOp const &) = delete;
                UploadOp(UploadOp &&);

                ~UploadOp();

                void transfer() {
                    while(!transfer_single_block()) {}
                }

                bool transfer_single_block();
            };

            class JLTAPI BufferUploadOp : public UploadOp {
                VkBuffer m_tgt_buffer; //< The destination buffer.

              protected:
                virtual void device_copy_to_destination(
                  bool const last_chunk, uint32_t const q_fam_idx, VkDeviceSize const bufsz);

              public:
                /**
                 * Create a new buffer transfer operation.
                 *
                 * @param transfer The transfer object that creates this operation.
                 * @param ptr Pointer to the host data to transfer.
                 * @param size Size of the data to transfer.
                 * @param tgt_queue Target queue for ownership transfer.
                 * @param tgt_buffer The target buffer where to send the transferred data.
                 * @param dst_stage_mask Desination stage mask for the last execution barrier.
                 */
                BufferUploadOp(
                  StagingBuffer &transfer,
                  void const *const ptr,
                  VkDeviceSize const size,
                  VkQueue const tgt_queue,
                  VkBuffer const tgt_buffer,
                  VkPipelineStageFlags const dst_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
                BufferUploadOp(BufferUploadOp const &) = delete;
                BufferUploadOp(BufferUploadOp &&other);
            };
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_TRANSFER_HPP */
