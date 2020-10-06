#ifndef JLT_GRAPHICS_VULKAN_TRANSFER_HPP
#define JLT_GRAPHICS_VULKAN_TRANSFER_HPP

#include "defs.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            class Renderer;

            /**
             * A data transfer facility that leverages a staging buffer.
             */
            class Transfer {
                Renderer const &m_renderer;
                VkQueue const m_queue;
                VkDeviceSize const m_buffer_size;
                VkBuffer m_buffer;
                VkDeviceMemory m_memory;
                void *m_map_ptr;

                void initialize();
                uint32_t choose_memory_type() const;

              public:
                /**
                 * Create a new transfer object.
                 *
                 * @param renderer The renderer.
                 * @param queue The queue that will be used to transfer the data.
                 * @param size The size of the staging buffer.
                 */
                Transfer(Renderer const &renderer, VkQueue const queue, VkDeviceSize const size);
                Transfer(Transfer const &other) = delete;

                ~Transfer() { dispose(); }

                /**
                 * Release any resources associated with the object.
                 *
                 * @remarks This function is automatically called upon object destruction.
                 */
                void dispose();
            };
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_TRANSFER_HPP */
