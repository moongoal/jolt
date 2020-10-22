#include <jolt/graphics/vulkan.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            GraphicsPipelineManager::GraphicsPipelineManager(Renderer &renderer) : m_renderer{renderer} {
                initialize_pipeline_cache();
            }

            GraphicsPipelineManager::~GraphicsPipelineManager() {
                destroy_pipelines();
                destroy_pipeline_cache();
            }

            VkPipelineLayoutCreateInfo GraphicsPipelineManager::get_pipeline_layout_create_info() const {
                return {
                  VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, // sType
                  nullptr,                                       // pNext
                  0,                                             // flags
                  0,                                             // setLayoutCount
                  nullptr,                                       // pSetLayouts
                  0,                                             // pushConstantRangeCount
                  nullptr                                        // pPushConstantRanges
                };
            }

            VkPipelineCacheCreateInfo GraphicsPipelineManager::get_pipeline_cache_create_info() const {
                return {
                  VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, // sType
                  nullptr,                                      // pNext
                  0,                                            // flags
                  0,                                            // initialDataSize
                  nullptr                                       // pInitialData
                };
            }

            void GraphicsPipelineManager::initialize_pipeline_cache() {
                VkPipelineCacheCreateInfo pcinfo = get_pipeline_cache_create_info();

                VkResult result = vkCreatePipelineCache(
                  get_renderer().get_device(), &pcinfo, get_vulkan_allocator(), &m_pipeline_cache);
                jltassert2(result == VK_SUCCESS, "Unable to create graphics pipeline cache");
            }

            void GraphicsPipelineManager::destroy_pipeline_cache() {
                vkDestroyPipelineCache(get_renderer().get_device(), m_pipeline_cache, get_vulkan_allocator());
            }

            void GraphicsPipelineManager::create_pipelines() {
                jltassert2(m_pipelines == nullptr, "Pipelines already created");

                m_pipelines = jltnew(pipeline_array, m_cfgs.get_length());

                collections::Array<VkGraphicsPipelineCreateInfo> cinfos{m_cfgs.get_length()};

                for(size_t i = 0; i < m_cfgs.get_length(); ++i) {
                    cinfos[i] = m_cfgs[i]->get_pipeline_create_info();
                }

                VkResult result = vkCreateGraphicsPipelines(
                  get_renderer().get_device(),
                  m_pipeline_cache,
                  static_cast<uint32_t>(cinfos.get_length()),
                  cinfos,
                  get_vulkan_allocator(),
                  *m_pipelines);
                jltassert2(result == VK_SUCCESS, "Unable to create graphics pipelines");
            }

            void GraphicsPipelineManager::destroy_pipelines() {
                if(m_pipelines) {
                    for(auto &pipeline : *m_pipelines) {
                        vkDestroyPipeline(get_renderer().get_device(), pipeline, get_vulkan_allocator());
                    }

                    jltfree(m_pipelines);
                    m_pipelines = nullptr;
                }
            }

            void GraphicsPipelineManager::recreate_pipelines() {
                destroy_pipelines();
                create_pipelines();
            }
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt
