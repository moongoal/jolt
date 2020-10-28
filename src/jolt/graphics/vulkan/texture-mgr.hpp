#ifndef JLT_GRAPHICS_VULKAN_TEXTURE_MGR_HPP
#define JLT_GRAPHICS_VULKAN_TEXTURE_MGR_HPP

#include "renderer.hpp"
#include "memory.hpp"
#include "texture-builder.hpp"
#include "texture.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            template<typename Pool, typename Builder = TextureBuilder>
            class TextureManager {
              public:
                using allocator_type = Pool;
                using builder_type = Builder;

              private:
                Renderer &m_renderer;
                Pool &m_pool;

              public:
                TextureManager(Renderer &renderer, Pool &pool) : m_renderer{renderer}, m_pool{pool} {}
                virtual ~TextureManager();

                Renderer const &get_renderer() const { return m_renderer; }

                void destroy_texture(Texture &texture) const {
                    vkDestroySampler(m_renderer.get_device(), texture.get_sampler(), get_vulkan_allocator());
                    vkDestroyImageView(m_renderer.get_device(), texture.get_view(), get_vulkan_allocator());
                    vkDestroyImage(m_renderer.get_device(), texture.get_image(), get_vulkan_allocator());

                    m_pool.free(texture.get_offset());
                }
            };
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_TEXTURE_MGR_HPP */
