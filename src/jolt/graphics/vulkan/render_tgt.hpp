#ifndef JLT_GRAPHICS_VULKAN_RENDER_TGT_HPP
#define JLT_GRAPHICS_VULKAN_RENDER_TGT_HPP

#include "defs.hpp"

#include <jolt/collections/array.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            class Renderer;
            class Window;

            class JLTAPI RenderTarget {
              public:
                using fb_array = collections::Array<VkFramebuffer>;
                using view_array = collections::Array<VkImageView>;

              private:
                Renderer const &m_renderer;
                VkImage m_ds_image = VK_NULL_HANDLE;
                VkImageView m_ds_image_view = VK_NULL_HANDLE;
                VkDeviceMemory m_ds_image_memory = VK_NULL_HANDLE;
                VkFormat m_ds_image_fmt = VK_FORMAT_UNDEFINED;
                VkRenderPass m_render_pass = VK_NULL_HANDLE;
                fb_array *m_framebuffers = nullptr;

                void initialize();
                void select_depth_stencil_image_format();
                void initialize_depth_stencil_buffer();
                void initialize_render_pass();
                void initialize_framebuffer(view_array const &views);
                void reset_device();

                void shutdown();

              public:
                RenderTarget(Renderer const &renderer) : m_renderer{renderer} {
                    initialize();
                }

                RenderTarget(RenderTarget const &other) = delete;

                ~RenderTarget() { shutdown(); }

                Renderer const &get_renderer() const { return m_renderer; }

                VkImage get_depth_stencil_image() const { return m_ds_image; }

                VkImageView get_depth_stencil_image_view() const { return m_ds_image_view; }

                VkDeviceMemory get_depth_stencil_image_memory() const { return m_ds_image_memory; }

                VkFormat get_depth_stencil_image_format() const { return m_ds_image_fmt; }

                VkRenderPass get_render_pass() const { return m_render_pass; }

                fb_array &get_framebuffers() const { return *m_framebuffers; }

                VkFramebuffer get_active_framebuffer() const;
            };
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_RENDER_TGT_HPP */
