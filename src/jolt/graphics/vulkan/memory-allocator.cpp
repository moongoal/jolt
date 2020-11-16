#include <jolt/graphics/vulkan.hpp>
#include <jolt/algorithms.hpp>
#include "memory-allocator.hpp"

using namespace jolt::algorithms;

namespace jolt::graphics::vulkan {
    PhysicalMemoryRegion::PhysicalMemoryRegion(
      uint32_t const memory_type_index, VkDeviceMemory const memory, VkDeviceSize const size) :
      m_memory_type_index{memory_type_index},
      m_memory{memory}, m_size{size} {}

    PhysicalMemoryRegion::~PhysicalMemoryRegion() {
        jltassert2(
          m_refs.get_length() == 0,
          "Finalized a physical memory region but referencing virtual memory regions still present");
    }

    void PhysicalMemoryRegion::register_ref(VirtualMemoryRegion *const region) {
        vmr_refs::Node *prev = nullptr, *cur = m_refs.get_first_node();
        VkDeviceSize const offset = region->get_offset();

        while(cur) {
            if(cur->get_value()->get_offset() > offset) {
                break;
            }

            prev = cur;
            cur = prev->get_next();
        }

        m_refs.add_after(region, prev);
        m_allocated_size += region->get_size();
    }

    void PhysicalMemoryRegion::unregister_ref(VirtualMemoryRegion *const region) {
        vmr_refs::Node *const node = m_refs.find(region);

        m_allocated_size -= region->get_size();

        jltassert2(node, "Attempting to unregister a non-registered reference");

        m_refs.remove(*node);
    }

    VkDeviceSize PhysicalMemoryRegion::find_free_region(VkDeviceSize const size, bool const exact) const {
        if(m_refs.get_length()) {
            VirtualMemoryRegion *const first = m_refs.get_first();

            if(choose(
                 first->get_offset() == size,
                 first->get_offset() >= size,
                 exact)) { // Gap between beginning and first allocation
                return 0;
            }

            // Inner gaps
            vmr_refs::Node *prev = m_refs.get_first_node(),
                           *cur = prev->get_next() /* Safe as we know length > 0 */;

            while(cur) {
                VirtualMemoryRegion &prev_region = *prev->get_value();
                VirtualMemoryRegion &cur_region = *cur->get_value();
                VkDeviceSize const gap_begin = prev_region.get_offset() + prev_region.get_size();
                VkDeviceSize const gap_size = cur_region.get_offset() - gap_begin;

                if(choose(gap_size == size, gap_size >= size, exact)) {
                    return gap_begin;
                }

                // Next gap
                prev = cur;
                cur = prev->get_next();
            }

            // Gap between last and end - prev now is the last region in the list
            VirtualMemoryRegion &prev_region = *prev->get_value();
            VkDeviceSize const gap_begin = prev_region.get_offset() + prev_region.get_size();
            VkDeviceSize const gap_size = m_size - gap_begin;

            if(gap_size >= size) {
                return gap_begin;
            }
        } else { // Region is empty
            if(m_size >= size) {
                return 0;
            }
        }

        return JLT_VULKAN_INVALIDSZ;
    }

    bool PhysicalMemoryRegion::operator==(PhysicalMemoryRegion &other) const {
        return m_memory_type_index == other.m_memory_type_index && m_memory == other.m_memory;
    }

    bool PhysicalMemoryRegion::operator!=(PhysicalMemoryRegion &other) const { return !(*this == other); }

    MemoryAllocator::MemoryAllocator(Renderer &renderer, VkDeviceSize const phy_sz) :
      m_renderer{renderer}, m_phy_region_min_sz{phy_sz} {}

    MemoryAllocator::~MemoryAllocator() {
        for(PhysicalMemoryRegion *const phy : m_phy_regions) {
            for(VirtualMemoryRegion *const vmr : phy->get_references()) { jltfree(vmr); }

            phy->get_references().clear();
            free_phy(phy);
        }
    }

    VkDeviceMemory
    allocate(VkDevice const device, VkDeviceSize const size, uint32_t const memory_type_index) {
        VkDeviceMemory memory;
        VkMemoryAllocateInfo ainfo{
          VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // sType
          nullptr,                                // pNext
          size,                                   // allocationSize
          memory_type_index                       // memoryTypeIndex
        };

        VkResult const result = vkAllocateMemory(device, &ainfo, get_vulkan_allocator(), &memory);

        switch(result) {
            case VK_SUCCESS:
                return memory;

            case VK_ERROR_OUT_OF_HOST_MEMORY:
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return VK_NULL_HANDLE;

            default:
                jltassert2(false, "Error while attempting to allocate device memory");
                return VK_NULL_HANDLE; // Will never return
        };
    }

