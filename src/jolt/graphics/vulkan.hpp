#ifndef JLT_GRAPHICS_VULKAN_HPP
#define JLT_GRAPHICS_VULKAN_HPP

#include <jolt/util.hpp>
#include <jolt/ui/window.hpp>

namespace jolt {
    namespace graphics {
        struct GraphicsEngineInitializationParams {
            const char *app_name;
            unsigned short app_version_major, app_version_minor, app_version_revision;
            ui::Window *wnd;
        };

        void JLTAPI initialize(GraphicsEngineInitializationParams &params);
        void JLTAPI shutdown();
    } // namespace graphics
} // namespace jolt

#endif /* JLT_GRAPHICS_VULKAN_HPP */
