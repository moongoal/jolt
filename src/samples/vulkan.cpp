/*
 * This example shows how to use the low-level Vulkan renderer API to initialize an application and
 * run its loop.
 */
#include <jolt/jolt.hpp>

#define APP_TITLE "Vulkan initialization"

using namespace jolt;
using namespace jolt::graphics::vulkan;

static constexpr const uint64_t ns_to_ms = 1'000'000;

void main_loop(Renderer &renderer);

int main(JLT_MAYBE_UNUSED int argc, JLT_MAYBE_UNUSED char **argv) {
    GraphicsEngineInitializationParams gparams{};

    initialize();
    console.set_output_stream(&io::standard_error_stream);
    ui::Window wnd{APP_TITLE};

    gparams.app_name = APP_TITLE;
    gparams.wnd = &wnd;
    gparams.n_queues_graphics = 1;

    jolt::main_loop(gparams, main_loop);

    shutdown();
}

void main_loop(Renderer &renderer) {
    // Window
    Window *const vk_window = renderer.get_window();
    ui::Window const &ui_window = vk_window->get_ui_window();

    // Queues
    VkQueue const gqueue = renderer.get_presentation_target()->get_queue();
    uint32_t const gqueue_fam_idx = renderer.get_queue_family_index(gqueue);

    // Shaders
    path::Path vertex_shader_path = "/build/src/shaders/vertex/triangle.vert.spv";
    hash::hash_t vertex_shader_hash = vertex_shader_path.hash<ShaderManager::hash_function>();

    path::Path fragment_shader_path = "/build/src/shaders/fragment/red.frag.spv";
    hash::hash_t fragment_shader_hash = fragment_shader_path.hash<ShaderManager::hash_function>();

    vfs::VirtualFileSystem vfs;
    ShaderManager shader_manager{renderer, vfs};

    shader_manager.register_shader(vertex_shader_path);
    shader_manager.register_shader(fragment_shader_path);
    renderer.set_shader_manager(&shader_manager);

    VkShaderModule const vertex_shader = shader_manager.get_vulkan_shader(vertex_shader_hash);
    VkShaderModule const fragment_shader = shader_manager.get_vulkan_shader(fragment_shader_hash);

    DescriptorManager::pool_size_vector pool_sizes;

    pool_sizes.push({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

    DescriptorManager desc_manager{renderer, 1, pool_sizes};
    DescriptorManager::descriptor_set_layout_vector desc_set_layouts;
    DescriptorManager::push_const_range_vector push_const_ranges;

    VkPipelineLayout pipeline_layout =
      desc_manager.create_pipeline_layout(desc_set_layouts, push_const_ranges);

    // Pipeline
    pipelines::DefaultGraphicsPipelineConfiguration pipeline_cfg{
      renderer, pipeline_layout, vertex_shader, fragment_shader};
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
    submit_synchro.wait_semaphores_stages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

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
        if(ui_window.is_minimized()) {
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
    } while(ui_window.cycle() && !renderer.is_lost());

    desc_manager.destroy_pipeline_layout(pipeline_layout);
}