    PhysicalMemoryRegion *
    MemoryAllocator::allocate_phy(VkDeviceSize const size, uint32_t const memory_type_index) {
        VkDeviceSize const actual_size = max(size, m_phy_region_min_sz);

        if(actual_size > m_phy_region_min_sz)
            JLT_UNLIKELY {
                /*
                    When too many of this are logged, you may want to increase your default chunk size or
                    use a different allocator for larger chunks. Many frequent allocations exceeding this
                    limit may end up slowing down the allocation.
                */
                console.debug("Requested allocation of a chunk bigger than the default");
            }

        VkDeviceMemory const memory =
          vulkan::allocate(m_renderer.get_device(), actual_size, memory_type_index);

        return memory != VK_NULL_HANDLE ? jltnew(PhysicalMemoryRegion, memory_type_index, memory, actual_size)
                                        : nullptr;
    }

    void MemoryAllocator::free_phy(PhysicalMemoryRegion *const phy) {
        vkFreeMemory(m_renderer.get_device(), phy->get_memory(), get_vulkan_allocator());

        jltfree(phy);
    }

    MemoryAllocator::find_region_by_memory_type_result MemoryAllocator::find_region_by_memory_type(
      uint32_t const memory_type_index, VkDeviceSize const required_size) {
        for(PhysicalMemoryRegion *const region : m_phy_regions) {
            if(region->get_memory_type_index() == memory_type_index) {
                VkDeviceSize const offset = region->find_free_region(required_size);

                if(offset != JLT_VULKAN_INVALIDSZ) {
                    return {region, offset};
                }
            }
        }

        return {nullptr, 0};
    }

    VirtualMemoryRegion *MemoryAllocator::allocate(
      VkDeviceSize const size,
      VkDeviceSize const alignment,
      uint32_t const memory_type_index,
      GPUAllocationFlags flags) {
        VkDeviceSize const required_size = size + alignment - 1;
        find_region_by_memory_type_result find_region_by_memory_type_result =
          find_region_by_memory_type(memory_type_index, required_size);
        PhysicalMemoryRegion *phy = find_region_by_memory_type_result.region;
        VkDeviceSize const phy_offset_unaligned = find_region_by_memory_type_result.offset;

        if(!phy) {
            phy = allocate_phy(size, memory_type_index);

            if(!phy) {
                return nullptr; // Out of memory
            }

            m_phy_regions.push(phy);

            // phy_offset_unaligned is already 0
        }

        VkDeviceSize const phy_offset = reinterpret_cast<VkDeviceSize>(
          jolt::align_raw_ptr(reinterpret_cast<void *>(phy_offset_unaligned), alignment));

        VkDeviceSize const padding = phy_offset - phy_offset_unaligned;

        auto region = jltnew(VirtualMemoryRegion, *phy, phy_offset, size, alignment, padding, flags);

        phy->register_ref(region);

        return region;
    }

    VirtualMemoryRegion *MemoryAllocator::allocate(
      VkMemoryPropertyFlags const mem_prop_flags,
      VkMemoryRequirements &memory_requirements,
      GPUAllocationFlags flags) {
        uint32_t const mem_type_index =
          m_renderer.get_memory_type_index(mem_prop_flags, 0, memory_requirements.memoryTypeBits);

        if(!mem_type_index) {
            return nullptr;
        }

        return allocate(memory_requirements.size, memory_requirements.alignment, mem_type_index, flags);
    }

    void MemoryAllocator::free(VirtualMemoryRegion *const region) {
        if(region->is_valid()) {
            region->get_physical_region()->unregister_ref(region);
        }

        jltfree(region);
    }

    void MemoryAllocator::recycle() {
        collections::Vector<PhysicalMemoryRegion *> recycled_regions;

        for(PhysicalMemoryRegion *const region : m_phy_regions) {
            if(region->get_references().get_length() == 0) {
                free_phy(region);
                recycled_regions.push(region);
            }
        }

        for(PhysicalMemoryRegion *const region : recycled_regions) { m_phy_regions.remove(region); }
    }

