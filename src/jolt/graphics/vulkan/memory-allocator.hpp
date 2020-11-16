#ifndef JLT_GRAPHICS_VULKAN_ALLOCATOR_HPP
#define JLT_GRAPHICS_VULKAN_ALLOCATOR_HPP

#include <jolt/collections/vector.hpp>
#include <jolt/collections/linkedlist.hpp>
#include <jolt/graphics/vulkan/defs.hpp>

namespace jolt::graphics::vulkan {
    class Renderer;
    class VirtualMemoryRegion;

    using GPUAllocationFlagsBits = uint32_t;

    enum GPUAllocationFlags : GPUAllocationFlagsBits {
        GPU_ALLOC_NONE = 0,
        GPU_ALLOC_NON_RELOCATABLE = 0x00000001 //< Allocations will not be moved when defragmenting memory.
    };

    /// Default physical memory region allocation size.
    static constexpr VkDeviceSize DEFAULT_PHY_REGION_ALLOC_SZ = 10 * 1024 * 1024; // 10 MiB

    /**
     * A memory region directly allocated from the GPU.
     */
    class JLTAPI PhysicalMemoryRegion {
        friend class PhysicalMemoryRegionEditor;

      public:
        using vmr_refs =
          collections::LinkedList<VirtualMemoryRegion *>; /*< Collection of virtual memory regions allocated
                                                             from the same physical region. */

      private:
        uint32_t m_memory_type_index;
        VkDeviceMemory m_memory;
        VkDeviceSize m_size; //< Total memory region size.
        VkDeviceSize m_allocated_size = 0;
        vmr_refs m_refs; //< Virtual memory regions allocated from this physical region.

      public:
        PhysicalMemoryRegion() = default;
        PhysicalMemoryRegion(PhysicalMemoryRegion const &) = delete;
        PhysicalMemoryRegion(PhysicalMemoryRegion &&other) = delete;
        PhysicalMemoryRegion(
          uint32_t const memory_type_index, VkDeviceMemory const memory, VkDeviceSize const size);
        ~PhysicalMemoryRegion();

        /**
         * Register a virtual memory region.
         *
         * @remarks Registration is done by allocators and is used to add a reference to the virtual memory
         * region. This is used both for defragmentation purposes and to keep track of the live allocations.
         *
         * @see unregister_ref()
         */
        void register_ref(VirtualMemoryRegion *const region);

        /**
         * Unregister a virtual memory region.
         *
         * @remarks Unregistration is done by allocators when a virtual memory region is freed.
         *
         * @see register_ref()
         */
        void unregister_ref(VirtualMemoryRegion *const region);

        /**
         * Find a free subregion in this physical memory region.
         *
         * @param size The minimum size requirements for the free region.
         * @param exact True if the region has to be of `size` bytes. False if it can be of the same size or
         * larger than `size`. This only applies to internal gaps - see remarks.
         *
         * @return The offset of the region or JLT_VULKAN_INVALIDSZ if none matchng the criterion was found.
         *
         * @remarks If exact is true and the returned subregion is the at the end of the physical memory
         * region, it is returned even if its size is larger than `size`. Same happens when a region is empty.
         */
        VkDeviceSize find_free_region(VkDeviceSize const size, bool const exact = false) const;

        uint32_t get_memory_type_index() const { return m_memory_type_index; }
        VkDeviceMemory get_memory() const { return m_memory; }

        /**
         * @return The total size of the region.
         */
        VkDeviceSize get_size() const { return m_size; }

        vmr_refs const &get_references() const { return m_refs; }
        vmr_refs &get_references() { return m_refs; }

        /**
         * @return The total number of allocated bytes.
         */
        VkDeviceSize get_allocated_size() const { return m_allocated_size; }

        /**
         * @return The total number of non-allocated bytes.
         */
        VkDeviceSize get_available_size() const { return get_size() - get_allocated_size(); }

        /// @{
        /**
         * Compare two physical memory regions for equality. Two equal physical memory regions have the same
         * memory type index and memory location.
         */
        bool operator==(PhysicalMemoryRegion &other) const;
        bool operator!=(PhysicalMemoryRegion &other) const;
        /// @}
    };

    class PhysicalMemoryRegionEditor {
        PhysicalMemoryRegion &m_region;

      public:
        PhysicalMemoryRegionEditor(PhysicalMemoryRegion &region) : m_region{region} {}

        void set_allocated_size(VkDeviceSize const value) { m_region.m_allocated_size = value; }
        void increase_allocated_size(VkDeviceSize const value) { m_region.m_allocated_size += value; }
        void decrease_allocated_size(VkDeviceSize const value) { increase_allocated_size(-value); }
        void set_memory(VkDeviceMemory const value) { m_region.m_memory = value; }
    };

