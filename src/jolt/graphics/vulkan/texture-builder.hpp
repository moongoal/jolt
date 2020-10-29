#ifndef JLT_GRAPHICS_VULKAN_TEXTURE_BUILDER_HPP
#define JLT_GRAPHICS_VULKAN_TEXTURE_BUILDER_HPP

#include <jolt/collections/vector.hpp>
#include "defs.hpp"
#include "texture.hpp"

namespace jolt::graphics::vulkan {
    class Renderer;

    class JLTAPI TextureBuilder {
      public:
        using texture_vector = collections::Vector<Texture>;

      private:
        Renderer &m_renderer;
        VkImageCreateInfo m_image_create_info;
        VkImageViewCreateInfo m_image_view_create_info;
        VkSamplerCreateInfo m_sampler_create_info;

        virtual void initialize_image(uint32_t const width, uint32_t const height);
        virtual void initialize_view();
        virtual void initialize_sampler();

        VkDeviceMemory allocate_texture_memory(VkMemoryRequirements const &mem_req, uint32_t const n);

      public:
        TextureBuilder(Renderer &renderer, uint32_t const width, uint32_t const height);

        Renderer const &get_renderer() const { return m_renderer; }
        Texture build_texture();

        texture_vector build_texture_array(uint32_t const n);
    };
} // namespace jolt::graphics::vulkan

#endif /* JLT_GRAPHICS_VULKAN_TEXTURE_BUILDER_HPP */
