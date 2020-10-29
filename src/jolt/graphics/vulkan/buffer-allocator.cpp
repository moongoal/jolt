#include <utility>
#include <jolt/graphics/vulkan.hpp>

namespace jolt::graphics::vulkan {
    BufferAllocator::BufferAllocator(BufferAllocator &&other) :
      m_renderer{other.m_renderer}, m_unused_buffers{std::move(other.m_unused_buffers)} {
        other.m_moved = true;
    }

    BufferAllocator::~BufferAllocator() {
        if(!m_moved) {
            recycle();
        }
    }

    Buffer BufferAllocator::allocate(
      VkDeviceSize const size, VkMemoryPropertyFlags const mem_flags, VkBufferUsageFlags const usage) {
        Buffer *const compat = find_compatible_buffer(size, mem_flags, usage);

        if(compat && compat->get_size() <= MAX_SIZE_COMPATIBILITY_FACTOR * size) {
            Buffer b = *compat;

            m_unused_buffers.remove(*compat);

            return b;
        }

        return create_buffer(size, mem_flags, usage);
    }

    void BufferAllocator::free(Buffer const &buffer) { m_unused_buffers.push(buffer); }

    void BufferAllocator::recycle() {
        for(auto const &buffer : m_unused_buffers) { destroy_buffer(buffer); }

        m_unused_buffers.clear();
    }

    Buffer BufferAllocator::create_buffer(
      VkDeviceSize const size, VkMemoryPropertyFlags const mem_flags, VkBufferUsageFlags const usage) {
        VkDeviceMemory memory;
        VkBuffer buffer;

        VkBufferCreateInfo cinfo{
          VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // sType
          nullptr,                              // pNext
          0,                                    // flags
          size,                                 // size
          usage,                                // usage
          VK_SHARING_MODE_EXCLUSIVE,            // sharingMode
          0,                                    // queueFamilyIndexCount
          nullptr                               // nQueueFamilyIndices
        };

        VkResult result = vkCreateBuffer(m_renderer.get_device(), &cinfo, get_vulkan_allocator(), &buffer);
        jltassert2(result == VK_SUCCESS, "Unable to create buffer");

        VkMemoryRequirements mem_req;

        vkGetBufferMemoryRequirements(m_renderer.get_device(), buffer, &mem_req);

        uint32_t mem_type_index = m_renderer.get_memory_type_index(mem_flags, 0, mem_req.memoryTypeBits);
        jltassert2(mem_type_index != JLT_VULKAN_INVALID32, "Invalid memory type index for buffer");

        VkMemoryAllocateInfo ainfo{
          VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // sType
          nullptr,                                // pNext
          mem_req.size,                           // allocationSize
          mem_type_index,                         // memoryTypeIndex
        };

        result = vkAllocateMemory(m_renderer.get_device(), &ainfo, get_vulkan_allocator(), &memory);
        jltassert2(result == VK_SUCCESS, "Unable to allocate buffer memory");

        result = vkBindBufferMemory(m_renderer.get_device(), buffer, memory, 0);
        jltassert2(result == VK_SUCCESS, "Unable to bind buffer memory");

        return Buffer{memory, buffer, mem_req.size, mem_flags, usage};
    }

    void BufferAllocator::destroy_buffer(Buffer const &buffer) {
        vkDestroyBuffer(m_renderer.get_device(), buffer.get_buffer(), get_vulkan_allocator());
        vkFreeMemory(m_renderer.get_device(), buffer.get_memory(), get_vulkan_allocator());
    }

    Buffer *BufferAllocator::find_compatible_buffer(
      VkDeviceSize const size, VkMemoryPropertyFlags const mem_flags, VkBufferUsageFlags const usage) {
        Buffer *candidate = nullptr;

        for(Buffer &b : m_unused_buffers) {
            if(
              (b.get_memory_property_flags() & mem_flags) == mem_flags && b.get_size() >= size
              && (b.get_usage() & usage) == usage) {
                if(candidate == nullptr || b.get_size() < candidate->get_size()) {
                    candidate = &b;
                }
            }
        }

        return candidate;
    }
} // namespace jolt::graphics::vulkan