    /**
     * A virtual memory region allocated from a physical memory region.
     *
     * @remarks Virtual meomry regions are much cheaper to allocate and free in comparison to physical memory
     * regions because they require no driver intervention.
     */
    class VirtualMemoryRegion {
        friend class VirtualMemoryRegionEditor;

        PhysicalMemoryRegion *m_phy_region = nullptr;
        VkDeviceSize m_phy_offset = 0; //< Offset from the beginning of the physical memory region.
        VkDeviceSize m_size = 0;
        VkDeviceSize m_alignment = 0; //< Alignment requirement.
        VkDeviceSize m_padding = 0;   //< Amount of bytes used as padding to ensure alignment.
        GPUAllocationFlags m_flags = GPU_ALLOC_NONE;

      public:
        VirtualMemoryRegion() = default;
        VirtualMemoryRegion(VirtualMemoryRegion const &) = delete;
        VirtualMemoryRegion(VirtualMemoryRegion &&other) = delete;

        VirtualMemoryRegion(
          PhysicalMemoryRegion &phy_region,
          VkDeviceSize const phy_offset,
          VkDeviceSize const size,
          VkDeviceSize const alignment,
          VkDeviceSize const padding,
          GPUAllocationFlags const flags) :
          m_phy_region{&phy_region},
          m_phy_offset{phy_offset}, m_size{size}, m_alignment{alignment}, m_padding{padding}, m_flags{flags} {
        }

        /**
         * Check if this region is valid (i.e. its parent physical memory region is not NULL).
         */
        bool is_valid() const { return m_phy_region; }
        PhysicalMemoryRegion *get_physical_region() const { return m_phy_region; }
        VkDeviceSize get_offset() const { return m_phy_offset; }
        VkDeviceSize get_size() const { return m_size; }
        VkDeviceSize get_alignment() const { return m_alignment; }
        VkDeviceSize get_padding() const { return m_padding; }
        VkDeviceMemory get_memory() const { return m_phy_region ? m_phy_region->get_memory() : nullptr; }

        /**
         * @return A boolean value stating whether the this region can be relocated during defragmentation.
         */
        bool is_relocatable() const { return !(m_flags & GPU_ALLOC_NON_RELOCATABLE); }
        GPUAllocationFlags get_flags() const { return m_flags; }

        void set_flags(GPUAllocationFlags const value) { m_flags = value; }

        /// @{
        /**
         * Compares two virtual memory regions for equality. Two virtual memory regions are equal if their
         * physical memory regions and offsets are the same.
         */
        bool operator==(VirtualMemoryRegion &other) const {
            return m_phy_region == other.m_phy_region && m_phy_offset == other.m_phy_offset;
        }

        bool operator!=(VirtualMemoryRegion &other) const { return !(*this == other); }
        ///@}
    };

    class VirtualMemoryRegionEditor {
        VirtualMemoryRegion &m_region;

      public:
        explicit VirtualMemoryRegionEditor(VirtualMemoryRegion &region) : m_region{region} {}

        void set_offset(VkDeviceSize const value) { m_region.m_phy_offset = value; }
        void set_size(VkDeviceSize const value) { m_region.m_size = value; }
        void set_padding(VkDeviceSize const value) { m_region.m_padding = value; }
        void set_physical_region(PhysicalMemoryRegion *const value) { m_region.m_phy_region = value; }
    };

    /**
     * A GPU memory allocator.
     */
    class JLTAPI MemoryAllocator {
      public:
        using phy_regions = collections::Vector<PhysicalMemoryRegion *>;

      private:
        struct find_region_by_memory_type_result {
            PhysicalMemoryRegion *region;
            VkDeviceSize offset;
        };

        Renderer &m_renderer;
        phy_regions m_phy_regions;              //< Collection of physical memory regions currently allocated.
        VkDeviceSize const m_phy_region_min_sz; //< Minimum size for each allocated physical memory region.

        /**
         * Free a physical memory region.
         */
        void free_phy(PhysicalMemoryRegion *const phy);

        /**
         * Allocate a physical memory region.
         *
         * @param size The size of the allocation.
         * @param memory_type_index Index of the memory type.
         */
        PhysicalMemoryRegion *allocate_phy(VkDeviceSize const size, uint32_t const memory_type_index);

        /**
         * Find an allocated physical memory region with the given memory type index and minimum required
         * size.
         *
         * @param memory_type_index The memory type index to look for.
         * @param required_size The minimum contiguous size requirement to look for.
         *
         * @return A valid `find_region_by_memory_type_result` instance or {nullptr, 0}.
         */
        find_region_by_memory_type_result
        find_region_by_memory_type(uint32_t const memory_type_index, VkDeviceSize const required_size);

