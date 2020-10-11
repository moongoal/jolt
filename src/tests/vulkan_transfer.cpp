#define _CRT_SECURE_NO_WARNINGS
#include <jolt/jolt.hpp>
#include <jolt/test.hpp>

using namespace jolt;
using namespace jolt::graphics::vulkan;

VkDeviceSize const STAGING_BUFFER_SIZE = 1024;

Renderer renderer;
ui::Window *ui_window;

GraphicsEngineInitializationParams gparams{"Jolt test", 1, 0, 0, nullptr, 1, 1, 1};

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
      STAGING_BUFFER_SIZE * 8,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
}

CLEANUP {
    jltfree(arena);
    renderer.shutdown();
    ui_window->close();

    shutdown();
}

TEST(transfer_to_buffer__single_chunk__single_queue) {
    char const data[] = "TEST STRING";
    size_t constexpr const data_sz = sizeof(data);
    unsigned char data_out[data_sz];

    memset(data_out, 0, data_sz);

    VkQueue const queue = renderer.acquire_graphics_queue();
    renderer.release_queue(queue);

    BufferDeviceAlloc const buffer = arena->allocate(STAGING_BUFFER_SIZE, 1);
    StagingBuffer stg{renderer, queue, STAGING_BUFFER_SIZE};
    CommandPool pool{renderer, true, false, renderer.get_queue_family_index(queue)};

    Transfer tfer_up{
      {.const_data = data},           // data
      data_sz,                        // size
      {.buffer = buffer},             // buffer
      buffer,                         // offset
      queue,                          // src_queue
      queue,                          // dst_queue
      VK_PIPELINE_STAGE_HOST_BIT,     // src_stage_mask
      VK_PIPELINE_STAGE_TRANSFER_BIT, // dst_stage_mask
      0,                              // src_access_mask
      VK_ACCESS_TRANSFER_READ_BIT     // dst_access_mask
    };

    Transfer tfer_down{
      {.data = data_out},             // data
      data_sz,                        // size
      {.buffer = buffer},             // buffer
      buffer,                         // offset
      queue,                          // src_queue
      queue,                          // dst_queue
      VK_PIPELINE_STAGE_TRANSFER_BIT, // src_stage_mask
      VK_PIPELINE_STAGE_HOST_BIT,     // dst_stage_mask
      VK_ACCESS_TRANSFER_WRITE_BIT,   // src_access_mask
      VK_ACCESS_HOST_READ_BIT         // dst_access_mask
    };

    CommandBuffer clear_cmd = pool.allocate_single_command_buffer(true);
    BufferUploadOp op_up = stg.upload_buffer(tfer_up);
    BufferDownloadOp op_down = stg.download_buffer(tfer_down);

    ActionSynchro clear_syn;
    Fence clear_fence{renderer};

    clear_syn.fence = clear_fence;

    clear_cmd.begin_record();
    vkCmdFillBuffer(clear_cmd, buffer, buffer, 4, 0xabcdef12);
    clear_cmd.end_record();

    op_up.transfer();
    clear_cmd.submit(queue, clear_syn);
    clear_fence.wait(500'000'000 /* 500 ms */);
    op_down.transfer();

    assert(data_out[0] == 0x12);
    assert(data_out[1] == 0xef);
    assert(data_out[2] == 0xcd);
    assert(data_out[3] == 0xab);

    for(size_t i = 4; i < data_sz; ++i) { assert(data[i] == data_out[i]); }

    renderer.wait_queues_idle();

    pool.free_single_command_buffer(clear_cmd);
    arena->free(buffer);
}

