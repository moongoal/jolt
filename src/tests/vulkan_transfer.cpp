#define _CRT_SECURE_NO_WARNINGS
#include <jolt/jolt.hpp>
#include <jolt/features.hpp>
#include <jolt/media/image.hpp>
#include <jolt/media/png.hpp>
#include <jolt/test.hpp>

using namespace jolt;
using namespace jolt::graphics::vulkan;

VkDeviceSize const STAGING_BUFFER_SIZE = 1024;

Renderer renderer;
ui::Window *ui_window;

GraphicsEngineInitializationParams gparams{"Jolt test", 1, 0, 0, nullptr, 1, 1, 1};

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

TEST(transfer__buffer) {
    BufferAllocator buf_allocator{renderer};
    VkMemoryPropertyFlags const mem_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkBufferUsageFlags const mem_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                                         | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    char const data[] = "TEST STRING";
    size_t constexpr data_sz = sizeof(data);
    unsigned char data_out[data_sz];

    memset(data_out, 0, data_sz);

    VkQueue const queue = renderer.acquire_graphics_queue();
    uint32_t const queue_family_idx = renderer.get_queue_family_index(queue);
    renderer.release_queue(queue);

    Buffer const buffer = buf_allocator.allocate(STAGING_BUFFER_SIZE, mem_flags, mem_usage);
    StagingBuffer stg{renderer, queue_family_idx, STAGING_BUFFER_SIZE};
    CommandPool pool{renderer, true, false, queue_family_idx};
    CommandBuffer clear_cmd = pool.allocate_single_command_buffer(true);

    TransferDescriptor desc_up{};
    desc_up.resource_type = TransferResourceType::Buffer;
    desc_up.handle.buffer = buffer.get_buffer();
    desc_up.info.buffer_info.offset = 0;
    desc_up.size = data_sz;
    desc_up.data.upload_data = data;

    TransferDescriptor desc_down{};
    desc_down.resource_type = TransferResourceType::Buffer;
    desc_down.handle.buffer = buffer.get_buffer();
    desc_down.info.buffer_info.offset = 0;
    desc_down.size = data_sz;
    desc_down.data.download_data = data_out;

    TransferFactory xfer_factory{renderer, queue};

    xfer_factory.add_resource_transfer(desc_up);
    UploadTransfer xfer_upload = xfer_factory.build_upload_transfer();

    xfer_factory.add_resource_transfer(desc_down);
    DownloadTransfer xfer_download = xfer_factory.build_download_transfer();

    ActionSynchro clear_syn;
    Fence clear_fence{renderer};

    clear_syn.fence = clear_fence;

    clear_cmd.begin_record();
    vkCmdFillBuffer(clear_cmd, buffer.get_buffer(), 0, 4, 0xabcdef12);
    clear_cmd.end_record();

    assert2(xfer_upload.transfer_next() == false, "Upload transfer not complete");
    clear_cmd.submit(queue, clear_syn);
    clear_fence.wait(SYNCHRO_WAIT_MAX);
    assert2(xfer_download.transfer_next() == false, "Download transfer not complete");

    assert2(data_out[0] == 0x12, "Invalid downloaded data");
    assert2(data_out[1] == 0xef, "Invalid downloaded data");
    assert2(data_out[2] == 0xcd, "Invalid downloaded data");
    assert2(data_out[3] == 0xab, "Invalid downloaded data");

    for(size_t i = 4; i < data_sz; ++i) { assert2(data[i] == data_out[i], "Invalid downloaded data"); }

    renderer.wait_queues_idle();

    pool.free_single_command_buffer(clear_cmd);
    buf_allocator.free(buffer);
}

TEST(transfer__image) {
    io::FileStream image_stream{JLT_ASSETS_DIR "/images/polish-forest.png", io::MODE_READ};
    media::Image image = media::load_image_png(image_stream);
    media::ImageHeader const &image_header = image.get_header();
    size_t const data_sz = image.get_size();
    collections::Array<media::Pixel> data_out{image_header.width * image_header.height};
    TextureAllocator tex_allocator{renderer};

    Texture tex = tex_allocator.create_builder(image_header.width, image_header.height).build_texture();

    memset(data_out, 0, data_sz);

    VkQueue const queue = renderer.acquire_graphics_queue();
    uint32_t const queue_family_idx = renderer.get_queue_family_index(queue);
    renderer.release_queue(queue);

    StagingBuffer stg{renderer, queue_family_idx, STAGING_BUFFER_SIZE};
    CommandPool pool{renderer, true, false, queue_family_idx};
    CommandBuffer clear_cmd = pool.allocate_single_command_buffer(true);

    TransferDescriptor desc_up{};
    desc_up.resource_type = TransferResourceType::Image;
    desc_up.handle.image = tex.get_image();
    desc_up.info.image_info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    desc_up.info.image_info.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    desc_up.info.image_info.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    desc_up.info.image_info.extent = {image_header.width, image_header.height, 1};
    desc_up.size = data_sz;
    desc_up.data.upload_data = image.get_data();

    TransferDescriptor desc_down{};
    desc_down.resource_type = TransferResourceType::Image;
    desc_down.handle.image = tex.get_image();
    desc_down.info.image_info.initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    desc_down.info.image_info.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    desc_down.info.image_info.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    desc_down.info.image_info.extent = {image_header.width, image_header.height, 1};
    desc_down.size = data_sz;
    desc_down.data.download_data = data_out;

    TransferFactory xfer_factory{renderer, queue};

    xfer_factory.add_resource_transfer(desc_up);
    UploadTransfer xfer_upload = xfer_factory.build_upload_transfer();

    xfer_factory.add_resource_transfer(desc_down);
    DownloadTransfer xfer_download = xfer_factory.build_download_transfer();

    assert2(xfer_upload.transfer_next() == false, "Upload transfer not complete");
    assert2(xfer_download.transfer_next() == false, "Download transfer not complete");

    for(size_t i = 0; i < data_out.get_length(); ++i) {
        assert2(
          image.get_data()[i].r == data_out[i].r && image.get_data()[i].a == data_out[i].a
            && image.get_data()[i].g == data_out[i].g && image.get_data()[i].b == data_out[i].b,
          "Invalid downloaded data");
    }

    renderer.wait_queues_idle();

    pool.free_single_command_buffer(clear_cmd);
    tex_allocator.free(tex);
}
