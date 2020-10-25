#ifndef JLT_GRAPHICS_VULKAN_MEMORY_POOL_HPP
#define JLT_GRAPHICS_VULKAN_MEMORY_POOL_HPP

#include <cstdint>
#include <jolt/collections/array.hpp>
#include <jolt/graphics/vulkan/defs.hpp>
#include "heap.hpp"
#include "alloc.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            namespace memory {
                class JLTAPI ObjectPool : public MemoryHeap {
                  public:
                    using slot_type = uint64_t;
                    using bit_map = collections::Array<slot_type>;

                  private:
                    static constexpr const slot_type SLOT_FULL = std::numeric_limits<slot_type>::max();
                    static constexpr const uint32_t s_slot_n_bits =
                      sizeof(slot_type) * 8; //< Number of bits in a single cluster.

                    uint32_t const m_obj_size;
                    uint32_t const m_pool_length;
                    uint32_t const m_n_slots; //< Number of slots to allocate in host memory.
                    bit_map m_bitmap;         //< The bit map of the allocated objects.

                    /**
                     * Initialize the bit map.
                     */
                    void initialize();

                  public:
                    /**
                     * Create a new object pool.
                     *
                     * @param renderer The renderer.
                     * @param m_obj_size The size of each object to be stored in the pool.
                     * @param m_pool_length The number of objects to be stored in the pool.
                     * @param mem_flags The memory type requirements.
                     * @param usage Usage flags for the internal buffer. Set to 0 not to bind the memory
                     * to any buffer.
                     */
                    ObjectPool(
                      Renderer const &renderer,
                      uint32_t const object_size,
                      uint32_t const pool_length,
                      VkMemoryPropertyFlags const mem_flags,
                      VkBufferUsageFlags const usage);

                    /**
                     * Return the internal bitmap.
                     */
                    bit_map const &get_bitmap() const { return m_bitmap; }

                    /**
                     * Allocate a slot.
                     *
                     * @return The offset of the allocation, in bytes, from the beginning of the memory
                     * region or JLT_VULKAN_INVALIDSZ if no empty slots are available.
                     *
                     * @see MemoryHeap::get_base().
                     */
                    VkDeviceSize allocate();

                    /**
                     * Return a boolean value stating whether this pool is full or further allocation is
                     * possible.
                     */
                    bool is_full() const;

                    /**
                     * Free a previous allocation made through this pool.
                     */
                    void free(VkDeviceSize const offset);
                };
            } // namespace memory
        }     // namespace vulkan
    }         // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_MEMORY_POOL_HPP */
