#ifndef JLT_GRAPHICS_VULKAN_UPLOAD_TRANSFER_HPP
#define JLT_GRAPHICS_VULKAN_UPLOAD_TRANSFER_HPP

#include <jolt/graphics/vulkan/defs.hpp>
#include "transfer.hpp"

namespace jolt::graphics::vulkan {
    class JLTAPI UploadTransfer : public Transfer {
        transfer_descriptors m_uploaded_image_descriptors;

        virtual void transfer_image(TransferDescriptor const &descriptor);
        virtual void transfer_buffer(TransferDescriptor const &descriptor);
        virtual void transfer_end();

      public:
        UploadTransfer(Renderer &renderer, VkQueue queue) : Transfer{renderer, queue} {}
    };
} // namespace jolt::graphics::vulkan

#endif /* JLT_GRAPHICS_VULKAN_UPLOAD_TRANSFER_HPP */
