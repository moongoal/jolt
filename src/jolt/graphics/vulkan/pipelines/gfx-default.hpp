#ifndef JLT_GRAPHICS_VULKAN_PIPELINES_GFX_DEFAULT_HPP
#define JLT_GRAPHICS_VULKAN_PIPELINES_GFX_DEFAULT_HPP

#include <jolt/graphics/vulkan/defs.hpp>
#include <jolt/graphics/vulkan/gfx-pipeline-cfg.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            namespace pipelines {
                class DefaultGraphicsPipelineConfiguration : public GraphicsPipelineConfiguration {
                    virtual void initialize_impl();

                  public:
                    DefaultGraphicsPipelineConfiguration(
                      Renderer &renderer, VkPipelineLayout pipeline_layout) :
                      GraphicsPipelineConfiguration{renderer, pipeline_layout} {}
                };
            } // namespace pipelines
        }     // namespace vulkan
    }         // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_PIPELINES_GFX_DEFAULT_HPP */
