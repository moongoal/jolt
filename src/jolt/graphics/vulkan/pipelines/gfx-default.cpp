#include <jolt/graphics/vulkan.hpp>
#include "gfx-default.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            namespace pipelines {
                void DefaultGraphicsPipelineConfiguration::initialize_impl() {
                    // Shader stages
                    m_shader_stage_create_infos.push({
                      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
                      nullptr,                                             // pNext
                      0,                                                   // flags
                      VK_SHADER_STAGE_VERTEX_BIT,                          // stage
                      m_shader_vert,                                       // module
                      "main",                                              // name
                      nullptr                                              // pSpecializationInfo
                    });

                    m_shader_stage_create_infos.push({
                      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
                      nullptr,                                             // pNext
                      0,                                                   // flags
                      VK_SHADER_STAGE_FRAGMENT_BIT,                        // stage
                      m_shader_frag,                                       // module
                      "main",                                              // name
                      nullptr                                              // pSpecializationInfo
                    });

                    // Fixed stages
                    memset(
                      &m_vertex_input_state_create_info, 0, sizeof(VkPipelineVertexInputStateCreateInfo));
                    m_vertex_input_state_create_info.sType =
                      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

                    m_input_assembly_state_cinfo = {
                      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, // sType
                      nullptr,                                                     // pNext
                      0,                                                           // flags
                      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,                         // topology
                      VK_FALSE                                                     // primitiveRestartEnable
                    };

                    const VkSurfaceCapabilitiesKHR &win_surface_caps =
                      get_renderer().get_window()->get_surface_capabilities();

                    m_viewports.push({
                      0,                                                         // x
                      0,                                                         // y
                      static_cast<float>(win_surface_caps.currentExtent.width),  // width
                      static_cast<float>(win_surface_caps.currentExtent.height), // height
                      .0f,                                                       // minDepth
                      1.0f,                                                      // maxDepth
                    });

                    m_scissors.push({
                      {
                        // offset
                        0, // x
                        0  // y
                      },
                      win_surface_caps.currentExtent // extent
                    });

                    m_viewport_state_create_info = {
                      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, // sType
                      nullptr,                                               // pNext
                      0,                                                     // flags
                      static_cast<uint32_t>(m_viewports.get_length()),       // viewportCount
                      &m_viewports[0],                                       // pViewports
                      static_cast<uint32_t>(m_scissors.get_length()),        // scissorCount
                      &m_scissors[0]                                         // pScissors
                    };

                    m_rasterization_state_create_info = {
                      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, // sType
                      nullptr,                                                    // pNext
                      0,                                                          // flags
                      VK_FALSE,                                                   // depthClampEnable
                      VK_FALSE,                                                   // rasterizerDiscardEnable
                      VK_POLYGON_MODE_FILL,                                       // polygonMode
                      VK_CULL_MODE_BACK_BIT,                                      // cullMode
                      VK_FRONT_FACE_COUNTER_CLOCKWISE,                            // frontFace
                      VK_FALSE,                                                   // depthBiasEnable
                      0.0f,                                                       // depthBiasConstantFactor
                      0.0f,                                                       // depthBiasClamp
                      0.0f,                                                       // depthBiasSlopeFactor
                      1.0f                                                        // lineWidth
                    };

                    m_multisample_state_create_info = {
                      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, // sType
                      nullptr,                                                  // pNext
                      0,                                                        // flags
                      VK_SAMPLE_COUNT_1_BIT,                                    // rasterizationSamples
                      VK_FALSE,                                                 // sampleShadingEnable
                      0.0f,                                                     // minSampleShading
                      nullptr,                                                  // pSampleMask
                      VK_FALSE,                                                 // alphaToCoverage
                      VK_FALSE                                                  // alphaToOne
                    };

                    m_depth_stencil_state_create_info = {
                      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, // sType
                      nullptr,                                                    // pNext
                      0,                                                          // flags
                      VK_FALSE,                                                   // depthTestEnable
                      VK_FALSE,                                                   // depthWriteEnable
                      VK_COMPARE_OP_ALWAYS,                                       // depthCompareOp
                      VK_FALSE,                                                   // depthBoundsTestEnable
                      VK_FALSE,                                                   // stencilTestEnable
                      {                                                           // front
                       VK_STENCIL_OP_ZERO,                                        // failOp
                       VK_STENCIL_OP_KEEP,                                        // passOp
                       VK_STENCIL_OP_ZERO,                                        // depthFailOp
                       VK_COMPARE_OP_ALWAYS,                                      // compareOp
                       0,                                                         // compareMask
                       0,                                                         // writeMask
                       0},
                      {                      // back
                       VK_STENCIL_OP_ZERO,   // failOp
                       VK_STENCIL_OP_KEEP,   // passOp
                       VK_STENCIL_OP_ZERO,   // depthFailOp
                       VK_COMPARE_OP_ALWAYS, // compareOp
                       0,                    // compareMask
                       0,                    // writeMask
                       0},
                      0.0f, // minDepthBounds
                      1.0f  // maxDepthBounds
                    };

                    m_color_blend_attachment_states.push({
                      // color attachment
                      VK_FALSE,            // blendEnable
                      VK_BLEND_FACTOR_ONE, // srcColorBlendFactor
                      VK_BLEND_FACTOR_ONE, // dstColorBlendFactor
                      VK_BLEND_OP_ADD,     // colorBlendOp
                      VK_BLEND_FACTOR_ONE, // srcAlphaBlendFactor
                      VK_BLEND_FACTOR_ONE, // dstAlphaBlendFactor
                      VK_BLEND_OP_ADD,     // alphaBlendOp
                      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                        | VK_COLOR_COMPONENT_A_BIT // colorWriteMask
                    });

                    m_color_blend_state_create_info = {
                      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,            // sType
                      nullptr,                                                             // pNext
                      0,                                                                   // flags
                      VK_FALSE,                                                            // logicOpEnable
                      VK_LOGIC_OP_COPY,                                                    // logicOp
                      static_cast<uint32_t>(m_color_blend_attachment_states.get_length()), // attachmentCount
                      &m_color_blend_attachment_states[0],                                 // pAttachments
                      {}                                                                   // blendConstants
                    };

                    m_dynamic_states.push(VK_DYNAMIC_STATE_VIEWPORT);
                    m_dynamic_states.push(VK_DYNAMIC_STATE_SCISSOR);

                    m_dynamic_state_create_info = {
                      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, // sType
                      nullptr,                                              // pNext
                      0,                                                    // flags
                      static_cast<uint32_t>(m_dynamic_states.get_length()), // dynamicStateCount
                      &m_dynamic_states[0]                                  // dynamicStates
                    };

                    // Pipeline
                    m_pipeline_create_info = {
                      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,                 // sType
                      nullptr,                                                         // pNext
                      0,                                                               // flags
                      static_cast<uint32_t>(m_shader_stage_create_infos.get_length()), // stageCount
                      &m_shader_stage_create_infos[0],                                 // pStages
                      &m_vertex_input_state_create_info,                               // pVertexInputState
                      &m_input_assembly_state_cinfo,                                   // pInputAssemblyState
                      nullptr,                                                         // pTessellationState
                      &m_viewport_state_create_info,                                   // pViewportState
                      &m_rasterization_state_create_info,                              // pRasterizationState
                      &m_multisample_state_create_info,                                // pMultisampleState
                      &m_depth_stencil_state_create_info,                              // pDepthStencilState
                      &m_color_blend_state_create_info,                                // pColorBlendState
                      &m_dynamic_state_create_info,                                    // pDynamicState
                      m_pipeline_layout,                                               // layout
                      get_renderer().get_render_target()->get_render_pass(),           // renderPass
                      0,                                                               // subpass
                      VK_NULL_HANDLE,                                                  // basePipelineHandle
                      0                                                                // basePipelineIndex
                    };
                }
            } // namespace pipelines
        }     // namespace vulkan
    }         // namespace graphics
} // namespace jolt
