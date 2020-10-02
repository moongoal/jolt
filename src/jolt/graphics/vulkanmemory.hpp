#ifndef JLT_GRAPHICS_VULKANMEMORY_HPP
#define JLT_GRAPHICS_VULKANMEMORY_HPP

#include "defs.hpp"

#include <jolt/collections/array.hpp>

namespace jolt {
    namespace graphics {
        class VulkanRenderer;

        /**
         * A generic device memory heap.
         *
         * This class is intended to be subclassed, not to be used directly.
         */
        class JLTAPI VulkanMemoryHeap {
            VulkanRenderer const &m_renderer;
            VkDeviceMemory m_memory = VK_NULL_HANDLE; //< Device memory allocation handle.
            VkDeviceSize m_size;                      //< Device memory region size.

            /**
             * Allocate `m_size` memory from the device and store the handle to `m_memory`.
             *
             * @param mem_flags The memory type requirements.
             */
            void allocate(VkMemoryPropertyFlags const mem_flags);

            /**
             * Find the memory type index for a specific set of requirements.
             */
            uint32_t find_memory_type(VkMemoryPropertyFlags const mem_flags);

          public:
            /**
             * Create a new device memory heap.
             *
             * @param renderer The renderer.
             * @param size The size of the memory regoin to allocate.
             * @param mem_flags Memory type requirements.
             */
            VulkanMemoryHeap(
              VulkanRenderer const &renderer,
              VkDeviceSize const size,
              VkMemoryPropertyFlags const mem_flags) :
              m_renderer{renderer},
              m_size{size} {
                allocate(mem_flags);
            }

            VulkanMemoryHeap(VulkanMemoryHeap const &other) = delete;

            VulkanMemoryHeap(VulkanMemoryHeap &&other) :
              m_renderer{other.m_renderer}, m_memory{other.m_memory}, m_size{other.m_size} {
                other.m_memory = VK_NULL_HANDLE;
            }

            virtual ~VulkanMemoryHeap() { dispose(); }

            VulkanRenderer const &get_renderer() const { return m_renderer; }
            VkDeviceMemory get_memory() const { return m_memory; }
            VkDeviceSize get_size() const { return m_size; }

            /**
             * Release the allocated device memory.
             *
             * @remarks This function is called automatically upon destruction.
             */
            void dispose();
        };

        /**
         * A pool of identical objects allocated in device memory.
         */
        class JLTAPI VulkanObjectPool : public VulkanMemoryHeap {
          public:
            using bit_map = collections::Array<uint64_t>;
            using pool_handle = VkDeviceSize;

          private:
            VkDeviceSize m_object_size; //< Size of the single object.
            bit_map m_bitmap;           //< The bit map of the allocated objects.

            /**
             * Initialize the bit map.
             */
            void initialize(uint64_t const n_objects);

            /**
             * Allocate a free slot.
             *
             * @return The offset of the allocation, in bytes, from the beginning of the memory
             * region.
             */
            uint64_t allocate_free_slot();

          public:
            /**
             * Create a new object pool.
             *
             * @param renderer The renderer.
             * @param object_size The size of the object this pool will allocate.
             * @param n_objects The number of objects that will be available for allocation within
             * this pool.
             * @param mem_flags The memory type requirements.
             */
            VulkanObjectPool(
              VulkanRenderer const &renderer,
              VkDeviceSize const object_size,
              uint32_t const n_objects,
              VkMemoryPropertyFlags const mem_flags) :
              VulkanMemoryHeap{renderer, object_size * n_objects, mem_flags},
              m_object_size{object_size}, m_bitmap{(n_objects / 64) + (1 - !(n_objects % 64))} {
                initialize(n_objects);
            }

            /**
             * Return the internal bitmap.
             */
            bit_map const &get_bitmap() const { return m_bitmap; }

            /**
             * Return a boolean value stating whether this pool is full or further allocation is
             * possible.
             */
            bool is_full() const;

            /**
             * Allocate one object from the pool and bind its memory to a Vulkan buffer.
             *
             * @param buffer The buffer to allocate the slot to.
             *
             * @return value The allocation handle or JLT_VULKAN_INVALID64 if the allocation fails.
             */
            pool_handle bind_to_buffer(VkBuffer const buffer);

            /**
             * Allocate one object from the pool and bind its memory to a Vulkan image.
             *
             * @param image The image to allocate the slot to.
             *
             * @return value The allocation handle or JLT_VULKAN_INVALID64 if the allocation fails.
             */
            pool_handle bind_to_image(VkImage const image);

            /**
             * Free a previous allocation made through this pool.
             */
            void free(pool_handle const handle);
        };
    } // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKANMEMORY_HPP */
