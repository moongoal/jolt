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
            class BufferDownloadOp;

            union TransferSubject {
                VkBuffer buffer;
                VkImage image;
            };

            union TransferData {
                void *data;
                void const *const_data;
            };

            struct Transfer {
                TransferData data;                   //< Pointer to the host-side buffer.
                VkDeviceSize size;                   //< Size of the data to transfer.
                TransferSubject subject;             //< Handle of the device-side object.
                VkDeviceSize offset;                 //< Transfer subject base offset.
                VkQueue src_queue;                   //< Source queue for the initial ownership transfer.
                VkQueue dst_queue;                   //< Destination queue for the final ownership transfer.
                VkPipelineStageFlags src_stage_mask; //< Source stage mask for the first execution barrier.
                VkPipelineStageFlags dst_stage_mask; //< Desination stage mask for the last execution barrier.
                VkAccessFlags src_access_mask;       //< Source access mask for the first memory barrier.
                VkAccessFlags dst_access_mask;       //< Destination access mask for the last memory barrier.
            };

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
                StagingBuffer(Renderer const &renderer, VkQueue const queue, VkDeviceSize const size);
                StagingBuffer(StagingBuffer const &) = delete;

                ~StagingBuffer() { dispose(); }

                BufferUploadOp upload_buffer(Transfer const &transfer);
                BufferDownloadOp download_buffer(Transfer const &transfer);

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
                VkDeviceMemory get_memory() const { return m_memory; }
                CommandPool &get_command_pool() { return m_cmdpool; }
                VkBuffer get_buffer() const { return m_buffer; }
                VkQueue get_queue() const { return m_queue; }
                Fence &get_fence() { return m_fence; }
            };

            class TransferOp {
              protected:
                StagingBuffer &m_staging;       //< The staging buffer objet owner of this transfer operation.
                Transfer const m_xfer;          //< The transfer settings.
                bool m_finished;                //< True if the transfer is complete, false if it is ongoing.
                CommandBuffer m_cmdbuf;         //< The command buffer used to transfer the data.
                uint32_t const m_src_q_fam_idx; /*< Family index for the queue that has the
                                             ownership of the target buffer before the transfer. */
                uint32_t const m_dst_q_fam_idx; /*< Family index for the queue that will take
                                             ownership of the target buffer after the transfer. */
                VkDeviceSize m_cur_offset;      //< Current transfer offset.

                bool was_moved() const { return m_cur_offset == JLT_VULKAN_INVALIDSZ; }

                TransferOp(StagingBuffer &staging, Transfer const &transfer);
                TransferOp(TransferOp const &) = delete;
                TransferOp(TransferOp &&other);

                virtual ~TransferOp();
            };

            class JLTAPI UploadOp : public TransferOp {
              protected:
                VkDeviceSize host_begin_next_transfer();

                /**
                 * Copy the contents of the staging buffer to their destination location and
                 * complete the queue family transfer.
                 *
                 * @param last_chunk True if this chunk is the last one.
                 * @param q_fam_idx The queue family index of the queue that will use the data next.
                 * @param bufsz The size of the data contained in the staging buffer.
                 */
                virtual void device_copy_to_destination(
                  bool const last_chunk, uint32_t const q_fam_idx, VkDeviceSize const bufsz) = 0;

              public:
                /**
                 * Create a new transfer operation.
                 *
                 * @param staging The staging buffer object that creates this operation.
                 * @param transfer The transfer settings.
                 */
                UploadOp(StagingBuffer &staging, Transfer const &transfer);
                UploadOp(UploadOp const &) = delete;
                UploadOp(UploadOp &&) = default;

                /**
                 * Wait until the whole transfer completes.
                 */
                void transfer() {
                    while(!transfer_single_block()) {}
                }
                /**
                 * Transfer the next block of data.
                 *
                 * @return True if the transfer is finished, false if `transfer_single_block()`
                 * needs to be called again to send more data.
                 */
                bool transfer_single_block();
            };

            class JLTAPI BufferUploadOp : public UploadOp {
              protected:
                virtual void device_copy_to_destination(
                  bool const last_chunk, uint32_t const q_fam_idx, VkDeviceSize const bufsz);

              public:
                /**
                 * Create a new buffer transfer operation.
                 *
                 * @param staging The staging buffer that creates this operation.
                 * @param transfer The transfer settings.
                 */
                BufferUploadOp(StagingBuffer &staging, Transfer const &transfer);
                BufferUploadOp(BufferUploadOp const &) = delete;
                BufferUploadOp(BufferUploadOp &&) = default;
            };

            class JLTAPI DownloadOp : public TransferOp {
                uint32_t m_bufsz; //< Buffer size for last transfer operation.

                virtual VkDeviceSize device_begin_next_transfer(uint32_t const q_fam_idx) = 0;

                /**
                 * Copy the contents of the staging buffer to their destination location.
                 *
                 * @param bufsz The size of the data contained in the staging buffer.
                 */
                void host_copy_to_destination(VkDeviceSize const bufsz);

              public:
                /**
                 * Create a new transfer operation.
                 *
                 * @param staging The staging buffer object that creates this operation.
                 * @param transfer The transfer settings.
                 */
                DownloadOp(StagingBuffer &staging, Transfer const &transfer);
                DownloadOp(DownloadOp const &) = delete;
                DownloadOp(DownloadOp &&other);

                /**
                 * Wait until the whole transfer completes.
                 */
                void transfer() {
                    while(!transfer_single_block()) {}
                }
                /**
                 * Transfer the next block of data.
                 *
                 * @return True if the transfer is finished, false if `transfer_single_block()`
                 * needs to be called again to send more data.
                 */
                bool transfer_single_block();
            };

            class JLTAPI BufferDownloadOp : public DownloadOp {
              protected:
                virtual VkDeviceSize device_begin_next_transfer(uint32_t const q_fam_idx);

              public:
                /**
                 * Create a new buffer transfer operation.
                 *
                 * @param staging The staging buffer object that creates this operation.
                 * @param transfer The transfer settings.
                 */
                BufferDownloadOp(StagingBuffer &staging, Transfer const &transfer);
                BufferDownloadOp(BufferDownloadOp const &) = delete;
                BufferDownloadOp(BufferDownloadOp &&) = default;
            };
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_TRANSFER_HPP */
