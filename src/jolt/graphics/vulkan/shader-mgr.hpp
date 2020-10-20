#ifndef JLT_GRAPHICS_VULKAN_SHADER_MGR_HPP
#define JLT_GRAPHICS_VULKAN_SHADER_MGR_HPP

#include <jolt/hash.hpp>
#include <jolt/graphics/shader-mgr.hpp>
#include <jolt/collections/hashmap.hpp>
#include "defs.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            class Renderer;

            class ShaderManager : public graphics::ShaderManager {
              public:
                using vulkan_module_table = collections::HashMap<hash::hash_t, VkShaderModule, hash_function>;

              private:
                Renderer &m_renderer;
                vulkan_module_table m_modules; //< Mapping between hash and Vulkan shader module.

              public:
                ShaderManager(Renderer &renderer, vfs::VirtualFileSystem &vfs);

                VkShaderModule get_vulkan_shader(hash::hash_t const &id);
            };
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_SHADER_MGR_HPP */
