#ifndef JLT_GRAPHICS_VULKAN_GFX_PIPELINE_CFG_HPP
#define JLT_GRAPHICS_VULKAN_GFX_PIPELINE_CFG_HPP

#include <jolt/api.hpp>
#include <jolt/collections/vector.hpp>
#include "defs.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            class Renderer;

            /**
             * Graphics pipeline configuration base class.
             */
            class JLTAPI GraphicsPipelineConfiguration {
              public:
                using vertex_binding_descriptions = collections::Vector<VkVertexInputBindingDescription>;
                using vertex_attribute_descriptions = collections::Vector<VkVertexInputAttributeDescription>;
                using shader_stage_cinfos = collections::Vector<VkPipelineShaderStageCreateInfo>;
                using viewports = collections::Vector<VkViewport>;
                using scissors = collections::Vector<VkRect2D>;
                using color_blend_attachment_states =
                  collections::Vector<VkPipelineColorBlendAttachmentState>;
                using dynamic_states = collections::Vector<VkDynamicState>;

              private:
                Renderer &m_renderer;
                bool m_initialized = false;

                /**
                 * Initialize `m_pipeline_create_info`.
                 */
                virtual void initialize_impl() = 0;

                /**
                 * Deallocate any allocated resources.
                 */
                virtual void destroy_impl(){};

              protected:
                VkPipelineLayout m_pipeline_layout;
                VkGraphicsPipelineCreateInfo m_pipeline_create_info;
                VkPipelineVertexInputStateCreateInfo m_vertex_input_state_create_info;
                VkPipelineInputAssemblyStateCreateInfo m_input_assembly_state_cinfo;
                VkPipelineViewportStateCreateInfo m_viewport_state_create_info;
                VkPipelineRasterizationStateCreateInfo m_rasterization_state_create_info;
                VkPipelineMultisampleStateCreateInfo m_multisample_state_create_info;
                VkPipelineDepthStencilStateCreateInfo m_depth_stencil_state_create_info;
                VkPipelineColorBlendStateCreateInfo m_color_blend_state_create_info;
                VkPipelineDynamicStateCreateInfo m_dynamic_state_create_info;
                VkPipelineTessellationStateCreateInfo m_tessellation_state_create_info;

              public:
                shader_stage_cinfos m_shader_stage_create_infos;
                color_blend_attachment_states m_color_blend_attachment_states;
                viewports m_viewports;
                scissors m_scissors;
                dynamic_states m_dynamic_states;
                vertex_binding_descriptions m_vertex_binding_descriptions;
                vertex_attribute_descriptions m_vertex_attribute_descriptions;
                VkShaderModule m_shader_vert;
                VkShaderModule m_shader_frag;
                
                /**
                 * Create a new graphics pipeline configuration.
                 *
                 * @param renderer The renderer.
                 * @param pipeline_layout The pipeline layout to use to configure this pipeline.
                 */
                GraphicsPipelineConfiguration(Renderer &renderer, VkPipelineLayout pipeline_layout) :
                  m_renderer{renderer}, m_pipeline_layout{pipeline_layout} {}

                virtual ~GraphicsPipelineConfiguration() { destroy(); }

                /**
                 * Initialize the structures for usage.
                 */
                void initialize() { initialize_impl(); }

                /**
                 * Destroy the configuration.
                 */
                void destroy();

                VkGraphicsPipelineCreateInfo &get_pipeline_create_info();

                shader_stage_cinfos &get_shader_stage_create_infos() { return m_shader_stage_create_infos; }

                VkPipelineVertexInputStateCreateInfo &get_vertex_input_state_create_info() {
                    return m_vertex_input_state_create_info;
                }

                VkPipelineInputAssemblyStateCreateInfo &get_input_assembly_state_create_info() {
                    return m_input_assembly_state_cinfo;
                }

                VkPipelineViewportStateCreateInfo &get_viewport_state_create_info() {
                    return m_viewport_state_create_info;
                }

                VkPipelineRasterizationStateCreateInfo &get_rasterization_state_create_info() {
                    return m_rasterization_state_create_info;
                }

                VkPipelineMultisampleStateCreateInfo &get_multisample_state_create_info() {
                    return m_multisample_state_create_info;
                }

                VkPipelineDepthStencilStateCreateInfo &get_depth_stencil_state_create_info() {
                    return m_depth_stencil_state_create_info;
                }

                color_blend_attachment_states &get_color_blend_attachment_states() {
                    return m_color_blend_attachment_states;
                }

                VkPipelineColorBlendStateCreateInfo &get_color_blend_state_create_info() {
                    return m_color_blend_state_create_info;
                }

                dynamic_states &get_dynamic_states() { return m_dynamic_states; }

                VkPipelineDynamicStateCreateInfo &get_dynamic_state_create_info() {
                    return m_dynamic_state_create_info;
                }

                VkPipelineTessellationStateCreateInfo &get_tessellation_state_create_info() {
                    return m_tessellation_state_create_info;
                }

                viewports &get_viewports() { return m_viewports; }
                scissors &get_scissors() { return m_scissors; }

                Renderer const &get_renderer() const { return m_renderer; }
            };
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_GFX_PIPELINE_CFG_HPP */
