#include <jolt/util.hpp>
#include <jolt/graphics/vulkan.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            template class DeviceAlloc<VkBuffer>;
            template class DeviceAlloc<VkImage>;

            BufferDeviceAlloc const InvalidBufferDeviceAlloc{VK_NULL_HANDLE, 0, 0};
            ImageDeviceAlloc const InvalidImageDeviceAlloc{VK_NULL_HANDLE, 0, 0};

            MemoryHeap::MemoryHeap(
              Renderer const &renderer, VkDeviceSize const size, VkMemoryPropertyFlags const mem_flags) :
              m_renderer{renderer},
              m_size{size} {
                jltassert2(size < renderer.get_max_alloc_size(), "Allocation size too large");
                allocate(mem_flags);
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

            void MemoryHeap::dispose() {
                if(m_memory) {
                    vkFreeMemory(m_renderer.get_device(), m_memory, get_vulkan_allocator());
                    m_memory = VK_NULL_HANDLE;
                }
            }

            BufferDeviceAlloc Arena::allocate(VkDeviceSize const size, VkDeviceSize const alignment) {
                for(auto it = m_freelist.begin(), end = m_freelist.end(); it != end; ++it) {
                    FreeListNode &node = *it;
                    VkDeviceSize const alloc_ptr = node.m_base;
                    VkDeviceSize ret_ptr = reinterpret_cast<VkDeviceSize>(
                      align_raw_ptr(reinterpret_cast<void *>(alloc_ptr), alignment));
                    VkDeviceSize padding = ret_ptr - alloc_ptr;
                    VkDeviceSize total_size = size + padding;

                    if(node.m_size >= total_size) {
                        node.m_base += total_size;
                        node.m_size -= total_size;

                        m_allocs.add(ret_ptr, {total_size, padding});

                        if(node.m_size == 0) {
                            m_freelist.remove(*it.get_pointer());
                        }

                        m_total_alloc_size += total_size;

                        return BufferDeviceAlloc{m_buffer, ret_ptr, total_size};
                    }
                }

                return InvalidBufferDeviceAlloc;
            }

            Arena::~Arena() {
                if(is_bound()) {
                    vkDestroyBuffer(get_renderer().get_device(), m_buffer, get_vulkan_allocator());
                }
            }

            void Arena::initialize(VkBufferUsageFlags const usage) {
                if(usage != 0) {
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
                } else {
                    m_buffer = VK_NULL_HANDLE;
                }
            }

            void Arena::free(VkDeviceSize const ptr) {
                AllocMetadata *meta = m_allocs.get_value(ptr);

                jltassert2(meta, "Attempting to free an unallocated memory location");

                VkDeviceSize const size = meta->m_size;
                VkDeviceSize const real_ptr = ptr - meta->m_padding;
                free_list::Node *prev_closest = find_prev_closest_node(real_ptr);
                free_list::Node *next_closest = m_freelist.get_first_node();

                m_total_alloc_size -= size;
                m_allocs.remove(ptr);

                if(prev_closest) {
                    FreeListNode &node = prev_closest->get_value();
                    VkDeviceSize node_end = node.m_base + node.m_size;
                    next_closest = prev_closest->get_next();

                    if(node_end == real_ptr) {
                        node.m_size += size;
                        node_end += size;

                        if(next_closest) { // May need to merge
                            FreeListNode &next = next_closest->get_value();

                            if(node_end == next.m_base) { // Merge adjacent nodes
                                node.m_size += next.m_size;
                                m_freelist.remove(*next_closest);
                            }
                        }
                        return;
                    }
                }

                if(next_closest) {
                    FreeListNode &node = next_closest->get_value();

                    if(node.m_base == real_ptr + size) {
                        node.m_base -= size;
                        node.m_size += size;
                        return;
                    }
                }

                m_freelist.add_after({real_ptr, size}, prev_closest);
            }

            Arena::free_list::Node *Arena::find_prev_closest_node(VkDeviceSize const ptr) {
                for(auto it = m_freelist.begin(), end = m_freelist.end(); it != end; ++it) {
                    FreeListNode const &node = *it;

                    if(node.m_base + node.m_size <= ptr) {
                        free_list::Node *next = it.get_pointer()->get_next();

                        if(!next || next->get_value().m_base > ptr) {
                            return it.get_pointer();
                        }
                    }
                }

                return nullptr;
            }
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt
