#include <jolt/graphics/vulkan.hpp>
#include "transfer.hpp"

namespace jolt::graphics::vulkan {
    Transfer::Transfer(Renderer &renderer, VkQueue queue) :
      m_renderer{renderer}, m_queue{queue}, m_queue_family_index{m_renderer.get_queue_family_index(queue)},
      m_cmd_pool{renderer, true, true, m_queue_family_index}, m_staging_buffer{}, m_fence{renderer, true},
      m_cmd_buffer{m_cmd_pool.allocate_single_command_buffer(true)} {}

    Transfer::Transfer(Transfer &&other) :
      m_renderer{other.m_renderer}, m_queue{other.m_queue}, m_queue_family_index{other.m_queue_family_index},
      m_cmd_pool{std::move(other.m_cmd_pool)}, m_staging_buffer{other.m_staging_buffer},
      m_fence{std::move(other.m_fence)}, m_cmd_buffer{std::move(other.m_cmd_buffer)} {
        other.m_staging_buffer = nullptr;
    }

    void Transfer::transfer_all() {
        while(transfer_next())
            ;
    }

    void Transfer::dispose() {
        if(m_staging_buffer) {
            jltfree(m_staging_buffer);

            m_staging_buffer = nullptr;
        }
    }

    void Transfer::create_staging_buffer() {
        VkDeviceSize buf_sz = 0;

        // Allocate the biggest required staging buffer
        for(auto const &d : m_descriptors) {
            if(d.size > buf_sz) {
                buf_sz = d.size;
            }
        }

        m_staging_buffer = jltnew(StagingBuffer, m_renderer, m_queue_family_index, buf_sz);
    }

    bool Transfer::transfer_next() {
        if(!m_staging_buffer) {
            create_staging_buffer();

            transfer_begin();
        }

        transfer_descriptors &transfer_descriptors = get_descriptors();
        TransferDescriptor const descriptor = transfer_descriptors.pop();

        switch(descriptor.resource_type) {
            case TransferResourceType::Buffer:
                transfer_buffer(descriptor);
                break;

            case TransferResourceType::Image:
                transfer_image(descriptor);
                break;

            default:
#if _DEBUG
                console.err("Invalid transfer resource type");
#endif // _DEBUG
                abort();
        }

        bool const last_transfer = transfer_descriptors.get_length() == 0;

        if(last_transfer) {
            transfer_end();
        }

        return !last_transfer;
    }
} // namespace jolt::graphics::vulkan
