#include <jolt/debug.hpp>
#include <jolt/graphics/vulkan.hpp>

namespace jolt::graphics::vulkan {
    TextureBuilder::TextureBuilder(Renderer &renderer, uint32_t const width, uint32_t const height) :
      m_renderer{renderer} {
        initialize_image(width, height);
        initialize_view();
        initialize_sampler();
    }

    Texture TextureBuilder::build_texture() { return build_texture_array(1).pop(); }

    TextureBuilder::texture_vector TextureBuilder::build_texture_array(uint32_t const n) {
        texture_vector textures{n};

        if(n > 0) {
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkMemoryRequirements mem_req;

            for(uint32_t i = 0; i < n; ++i) {
                VkImage image;
                VkImageView view;
                VkSampler sampler;

                VkResult result = vkCreateImage(
                  m_renderer.get_device(), &m_image_create_info, get_vulkan_allocator(), &image);
                jltassert2(result == VK_SUCCESS, "Unable to create texture image");

                if(memory == VK_NULL_HANDLE) {
                    vkGetImageMemoryRequirements(m_renderer.get_device(), image, &mem_req);
                    memory = allocate_texture_memory(mem_req, n);
                }

                VkDeviceSize const offset = mem_req.size * 0;

                result = vkBindImageMemory(m_renderer.get_device(), image, memory, offset);
                jltassert2(result == VK_SUCCESS, "Unable to bind texture image memory");

                m_image_view_create_info.image = image;

                result = vkCreateImageView(
                  m_renderer.get_device(), &m_image_view_create_info, get_vulkan_allocator(), &view);
                jltassert2(result == VK_SUCCESS, "Unable to create texture image view");

                result = vkCreateSampler(
                  m_renderer.get_device(), &m_sampler_create_info, get_vulkan_allocator(), &sampler);
                jltassert2(result == VK_SUCCESS, "Unable to create texture sampler");

                textures.push(Texture{memory, offset, image, view, sampler});
            }
        }

        return textures;
    }

    VkDeviceMemory
    TextureBuilder::allocate_texture_memory(VkMemoryRequirements const &mem_req, uint32_t const n) {
        VkDeviceMemory memory;

        uint32_t mem_type_index =
          m_renderer.get_memory_type_index(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, mem_req.memoryTypeBits);
        jltassert2(mem_type_index != JLT_VULKAN_INVALID32, "Invalid texture memory type index");

        VkMemoryAllocateInfo ainfo{
          VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // sType
          nullptr,                                // pNext
          mem_req.size * n,                       // allocationSize
          mem_type_index                          // memoryTypeIndex
        };

        VkResult result = vkAllocateMemory(m_renderer.get_device(), &ainfo, get_vulkan_allocator(), &memory);
        jltassert2(result == VK_SUCCESS, "Unable to allocate texture memory");

        return memory;
    }

    void TextureBuilder::initialize_image(uint32_t const width, uint32_t const height) {
        m_image_create_info = {
          VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, // sType
          nullptr,                             // pNext
          0,                                   // flags
          VK_IMAGE_TYPE_2D,                    // imageType
          VK_FORMAT_R8G8B8A8_UINT,             // format
          {
            // extent
            width,  // width
            height, // height
            1       // depth
          },
          1,                       // mipLevels
          1,                       // arrayLayers
          VK_SAMPLE_COUNT_1_BIT,   // samples
          VK_IMAGE_TILING_OPTIMAL, // tiling
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, // usage
          VK_SHARING_MODE_EXCLUSIVE,                                        // sharingMode
          0,                                                                // queueFamilyIndexCount
          nullptr,                                                          // pQueueFamilyIndices
          VK_IMAGE_LAYOUT_UNDEFINED                                         // initialLayout
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
        VkBool32 anisotropy_enable = m_renderer.get_enabled_phy_device_features().features.samplerAnisotropy;

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
} // namespace jolt::graphics::vulkan
