#ifndef JLT_GRAPHICS_VULKAN_TRANSFER_BUILDER_HPP
#define JLT_GRAPHICS_VULKAN_TRANSFER_BUILDER_HPP

#include <jolt/collections/vector.hpp>
#include "defs.hpp"
#include "transfer-descriptor.hpp"
#include "download-transfer.hpp"
#include "upload-transfer.hpp"

namespace jolt::graphics::vulkan {
    class Renderer;

    class JLTAPI TransferFactory {
      public:
        using transfer_descriptors = collections::Vector<TransferDescriptor>;

      private:
        Renderer &m_renderer;
        VkQueue const m_queue;
        transfer_descriptors m_descriptors;

      public:
        TransferFactory(Renderer &renderer, VkQueue const queue) : m_renderer{renderer}, m_queue{queue} {}
        TransferFactory(TransferFactory const &) = delete;
        TransferFactory(TransferFactory &&) = default;

        void add_resource_transfer(TransferDescriptor const &descriptor) { m_descriptors.push(descriptor); }

        UploadTransfer build_upload_transfer();
        DownloadTransfer build_download_transfer();
    };
} // namespace jolt::graphics::vulkan

#endif /* JLT_GRAPHICS_VULKAN_TRANSFER_BUILDER_HPP */
