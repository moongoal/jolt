#ifndef JLT_GRAPHICS_VULKAN_MEMORY_ARENA_HPP
#define JLT_GRAPHICS_VULKAN_MEMORY_ARENA_HPP

#include <jolt/collections/linkedlist.hpp>
#include <jolt/collections/hashmap.hpp>
#include <jolt/graphics/vulkan/defs.hpp>
#include "heap.hpp"
#include "alloc.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            namespace memory {
                /**
                 * A device memory arena. Memory is allocated by usage of a free list and allocation
                 * metadata is kept in host memory.
                 */
                class JLTAPI Arena : public MemoryHeap {
                  public:
                    struct FreeListNode {
                        VkDeviceSize m_base;
                        VkDeviceSize m_size;

                        FreeListNode(VkDeviceSize const base, VkDeviceSize const size) :
                          m_base{base}, m_size{size} {}
                    };

                    struct AllocMetadata {
                        VkDeviceSize m_size;    //< Total size of the allocation, including padding.
                        VkDeviceSize m_padding; //< Padding added to `m_base` to obtain the final pointer.

                        AllocMetadata(VkDeviceSize const size, VkDeviceSize const padding) :
                          m_size{size}, m_padding{padding} {}
                    };

                    using free_list = collections::LinkedList<FreeListNode>;
                    using alloc_list =
                      collections::HashMap<VkDeviceSize, AllocMetadata, jolt::hash::Identity>;

                  private:
                    free_list m_freelist; //< List of free spots in memory.
                    alloc_list m_allocs;  //< Mapping between offsets and allocation medatada structures.
                    VkDeviceSize m_total_alloc_size; //< Amount of total allocated memory.

                    free_list::Node *find_prev_closest_node(VkDeviceSize const ptr);

                  public:
                    /**
                     * Create a new device memory arena.
                     *
                     * @param renderer The renderer.
                     * @param size The size of the memory regoin to allocate.
                     * @param mem_flags Memory type requirements.
                     * @param usage Usage flags for the internal buffer. Set to 0 not to bind the memory
                     * to any buffer.
                     */
                    Arena(
                      Renderer const &renderer,
                      VkDeviceSize const size,
                      VkMemoryPropertyFlags const mem_flags,
                      VkBufferUsageFlags const usage) :
                      MemoryHeap{renderer, size, mem_flags, usage},
                      m_total_alloc_size{0} {
                        m_freelist.add({0, size});
                    }

                    Arena(Arena const &) = delete;

                    /**
                     * Free a previous allocation made through this arena.
                     *
                     * @param ptr The pointer to the allocated region.
                     */
                    void free(VkDeviceSize const ptr);

                    /**
                     * Allocate a chunk of memory.
                     *
                     * @param size The size of the allocated memory region.
                     * @param alignment The alignment requirement.
                     *
                     * @return A device allocation object or `InvalidDeviceAlloc` if a
                     * contiguous memory region of the required size was not found.
                     *
                     * @see MemoryHeap::get_base().
                     */
                    DeviceAlloc allocate(VkDeviceSize const size, VkDeviceSize const alignment);

                    /**
                     * Return the total allocated memory size (in bytes).
                     */
                    VkDeviceSize get_allocated_size() const { return m_total_alloc_size; }

                    free_list const &get_free_list() const { return m_freelist; }
                    alloc_list const &get_alloc_list() const { return m_allocs; }
                };
            } // namespace memory
        }     // namespace vulkan
    }         // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_MEMORY_ARENA_HPP */
