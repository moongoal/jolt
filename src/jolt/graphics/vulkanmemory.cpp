#include "vulkan.hpp"

namespace jolt {
    namespace graphics {
        void VulkanMemoryHeap::allocate(VkMemoryPropertyFlags const mem_flags) {
            uint32_t mem_type_index = find_memory_type(mem_flags);

            jltassert2(
              mem_type_index != JLT_VULKAN_INVALID32, "Unable to satisfy memory heap requirements");

            VkMemoryAllocateInfo ainfo{
              VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // sType
              nullptr,                                // pNext
              m_size,                                 // size
              mem_type_index                          // memoryTypeIndex
            };

            VkResult result =
              vkAllocateMemory(m_renderer.get_device(), &ainfo, get_vulkan_allocator(), &m_memory);
            jltassert2(result == VK_SUCCESS, "Unable to allocate device memory");
        }

        void VulkanMemoryHeap::dispose() {
            if(m_memory) {
                vkFreeMemory(m_renderer.get_device(), m_memory, get_vulkan_allocator());
                m_memory = VK_NULL_HANDLE;
            }
        }

        uint32_t VulkanMemoryHeap::find_memory_type(VkMemoryPropertyFlags const mem_flags) {
            VkPhysicalDeviceMemoryProperties const &mem_props =
              m_renderer.get_phy_device_memory_properties();

            for(uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
                VkMemoryType const &mem_type = mem_props.memoryTypes[i];

                if(mem_type.propertyFlags & mem_flags) {
                    return i;
                }
            }

            return JLT_VULKAN_INVALID32;
        }

        void VulkanObjectPool::initialize(uint64_t const n_objects) {
            uint64_t rem = n_objects / 64;

            memset(m_bitmap, 0, m_object_size * n_objects);

            if(rem) {
                uint64_t val = 0;

                for(int i = rem; i > 0; --i) { val |= 1 << (64 - i); }

                m_bitmap[m_bitmap.get_length() - 1] = val;
            }
        }

        bool VulkanObjectPool::is_full() const {
            for(auto cluster : m_bitmap) {
                if(cluster != std::numeric_limits<bit_map::value_type>::max()) {
                    return false;
                }
            }

            return true;
        }

        VulkanObjectPool::pool_handle VulkanObjectPool::bind_to_buffer(VkBuffer const buffer) {
            pool_handle offset = allocate_free_slot() * m_object_size;

            VkResult result =
              vkBindBufferMemory(get_renderer().get_device(), buffer, get_memory(), offset);
            jltassert2(result == VK_SUCCESS, "Unable to bind buffer memory");

            return offset;
        }

        VulkanObjectPool::pool_handle VulkanObjectPool::bind_to_image(VkImage const image) {
            pool_handle offset = allocate_free_slot() * m_object_size;

            VkResult result =
              vkBindImageMemory(get_renderer().get_device(), image, get_memory(), offset);
            jltassert2(result == VK_SUCCESS, "Unable to bind image memory");

            return offset;
        }

        uint64_t VulkanObjectPool::allocate_free_slot() {
            for(size_t i = 0; i < m_bitmap.get_length(); ++i) {
                if(m_bitmap[i] != std::numeric_limits<bit_map::value_type>::max()) {
                    for(size_t j = 0; j < std::numeric_limits<bit_map::value_type>::max(); ++j) {
                        if(~m_bitmap[i] & (1 << j)) {
                            m_bitmap[i] |= (1 << j);

                            return 64 * i + j;
                        }
                    }
                }
            }

            return JLT_VULKAN_INVALID64;
        }

        void VulkanObjectPool::free(pool_handle const offset) {
            uint64_t const cluster = offset / 64;
            uint64_t const slot = offset % 64;
            uint64_t const slot_mask = (1 << slot);

            jltassert2(
              !(m_bitmap[cluster] & slot_mask), "Attempting to free a slot that is not allocated");
            m_bitmap[cluster] &= ~slot_mask;
        }
    } // namespace graphics
} // namespace jolt