      public:
        /**
         * Create a new memory allocator instance.
         *
         * @param renderer The renderer.
         * @param phy_sz The minimum size for each allocated physical memory region.
         */
        explicit MemoryAllocator(Renderer &renderer, VkDeviceSize const phy_sz = DEFAULT_PHY_REGION_ALLOC_SZ);
        ~MemoryAllocator();

        ///@{
        /**
         * Allocate a new GPU memory region.
         */

        /**
         * @param size The size of the allocation.
         * @param alignment The alignment requirement.
         * @param memory_type_index Index of the memory type.
         * @param flags The allocation flags.
         */
        VirtualMemoryRegion *allocate(
          VkDeviceSize const size,
          VkDeviceSize const alignment,
          uint32_t const memory_type_index,
          GPUAllocationFlags flags = GPU_ALLOC_NONE);

        /**
         * @param mem_prop_flags The memory property flags requirement.
         * @param memory_requirements The memory requirements.
         * @param flags The allocation flags.
         */
        VirtualMemoryRegion *allocate(
          VkMemoryPropertyFlags const mem_prop_flags,
          VkMemoryRequirements &memory_requirements,
          GPUAllocationFlags flags = GPU_ALLOC_NONE);
        ///@}

        /**
         * Free a GPU allocation.
         *
         * @param region The GPU memory region to free.
         */
        void free(VirtualMemoryRegion *const region);

        /**
         * Return any unused physical memory region to the GPU.
         *
         * @remarks Recycling memory is responsibility of the application.
         */
        void recycle();

        phy_regions get_physical_regions() { return m_phy_regions; }
        phy_regions const get_physical_regions() const { return m_phy_regions; }

        /**
         * Return the virtually allocated amount of memory for one specific memory type. This is the amount of
         * memory occupied by the virtual memory regions.
         *
         * @param memory_type_mask A mask of memory type indices to include in the count.
         */
        VkDeviceSize get_allocated_size(uint32_t memory_type_mask = UINT32_MAX) const;

        /**
         * Return the amount of memory allocated from the GPU for one specific memory type. This is the amount
         * of memory allocated from the device.
         *
         * @param memory_type_mask A mask of memory type indices to include in the count.
         */
        VkDeviceSize get_phy_allocated_size(uint32_t memory_type_mask = UINT32_MAX) const;

        Renderer &get_renderer() { return m_renderer; }
        Renderer const &get_renderer() const { return m_renderer; }
    };

    /**
     * GPU memory defragmentation facility.
     *
     * The MemoryDefrag class allows for the resources used during the defragmentation process to be reused.
     * The defragmentation process relocates relocatable virtual memory regions in order to compat fragmented
     * memory and reduce unusable (too small) gaps.
     *
     * This class defragments memory in two ways:
     * - Internally, within the same physical memory region
     * - Externally, between different physical memory region with the same memory type.
     *
     * A defragmentation process must be run in steps, must start with a call to `begin_defrag()` and end with
     * one to `end_defrag()`. Each call to `defrag_next_region()` will defragment one physical memory region.
     *
     * Since source and destination memory locations cannot overlap on the GPU during a copy operation, in
     * order to avoid gaps between memory locations, a new block is allocated when needed and used to compact
     * and relocate the data.
     *
     * Images can only be moved if in the VK_IMAGE_LAYOUT_GENERAL layout (see
     * https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/chap12.html#resources-memory-aliasing),
     * hence images with a different layout must either be transitioned or marked as non-relocatable.
     */
    class JLTAPI MemoryDefrag {
      public:
        using relocated_regions = collections::Vector<VirtualMemoryRegion *>;

      private:
        struct DefragPhysicalMemoryRegion {
            PhysicalMemoryRegion *phy;
            VkBuffer buffer = VK_NULL_HANDLE;               //< Temporary copy buffer associated with `phy`.
            VkDeviceMemory compact_memory = VK_NULL_HANDLE; //< Temporary memory used for compacting region.
            VkBuffer compact_memory_buffer =
              VK_NULL_HANDLE;       //< Temporary copy buffer associated with `compact_memory`.
            bool compacted = false; //< Boolean value stating whether the region has already been compacted.

            bool operator==(DefragPhysicalMemoryRegion const &other) const { return phy == other.phy; }
            bool operator!=(DefragPhysicalMemoryRegion const &other) const { return phy != other.phy; }
        };

