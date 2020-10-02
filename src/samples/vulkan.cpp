#include <jolt/jolt.hpp>

using namespace jolt;
using namespace jolt::graphics;

static constexpr const uint64_t ms = 1'000'000;
VulkanRenderer renderer;

void main_loop(ui::Window const &wnd);

int main(int argc, char **argv) {
    graphics::GraphicsEngineInitializationParams gparams{0};

    initialize();
    console.set_output_stream(&io::standard_error_stream);
    ui::Window wnd{"Vulkan initialization"};

    gparams.app_name = "Vulkan initialization";
    gparams.wnd = &wnd;

    renderer.initialize(gparams);

    wnd.show();
    main_loop(wnd);

    renderer.shutdown();
    shutdown();
}

void main_loop(ui::Window const &wnd) {
    // Cmd buffer
    VulkanCommandPool cmd_pool = renderer.create_graphics_command_pool(true, false);

    // Synchro
    VulkanSemaphore sem_acquire{renderer}, sem_present{renderer};
    VulkanFence fence_acquire{renderer}, fence_submit{renderer};

    VulkanActionSynchro submit_synchro;

    submit_synchro.wait_semaphore_count = 1;
    submit_synchro.wait_semaphores[0] = sem_acquire;
    submit_synchro.wait_semaphores_stages[0] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    submit_synchro.signal_semaphores[0] = sem_present;
    submit_synchro.signal_semaphore_count = 1;

    submit_synchro.fence = fence_submit;

    VulkanWaitSemaphoreActionSynchro present_synchro;

    present_synchro.wait_semaphores[0] = sem_present;
    present_synchro.wait_semaphore_count = 1;

    // Command execution
    do {
        VulkanCommandBuffer cmd = cmd_pool.allocate_single_command_buffer(true);
        renderer.get_presentation_target()->acquire_next_image(&sem_acquire, &fence_acquire);

        fence_acquire.wait(500 * ms);
        cmd.begin_record();

        cmd.cmd_begin_render_pass(true);
        cmd.cmd_end_render_pass();

        cmd.end_record();

        cmd.submit(renderer.get_graphics_queue(), submit_synchro);

        renderer.get_presentation_target()->present_active_image(present_synchro);
        fence_submit.wait(500 * ms);

        fence_acquire.reset();
        fence_submit.reset();
        cmd_pool.free_single_command_buffer(cmd);
    } while(wnd.cycle());
}
