#ifndef JLT_GRAPHICS_VULKAN_TEXTURE_HPP
#define JLT_GRAPHICS_VULKAN_TEXTURE_HPP

#include "defs.hpp"

namespace jolt::graphics::vulkan {
    class Renderer;

    class Texture {
        VkDeviceMemory const m_memory;
        VkDeviceSize const m_offset;
        VkImage const m_image = VK_NULL_HANDLE;
        VkImageView const m_view = VK_NULL_HANDLE;
        VkSampler const m_sampler = VK_NULL_HANDLE;

      public:
        Texture(
          VkDeviceMemory memory, VkDeviceSize offset, VkImage image, VkImageView view, VkSampler sampler) :
          m_memory{memory},
          m_offset{offset}, m_image{image}, m_view{view}, m_sampler{sampler} {}

        VkImage get_image() const { return m_image; }
        VkImageView get_view() const { return m_view; }
        VkSampler get_sampler() const { return m_sampler; }
        VkDeviceMemory get_memory() const { return m_memory; }
        VkDeviceSize get_offset() const { return m_offset; }
    };
} // namespace jolt::graphics::vulkan

#endif /* JLT_GRAPHICS_VULKAN_TEXTURE_HPP */