    VkDeviceSize MemoryAllocator::get_allocated_size(uint32_t memory_type_mask) const {
        VkDeviceSize size = 0;

        for(PhysicalMemoryRegion *const region : m_phy_regions) {
            if((1UL << region->get_memory_type_index()) & memory_type_mask) {
                size += region->get_allocated_size();
            }
        }

        return size;
    }

    VkDeviceSize MemoryAllocator::get_phy_allocated_size(uint32_t memory_type_mask) const {
        VkDeviceSize size = 0;

        for(PhysicalMemoryRegion *const region : m_phy_regions) {
            if((1UL << region->get_memory_type_index()) & memory_type_mask) {
                size += region->get_size();
            }
        }

        return size;
    }

    void MemoryDefrag::begin_defrag(MemoryAllocator &allocator) {
        jltassert2(
          m_renderer == nullptr, "Attmepting to begin defragmentation but process is already ongoing");

        m_relocated.clear();
        m_renderer = &allocator.get_renderer();

        // Populate the vector of regions to defragment, only taking into consideration non-empty and non-full
        // regions.
        for(PhysicalMemoryRegion *const phy : allocator.get_physical_regions()) {
            VkDeviceSize const allocated_size = phy->get_allocated_size();

            if(allocated_size > 0 && allocated_size < phy->get_size()) {
                DefragPhysicalMemoryRegion region{};

                region.phy = phy;

                create_copy_buffer(region);

                if(region.buffer != VK_NULL_HANDLE) {
                    m_regions.push(region);
                } else {
                    console.warn("Unable to create temporary copy buffer. Memory region will not participate "
                                 "to defragmentation.");
                }
            }
        }

        if(m_regions.get_length()) {
            quicksort(&m_regions[0], m_regions.get_length(), [](DefragPhysicalMemoryRegion &r) {
                return r.phy->get_allocated_size();
            });
        }
    }

    bool MemoryDefrag::defrag_next_region(VkCommandBuffer const cmd_buffer) {
        jltassert2(
          m_renderer, "Attmepting to defragment a region but no defragmentation process was started");

        jltassert2(
          cmd_buffer != VK_NULL_HANDLE,
          "Attempting to defrag a region but no defragmentation process is ongoing");

        if(!m_regions.get_length()) {
            return false;
        }

        clean_compacting_resources();

        bool remove_b = true;
        bool remove_e = false;
        size_t const b_index = m_regions.get_length() - 1;
        size_t e_index = 0;

        DefragPhysicalMemoryRegion &b = m_regions[b_index]; // Region with most allocated memory
        uint32_t const b_memory_type_index = b.phy->get_memory_type_index();

        // Find the region with least allocated memory of the same memory type as `b`.
        e_index = find_least_used_region_by_memory_type(m_regions, b_memory_type_index);

        compact_region(cmd_buffer, b);

        if(e_index != SIZE_MAX) {
            DefragPhysicalMemoryRegion &e = m_regions[e_index]; // Region with least allocated memory

            if(e != b) {
                defrag_region_couple(cmd_buffer, b, e);

                if(e.phy->get_allocated_size() == 0) {
                    remove_e = true;
                    remove_b = choose(false, remove_b, b.phy->get_allocated_size() != b.phy->get_size());
                }
            }
        }

        // `b` always comes after `e` in `m_regions` because of sorting.
        // Do not invert these two condition blocks.
        if(remove_b) {
            m_finished.push(b);
            m_regions.remove_at(b_index);
        }

        if(remove_e) {
            m_finished.push(m_regions[e_index]);
            m_regions.remove_at(e_index);
        }

        return m_regions.get_length();
    }

    size_t MemoryDefrag::find_least_used_region_by_memory_type(
      defrag_mem_regions &regions, uint32_t const mem_type_index) {
        size_t index = SIZE_MAX;
        VkDeviceSize allocated_mem = std::numeric_limits<VkDeviceSize>::max();

        for(size_t i = 0; i < regions.get_length(); ++i) {
            PhysicalMemoryRegion &phy = *regions[i].phy;

            if(phy.get_memory_type_index() == mem_type_index && phy.get_allocated_size() < allocated_mem) {
                index = i;
                allocated_mem = phy.get_allocated_size();
            }
        }

        return index;
    }