TEST(transfer_to_buffer__multi_chunk__single_queue) {
    uint32_t data[STAGING_BUFFER_SIZE];
    size_t constexpr const data_sz = sizeof(data);
    uint32_t data_out[data_sz];

    memset(data, 0xaa, data_sz);
    memset(data_out, 0, data_sz);

    VkQueue const queue = renderer.acquire_graphics_queue();
    renderer.release_queue(queue);

    BufferDeviceAlloc const buffer = arena->allocate(data_sz, 1);
    StagingBuffer stg{renderer, queue, STAGING_BUFFER_SIZE};
    CommandPool pool{renderer, true, false, renderer.get_queue_family_index(queue)};

    Transfer tfer_up{
      {.const_data = data},           // data
      data_sz,                        // size
      {.buffer = buffer},             // buffer
      buffer,                         // offset
      queue,                          // src_queue
      queue,                          // dst_queue
      VK_PIPELINE_STAGE_HOST_BIT,     // src_stage_mask
      VK_PIPELINE_STAGE_TRANSFER_BIT, // dst_stage_mask
      0,                              // src_access_mask
      VK_ACCESS_TRANSFER_READ_BIT     // dst_access_mask
    };

    Transfer tfer_down{
      {.data = data_out},             // data
      data_sz,                        // size
      {.buffer = buffer},             // buffer
      buffer,                         // offset
      queue,                          // src_queue
      queue,                          // dst_queue
      VK_PIPELINE_STAGE_TRANSFER_BIT, // src_stage_mask
      VK_PIPELINE_STAGE_HOST_BIT,     // dst_stage_mask
      VK_ACCESS_TRANSFER_WRITE_BIT,   // src_access_mask
      VK_ACCESS_HOST_READ_BIT         // dst_access_mask
    };

    CommandBuffer clear_cmd = pool.allocate_single_command_buffer(true);
    BufferUploadOp op_up = stg.upload_buffer(tfer_up);
    BufferDownloadOp op_down = stg.download_buffer(tfer_down);

    ActionSynchro clear_syn;
    Fence clear_fence{renderer};

    clear_syn.fence = clear_fence;

    clear_cmd.begin_record();
    vkCmdFillBuffer(clear_cmd, buffer, buffer, data_sz / 2, 0xbbbbbbbb);
    clear_cmd.end_record();

    op_up.transfer();
    clear_cmd.submit(queue, clear_syn);
    clear_fence.wait(500'000'000 /* 500 ms */);
    op_down.transfer();

    for(size_t i = 0; i < STAGING_BUFFER_SIZE / 2; ++i) { assert(data_out[i] == 0xbbbbbbbb); }

    for(size_t i = STAGING_BUFFER_SIZE / 2; i < STAGING_BUFFER_SIZE; ++i) {
        assert(data_out[i] == 0xaaaaaaaa);
    }

    renderer.wait_queues_idle();

    pool.free_single_command_buffer(clear_cmd);
    arena->free(buffer);
}

TEST(transfer_to_buffer__single_chunk__multi_queue) {
    char const data[] = "TEST STRING";
    size_t constexpr const data_sz = sizeof(data);
    unsigned char data_out[data_sz];

    memset(data_out, 0, data_sz);

    VkQueue const xfer_queue = renderer.acquire_transfer_queue();
    VkQueue const gfx_queue = renderer.acquire_graphics_queue();
    renderer.release_queue(gfx_queue);
    renderer.release_queue(xfer_queue);

    if(renderer.get_queue_family_index(gfx_queue) == renderer.get_queue_family_index(xfer_queue)) {
        // If this test gets ignored but you need to run it, try requesting and acquiring a compute queue
        ignore();
    }

    BufferDeviceAlloc const buffer = arena->allocate(STAGING_BUFFER_SIZE, 1);
    StagingBuffer stg{renderer, xfer_queue, STAGING_BUFFER_SIZE};
    CommandPool pool{renderer, true, false, renderer.get_queue_family_index(gfx_queue)};

    Transfer tfer_up{
      {.const_data = data},           // data
      data_sz,                        // size
      {.buffer = buffer},             // buffer
      buffer,                         // offset
      gfx_queue,                      // src_queue
      gfx_queue,                      // dst_queue
      VK_PIPELINE_STAGE_HOST_BIT,     // src_stage_mask
      VK_PIPELINE_STAGE_TRANSFER_BIT, // dst_stage_mask
      0,                              // src_access_mask
      VK_ACCESS_TRANSFER_READ_BIT     // dst_access_mask
    };

    Transfer tfer_down{
      {.data = data_out},             // data
      data_sz,                        // size
      {.buffer = buffer},             // buffer
      buffer,                         // offset
      gfx_queue,                      // src_queue
      gfx_queue,                      // dst_queue
      VK_PIPELINE_STAGE_TRANSFER_BIT, // src_stage_mask
      VK_PIPELINE_STAGE_HOST_BIT,     // dst_stage_mask
      VK_ACCESS_TRANSFER_WRITE_BIT,   // src_access_mask
      VK_ACCESS_HOST_READ_BIT         // dst_access_mask
    };

    CommandBuffer clear_cmd = pool.allocate_single_command_buffer(true);
    BufferUploadOp op_up = stg.upload_buffer(tfer_up);
    BufferDownloadOp op_down = stg.download_buffer(tfer_down);

    ActionSynchro clear_syn;
    Fence clear_fence{renderer};

    clear_syn.fence = clear_fence;

    clear_cmd.begin_record();
    vkCmdFillBuffer(clear_cmd, buffer, buffer, 4, 0xabcdef12);
    clear_cmd.end_record();

    op_up.transfer();
    clear_cmd.submit(gfx_queue, clear_syn);
    clear_fence.wait(500'000'000 /* 500 ms */);
    op_down.transfer();

    assert(data_out[0] == 0x12);
    assert(data_out[1] == 0xef);
    assert(data_out[2] == 0xcd);
    assert(data_out[3] == 0xab);

    for(size_t i = 4; i < data_sz; ++i) { assert(data[i] == data_out[i]); }

    renderer.wait_queues_idle();

    pool.free_single_command_buffer(clear_cmd);
    arena->free(buffer);
}

