#ifndef JLT_GRAPHICS_VULKAN_MEMORY_HEAP_HPP
#define JLT_GRAPHICS_VULKAN_MEMORY_HEAP_HPP

#include <jolt/graphics/vulkan/defs.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            class Renderer;

            namespace memory {
                /**
                 * A generic device memory heap.
                 *
                 * This class is intended to be subclassed, not to be used directly.
                 */
                class JLTAPI MemoryHeap {
                    Renderer const &m_renderer;
                    VkDeviceMemory m_memory = VK_NULL_HANDLE; //< Device memory allocation handle.
                    VkDeviceSize m_size;                      //< Device memory region size.
                    VkBuffer m_buffer; //< The internal buffer associated with the memory chunk.

                    /**
                     * Allocate `m_size` memory from the device and store the handle to `m_memory`.
                     *
                     * @param mem_flags The memory type requirements.
                     */
                    void allocate(VkMemoryPropertyFlags const mem_flags);

                    /**
                     * Bind the whole heap to a new buffer.
                     *
                     * @param usage Usage flags for the internal buffer.
                     */
                    void bind(VkBufferUsageFlags const usage);

                  public:
                    /**
                     * Create a new device memory heap.
                     *
                     * @param renderer The renderer.
                     * @param size The size of the memory regoin to allocate.
                     * @param mem_flags Memory type requirements.
                     * @param usage Usage flags for the internal buffer. Set to 0 not to bind the memory
                     * to any buffer.
                     */
                    MemoryHeap(
                      Renderer const &renderer,
                      VkDeviceSize const size,
                      VkMemoryPropertyFlags const mem_flags,
                      VkBufferUsageFlags const usage);

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
            } // namespace memory
        }     // namespace vulkan
    }         // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_MEMORY_HEAP_HPP */
