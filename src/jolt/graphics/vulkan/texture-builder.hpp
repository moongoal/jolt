#ifndef JLT_GRAPHICS_VULKAN_TEXTURE_BUILDER_HPP
#define JLT_GRAPHICS_VULKAN_TEXTURE_BUILDER_HPP

#include <jolt/media/image.hpp>
#include "defs.hpp"
#include "texture.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            class Renderer;

            class TextureBuilder {
                Renderer &m_renderer;
                media::Image &m_image;
                VkDeviceMemory m_memory;
                VkDeviceSize m_offset;
                VkImageCreateInfo m_image_create_info;
                VkImageViewCreateInfo m_image_view_create_info;
                VkSamplerCreateInfo m_sampler_create_info;

                virtual void initialize_image();
                virtual void initialize_view();
                virtual void initialize_sampler();

              public:
                TextureBuilder(
                  Renderer &renderer, media::Image &image, VkDeviceMemory memory, VkDeviceSize offset);

                Renderer const &get_renderer() const { return m_renderer; }
                Texture build_texture();
            };
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_TEXTURE_BUILDER_HPP */
