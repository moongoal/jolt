#ifndef JLT_GRAPHICS_VULKAN_TRANSFER_HPP
#define JLT_GRAPHICS_VULKAN_TRANSFER_HPP

#include <jolt/collections/vector.hpp>
#include "defs.hpp"
#include "transfer-descriptor.hpp"
#include "cmd.hpp"
#include "staging-buffer.hpp"
#include "synchro.hpp"

namespace jolt::graphics::vulkan {
    class Renderer;

    class JLTAPI Transfer {
        friend class TransferFactory;

      public:
        using transfer_descriptors = collections::Vector<TransferDescriptor>;

      private:
        Renderer &m_renderer;
        VkQueue m_queue;
        uint32_t m_queue_family_index;
        CommandPool m_cmd_pool;
        StagingBuffer *m_staging_buffer;
        transfer_descriptors m_descriptors;
        Fence m_fence;
        CommandBuffer m_cmd_buffer;

        void add_resource(TransferDescriptor const &descriptor) { m_descriptors.push(descriptor); }

        void create_staging_buffer();

        virtual void transfer_image(TransferDescriptor const &descriptor) = 0;
        virtual void transfer_buffer(TransferDescriptor const &descriptor) = 0;
        virtual void transfer_begin() {}
        virtual void transfer_end() {}

      protected:
        transfer_descriptors &get_descriptors() { return m_descriptors; }
        CommandBuffer &get_command_buffer() { return m_cmd_buffer; }
        Fence &get_fence() { return m_fence; }

      public:
        Transfer(Renderer &renderer, VkQueue queue);
        Transfer(Transfer const &) = delete;
        Transfer(Transfer &&other);

        virtual ~Transfer() { dispose(); }

        bool transfer_next();
        void transfer_all();

        Renderer const &get_renderer() const { return m_renderer; }
        VkQueue get_queue() const { return m_queue; }
        uint32_t get_queue_family_index() const { return m_queue_family_index; }
        StagingBuffer const *get_staging_buffer() const { return m_staging_buffer; }
        CommandPool const &get_command_pool() const { return m_cmd_pool; }
        StagingBuffer *get_staging_buffer() { return m_staging_buffer; }
        CommandPool &get_command_pool() { return m_cmd_pool; }

        void dispose();
    };
} // namespace jolt::graphics::vulkan

#endif /* JLT_GRAPHICS_VULKAN_TRANSFER_HPP */
