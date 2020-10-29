#include <jolt/graphics/vulkan.hpp>

namespace jolt::graphics::vulkan {
    void TextureAllocator::free(Texture const &texture) {
        VkSampler sampler = texture.get_sampler();
        VkImage image = texture.get_image();
        VkImageView view = texture.get_view();

        if(texture.get_sampler() != VK_NULL_HANDLE) {
            vkDestroySampler(m_renderer.get_device(), sampler, get_vulkan_allocator());
        }

        if(view != VK_NULL_HANDLE) {
            vkDestroyImageView(m_renderer.get_device(), view, get_vulkan_allocator());
        }

        if(image != VK_NULL_HANDLE) {
            vkDestroyImage(m_renderer.get_device(), image, get_vulkan_allocator());
        }

        // Allow this function to be called for each element of a texture array without attempting to free the
        // memory more than once.
        if(texture.get_offset() == 0) {
            VkDeviceMemory memory = texture.get_memory();

            if(memory != VK_NULL_HANDLE) {
                vkFreeMemory(m_renderer.get_device(), memory, get_vulkan_allocator());
            }
        }
    }
} // namespace jolt::graphics::vulkan