        using defrag_mem_regions = collections::Vector<DefragPhysicalMemoryRegion>;
        using defrag_mem_region_ptrs = collections::Vector<DefragPhysicalMemoryRegion>;

        Renderer *m_renderer = nullptr;
        relocated_regions m_relocated; //< Collection of relocated regions.
        defrag_mem_regions m_regions;  //< All physical regions to defragment.
        defrag_mem_regions m_finished; //< Defragmented physical regions.

        /**
         * Compact a single memory region by moving virtual memory regions within the boundaries of the
         * physical memory region.
         *
         * @param region The region to compact.
         */
        void compact_region(VkCommandBuffer const cmd_buffer, DefragPhysicalMemoryRegion &region);

        /**
         * Defrag a couple of memory regions with the same memory type index by moving virtual memory regions
         * between them.
         *
         * @param b The memory region with the highest allocated size.
         * @param e The memory region with the lowest allocated size.
         */
        void defrag_region_couple(
          VkCommandBuffer const cmd_buffer, DefragPhysicalMemoryRegion &b, DefragPhysicalMemoryRegion &e);

        /**
         * Relocate a virtual memory region to a different physical memory region.
         *
         * @param cmd_buffer The command buffer where to record the copy commands.
         * @param v The virtual memory region to move.
         * @param e The source physical memory region.
         * @param b The destination physical memory region.
         *
         * @remarks Moving a virtual memory region to its own physical memory region is an error.
         */
        void relocate_region(
          VkCommandBuffer const cmd_buffer,
          VirtualMemoryRegion &v,
          DefragPhysicalMemoryRegion &e,
          DefragPhysicalMemoryRegion &b);

        /**
         * Create a temporary copy buffer for the given memory region region.
         *
         * @param region The memory region to create the buffer for.
         *
         * @remarks It is an error to create a buffer twice for the same region.
         */
        void create_copy_buffer(DefragPhysicalMemoryRegion &region);

        /**
         * Create a temporary copy buffer for the given memory region region.
         *
         * @param memory The memory region for which to create the buffer.
         * @param size The size of the region.
         * @param memory_type_index The index of the memory type of the region.
         *
         * @return A valid Vulkan buffer handle or `VK_NULL_HANDLE` on failure.
         */
        VkBuffer create_copy_buffer(
          VkDeviceMemory const memory, VkDeviceSize const size, uint32_t const memory_type_index);

        /**
         * Find the region in the collection with the least used memory region having a specific memory type.
         *
         * @param regions The collection of regions.
         * @param mem_type_index The memory type index.
         *
         * @return The index in `regions` of the region with memory type index `mem_type_index` with the
         * smallest amount of allocated memory or `SIZE_MAX` if no region with that memory type was found.
         */
        static size_t
        find_least_used_region_by_memory_type(defrag_mem_regions &regions, uint32_t const mem_type_index);

        /**
         * Return a value stating whether a region needs compacting.
         *
         * @param region The region to check.
         *
         * @return True if the region needs to be compacted, false otherwise.
         */
        bool needs_compacting(DefragPhysicalMemoryRegion &region);

        /**
         * Free any resources used in the previous step for compacting physical memory regions.
         */
        void clean_compacting_resources();

      public:
        /**
         * Begin the defragmentation process.
         *
         * @param allocator The allocator to defragment.
         * @param cmd_buffer The command buffer where the defragmentation commands will be recorded.
         * The given command buffer must already be in a recordable state.
         *
         * @remarks It is an error to call this function if another defragmentation process is ongoing.
         */
        void begin_defrag(MemoryAllocator &allocator);

        /**
         * Defragment the next region.
         *
         * @return True if there are more regions left to defrag, false if all regions have been defragmented.
         */
        bool defrag_next_region(VkCommandBuffer const cmd_buffer);

        /**
         * End the defragmentation process.
         *
         * @remarks It is an error to call this function if no defragmentation process is ongoing.
         */
        void end_defrag();

        /**
         * Get the collection of virtual memory regions that were relocated during the defragmentation
         * process.
         */
        relocated_regions const &get_relocated_regions() const { return m_relocated; }
    };

    /**
     * Allocate raw memory from the GPU.
     *
     * @param device The device to allocate the memory from.
     * @param size The amount of memory to allocate.
     * @param memory_type_index The index of the memory type.
     *
     * @return A valid device memory value or VK_NULL_HANDLE if the memory could not be allocated.
     */
    VkDeviceMemory allocate(VkDevice const device, VkDeviceSize const size, uint32_t const memory_type_index);
} // namespace jolt::graphics::vulkan

#endif /* JLT_GRAPHICS_VULKAN_ALLOCATOR_HPP */
