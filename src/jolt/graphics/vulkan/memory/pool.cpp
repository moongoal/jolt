#include "pool.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            namespace memory {
                void ObjectPool::initialize() {
                    uint64_t const rem = m_pool_length % s_slot_n_bits;

                    memset(m_bitmap, 0, sizeof(slot_type) * m_n_slots);

                    if(rem) {
                        uint64_t const n_non_alloc = s_slot_n_bits - rem;
                        uint64_t val = 0;

                        for(uint32_t i = 0; i < n_non_alloc; ++i) { val |= 1ULL << (s_slot_n_bits - i); }

                        m_bitmap[m_bitmap.get_length() - 1] = val;
                    }
                }

                ObjectPool::ObjectPool(
                  Renderer const &renderer,
                  uint32_t const object_size,
                  uint32_t const pool_length,
                  VkMemoryPropertyFlags const mem_flags,
                  VkBufferUsageFlags const usage) :
                  MemoryHeap{renderer, object_size * pool_length, mem_flags, usage},
                  m_obj_size{object_size}, m_pool_length{pool_length},
                  m_n_slots{pool_length / s_slot_n_bits + 1 - !(pool_length % s_slot_n_bits)}, m_bitmap{
                                                                                                 m_n_slots} {
                    initialize();
                }

                VkDeviceSize ObjectPool::allocate() {
                    for(uint32_t i = 0; i < m_bitmap.get_length(); ++i) {
                        if(m_bitmap[i] != SLOT_FULL) {
                            for(uint32_t j = 0; j < s_slot_n_bits; ++j) {
                                if(~m_bitmap[i] & (1ULL << j)) {
                                    m_bitmap[i] |= (1ULL << j);

                                    return static_cast<VkDeviceSize>((s_slot_n_bits * i + j) * m_obj_size);
                                }
                            }
                        }
                    }

                    return JLT_VULKAN_INVALIDSZ;
                }

                bool ObjectPool::is_full() const {
                    for(auto cluster : m_bitmap) {
                        if(cluster != SLOT_FULL) {
                            return false;
                        }
                    }

                    return true;
                }

                void ObjectPool::free(VkDeviceSize const offset) {
                    uint64_t const object_n = offset / m_obj_size;
                    uint64_t const cluster = object_n / s_slot_n_bits;
                    uint64_t const slot = offset % s_slot_n_bits;
                    uint64_t const slot_mask = 1ULL << slot;

                    jltassert2(
                      m_bitmap[cluster] & slot_mask, "Attempting to free a slot that is not allocated");
                    m_bitmap[cluster] &= ~slot_mask;
                }
            } // namespace memory
        }     // namespace vulkan
    }         // namespace graphics
} // namespace jolt
