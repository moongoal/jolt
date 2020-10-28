#include <jolt/graphics/vulkan.hpp>
#include "download-transfer.hpp"

namespace jolt::graphics::vulkan {
    void DownloadTransfer::transfer_image(TransferDescriptor const &descriptor) {
        CommandBuffer &cmd_buffer = get_command_buffer();
        Fence &fence = get_fence();
        StagingBuffer &staging_buffer = *get_staging_buffer();

        fence.wait(SYNCHRO_WAIT_MAX);
        fence.reset();
        cmd_buffer.reset(false);

        cmd_buffer.begin_record(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkBufferMemoryBarrier buff_barrier{
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
          nullptr,                                 // pNext
          VK_ACCESS_HOST_READ_BIT,                 // srcAccessMask
          VK_ACCESS_TRANSFER_WRITE_BIT,            // dstAccessMask
          VK_QUEUE_FAMILY_IGNORED,                 // srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED,                 // dstQueueFamilyIndex
          staging_buffer.get_device_buffer(),      // buffer
          0,                                       // offset
          staging_buffer.get_buffer_size()         // size
        };

        vkCmdPipelineBarrier(
          cmd_buffer,
          VK_PIPELINE_STAGE_HOST_BIT,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_DEPENDENCY_BY_REGION_BIT,
          0,
          nullptr,
          1,
          &buff_barrier,
          0,
          nullptr);

        VkBufferImageCopy region{};

        vkCmdCopyImageToBuffer(
          cmd_buffer,
          descriptor.handle.image,
          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
          staging_buffer.get_device_buffer(),
          1,
          &region);

        cmd_buffer.end_record();

        ActionSynchro synchro{};
        synchro.fence = fence;

        cmd_buffer.submit(get_queue(), synchro);

        fence.wait(SYNCHRO_WAIT_MAX);
        staging_buffer.download(descriptor.data.download_data, descriptor.size);
    }

    void DownloadTransfer::transfer_buffer(TransferDescriptor const &descriptor) {
        CommandBuffer &cmd_buffer = get_command_buffer();
        Fence &fence = get_fence();
        StagingBuffer &staging_buffer = *get_staging_buffer();

        fence.wait(SYNCHRO_WAIT_MAX);
        fence.reset();
        cmd_buffer.reset(false);

        cmd_buffer.begin_record(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkBufferMemoryBarrier buff_barrier{
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
          nullptr,                                 // pNext
          VK_ACCESS_HOST_READ_BIT,                 // srcAccessMask
          VK_ACCESS_TRANSFER_WRITE_BIT,            // dstAccessMask
          VK_QUEUE_FAMILY_IGNORED,                 // srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED,                 // dstQueueFamilyIndex
          staging_buffer.get_device_buffer(),      // buffer
          0,                                       // offset
          VK_WHOLE_SIZE                            // size
        };

        vkCmdPipelineBarrier(
          cmd_buffer,
          VK_PIPELINE_STAGE_HOST_BIT,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_DEPENDENCY_BY_REGION_BIT,
          0,
          nullptr,
          1,
          &buff_barrier,
          0,
          nullptr);

        VkBufferCopy region{
          descriptor.info.buffer_info.offset, // srcOffset
          0,                                  // dstOffset
          descriptor.size                     // size
        };

        vkCmdCopyBuffer(cmd_buffer, descriptor.handle.buffer, staging_buffer.get_device_buffer(), 1, &region);

        cmd_buffer.end_record();

        ActionSynchro synchro{};
        synchro.fence = fence;

        cmd_buffer.submit(get_queue(), synchro);

        fence.wait(SYNCHRO_WAIT_MAX);
        staging_buffer.download(descriptor.data.download_data, descriptor.size);
    }

    void DownloadTransfer::transfer_begin() {
        Fence &fence = get_fence();
        CommandBuffer &cmd_buf = get_command_buffer();

        fence.wait(SYNCHRO_WAIT_MAX);
        fence.reset();
        cmd_buf.reset(false);
        cmd_buf.begin_record(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        collections::Vector<VkBufferMemoryBarrier> buf_barriers;
        collections::Vector<VkImageMemoryBarrier> img_barriers;

        for(TransferDescriptor const &descriptor : get_descriptors()) {
            switch(descriptor.resource_type) {
                case TransferResourceType::Image:
                    img_barriers.push({
                      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                 // sType
                      nullptr,                                                // pNext
                      VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, // srcAccessMask
                      VK_ACCESS_TRANSFER_READ_BIT,                            // dstAccessMask
                      descriptor.info.image_info.layout,                      // initialLayout
                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,                   // finalLayout
                      VK_QUEUE_FAMILY_IGNORED,                                // srcQueueFamilyIgnored
                      VK_QUEUE_FAMILY_IGNORED,                                // dstQueueFamilyIgnored
                      descriptor.handle.image,                                // image
                      {
                        // subresourceRange
                        descriptor.info.image_info.aspect, // aspectMask
                        0,                                 // baseMipmapLevel
                        1,                                 // levelCount
                        0,                                 // baseArrayLayer
                        1                                  // layerCount
                      }                                    //
                    });
                    break;

                case TransferResourceType::Buffer:
                    buf_barriers.push({
                      VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
                      nullptr,                                 // pNext
                      VK_ACCESS_MEMORY_WRITE_BIT,              // srcAccessMask
                      VK_ACCESS_TRANSFER_READ_BIT,             // dstAccessMask
                      VK_QUEUE_FAMILY_IGNORED,                 // srcQueueFamilyIndex
                      VK_QUEUE_FAMILY_IGNORED,                 // dstQueueFamilyIndex
                      descriptor.handle.buffer,                // buffer
                      descriptor.info.buffer_info.offset,      // offset
                      descriptor.size                          // size
                    });
                    break;

                default:
#if _DEBUG
                    console.err("Invalid transfer resource type");
#endif // _DEBUG
                    abort();
            }
        }

        uint32_t const buf_barriers_count = buf_barriers.get_length();
        VkBufferMemoryBarrier const *const buf_barriers_ptr = buf_barriers_count ? &buf_barriers[0] : nullptr;

        uint32_t const img_barriers_count = img_barriers.get_length();
        VkImageMemoryBarrier const *const img_barriers_ptr = img_barriers_count ? &img_barriers[0] : nullptr;

        vkCmdPipelineBarrier(
          cmd_buf,
          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_DEPENDENCY_BY_REGION_BIT,
          0,
          nullptr,
          buf_barriers_count,
          buf_barriers_ptr,
          img_barriers_count,
          img_barriers_ptr);

        cmd_buf.end_record();

        ActionSynchro synchro{};
        synchro.fence = fence;

        cmd_buf.submit(get_queue(), synchro);
    }
} // namespace jolt::graphics::vulkan
