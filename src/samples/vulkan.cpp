/*
 * This example shows how to use the low-level Vulkan renderer API to initialize an application and
 * run its loop.
 */
#include <jolt/jolt.hpp>

#define APP_TITLE "Vulkan initialization"

using namespace jolt;
using namespace jolt::graphics::vulkan;

static constexpr const uint64_t ns_to_ms = 1'000'000;
Renderer renderer;

void main_loop(ui::Window const &wnd);

int main(JLT_MAYBE_UNUSED int argc, JLT_MAYBE_UNUSED char **argv) {
    GraphicsEngineInitializationParams gparams{};

    initialize();
    console.set_output_stream(&io::standard_error_stream);
    ui::Window wnd{APP_TITLE};

    gparams.app_name = APP_TITLE;
    gparams.wnd = &wnd;
    gparams.n_queues_graphics = 1;

    renderer.initialize(gparams);
    wnd.show();

    bool exit_loop = false;

    while(!exit_loop) {
        main_loop(wnd);

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
    shutdown();
}

void main_loop(ui::Window const &wnd) {
    // Queue
    VkQueue gqueue = renderer.acquire_graphics_queue();
    uint32_t gqueue_fam_idx = renderer.get_queue_family_index(gqueue);

    jltassert2(gqueue != VK_NULL_HANDLE, "Null graphics queue");
    jltassert2(gqueue_fam_idx != JLT_VULKAN_INVALID32, "Invalid graphics queue family index");

    // Window & targets
    Window *vk_window;
    RenderTarget *rt;
    PresentationTarget *pt;
    vk_window = jltnew(Window, renderer, wnd, gqueue);
    renderer.set_window(vk_window);

    pt = jltnew(PresentationTarget, renderer, gqueue);
    renderer.set_presentation_target(pt);

    rt = jltnew(RenderTarget, renderer);
    renderer.set_render_target(rt);

    {
        // Shaders
        vfs::VirtualFileSystem vfs;
        ShaderManager shader_manager{renderer, vfs};

        shader_manager.scan_shaders();
        renderer.set_shader_manager(&shader_manager);

        DescriptorManager::pool_size_vector pool_sizes;

        pool_sizes.push({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

        DescriptorManager desc_manager{renderer, 1, pool_sizes};
        DescriptorManager::desc_set_layout_vector desc_set_layouts;
        DescriptorManager::push_const_range_vector push_const_ranges;

        VkPipelineLayout pipeline_layout =
          desc_manager.create_pipeline_layout(desc_set_layouts, push_const_ranges);

        // Pipeline
        pipelines::DefaultGraphicsPipelineConfiguration pipeline_cfg{renderer, pipeline_layout};
        GraphicsPipelineManager pipeline_manager{renderer};

        pipeline_manager.add_configuration(pipeline_cfg);

        pipeline_manager.create_pipelines();
        VkPipeline pipeline = pipeline_manager.get_pipelines()[0];

        // Cmd pool
        CommandPool cmd_pool{renderer, true, true, gqueue_fam_idx};

        // Synchro
        Semaphore sem_acquire{renderer}, sem_present{renderer};
        Fence fence_acquire{renderer}, fence_submit{renderer};

        ActionSynchro submit_synchro;

        submit_synchro.wait_semaphore_count = 1;
        submit_synchro.wait_semaphores[0] = sem_acquire;
        submit_synchro.wait_semaphores_stages[0] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        submit_synchro.signal_semaphores[0] = sem_present;
        submit_synchro.signal_semaphore_count = 1;

        submit_synchro.fence = fence_submit;

        WaitSemaphoreActionSynchro present_synchro;

        present_synchro.wait_semaphores[0] = sem_present;
        present_synchro.wait_semaphore_count = 1;

        VkViewport viewport{
          0,                                                                              // x
          0,                                                                              // y
          static_cast<float>(vk_window->get_surface_capabilities().currentExtent.width),  // width
          static_cast<float>(vk_window->get_surface_capabilities().currentExtent.height), // height
          0.0f,
          1.0f};

        VkRect2D scissor{
          {0, 0},
          {vk_window->get_surface_capabilities().currentExtent.width,
           vk_window->get_surface_capabilities().currentExtent.height}};

        // Command execution
        do {
            if(wnd.is_minimized()) {
                jolt::threading::sleep(50);
                continue;
            }

            CommandBuffer cmd = cmd_pool.allocate_single_command_buffer(true);
            renderer.get_presentation_target()->acquire_next_image(&sem_acquire, &fence_acquire);

            fence_acquire.wait(500 * ns_to_ms);
            cmd.begin_record();
            cmd.cmd_begin_render_pass(true);

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            vkCmdDraw(cmd, 3, 1, 0, 0);

            cmd.cmd_end_render_pass();
            cmd.end_record();

            cmd.submit(gqueue, submit_synchro);

            renderer.get_presentation_target()->present_active_image(present_synchro);
            fence_submit.wait(500 * ns_to_ms);

            fence_acquire.reset();
            fence_submit.reset();
            cmd_pool.free_single_command_buffer(cmd);
            cmd_pool.reset(false);
        } while(wnd.cycle() && !renderer.is_lost());

        desc_manager.destroy_pipeline_layout(pipeline_layout);
    }

    jltfree(rt);
    jltfree(pt);
    jltfree(vk_window);
}