    bool MemoryDefrag::needs_compacting(DefragPhysicalMemoryRegion &region) {
        PhysicalMemoryRegion &phy = *region.phy;
        bool has_non_relocatable_vmrs = false;

        for(VirtualMemoryRegion *const vmr : region.phy->get_references()) {
            if(!vmr->is_relocatable()) {
                has_non_relocatable_vmrs = true;
                break;
            }
        }

        return !region.compacted && phy.get_references().get_length() != 0
               && phy.find_free_region(1) != phy.get_allocated_size() && !has_non_relocatable_vmrs;
    }

    void MemoryDefrag::compact_region(VkCommandBuffer const cmd_buffer, DefragPhysicalMemoryRegion &region) {
        if(!needs_compacting(region)) {
            region.compacted = true;

            return;
        }
        PhysicalMemoryRegion &phy = *region.phy;

        region.compact_memory =
          vulkan::allocate(m_renderer->get_device(), phy.get_size(), phy.get_memory_type_index());

        if(region.compact_memory == VK_NULL_HANDLE) { // No space left
            region.compacted = true;

            return;
        }

        region.compact_memory_buffer =
          create_copy_buffer(region.compact_memory, phy.get_size(), phy.get_memory_type_index());

        if(region.compact_memory_buffer == VK_NULL_HANDLE) { // No space left
            region.compacted = true;

            return;
        }

        VkDeviceSize cur_offset_unaligned = 0;
        collections::Vector<VkBufferMemoryBarrier> barriers;

        for(VirtualMemoryRegion *const vmr : phy.get_references()) {
            VkDeviceSize const new_offset = reinterpret_cast<VkDeviceSize>(
              jolt::align_raw_ptr(reinterpret_cast<void *>(cur_offset_unaligned), vmr->get_alignment()));

            VkBufferCopy copy_region{
              vmr->get_offset() + vmr->get_padding(), // srcOffset
              new_offset,                             // dstOffset
              vmr->get_size() - vmr->get_padding()    // size
            };

            vkCmdCopyBuffer(cmd_buffer, region.buffer, region.compact_memory_buffer, 1, &copy_region);

            barriers.push({
              VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,                // sType
              nullptr,                                                // pNext
              VK_ACCESS_TRANSFER_WRITE_BIT,                           // srcAccessMask
              VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT, // dstAccessMask
              VK_QUEUE_FAMILY_IGNORED,                                // srcQueueFamilyIndex
              VK_QUEUE_FAMILY_IGNORED,                                // dstQueueFamilyIndex
              region.compact_memory_buffer,                           // buffer
              copy_region.dstOffset,                                  // offset
              copy_region.size                                        // size
            });

            // Swap memory refs
            PhysicalMemoryRegionEditor p_editor{phy};
            VkDeviceMemory const tmp_memory = phy.get_memory();

            p_editor.set_memory(region.compact_memory);
            region.compact_memory = tmp_memory;

            // Update vmr metadata
            VirtualMemoryRegionEditor v_editor{*vmr};

            v_editor.set_offset(new_offset);
            v_editor.set_padding(new_offset - cur_offset_unaligned);

            m_relocated.push(vmr);
            cur_offset_unaligned += vmr->get_size();
        }

        vkCmdPipelineBarrier(
          cmd_buffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_DEPENDENCY_BY_REGION_BIT,
          0,
          nullptr,
          barriers.get_length(),
          barriers.get_length() ? &barriers[0] : nullptr,
          0,
          nullptr);

        region.compacted = true;
    }

    void MemoryDefrag::defrag_region_couple(
      VkCommandBuffer const cmd_buffer, DefragPhysicalMemoryRegion &b, DefragPhysicalMemoryRegion &e) {
        jltassert2(b != e, "Attempting to defragment a couple of regions made by the same region");

        VkDeviceSize b_free_size = b.phy->get_available_size();

        collections::Vector<VirtualMemoryRegion *> vm_regions;

        // Only consider relocatable regions.
        for(VirtualMemoryRegion *region : e.phy->get_references()) {
            if(region->is_relocatable()) {
                vm_regions.push(region);
            }
        }

        if(!vm_regions.get_length()) {
            return;
        }

        quicksort(&vm_regions[0], vm_regions.get_length(), [](VirtualMemoryRegion *r) {
            return static_cast<long long>(r->get_size()) * -1;
        });

        auto it = vm_regions.begin(), end = vm_regions.end();

        for(; it != end; ++it) {
            VirtualMemoryRegion *const v = *it;

            if(b_free_size >= v->get_size()) {
                relocate_region(cmd_buffer, *v, e, b);
            }
        }
    }

