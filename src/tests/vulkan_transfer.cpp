#include <jolt/test.hpp>
#include <jolt/jolt.hpp>

using namespace jolt;
using namespace jolt::graphics::vulkan;

VkDeviceSize const STAGING_BUFFER_SIZE = 1024;

Renderer renderer;
ui::Window *ui_window;

GraphicsEngineInitializationParams gparams{"Jolt test", 1, 0, 0, nullptr, 1, 0, 0};

Arena *arena;

SETUP {
    initialize();

    console.set_output_stream(&io::standard_error_stream);

    ui_window = jltnew(ui::Window, "Test window - don't close");

    gparams.wnd = ui_window;
    renderer.initialize(gparams);

    arena = jltnew(
      Arena,
      renderer,
      STAGING_BUFFER_SIZE,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
}

CLEANUP {
    jltfree(arena);
    renderer.shutdown();
    ui_window->close();

    shutdown();
}

TEST(transfer_to_buffer__single_chunk) {
    char const data[] = "TEST STRING";
    size_t const data_sz = sizeof(data);

    VkQueue const queue = renderer.get_graphics_queue();

    VkDeviceSize const buffer_dev_ptr = arena->allocate(STAGING_BUFFER_SIZE, 1);
    Transfer transfer{renderer, queue, STAGING_BUFFER_SIZE};
    BufferTransferOp op{
      transfer.transfer_to_buffer(data, data_sz, arena->get_buffer(), buffer_dev_ptr, queue)};

    op.transfer();

    renderer.wait_queues_idle();

    arena->free(buffer_dev_ptr);
}
