#ifndef JLT_GRAPHICS_VULKAN_TRANSFER_DESCRIPTOR_HPP
#define JLT_GRAPHICS_VULKAN_TRANSFER_DESCRIPTOR_HPP

#include "defs.hpp"

namespace jolt::graphics::vulkan {
    enum class TransferResourceType { Buffer, Image };

    union TransferData {
        void *download_data;
        void const *upload_data;
    };

    union TransferHandle {
        VkBuffer buffer;
        VkImage image;
    };

    union TransferDescriptorInfo {
        struct {
            VkDeviceSize offset; //< Transfer descriptor base offset.
        } buffer_info;

        struct {
            VkExtent3D extent;            //< Extent for the transferred images.
            VkImageLayout initial_layout; //< Initial transition layout.
            VkImageLayout final_layout;   //< Final transition layout.
            VkImageAspectFlags aspect;    //< Image aspect.
        } image_info;
    };

    struct TransferDescriptor {
        TransferResourceType resource_type; //< The resource type.
        TransferDescriptorInfo info; //< Information describing the format of the data being transferred.
        TransferData data;           //< Pointer to the host-side buffer.
        TransferHandle handle;       //< The Vulkan handle for the device-side object.
        VkDeviceSize size;           //< Size of the data to transfer.
    };
} // namespace jolt::graphics::vulkan

#endif /* JLT_GRAPHICS_VULKAN_TRANSFER_DESCRIPTOR_HPP */
