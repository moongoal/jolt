#include <jolt/graphics/vulkan.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            VkGraphicsPipelineCreateInfo &GraphicsPipelineConfiguration::get_pipeline_create_info() {
                if(!m_initialized) {
                    initialize();
                    m_initialized = true;
                }

                return m_pipeline_create_info;
            }

            void GraphicsPipelineConfiguration::destroy() {
                destroy_impl();

                m_shader_stage_create_infos.clear();
                m_color_blend_attachment_states.clear();
                m_viewports.clear();
                m_scissors.clear();
                m_dynamic_states.clear();
            }
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt
