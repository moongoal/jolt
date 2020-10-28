#ifndef JLT_GRAPHICS_VULKAN_DOWNLOAD_TRANSFER_HPP
#define JLT_GRAPHICS_VULKAN_DOWNLOAD_TRANSFER_HPP

#include <jolt/graphics/vulkan/defs.hpp>
#include "transfer.hpp"

namespace jolt::graphics::vulkan {
    class JLTAPI DownloadTransfer : public Transfer {
        virtual void transfer_image(TransferDescriptor const &descriptor);
        virtual void transfer_buffer(TransferDescriptor const &descriptor);
        virtual void transfer_begin();

      public:
        DownloadTransfer(Renderer &renderer, VkQueue queue) : Transfer{renderer, queue} {}
    };
} // namespace jolt::graphics::vulkan

#endif /* JLT_GRAPHICS_VULKAN_DOWNLOAD_TRANSFER_HPP */
