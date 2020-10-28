#include <jolt/graphics/vulkan.hpp>
#include "upload-transfer.hpp"

namespace jolt::graphics::vulkan {
    void UploadTransfer::transfer_image(TransferDescriptor const &descriptor) {
        StagingBuffer &staging_buffer = *get_staging_buffer();
        CommandBuffer &cmd_buffer = get_command_buffer();
        Fence &fence = get_fence();

        fence.wait(SYNCHRO_WAIT_MAX);

        staging_buffer.upload(descriptor.data.upload_data, static_cast<uint32_t>(descriptor.size));

        fence.reset();
        cmd_buffer.reset(false);
        cmd_buffer.begin_record(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // Ensure the data from the host has arrived
        VkBufferMemoryBarrier mem_barrier{
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
          nullptr,                                 // pNext
          VK_ACCESS_HOST_WRITE_BIT,                // srcAccessMask
          VK_ACCESS_TRANSFER_READ_BIT,             // dstAccessMask
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
          &mem_barrier,
          0,
          nullptr);

        // Copy the buffer
        VkBufferImageCopy region{
          0, // bufferOffset
          0, // bufferRowLength
          0, // bufferImageHeight
          {
            // imageSubresource
            descriptor.info.image_info.aspect, // aspectMask
            0,                                 // mipLevel
            0,                                 // baseArrayLayer
            1                                  // layerCount
          },
          {
            // offset
          },
          descriptor.info.image_info.extent // extent
        };

        vkCmdCopyBufferToImage(
          cmd_buffer,
          staging_buffer.get_device_buffer(),
          descriptor.handle.image,
          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
          1,
          &region);

        cmd_buffer.end_record();

        ActionSynchro synchro{};
        synchro.fence = fence;

        cmd_buffer.submit(get_queue(), synchro);
        m_uploaded_image_descriptors.push(descriptor);
    }

    void UploadTransfer::transfer_buffer(TransferDescriptor const &descriptor) {
        StagingBuffer &staging_buffer = *get_staging_buffer();
        CommandBuffer &cmd_buffer = get_command_buffer();
        Fence &fence = get_fence();

        fence.wait(SYNCHRO_WAIT_MAX);

        staging_buffer.upload(descriptor.data.upload_data, static_cast<uint32_t>(descriptor.size));

        fence.reset();
        cmd_buffer.reset(false);
        cmd_buffer.begin_record(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // Ensure the data from the host has arrived
        VkBufferMemoryBarrier mem_barrier{
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
          nullptr,                                 // pNext
          VK_ACCESS_HOST_WRITE_BIT,                // srcAccessMask
          VK_ACCESS_TRANSFER_READ_BIT,             // dstAccessMask
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
          &mem_barrier,
          0,
          nullptr);

        // Copy the buffer
        VkBufferCopy region{
          0,                                  // srcOffset
          descriptor.info.buffer_info.offset, // dstOffset
          descriptor.size                     // size
        };

        vkCmdCopyBuffer(cmd_buffer, staging_buffer.get_device_buffer(), descriptor.handle.buffer, 1, &region);

        cmd_buffer.end_record();

        ActionSynchro synchro{};
        synchro.fence = fence;

        cmd_buffer.submit(get_queue(), synchro);
    }

    void UploadTransfer::transfer_end() {
        Fence &fence = get_fence();
        fence.wait(SYNCHRO_WAIT_MAX);

        // If needed, perform layout transitions.
        if(m_uploaded_image_descriptors.get_length()) {
            CommandBuffer &cmd_buf = get_command_buffer();

            fence.reset();
            cmd_buf.reset(false);
            cmd_buf.begin_record(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            collections::Vector<VkImageMemoryBarrier> barriers;

            for(TransferDescriptor const &image_descriptor : m_uploaded_image_descriptors) {
                barriers.push({
                  VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,  // sType
                  nullptr,                                 // pNext
                  VK_ACCESS_MEMORY_WRITE_BIT,              // srcAccessMask
                  VK_ACCESS_MEMORY_READ_BIT,               // dstAccessMask
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,    // initialLayout
                  image_descriptor.info.image_info.layout, // finalLayout
                  VK_QUEUE_FAMILY_IGNORED,                 // srcQueueFamilyIgnored
                  VK_QUEUE_FAMILY_IGNORED,                 // dstQueueFamilyIgnored
                  image_descriptor.handle.image,           // image
                  {
                    // subresourceRange
                    image_descriptor.info.image_info.aspect, // aspectMask
                    0,                                       // baseMipmapLevel
                    1,                                       // levelCount
                    0,                                       // baseArrayLayer
                    1                                        // layerCount
                  }                                          //
                });
            }

            uint32_t const barriers_count = barriers.get_length();
            VkImageMemoryBarrier const *const barriers_ptr = barriers_count ? &barriers[0] : nullptr;

            vkCmdPipelineBarrier(
              cmd_buf,
              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
              VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
              VK_DEPENDENCY_BY_REGION_BIT,
              0,
              nullptr,
              0,
              nullptr,
              barriers_count,
              barriers_ptr);

            cmd_buf.end_record();

            ActionSynchro synchro{};
            synchro.fence = fence;

            cmd_buf.submit(get_queue(), synchro);

            fence.wait(SYNCHRO_WAIT_MAX);
        }
    }
} // namespace jolt::graphics::vulkan
