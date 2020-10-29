#ifndef JLT_GRAPHICS_VULKAN_MEMORY_BUFFER_HPP
#define JLT_GRAPHICS_VULKAN_MEMORY_BUFFER_HPP

#include <jolt/graphics/vulkan/defs.hpp>

namespace jolt::graphics::vulkan {
    class Buffer {
        VkDeviceMemory m_memory;
        VkBuffer m_buffer;
        VkDeviceSize m_size;
        VkMemoryPropertyFlags m_mem_flags;
        VkBufferUsageFlags m_usage;

      public:
        Buffer(
          VkDeviceMemory memory,
          VkBuffer buffer,
          VkDeviceSize size,
          VkMemoryPropertyFlags const mem_flags,
          VkBufferUsageFlags const usage) :
          m_memory{memory},
          m_buffer{buffer}, m_size{size}, m_mem_flags{mem_flags}, m_usage{usage} {}

        VkDeviceMemory get_memory() const { return m_memory; }
        VkBuffer get_buffer() const { return m_buffer; }
        VkDeviceSize get_size() const { return m_size; }
        VkMemoryPropertyFlags get_memory_property_flags() const { return m_mem_flags; }
        VkBufferUsageFlags get_usage() const { return m_usage; }

        bool operator==(Buffer const &other) const {
            return m_memory == other.m_memory && m_buffer == other.m_buffer;
        }

        bool operator!=(Buffer const &other) const {
            return m_memory != other.m_memory || m_buffer != other.m_buffer;
        }
    };

    class SubBuffer {
        VkBuffer m_buffer;
        VkDeviceSize m_offset;
        VkDeviceSize m_size;

      public:
        SubBuffer(Buffer &parent, VkDeviceSize const size, VkDeviceSize const offset) :
          m_buffer{parent.get_buffer()}, m_offset{offset}, m_size{size} {}

        VkBuffer get_buffer() const { return m_buffer; }
        VkDeviceSize get_size() const { return m_size; }
        VkDeviceSize get_offset() const { return m_offset; }
    };
} // namespace jolt::graphics::vulkan

#endif /* JLT_GRAPHICS_VULKAN_MEMORY_BUFFER_HPP */
