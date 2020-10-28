#ifndef JLT_GRAPHICS_VULKAN_MEMORY_ALLOC_HPP
#define JLT_GRAPHICS_VULKAN_MEMORY_ALLOC_HPP

#include <jolt/graphics/vulkan/defs.hpp>

namespace jolt::graphics::vulkan::memory {
    class DeviceAlloc {
        VkBuffer m_handle;
        VkDeviceSize m_offset;
        VkDeviceSize m_size;

      public:
        DeviceAlloc() : DeviceAlloc{VK_NULL_HANDLE, 0, 0} {}

        DeviceAlloc(VkBuffer const handle, VkDeviceSize const offset, VkDeviceSize const size) :
          m_handle{handle}, m_offset{offset}, m_size{size} {}

        DeviceAlloc(DeviceAlloc const &) = default;
        DeviceAlloc(DeviceAlloc &&) = default;

        operator VkBuffer() const { return m_handle; }
        operator VkDeviceSize() const { return m_offset; }

        bool operator==(DeviceAlloc const &other) const {
            return m_handle == other.m_handle && m_offset == other.m_offset && m_size == other.m_size;
        }

        bool operator!=(DeviceAlloc const &other) const {
            return m_handle != other.m_handle || m_offset != other.m_offset || m_size != other.m_size;
        }

        DeviceAlloc &operator=(DeviceAlloc const &) = default;
        DeviceAlloc &operator=(DeviceAlloc &&) = default;

        VkBuffer get_handle() const { return m_handle; }
        VkDeviceSize get_size() const { return m_size; }
        VkDeviceSize get_offset() const { return m_offset; }
        bool is_valid() const { return m_handle != VK_NULL_HANDLE; }
    };

    extern JLTAPI DeviceAlloc const InvalidDeviceAlloc;
} // namespace jolt::graphics::vulkan::memory

#endif /* JLT_GRAPHICS_VULKAN_MEMORY_ALLOC_HPP */
