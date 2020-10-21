#include <jolt/graphics/vulkan.hpp>
#include "gfx-default.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            namespace pipelines {
                void DefaultGraphicsPipelineConfiguration::initialize_impl() {
                    ShaderManager &shader_manager = get_renderer().get_shader_manager();

                    path::Path path_vertex_shader = "/build/src/shaders/vertex/triangle.vert.spv";
                    hash::hash_t vertex_shader = path_vertex_shader.hash<ShaderManager::hash_function>();

                    path::Path path_fragment_shader = "/build/src/shaders/fragment/red.frag.spv";
                    hash::hash_t fragment_shader = path_fragment_shader.hash<ShaderManager::hash_function>();

                    // Shader stages
                    m_shader_stage_create_infos.push({
                      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
                      nullptr,                                             // pNext
                      0,                                                   // flags
                      VK_SHADER_STAGE_VERTEX_BIT,                          // stage
                      shader_manager.get_vulkan_shader(vertex_shader),     // module
                      "main",                                              // name
                      nullptr                                              // pSpecializationInfo
                    });

                    m_shader_stage_create_infos.push({
                      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
                      nullptr,                                             // pNext
                      0,                                                   // flags
                      VK_SHADER_STAGE_FRAGMENT_BIT,                        // stage
                      shader_manager.get_vulkan_shader(fragment_shader),   // module
                      "main",                                              // name
                      nullptr                                              // pSpecializationInfo
                    });

                    // Fixed stages
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
                      VK_CULL_MODE_NONE,                                          // cullMode
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
                      0                    // colorWriteMask
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
