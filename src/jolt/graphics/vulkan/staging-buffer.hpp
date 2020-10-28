#ifndef JLT_GRAPHICS_VULKAN_TRANSFER_STAGING_BUFFER_HPP
#define JLT_GRAPHICS_VULKAN_TRANSFER_STAGING_BUFFER_HPP

#include <jolt/graphics/vulkan/defs.hpp>

namespace jolt::graphics::vulkan {
    class Renderer;
    class BufferUploadOp;
    class BufferDownloadOp;

    /**
     * A data transfer facility that leverages a staging buffer.
     */
    class JLTAPI StagingBuffer {
        static constexpr const uint32_t COHERENT_BIT = 0x00010000;

        Renderer const &m_renderer;
        VkDeviceSize const m_buffer_size;
        VkBuffer m_buffer;
        VkDeviceMemory m_memory;
        void *m_map_ptr;
        bool m_memory_is_coherent;

        void initialize(uint32_t const q_fam_idx);

        /**
         * Choose and return a proper memory type.
         *
         * @return The best memory type index available for the task, masked with
         * `COHERENT_BIT` if the memory type has coherency.
         */
        uint32_t choose_memory_type(uint32_t const mem_req_bits) const;

      public:
        /**
         * Create a new staging buffer object.
         *
         * @param renderer The renderer.
         * @param q_fam_idx The index of the queue family that will be used to transfer the data.
         * @param size The size of the staging buffer.
         */
        StagingBuffer(Renderer const &renderer, uint32_t const q_fam_idx, VkDeviceSize const size);
        StagingBuffer(StagingBuffer const &) = delete;

        ~StagingBuffer() { dispose(); }

        /**
         * Release any resources associated with the object.
         *
         * @remarks This function is automatically called upon object destruction.
         */
        void dispose();

        void *get_host_buffer() { return m_map_ptr; }
        VkDeviceSize get_buffer_size() const { return m_buffer_size; }
        bool is_coherent() const { return m_memory_is_coherent; }
        Renderer const &get_renderer() const { return m_renderer; }
        VkDeviceMemory get_memory() const { return m_memory; }
        VkBuffer get_device_buffer() const { return m_buffer; }

        void upload(void const *const data_ptr, uint32_t const data_sz);
        void download(void *const data_out_ptr, uint32_t const data_sz);
    };
} // namespace jolt::graphics::vulkan

#endif /* JLT_GRAPHICS_VULKAN_TRANSFER_STAGING_BUFFER_HPP */
