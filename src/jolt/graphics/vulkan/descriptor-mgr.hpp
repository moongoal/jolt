#ifndef JLT_GRAPHICS_VULKAN_DESCRIPTOR_MGR_HPP
#define JLT_GRAPHICS_VULKAN_DESCRIPTOR_MGR_HPP

#include <jolt/api.hpp>
#include <jolt/collections/vector.hpp>
#include "defs.hpp"

namespace jolt {
    namespace graphics {
        namespace vulkan {
            class Renderer;

            class JLTAPI DescriptorManager {
              public:
                using descriptor_set_vector = collections::Vector<VkDescriptorSet>;
                using descriptor_set_layout_vector = collections::Vector<VkDescriptorSetLayout>;
                using push_const_range_vector = collections::Vector<VkPushConstantRange>;
                using descriptor_set_layout_binding_vector =
                  collections::Vector<VkDescriptorSetLayoutBinding>;
                using pool_size_vector = collections::Vector<VkDescriptorPoolSize>;

              private:
                Renderer &m_renderer;
                VkDescriptorPool m_descriptor_pool;

                void create_descriptor_pool(
                  uint32_t const max_descriptor_sets, pool_size_vector const &pool_sizes);
                void destroy_descriptor_pool();

              public:
                /**
                 * Create a new descriptor manager.
                 *
                 * @param renderer The renderer.
                 * @param max_descriptor_sets Maximum number of descriptor sets allocatable from the
                 * descriptor pool.
                 * @param pool_sizes Collection of pool size objects to initialize the descriptor pool.
                 */
                DescriptorManager(
                  Renderer &renderer, uint32_t const max_descriptor_sets, pool_size_vector const &pool_sizes);
                ~DescriptorManager();

                VkDescriptorPool get_descriptor_pool() const { return m_descriptor_pool; }
                Renderer const &get_renderer() const { return m_renderer; }

                descriptor_set_vector
                allocate_descriptor_sets(descriptor_set_layout_vector const &descriptor_set_layouts);

                void free_descriptor_sets(descriptor_set_vector const &descriptor_sets);

                /**
                 * Create a new descriptor set layout.
                 *
                 * @param bindings The collection of bindings for the layout.
                 */
                VkDescriptorSetLayout
                create_descriptor_set_layout(descriptor_set_layout_binding_vector const &bindings);

                /**
                 * Destroy a descriptor set layout.
                 *
                 * @param layout The descriptor set layout.
                 */
                void destroy_descriptor_set_layout(VkDescriptorSetLayout layout);

                /**
                 * Create a new pipeline layout.
                 *
                 * @param layouts The descriptor set layouts.
                 * @param pc_ranges The push constant ranges.
                 */
                VkPipelineLayout create_pipeline_layout(
                  descriptor_set_layout_vector const &layouts, push_const_range_vector const &pc_ranges);

                /**
                 * Destroy a pipeline layout.
                 *
                 * @param layout The pipeline layout.
                 */
                void destroy_pipeline_layout(VkPipelineLayout layout);
            };
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_DESCRIPTOR_MGR_HPP */
