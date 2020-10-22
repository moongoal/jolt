#ifndef JLT_GRAPHICS_VULKAN_GFX_PIPELINE_HPP
#define JLT_GRAPHICS_VULKAN_GFX_PIPELINE_HPP

#include <jolt/collections/vector.hpp>
#include <jolt/collections/array.hpp>
#include "defs.hpp"
#include "gfx-pipeline-cfg.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            class Renderer;

            class JLTAPI GraphicsPipelineManager {
              public:
                using pipeline_array = collections::Array<VkPipeline>;
                using const_pipeline_array = const collections::Array<VkPipeline>;
                using cfg_vector = collections::Vector<GraphicsPipelineConfiguration*>;

              private:
                Renderer &m_renderer;
                VkPipelineLayout m_pipeline_layout;
                pipeline_array *m_pipelines = nullptr;
                VkPipelineCache m_pipeline_cache;
                cfg_vector m_cfgs;

                void initialize_pipeline_cache();
                void destroy_pipeline_cache();

                /**
                 * Return a pipeline cache creation info structure. The default implementation will return a
                 * structure for an empty cache.
                 */
                virtual VkPipelineCacheCreateInfo get_pipeline_cache_create_info() const;

                /**
                 * Return a pipeline layout creation info structure. The default implementation will return a
                 * structure for a layout without descriptors or push constant ranges.
                 */
                virtual VkPipelineLayoutCreateInfo get_pipeline_layout_create_info() const;

              public:
                /**
                 * Create a new graphics pipeline object.
                 *
                 * @param renderer The renderer.
                 */
                GraphicsPipelineManager(Renderer &renderer);

                ~GraphicsPipelineManager();

                void add_configuration(GraphicsPipelineConfiguration &cfg) { m_cfgs.push(&cfg); }

                Renderer const &get_renderer() const { return m_renderer; }
                VkPipelineLayout get_pipeline_layout() const { return m_pipeline_layout; }
                const_pipeline_array &get_pipelines() const { return *m_pipelines; }
                VkPipelineCache get_pipeline_cache() const { return m_pipeline_cache; }

                void create_pipelines();
                void destroy_pipelines();
                void recreate_pipelines();
            };
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_GFX_PIPELINE_HPP */
