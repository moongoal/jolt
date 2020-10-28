#include <jolt/debug.hpp>
#include "texture-builder.hpp"

using namespace jolt::media;

namespace jolt {
    namespace graphics {
        namespace vulkan {
            TextureBuilder::TextureBuilder(
              Renderer &renderer, media::Image &image, VkDeviceMemory memory, VkDeviceSize offset) :
              m_renderer{renderer},
              m_image{image}, m_memory{memory}, m_offset{offset} {
                initialize_image();
                initialize_view();
                initialize_sampler();
            }

            Texture TextureBuilder::build_texture() {
                VkImage image;
                VkImageView view;
                VkSampler sampler;

                VkResult result = vkCreateImage(
                  m_renderer.get_device(), &m_image_create_info, get_vulkan_allocator(), &image);
                jltassert2(result == VK_SUCCESS, "Unable to create texture image");

                result = vkBindImageMemory(m_renderer.get_device(), image, m_memory, m_offset);
                jltassert2(result == VK_SUCCESS, "Unable to bind texture image memory");

                m_image_view_create_info.image = image;

                result = vkCreateImageView(
                  m_renderer.get_device(), &m_image_view_create_info, get_vulkan_allocator(), &view);
                jltassert2(result == VK_SUCCESS, "Unable to create texture image view");

                result = vkCreateSampler(
                  m_renderer.get_device(), &m_sampler_create_info, get_vulkan_allocator(), &sampler);
                jltassert2(result == VK_SUCCESS, "Unable to create texture sampler");

                return Texture{m_memory, m_offset, image, view, sampler};
            }

            void TextureBuilder::initialize_image() {
                ImageHeader const &hdr = m_image.get_header();

                jltassert2(hdr.image_type == IMAGE_TYPE_2D, "Not a 2D image");

                m_image_create_info = {
                  VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, // sType
                  nullptr,                             // pNext
                  0,                                   // flags
                  VK_IMAGE_TYPE_2D,                    // imageType
                  VK_FORMAT_R8G8B8A8_UINT,             // format
                  {
                    // extent
                    hdr.width,  // width
                    hdr.height, // height
                    1           // depth
                  },
                  1,                                                            // mipLevels
                  1,                                                            // arrayLayers
                  VK_SAMPLE_COUNT_1_BIT,                                        // samples
                  VK_IMAGE_TILING_OPTIMAL,                                      // tiling
                  VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, // usage
                  VK_SHARING_MODE_EXCLUSIVE,                                    // sharingMode
                  0,                                                            // queueFamilyIndexCount
                  nullptr,                                                      // pQueueFamilyIndices
                  VK_IMAGE_LAYOUT_UNDEFINED                                     // initialLayout
                };
            }

            void TextureBuilder::initialize_view() {
                m_image_view_create_info = {
                  VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
                  nullptr,                                  // pNext
                  0,                                        // flags
                  VK_NULL_HANDLE,                           // image
                  VK_IMAGE_VIEW_TYPE_2D,                    // viewType
                  VK_FORMAT_R8G8B8A8_UINT,                  // format
                  {
                    // components
                    VK_COMPONENT_SWIZZLE_IDENTITY, // r
                    VK_COMPONENT_SWIZZLE_IDENTITY, // g
                    VK_COMPONENT_SWIZZLE_IDENTITY, // b
                    VK_COMPONENT_SWIZZLE_IDENTITY  // a
                  },
                  {
                    // subresourceRange
                    VK_IMAGE_ASPECT_COLOR_BIT, // aspect
                    0,                         // baseMipLevel
                    1,                         // levelCount
                    0,                         // baseArrayLayer
                    1                          // layerCount
                  }};
            }

            void TextureBuilder::initialize_sampler() {
                VkBool32 anisotropy_enable = m_renderer.get_phy_device_features().features.samplerAnisotropy;

                m_sampler_create_info = {
                  VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,   // sType
                  nullptr,                                 // pNext
                  0,                                       // flags
                  VK_FILTER_LINEAR,                        // magFilter
                  VK_FILTER_LINEAR,                        // minFilter
                  VK_SAMPLER_MIPMAP_MODE_LINEAR,           // mipmapMode
                  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, // addressModeU
                  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, // addressModeV
                  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, // addressModeW
                  0,                                       // mipLodBias
                  anisotropy_enable,                       // anisotropyEnable
                  16,                                      // maxAnisotropy
                  VK_FALSE,                                // compareEnable
                  VK_COMPARE_OP_ALWAYS,                    // compareOp
                  0,                                       // minLod
                  1,                                       // maxLod
                  VK_BORDER_COLOR_INT_OPAQUE_BLACK,        // borderColor
                  VK_FALSE                                 // unnormalizedCoordinates
                };
            }
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt
