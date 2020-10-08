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

    arena = jltnew(Arena, renderer, STAGING_BUFFER_SIZE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
    uint32_t const queue_fam_idx = renderer.get_queue_family_index(queue);
    VkBuffer buffer;

    VkBufferCreateInfo bcinfo{
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,                                  // sType
      nullptr,                                                               // pInfo
      0,                                                                     // flags
      STAGING_BUFFER_SIZE,                                                   // size
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, // usage
      VK_SHARING_MODE_EXCLUSIVE,                                             // sharingMode
      1,             // queueFamilyIndexCount
      &queue_fam_idx // pQueueFamilyIndices
    };

    VkResult result =
      vkCreateBuffer(renderer.get_device(), &bcinfo, get_vulkan_allocator(), &buffer);

    assert2(result == VK_SUCCESS, "Unable to create destination buffer");

    VkDeviceSize const buffer_dev_ptr = arena->bind_to_buffer(buffer, STAGING_BUFFER_SIZE, 1);
    Transfer transfer{renderer, queue, STAGING_BUFFER_SIZE};
    BufferTransferOp op{transfer.transfer_to_buffer(data, data_sz, buffer, 0, queue)};

    op.transfer();

    renderer.wait_queues_idle();

    vkDestroyBuffer(renderer.get_device(), buffer, get_vulkan_allocator());
    arena->free(buffer_dev_ptr);
}