TEST(transfer_to_buffer__multi_chunk__multi_queue) {
    uint32_t data[STAGING_BUFFER_SIZE];
    size_t constexpr const data_sz = sizeof(data);
    uint32_t data_out[data_sz];

    memset(data, 0xaa, data_sz);
    memset(data_out, 0, data_sz);

    VkQueue const xfer_queue = renderer.acquire_transfer_queue();
    VkQueue const gfx_queue = renderer.acquire_graphics_queue();
    renderer.release_queue(gfx_queue);
    renderer.release_queue(xfer_queue);

    if(renderer.get_queue_family_index(gfx_queue) == renderer.get_queue_family_index(xfer_queue)) {
        // If this test gets ignored but you need to run it, try requesting and acquiring a compute queue
        ignore();
    }

    BufferDeviceAlloc const buffer = arena->allocate(data_sz, 1);
    StagingBuffer stg{renderer, xfer_queue, STAGING_BUFFER_SIZE};
    CommandPool pool{renderer, true, false, renderer.get_queue_family_index(gfx_queue)};

    Transfer tfer_up{
      {.const_data = data},           // data
      data_sz,                        // size
      {.buffer = buffer},             // buffer
      buffer,                         // offset
      gfx_queue,                      // src_queue
      gfx_queue,                      // dst_queue
      VK_PIPELINE_STAGE_HOST_BIT,     // src_stage_mask
      VK_PIPELINE_STAGE_TRANSFER_BIT, // dst_stage_mask
      0,                              // src_access_mask
      VK_ACCESS_TRANSFER_READ_BIT     // dst_access_mask
    };

    Transfer tfer_down{
      {.data = data_out},             // data
      data_sz,                        // size
      {.buffer = buffer},             // buffer
      buffer,                         // offset
      gfx_queue,                      // src_queue
      gfx_queue,                      // dst_queue
      VK_PIPELINE_STAGE_TRANSFER_BIT, // src_stage_mask
      VK_PIPELINE_STAGE_HOST_BIT,     // dst_stage_mask
      VK_ACCESS_TRANSFER_WRITE_BIT,   // src_access_mask
      VK_ACCESS_HOST_READ_BIT         // dst_access_mask
    };

    CommandBuffer clear_cmd = pool.allocate_single_command_buffer(true);
    BufferUploadOp op_up = stg.upload_buffer(tfer_up);
    BufferDownloadOp op_down = stg.download_buffer(tfer_down);

    ActionSynchro clear_syn;
    Fence clear_fence{renderer};

    clear_syn.fence = clear_fence;

    clear_cmd.begin_record();
    vkCmdFillBuffer(clear_cmd, buffer, buffer, data_sz / 2, 0xbbbbbbbb);
    clear_cmd.end_record();

    op_up.transfer();
    clear_cmd.submit(gfx_queue, clear_syn);
    clear_fence.wait(500'000'000 /* 500 ms */);
    op_down.transfer();

    for(size_t i = 0; i < STAGING_BUFFER_SIZE / 2; ++i) { assert(data_out[i] == 0xbbbbbbbb); }

    for(size_t i = STAGING_BUFFER_SIZE / 2; i < STAGING_BUFFER_SIZE; ++i) {
        assert(data_out[i] == 0xaaaaaaaa);
    }

    renderer.wait_queues_idle();

    pool.free_single_command_buffer(clear_cmd);
    arena->free(buffer);
}
