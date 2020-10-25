#include <jolt/graphics/vulkan.hpp>
#include "heap.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            namespace memory {
                MemoryHeap::MemoryHeap(
                  Renderer const &renderer,
                  VkDeviceSize const size,
                  VkMemoryPropertyFlags const mem_flags,
                  VkBufferUsageFlags const usage) :
                  m_renderer{renderer},
                  m_size{size} {
                    jltassert2(size < renderer.get_max_alloc_size(), "Allocation size too large");
                    allocate(mem_flags);

                    if(usage != 0) {
                        bind(usage);
                    } else {
                        m_buffer = VK_NULL_HANDLE;
                    }
                }

                void MemoryHeap::allocate(VkMemoryPropertyFlags const mem_flags) {
                    uint32_t mem_type_index = m_renderer.get_memory_type_index(mem_flags, 0);

                    jltassert2(
                      mem_type_index != JLT_VULKAN_INVALID32, "Unable to satisfy memory heap requirements");

                    VkMemoryAllocateInfo ainfo{
                      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // sType
                      nullptr,                                // pNext
                      m_size,                                 // size
                      mem_type_index                          // memoryTypeIndex
                    };

                    VkResult result =
                      vkAllocateMemory(m_renderer.get_device(), &ainfo, get_vulkan_allocator(), &m_memory);
                    jltassert2(result == VK_SUCCESS, "Unable to allocate device memory");
                }

                void MemoryHeap::bind(VkBufferUsageFlags const usage) {
                    VkBufferCreateInfo cinfo{
                      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // sType
                      nullptr,                              // pNext
                      0,                                    // flags
                      get_size(),                           // size
                      usage,                                // usage
                      VK_SHARING_MODE_EXCLUSIVE,            // sharingMode
                      0,                                    // queueFamilyIndexCount
                      nullptr                               // pQueueFamilyIndices
                    };

                    VkResult result =
                      vkCreateBuffer(get_renderer().get_device(), &cinfo, get_vulkan_allocator(), &m_buffer);
                    jltassert2(result == VK_SUCCESS, "Unable to create arena buffer");

                    result = vkBindBufferMemory(get_renderer().get_device(), m_buffer, get_base(), 0);
                    jltassert2(result == VK_SUCCESS, "Unable to bind arena buffer");
                }

                void MemoryHeap::dispose() {
                    if(m_buffer != VK_NULL_HANDLE) {
                        vkDestroyBuffer(get_renderer().get_device(), m_buffer, get_vulkan_allocator());
                        m_buffer = VK_NULL_HANDLE;
                    }

                    if(m_memory != VK_NULL_HANDLE) {
                        vkFreeMemory(m_renderer.get_device(), m_memory, get_vulkan_allocator());
                        m_memory = VK_NULL_HANDLE;
                    }
                }
            } // namespace memory
        }     // namespace vulkan
    }         // namespace graphics
} // namespace jolt
