#include <jolt/jolt.hpp>

using namespace jolt::graphics;
using namespace jolt::graphics::vulkan;

namespace jolt {
    static void internal_loop(Renderer &renderer, ui::Window const &wnd, jlt_loop_func_t loop_func) {
        VkQueue const gqueue = renderer.acquire_graphics_queue();
        jltassert2(gqueue != VK_NULL_HANDLE, "Null graphics queue");

        Window vk_window{renderer, wnd, gqueue};
        renderer.set_window(&vk_window);

        PresentationTarget pt{renderer, gqueue};
        renderer.set_presentation_target(&pt);

        RenderTarget rt{renderer};
        renderer.set_render_target(&rt);

        loop_func(renderer);

        renderer.set_render_target(nullptr);
        renderer.set_presentation_target(nullptr);
        renderer.set_window(nullptr);
    }

    void main_loop(GraphicsEngineInitializationParams &gparams, jlt_loop_func_t loop_func) {
        Renderer renderer;

        renderer.initialize(gparams);
        gparams.wnd->show();

        bool exit_loop = false;

        while(!exit_loop) {
            internal_loop(renderer, *gparams.wnd, loop_func);

            switch(renderer.get_lost_state()) {
                case jolt::graphics::vulkan::RENDERER_LOST_DEVICE:
                    renderer.reset(gparams);
                    break;

                case jolt::graphics::vulkan::RENDERER_LOST_PRESENT:
                    renderer.reset_lost_state();
                    break; // Restarting the main loop will reset the render/presentation chain

                case jolt::graphics::vulkan::RENDERER_NOT_LOST:
                    exit_loop = true;
                    break;

                default:
                    console.err("Renderer lost state not handled. Resetting the renderer");
                    renderer.reset(gparams);
                    break;
            }
        }

        renderer.shutdown();
    }
} // namespace jolt
