#ifndef JLT_GRAPHICS_VULKAN_TEXTURE_ALLOCATOR_HPP
#define JLT_GRAPHICS_VULKAN_TEXTURE_ALLOCATOR_HPP

#include <jolt/graphics/vulkan/defs.hpp>
#include "texture-builder.hpp"

namespace jolt::graphics::vulkan {
    class Renderer;

    class JLTAPI TextureAllocator {
        Renderer &m_renderer;

      public:
        TextureAllocator(Renderer &renderer) : m_renderer{renderer} {}

        template<typename Builder = TextureBuilder>
        Builder create_builder(uint32_t const width, uint32_t const height) {
            return Builder{m_renderer, width, height};
        }

        void free(Texture const &texture);
    };
} // namespace jolt::graphics::vulkan

#endif /* JLT_GRAPHICS_VULKAN_TEXTURE_ALLOCATOR_HPP */
