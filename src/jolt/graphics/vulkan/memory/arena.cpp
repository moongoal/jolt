#include <jolt/util.hpp>
#include <jolt/graphics/vulkan.hpp>
#include "arena.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            namespace memory {
                DeviceAlloc Arena::allocate(VkDeviceSize const size, VkDeviceSize const alignment) {
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

                            return DeviceAlloc{get_buffer(), ret_ptr, total_size};
                        }
                    }

                    return InvalidDeviceAlloc;
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
            } // namespace memory
        }     // namespace vulkan
    }         // namespace graphics
} // namespace jolt
