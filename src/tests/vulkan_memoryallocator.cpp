#include <jolt/test.hpp>
#include <jolt/jolt.hpp>
#include <jolt/graphics/vulkan/memory-allocator.hpp>

using namespace jolt;
using namespace jolt::graphics::vulkan;

GraphicsEngineInitializationParams gparams{"Jolt test", 1, 0, 0, nullptr, 1, 1, 1};

Renderer renderer;
ui::Window *ui_window;
size_t alloc_mem;

SETUP {
    jolt::initialize();

    console.set_output_stream(&io::standard_error_stream);

    ui_window = jltnew(ui::Window, "Test window - don't close");

    gparams.wnd = ui_window;
    renderer.initialize(gparams);
    alloc_mem = jolt::memory::get_allocated_size();
}

CLEANUP {
    renderer.shutdown();
    ui_window->close();

    jolt::shutdown();
}

TEST(ctor) {
    MemoryAllocator allocator{renderer};

    assert2(allocator.get_physical_regions().get_length() == 0, "Physical memory regions");
    assert2(allocator.get_phy_allocated_size() == 0, "Physical allocated size");
    assert2(allocator.get_allocated_size() == 0, "Virtual allocated size");
}

TEST(allocate) {
    MemoryAllocator allocator{renderer};

    VirtualMemoryRegion *const vmr = allocator.allocate(1024, 1, 0);
    VirtualMemoryRegion *const vmr2 = allocator.allocate(1024, 1, 0);

    assert2(vmr, "NULL");
    assert2(vmr->is_valid(), "Invalid allocation");
    assert2(vmr2 != vmr, "Returned the same virtual memory region for two consecutive allocations");
    assert2(
      vmr->get_physical_region() == vmr2->get_physical_region(),
      "Two different physical memory regions allocated");
}

TEST(allocate__big_chunk) {
    MemoryAllocator allocator{renderer, 1024};

    VirtualMemoryRegion *const vmr = allocator.allocate(2048, 1, 0);

    assert2(vmr, "NULL");
    assert2(vmr->is_valid(), "Invalid allocation");
    assert2(vmr->get_size() == 2048, "Allocation size");
    assert2(
      allocator.get_allocated_size() == allocator.get_phy_allocated_size()
        && allocator.get_allocated_size() == 2048,
      "Allocator size");
}

TEST(free) {
    MemoryAllocator allocator{renderer, 2048};

    VirtualMemoryRegion *const vmr = allocator.allocate(1024, 1, 0);
    assert2(allocator.get_allocated_size() == 1024, "First allocation virtual");
    assert2(allocator.get_phy_allocated_size() == 2048, "First allocation physical");

    VirtualMemoryRegion *const vmr2 = allocator.allocate(1024, 1, 0);
    assert2(allocator.get_allocated_size() == 2048, "Second allocation virtual");
    assert2(allocator.get_phy_allocated_size() == 2048, "Second allocation physical");

    VirtualMemoryRegion *const vmr3 = allocator.allocate(1024, 1, 0);
    assert2(allocator.get_allocated_size() == 3072, "Third allocation virtual");
    assert2(allocator.get_phy_allocated_size() == 4096, "Third allocation physical");

    allocator.free(vmr3);

    assert2(allocator.get_allocated_size() == 2048, "First free virtual");
    assert2(allocator.get_phy_allocated_size() == 4096, "First free physical");

    allocator.free(vmr2);

    assert2(allocator.get_allocated_size() == 1024, "Second free virtual");
    assert2(allocator.get_phy_allocated_size() == 4096, "Second free physical");

    allocator.free(vmr);

    assert2(allocator.get_allocated_size() == 0, "Third free virtual");
    assert2(allocator.get_phy_allocated_size() == 4096, "Third free physical");
}

TEST(allocate__offsets) {
    MemoryAllocator allocator{renderer};

    VirtualMemoryRegion *const vmr = allocator.allocate(1024, 1, 0);
    VkDeviceSize const vmr_offset = vmr->get_offset();
    VirtualMemoryRegion *const vmr2 = allocator.allocate(1024, 1, 0);
    VkDeviceSize const vmr2_offset = vmr2->get_offset();

    allocator.free(vmr2);
    VirtualMemoryRegion *const vmr3 = allocator.allocate(1024, 1, 0);
    VkDeviceSize const vmr3_offset = vmr3->get_offset();

    allocator.free(vmr);
    VirtualMemoryRegion *const vmr4 = allocator.allocate(1024, 1, 0);
    VkDeviceSize const vmr4_offset = vmr4->get_offset();

    assert2(vmr_offset == vmr4_offset, "First offset not correct");
    assert2(vmr2_offset == vmr3_offset, "Last offset not correct");
}

TEST(recycle) {
    MemoryAllocator allocator{renderer, 1024};

    VirtualMemoryRegion *const vmr = allocator.allocate(1024, 1, 0);

    allocator.recycle();

    assert2(allocator.get_allocated_size() == 1024, "Recycled in-use memory region");

    allocator.free(vmr);
    allocator.recycle();

    assert2(allocator.get_allocated_size() == 0, "Didn't recycle properly");
}

TEST(defrag__compact) {
    MemoryAllocator allocator{renderer, 1024};
    MemoryDefrag defrag;
    VkQueue const queue = renderer.acquire_graphics_queue();
    uint32_t const queue_index = renderer.get_queue_family_index(queue);
    CommandPool cmd_pool{renderer, true, false, queue_index};
    CommandBuffer cmd_buffer = cmd_pool.allocate_single_command_buffer(true);

    VirtualMemoryRegion *const vmr1_1 = allocator.allocate(128, 1, 0);
    VirtualMemoryRegion *const vmr1_2 = allocator.allocate(1024 - 128, 1, 0);

    VkDeviceMemory const vmr1_2_memory = vmr1_2->get_physical_region()->get_memory();

    allocator.free(vmr1_1);

    ActionSynchro synchro{};
    cmd_buffer.begin_record();

    defrag.begin_defrag(allocator);
    assert2(defrag.defrag_next_region(cmd_buffer) == false, "defrag_next_region() should return false");

    cmd_buffer.end_record();
    cmd_buffer.submit(queue, synchro);

    vkQueueWaitIdle(queue);
    defrag.end_defrag();

    cmd_pool.free_single_command_buffer(cmd_buffer);

    assert2(allocator.get_physical_regions().get_length() == 1, "Invalid region number");
    assert2(
      allocator.get_physical_regions()[0]->get_allocated_size() == 1024 - 128, "Invalid allocated size");
    assert2(vmr1_2->get_offset() == 0, "Invalid offset");
    assert2(vmr1_2->get_physical_region()->get_memory() != vmr1_2_memory, "Memory handle not updated");
}

TEST(defrag__relocate) {
    MemoryAllocator allocator{renderer, 1024};
    MemoryDefrag defrag;
    VkQueue const queue = renderer.acquire_graphics_queue();
    uint32_t const queue_index = renderer.get_queue_family_index(queue);
    CommandPool cmd_pool{renderer, true, false, queue_index};
    CommandBuffer cmd_buffer = cmd_pool.allocate_single_command_buffer(true);

    VirtualMemoryRegion *const vmr1_1 = allocator.allocate(128, 1, 0);
    VirtualMemoryRegion *const vmr1_2 = allocator.allocate(1024 - 128, 1, 0);
    VirtualMemoryRegion *const vmr2_1 = allocator.allocate(128, 1, 0); // In new region

    allocator.free(vmr1_1);

    ActionSynchro synchro{};
    cmd_buffer.begin_record();

    defrag.begin_defrag(allocator);
    assert2(defrag.defrag_next_region(cmd_buffer) == false, "defrag_next_region() should return false");

    cmd_buffer.end_record();
    cmd_buffer.submit(queue, synchro);

    vkQueueWaitIdle(queue);
    defrag.end_defrag();

    cmd_pool.free_single_command_buffer(cmd_buffer);

    assert2(allocator.get_physical_regions().get_length() == 2, "Invalid region number");
    assert2(allocator.get_physical_regions()[0]->get_allocated_size() == 1024, "Invalid allocated size 1");
    assert2(allocator.get_physical_regions()[1]->get_allocated_size() == 0, "Invalid allocated size 0");
    assert2(vmr2_1->get_offset() == 1024 - 128, "Invalid offset");
    assert2(vmr2_1->get_physical_region() == vmr1_2->get_physical_region(), "Memory region not updated");
}

TEST(memory_leaks) { // Must be last test
    assert(memory::get_allocated_size() == alloc_mem);
}