    void MemoryDefrag::relocate_region(
      VkCommandBuffer const cmd_buffer,
      VirtualMemoryRegion &v,
      DefragPhysicalMemoryRegion &e,
      DefragPhysicalMemoryRegion &b) {
        jltassert2(v.is_valid(), "Attempting to relocate an invalid virtual memory region");

        jltassert2(
          v.get_physical_region() != b.phy,
          "Attempting to relocate a virtual memory region in its own physical memory region");

        VkDeviceSize const new_unaligned_offset = b.phy->find_free_region(v.get_size(), true);

        if(new_unaligned_offset != JLT_VULKAN_INVALIDSZ) {
            VkDeviceSize const new_offset = reinterpret_cast<VkDeviceSize>(
              jolt::align_raw_ptr(reinterpret_cast<void *>(new_unaligned_offset), v.get_alignment()));

            VkBufferCopy region{
              v.get_offset() + v.get_padding(), // srcOffset
              new_offset,                       // dstOffset
              v.get_size() - v.get_padding()    // size
            };

            vkCmdCopyBuffer(cmd_buffer, e.buffer, b.buffer, 1, &region);

            // Update metadata
            VirtualMemoryRegionEditor v_editor{v};
            v_editor.set_offset(new_offset);
            v_editor.set_physical_region(b.phy);

            e.phy->unregister_ref(&v);
            b.phy->register_ref(&v);

            m_relocated.push(&v);
        }
    }

    void MemoryDefrag::create_copy_buffer(DefragPhysicalMemoryRegion &region) {
        region.buffer = create_copy_buffer(
          region.phy->get_memory(), region.phy->get_size(), region.phy->get_memory_type_index());
    }

    VkBuffer MemoryDefrag::create_copy_buffer(
      VkDeviceMemory const memory, VkDeviceSize const size, uint32_t const memory_type_index) {
        VkBuffer buffer;
        VkBufferCreateInfo cinfo{
          VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,                                // sType
          nullptr,                                                             // pNext
          0,                                                                   // flags
          size,                                                                // size
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, // usage
          VK_SHARING_MODE_EXCLUSIVE,                                           // sharingMode
          0,                                                                   // queueFamilyIndexCount
          nullptr                                                              // pQueueFamilyIndices
        };

        VkResult result = vkCreateBuffer(m_renderer->get_device(), &cinfo, get_vulkan_allocator(), &buffer);

        if(result == VK_SUCCESS) {
            VkMemoryRequirements mem_req;

            vkGetBufferMemoryRequirements(m_renderer->get_device(), buffer, &mem_req);

            if(mem_req.size == size && ((1UL << memory_type_index) & mem_req.memoryTypeBits)) {
                result = vkBindBufferMemory(m_renderer->get_device(), buffer, memory, 0);

                if(result == VK_SUCCESS) {
                    return buffer;
                }
            }
        }

        return VK_NULL_HANDLE;
    }

    void MemoryDefrag::clean_compacting_resources() {
        for(DefragPhysicalMemoryRegion &region : m_finished) {
            if(region.compact_memory_buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(
                  m_renderer->get_device(), region.compact_memory_buffer, get_vulkan_allocator());

                region.compact_memory_buffer = VK_NULL_HANDLE;
            }

            if(region.compact_memory != VK_NULL_HANDLE) {
                vkFreeMemory(m_renderer->get_device(), region.compact_memory, get_vulkan_allocator());

                region.compact_memory = VK_NULL_HANDLE;
            }
        }
    }

    void MemoryDefrag::end_defrag() {
        for(DefragPhysicalMemoryRegion &region : m_finished) {
            if(region.compact_memory_buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(
                  m_renderer->get_device(), region.compact_memory_buffer, get_vulkan_allocator());
            }

            if(region.compact_memory != VK_NULL_HANDLE) {
                vkFreeMemory(m_renderer->get_device(), region.compact_memory, get_vulkan_allocator());
            }

            if(region.buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(m_renderer->get_device(), region.buffer, get_vulkan_allocator());
            }
        }

        m_finished.clear();
        m_renderer = nullptr;
    }
} // namespace jolt::graphics::vulkan
