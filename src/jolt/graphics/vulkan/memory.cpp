#include <jolt/util.hpp>
#include <jolt/graphics/vulkan.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            void MemoryHeap::allocate(VkMemoryPropertyFlags const mem_flags) {
                uint32_t mem_type_index = find_memory_type(mem_flags);

                jltassert2(
                  mem_type_index != JLT_VULKAN_INVALID32,
                  "Unable to satisfy memory heap requirements");

                VkMemoryAllocateInfo ainfo{
                  VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // sType
                  nullptr,                                // pNext
                  m_size,                                 // size
                  mem_type_index                          // memoryTypeIndex
                };

                VkResult result = vkAllocateMemory(
                  m_renderer.get_device(), &ainfo, get_vulkan_allocator(), &m_memory);
                jltassert2(result == VK_SUCCESS, "Unable to allocate device memory");
            }

            void MemoryHeap::dispose() {
                if(m_memory) {
                    vkFreeMemory(m_renderer.get_device(), m_memory, get_vulkan_allocator());
                    m_memory = VK_NULL_HANDLE;
                }
            }

            uint32_t MemoryHeap::find_memory_type(VkMemoryPropertyFlags const mem_flags) {
                VkPhysicalDeviceMemoryProperties const &mem_props =
                  m_renderer.get_phy_device_memory_properties();

                for(uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
                    VkMemoryType const &mem_type = mem_props.memoryTypes[i];

                    if(mem_type.propertyFlags & mem_flags) {
                        return i;
                    }
                }

                return JLT_VULKAN_INVALID32;
            }

            VkDeviceSize Arena::bind_to_buffer(
              VkBuffer const buffer, VkDeviceSize const size, VkDeviceSize const alignment) {
                VkDeviceSize offset = allocate(size, alignment);

                if(offset != JLT_VULKAN_INVALIDSZ) {
                    VkResult result =
                      vkBindBufferMemory(get_renderer().get_device(), buffer, get_base(), offset);
                    jltassert2(result == VK_SUCCESS, "Unable to bind buffer memory");

                    return offset;
                }

                return JLT_VULKAN_INVALIDSZ;
            }

            VkDeviceSize Arena::bind_to_image(
              VkImage const image, VkDeviceSize const size, VkDeviceSize const alignment) {
                VkDeviceSize offset = allocate(size, alignment);

                if(offset != JLT_VULKAN_INVALIDSZ) {
                    VkResult result =
                      vkBindImageMemory(get_renderer().get_device(), image, get_base(), offset);
                    jltassert2(result == VK_SUCCESS, "Unable to bind image memory");

                    return offset;
                }

                return JLT_VULKAN_INVALIDSZ;
            }

            VkDeviceSize
            Arena::allocate(VkDeviceSize const size, VkDeviceSize const alignment) {
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

                        return ret_ptr;
                    }
                }

                return JLT_VULKAN_INVALIDSZ;
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

            Arena::free_list::Node *
            Arena::find_prev_closest_node(VkDeviceSize const ptr) {
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
