#ifndef JLT_GRAPHICS_VULKAN_MEMORY_HPP
#define JLT_GRAPHICS_VULKAN_MEMORY_HPP

#include "defs.hpp"

#include <jolt/collections/array.hpp>
#include <jolt/collections/linkedlist.hpp>
#include <jolt/collections/hashmap.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            class Renderer;

            template<typename Handle, typename Size = VkDeviceSize>
            class DeviceAlloc {
              public:
                using handle_type = Handle;
                using size_type = Size;

              private:
                handle_type const m_handle;
                size_type const m_offset;
                size_type const m_size;

              public:
                DeviceAlloc(
                  handle_type const handle, size_type const offset, size_type const size) :
                  m_handle{handle},
                  m_offset{offset}, m_size{size} {}

                operator handle_type() const { return m_handle; }
                operator size_type() const { return m_offset; }

                handle_type get_handle() const { return m_handle; }
                size_type get_size() const { return m_size; }
                size_type get_offset() const { return m_offset; }
                bool is_valid() const { return m_handle != VK_NULL_HANDLE; }
            };

#ifdef JLT_INTERNAL
            extern template class DeviceAlloc<VkBuffer>;
            extern template class DeviceAlloc<VkImage>;
#endif // JLT_INTERNAL

            using BufferDeviceAlloc = DeviceAlloc<VkBuffer>;
            using ImageDeviceAlloc = DeviceAlloc<VkImage>;

            extern JLTAPI BufferDeviceAlloc const InvalidBufferDeviceAlloc;
            extern JLTAPI ImageDeviceAlloc const InvalidImageDeviceAlloc;

            /**
             * A generic device memory heap.
             *
             * This class is intended to be subclassed, not to be used directly.
             */
            class JLTAPI MemoryHeap {
                Renderer const &m_renderer;
                VkDeviceMemory m_memory = VK_NULL_HANDLE; //< Device memory allocation handle.
                VkDeviceSize m_size;                      //< Device memory region size.

                /**
                 * Allocate `m_size` memory from the device and store the handle to `m_memory`.
                 *
                 * @param mem_flags The memory type requirements.
                 */
                void allocate(VkMemoryPropertyFlags const mem_flags);

              public:
                /**
                 * Create a new device memory heap.
                 *
                 * @param renderer The renderer.
                 * @param size The size of the memory regoin to allocate.
                 * @param mem_flags Memory type requirements.
                 */
                MemoryHeap(
                  Renderer const &renderer,
                  VkDeviceSize const size,
                  VkMemoryPropertyFlags const mem_flags);

                MemoryHeap(MemoryHeap const &other) = delete;

                MemoryHeap(MemoryHeap &&other) :
                  m_renderer{other.m_renderer}, m_memory{other.m_memory}, m_size{other.m_size} {
                    other.m_memory = VK_NULL_HANDLE;
                }

                virtual ~MemoryHeap() { dispose(); }

                Renderer const &get_renderer() const { return m_renderer; }
                VkDeviceMemory get_base() const { return m_memory; }
                VkDeviceSize get_size() const { return m_size; }

                /**
                 * Release the allocated device memory.
                 *
                 * @remarks This function is called automatically upon destruction.
                 */
                void dispose();
            };

            /**
             * A pool of identical objects (in terms of size) allocated in device memory.
             *
             * @tparam S The size of each object to be stored in the pool.
             * @tparam N The number of objects to be stored in the pool.
             */
            template<uint32_t S, uint32_t N>
            class ObjectPool : public MemoryHeap {
              public:
                using slot_type = uint64_t;

              private:
                static constexpr const slot_type SLOT_FULL = std::numeric_limits<slot_type>::max();
                static constexpr const uint32_t s_slot_n_bits =
                  sizeof(slot_type) * 8; //< Number of bits in a single cluster.
                static constexpr const uint32_t s_n_slots =
                  N / s_slot_n_bits + 1
                  - !(N % s_slot_n_bits); //< Number of slots to allocate in host memory.

              public:
                using bit_map = collections::StaticArray<slot_type, s_n_slots>;

              private:
                bit_map m_bitmap; //< The bit map of the allocated objects.

                /**
                 * Initialize the bit map.
                 */
                void initialize() {
                    uint64_t const rem = N % s_slot_n_bits;

                    memset(m_bitmap, 0, sizeof(slot_type) * s_n_slots);

                    if(rem) {
                        uint64_t const n_non_alloc = s_slot_n_bits - rem;
                        uint64_t val = 0;

                        for(uint32_t i = 0; i < n_non_alloc; ++i) {
                            val |= 1ULL << (s_slot_n_bits - i);
                        }

                        m_bitmap[m_bitmap.get_length() - 1] = val;
                    }
                }

              public:
                /**
                 * Create a new object pool.
                 *
                 * @param renderer The renderer.
                 * @param mem_flags The memory type requirements.
                 */
                ObjectPool(Renderer const &renderer, VkMemoryPropertyFlags const mem_flags) :
                  MemoryHeap{renderer, S * N, mem_flags} {
                    initialize();
                }

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
                VkDeviceSize allocate() {
                    for(uint32_t i = 0; i < m_bitmap.get_length(); ++i) {
                        if(m_bitmap[i] != SLOT_FULL) {
                            for(uint32_t j = 0; j < s_slot_n_bits; ++j) {
                                if(~m_bitmap[i] & (1ULL << j)) {
                                    m_bitmap[i] |= (1ULL << j);

                                    return static_cast<VkDeviceSize>((s_slot_n_bits * i + j) * S);
                                }
                            }
                        }
                    }

                    return JLT_VULKAN_INVALIDSZ;
                }

                /**
                 * Return a boolean value stating whether this pool is full or further allocation is
                 * possible.
                 */
                bool is_full() const {
                    for(auto cluster : m_bitmap) {
                        if(cluster != SLOT_FULL) {
                            return false;
                        }
                    }

                    return true;
                }

                /**
                 * Allocate one object from the pool and bind its memory to a Vulkan buffer.
                 *
                 * @param buffer The buffer to allocate the slot to.
                 *
                 * @return value The allocation or InvalidBufferDeviceAlloc if no slots are
                 * available.
                 */
                BufferDeviceAlloc bind_to_buffer(VkBuffer const buffer) {
                    VkDeviceSize offset = allocate();

                    if(offset != JLT_VULKAN_INVALIDSZ) {
                        VkResult result = vkBindBufferMemory(
                          get_renderer().get_device(), buffer, get_base(), offset);
                        jltassert2(result == VK_SUCCESS, "Unable to bind buffer memory");

                        return BufferDeviceAlloc{buffer, offset, S};
                    } else {
                        return InvalidBufferDeviceAlloc;
                    }
                }

                /**
                 * Allocate one object from the pool and bind its memory to a Vulkan image.
                 *
                 * @param image The image to allocate the slot to.
                 *
                 * @return value The allocation or InvalidImageDeviceAlloc if no slots are
                 * available.
                 */
                ImageDeviceAlloc bind_to_image(VkImage const image) {
                    VkDeviceSize offset = allocate();

                    if(offset != JLT_VULKAN_INVALIDSZ) {
                        VkResult result =
                          vkBindImageMemory(get_renderer().get_device(), image, get_base(), offset);
                        jltassert2(result == VK_SUCCESS, "Unable to bind image memory");

                        return ImageDeviceAlloc{image, offset, S};
                    } else {
                        return InvalidImageDeviceAlloc;
                    }
                }

                /**
                 * Free a previous allocation made through this pool.
                 */
                void free(VkDeviceSize const offset) {
                    uint64_t const object_n = offset / S;
                    uint64_t const cluster = object_n / s_slot_n_bits;
                    uint64_t const slot = offset % s_slot_n_bits;
                    uint64_t const slot_mask = (1ULL << slot);

                    // TODO: Add check to see if allocation is within boundaries

                    jltassert2(
                      m_bitmap[cluster] & slot_mask,
                      "Attempting to free a slot that is not allocated");
                    m_bitmap[cluster] &= ~slot_mask;
                }
            };

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
                    VkDeviceSize m_size; //< Total size of the allocation, including padding.
                    VkDeviceSize
                      m_padding; //< Padding added to `m_base` to obtain the final pointer.

                    AllocMetadata(VkDeviceSize const size, VkDeviceSize const padding) :
                      m_size{size}, m_padding{padding} {}
                };

                using free_list = collections::LinkedList<FreeListNode>;
                using alloc_list = collections::
                  HashMap<VkDeviceSize, AllocMetadata, jolt::hash::Identity<VkDeviceSize>>;

              private:
                free_list m_freelist; //< List of free spots in memory.
                alloc_list m_allocs; //< Mapping between offsets and allocation medatada structures.
                VkDeviceSize m_total_alloc_size; //< Amount of total allocated memory.
                VkBuffer m_buffer; //< The internal buffer associated with the memory chunk.

                free_list::Node *find_prev_closest_node(VkDeviceSize const ptr);

                void initialize(VkBufferUsageFlags const usage);

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
                  MemoryHeap(renderer, size, mem_flags),
                  m_total_alloc_size{0} {
                    m_freelist.add({0, size});
                    initialize(usage);
                }

                Arena(Arena const &) = delete;

                virtual ~Arena();

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
                 * @return A device allocation object or `InvalidBufferDeviceAlloc` if a
                 * contiguous memory region of the required size was not found.
                 *
                 * @see MemoryHeap::get_base().
                 */
                BufferDeviceAlloc allocate(VkDeviceSize const size, VkDeviceSize const alignment);

                /**
                 * Return the total allocated memory size (in bytes).
                 */
                VkDeviceSize get_allocated_size() const { return m_total_alloc_size; }

                free_list const &get_free_list() const { return m_freelist; }
                alloc_list const &get_alloc_list() const { return m_allocs; }

                /**
                 * Return a value stating whether the whole arena is already bound to an internal
                 * buffer.
                 *
                 * @return True if the arena is bound to a buffer, otherwise false.
                 *
                 * @remarks If the arena is bound, the `bind_to_buffer()` and `bind_to_image()`
                 * functions must not be used.
                 */
                bool is_bound() const { return m_buffer != VK_NULL_HANDLE; }

                /**
                 * Return the internal buffer.
                 *
                 * @return The internal buffer handle or VK_NULL_HANDLE if the arena is not bound.
                 */
                VkBuffer get_buffer() const { return m_buffer; }
            };
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_MEMORY_HPP */
