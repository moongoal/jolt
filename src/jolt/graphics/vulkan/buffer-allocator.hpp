#ifndef JLT_GRAPHICS_VULKAN_MEMORY_BUFFER_ALLOCATOR_HPP
#define JLT_GRAPHICS_VULKAN_MEMORY_BUFFER_ALLOCATOR_HPP

#include <jolt/graphics/vulkan/defs.hpp>
#include <jolt/collections/vector.hpp>
#include "buffer.hpp"

namespace jolt::graphics::vulkan {
    class Renderer;

    class JLTAPI BufferAllocator {
      public:
        using unused_buffers = collections::Vector<Buffer>;

        /**
         * A compatible buffer is compared to the requested allocation based on its size. If the size of the
         * candidate buffer is `<= MAX_SIZE_COMPATIBILITY_FACTOR * requested_size`, it is selected as the
         * allocation, otherwise it's not.
         */
        static constexpr float MAX_SIZE_COMPATIBILITY_FACTOR = 1.1;

      private:
        Renderer &m_renderer;            //< The renderer.
        unused_buffers m_unused_buffers; //< Collection of unused buffer.
        bool m_moved = false; //< Flag indicating this allocator has been moved and doesn't own any resource.

        /**
         * Create a new buffer object.
         *
         * @param size The requested size.
         * @param mem_flags The memory property flags for the allocation.
         * @param usage The usage flags for the buffer.
         */
        Buffer create_buffer(
          VkDeviceSize const size, VkMemoryPropertyFlags const mem_flags, VkBufferUsageFlags const usage);

        /**
         * Destroy a buffer object.
         *
         * @param buffer The buffer to destroy.
         */
        void destroy_buffer(Buffer const &buffer);

        /**
         * Find an unused, compatible buffer to use in order to avoid allocating memory.
         *
         * @param size The requested size.
         * @param mem_flags The memory property flags for the allocation.
         * @param usage The usage flags for the buffer.
         */
        Buffer *find_compatible_buffer(
          VkDeviceSize const size, VkMemoryPropertyFlags const mem_flags, VkBufferUsageFlags const usage);

      public:
        BufferAllocator(Renderer &renderer) : m_renderer{renderer} {}
        BufferAllocator(BufferAllocator const &) = delete;
        BufferAllocator(BufferAllocator &&other);

        ~BufferAllocator();

        /**
         * Allocate a new buffer.
         *
         * @param size The requested size.
         * @param mem_flags The memory property flags for the allocation.
         * @param usage The usage flags for the buffer.
         */
        Buffer allocate(
          VkDeviceSize const size, VkMemoryPropertyFlags const mem_flags, VkBufferUsageFlags const usage);

        /**
         * Free a buffer.
         *
         * @param buffer The buffer to free.
         *
         * @remarks After this function returns, the object used as argument shall not be used again.
         */
        void free(Buffer const &buffer);

        /**
         * Free any unused buffer.
         */
        void recycle();
    };
} // namespace jolt::graphics::vulkan

#endif /* JLT_GRAPHICS_VULKAN_MEMORY_BUFFER_ALLOCATOR_HPP */
