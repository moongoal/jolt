#include <jolt/graphics/vulkan.hpp>
#include "staging-buffer.hpp"

namespace jolt::graphics::vulkan {
    StagingBuffer::StagingBuffer(
      Renderer const &renderer, uint32_t const q_fam_idx, VkDeviceSize const size) :
      m_renderer{renderer},
      m_buffer_size{size} {
        initialize(q_fam_idx);
    }

    void StagingBuffer::initialize(uint32_t const q_fam_idx) {
        VkBufferCreateInfo cinfo{
          VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,                                // sType
          nullptr,                                                             // pNext
          0,                                                                   // flags
          m_buffer_size,                                                       // size
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, // usage
          VK_SHARING_MODE_EXCLUSIVE,                                           // sharingMode
          1,                                                                   // queueFamilyIndexCount
          &q_fam_idx                                                           // pQueueFamilyIndices
        };

        VkResult result = vkCreateBuffer(m_renderer.get_device(), &cinfo, get_vulkan_allocator(), &m_buffer);
        jltassert2(result == VK_SUCCESS, "Unable to create staging buffer");

        VkMemoryRequirements mem_req;

        vkGetBufferMemoryRequirements(m_renderer.get_device(), m_buffer, &mem_req);

        uint32_t mem_type_idx = choose_memory_type(mem_req.memoryTypeBits);
        mem_type_idx &= 0x0000ffffUL;

        VkMemoryAllocateInfo ainfo{
          VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // sType
          nullptr,                                // pNext
          mem_req.size,                           // allocationSize
          mem_type_idx                            // memoryTypeIndex
        };

        result = vkAllocateMemory(m_renderer.get_device(), &ainfo, get_vulkan_allocator(), &m_memory);

        result = vkBindBufferMemory(m_renderer.get_device(), m_buffer, m_memory, 0);
        jltassert2(result == VK_SUCCESS, "Unable to bind staging buffer memory");

        result = vkMapMemory(m_renderer.get_device(), m_memory, 0, m_buffer_size, 0, &m_map_ptr);
        jltassert2(result == VK_SUCCESS, "Unable to map staging buffer memory");
    }

    uint32_t StagingBuffer::choose_memory_type(uint32_t const mem_req_bits) const {
        uint32_t mt = m_renderer.get_memory_type_index(
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT, 0, mem_req_bits);

        if(mt != JLT_VULKAN_INVALID32) {
            return mt;
        }

        mt = m_renderer.get_memory_type_index(
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          0,
          mem_req_bits);

        if(mt != JLT_VULKAN_INVALID32) {
            return mt | COHERENT_BIT;
        }

        // Directly return as the standard guarantees the presence of a memory type with
        // these requirements
        return m_renderer.get_memory_type_index(
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0, mem_req_bits)
               | COHERENT_BIT;
    }

    void StagingBuffer::dispose() {
        if(m_map_ptr) {
            vkUnmapMemory(m_renderer.get_device(), m_memory);
            m_map_ptr = nullptr;
        }

        if(m_buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_renderer.get_device(), m_buffer, get_vulkan_allocator());
            m_buffer = VK_NULL_HANDLE;
        }

        if(m_memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_renderer.get_device(), m_memory, get_vulkan_allocator());
            m_memory = VK_NULL_HANDLE;
        }
    }

    void StagingBuffer::upload(void const *const data_ptr, uint32_t const data_sz) {
        jltassert2(data_sz <= m_buffer_size, "Object too big for staging buffer");

        memcpy(get_host_buffer(), data_ptr, data_sz);

        if(!is_coherent()) {
            VkMappedMemoryRange mrange{
              VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, // sType
              nullptr,                               // pNext
              get_memory(),                          // memory
              0,                                     // offset
              VK_WHOLE_SIZE                          // size
            };

            VkResult result = vkFlushMappedMemoryRanges(get_renderer().get_device(), 1, &mrange);
            jltassert2(result == VK_SUCCESS, "Unable to flush staging buffer");
        }
    }

    void StagingBuffer::download(void *const data_out_ptr, uint32_t const data_sz) {
        if(!is_coherent()) {
            VkMappedMemoryRange memrange{
              VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, // sType
              nullptr,                               // pNext
              get_memory(),                          // memory
              0,                                     // offset
              VK_WHOLE_SIZE                          // size
            };

            VkResult result = vkInvalidateMappedMemoryRanges(get_renderer().get_device(), 1, &memrange);
            jltassert2(result == VK_SUCCESS, "Unable to invalidate staging buffer");
        }

        memcpy(data_out_ptr, get_host_buffer(), data_sz);
    }
} // namespace jolt::graphics::vulkan
