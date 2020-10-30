#include <jolt/graphics/vulkan.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            DescriptorManager::DescriptorManager(
              Renderer &renderer, uint32_t const max_descriptor_sets, pool_size_vector const &pool_sizes) :
              m_renderer{renderer} {
                create_descriptor_pool(max_descriptor_sets, pool_sizes);
            }

            DescriptorManager::~DescriptorManager() { destroy_descriptor_pool(); }

            void DescriptorManager::create_descriptor_pool(
              uint32_t const max_descriptor_sets, pool_size_vector const &pool_sizes) {
                VkDescriptorPoolCreateInfo pcinfo{
                  VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,     // sType
                  nullptr,                                           // pNext
                  VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, // flags
                  max_descriptor_sets,                               // maxSets
                  static_cast<uint32_t>(pool_sizes.get_length()),    // poolSizeCount
                  &pool_sizes[0]                                     // pPoolSizes
                };

                VkResult result = vkCreateDescriptorPool(
                  get_renderer().get_device(), &pcinfo, get_vulkan_allocator(), &m_descriptor_pool);
                jltassert2(result == VK_SUCCESS, "Unable to create graphics pipeline descriptor pool");
            }

            void DescriptorManager::destroy_descriptor_pool() {
                vkDestroyDescriptorPool(
                  get_renderer().get_device(), get_descriptor_pool(), get_vulkan_allocator());
            }

            VkDescriptorSetLayout DescriptorManager::create_descriptor_set_layout(
              descriptor_set_layout_binding_vector const &bindings) {
                VkDescriptorSetLayout layout;

                VkDescriptorSetLayoutCreateInfo cinfo{
                  VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, // sType
                  nullptr,                                             // pNext
                  0,                                                   // flags
                  static_cast<uint32_t>(bindings.get_length()),        // bindingCount
                  &bindings[0],                                        // pBindings
                };

                VkResult result = vkCreateDescriptorSetLayout(
                  get_renderer().get_device(), &cinfo, get_vulkan_allocator(), &layout);
                jltassert2(result == VK_SUCCESS, "Unable to create descriptor set layout");

                return layout;
            }

            void DescriptorManager::destroy_descriptor_set_layout(VkDescriptorSetLayout layout) {
                vkDestroyDescriptorSetLayout(get_renderer().get_device(), layout, get_vulkan_allocator());
            }

            void DescriptorManager::destroy_pipeline_layout(VkPipelineLayout layout) {
                vkDestroyPipelineLayout(get_renderer().get_device(), layout, get_vulkan_allocator());
            }

            VkPipelineLayout DescriptorManager::create_pipeline_layout(
              descriptor_set_layout_vector const &layouts, push_const_range_vector const &pc_ranges) {
                VkPipelineLayoutCreateInfo cinfo{
                  VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,   // sType
                  nullptr,                                         // pNext
                  0,                                               // flags
                  static_cast<uint32_t>(layouts.get_length()),     // setLayoutCount
                  layouts.get_length() ? &layouts[0] : nullptr,    // pSetLayouts
                  static_cast<uint32_t>(pc_ranges.get_length()),   // pushConstanteRangeCount
                  pc_ranges.get_length() ? &pc_ranges[0] : nullptr // pPushConstantRanges
                };

                VkPipelineLayout pipeline_layout;

                VkResult result = vkCreatePipelineLayout(
                  get_renderer().get_device(), &cinfo, get_vulkan_allocator(), &pipeline_layout);
                jltassert2(result == VK_SUCCESS, "Unable to create pipeline layout");

                return pipeline_layout;
            }

            DescriptorManager::descriptor_set_vector DescriptorManager::allocate_descriptor_sets(
              descriptor_set_layout_vector const &descriptor_set_layouts) {
                descriptor_set_vector sets{descriptor_set_layouts.get_length()};

                VkDescriptorSetAllocateInfo ainfo{
                  VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,             // sType
                  nullptr,                                                    // pNext
                  m_descriptor_pool,                                          // descriptorPool
                  static_cast<uint32_t>(descriptor_set_layouts.get_length()), // descriptorSetCount
                  &descriptor_set_layouts[0]                                  // pSetLayouts
                };

                sets.set_length(descriptor_set_layouts.get_length());

                VkResult result = vkAllocateDescriptorSets(m_renderer.get_device(), &ainfo, &sets[0]);
                jltassert2(result == VK_SUCCESS, "Unable to allocate descriptor sets");

                return sets;
            }

            void DescriptorManager::free_descriptor_sets(descriptor_set_vector const &descriptor_sets) {
                if(descriptor_sets.get_length()) {
                    vkFreeDescriptorSets(
                      m_renderer.get_device(),
                      m_descriptor_pool,
                      static_cast<uint32_t>(descriptor_sets.get_length()),
                      &descriptor_sets[0]);
                }
            }
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt
