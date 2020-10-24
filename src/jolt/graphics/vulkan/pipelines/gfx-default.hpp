#ifndef JLT_GRAPHICS_VULKAN_PIPELINES_GFX_DEFAULT_HPP
#define JLT_GRAPHICS_VULKAN_PIPELINES_GFX_DEFAULT_HPP

#include <type_traits>
#include <jolt/graphics/vulkan/defs.hpp>
#include <jolt/graphics/vulkan/gfx-pipeline-cfg.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            namespace pipelines {
                class JLTAPI DefaultGraphicsPipelineConfiguration : public GraphicsPipelineConfiguration {
                    virtual void initialize_impl();

                  public:
                    DefaultGraphicsPipelineConfiguration(
                      Renderer &renderer,
                      VkPipelineLayout pipeline_layout,
                      VkShaderModule vertex_shader,
                      VkShaderModule fragment_shader) :
                      GraphicsPipelineConfiguration{renderer, pipeline_layout} {
                        m_shader_vert = vertex_shader;
                        m_shader_frag = fragment_shader;
                    }
                };
            } // namespace pipelines
        }     // namespace vulkan
    }         // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_PIPELINES_GFX_DEFAULT_HPP */
