#include <jolt/graphics/vulkan.hpp>
#include "shader-mgr.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            ShaderManager::ShaderManager(Renderer &renderer, vfs::VirtualFileSystem &vfs) :
              graphics::ShaderManager{vfs}, m_renderer{renderer} {}

            VkShaderModule ShaderManager::get_vulkan_shader(hash::hash_t const &id) {
                VkShaderModule module = m_modules.get_value_with_default(id, VK_NULL_HANDLE);

                if(module == VK_NULL_HANDLE) {
                    shader_data const &data = get_shader(id);

                    VkShaderModuleCreateInfo cinfo{
                      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, // sType
                      nullptr,                                     // pNext
                      0,                                           // flags
                      static_cast<uint32_t>(data.get_length()),    // codeSize
                      reinterpret_cast<const uint32_t *>(&data[0]) // pCode
                    };

                    VkResult result = vkCreateShaderModule(
                      get_renderer().get_device(), &cinfo, get_vulkan_allocator(), &module);

                    jltassert2(result == VK_SUCCESS, "Unable to create Vulkan shader module");

                    m_modules.add(id, module);
                }

                return module;
            }

            ShaderManager::~ShaderManager() {
                for(auto [hash, module] : m_modules) {
                    vkDestroyShaderModule(get_renderer().get_device(), module, get_vulkan_allocator());
                }
            }
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt
