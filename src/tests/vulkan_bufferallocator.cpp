#include <jolt/test.hpp>
#include <jolt/jolt.hpp>

using namespace jolt;
using namespace jolt::graphics::vulkan;

Renderer renderer;
VkMemoryPropertyFlags const mem_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
VkBufferUsageFlags const usage = 0;
ui::Window *ui_window;

GraphicsEngineInitializationParams gparams{"Jolt test", 1, 0, 0, nullptr, 1, 0, 0};

SETUP {
    initialize();

    console.set_output_stream(&io::standard_error_stream);

    ui_window = jltnew(ui::Window, "Test window - don't close");

    gparams.wnd = ui_window;
    renderer.initialize(gparams);
}

CLEANUP {
    renderer.shutdown();
    ui_window->close();

    shutdown();
}

TEST(allocate) {
    BufferAllocator buf_allocator{renderer};

    Buffer const b1 = buf_allocator.allocate(1024, mem_flags, usage);

    assert2(b1.get_buffer() != VK_NULL_HANDLE, "Buffer handle");
    assert2(b1.get_memory() != VK_NULL_HANDLE, "Memory handle");
    assert2(b1.get_size() == 1024, "Memory size");
    assert2(b1.get_memory_property_flags() == mem_flags, "Memory property flags");
    assert2(b1.get_usage() == usage, "Usage");

    Buffer const b2 = buf_allocator.allocate(1024, mem_flags, usage);

    assert2(b1 != b2, "Two consecutive allocations must not be the same");
}

TEST(free) {
    BufferAllocator buf_allocator{renderer};

    Buffer const b1 = buf_allocator.allocate(1024, mem_flags, usage);

    buf_allocator.free(b1);

    Buffer const b2 = buf_allocator.allocate(1024, mem_flags, usage);
    Buffer const b3 = buf_allocator.allocate(1024, mem_flags, usage);

    assert2(b1 == b2, "Allocations should be the same");
    assert2(b2 != b3, "Two consecutive allocations must not be the same");
}
