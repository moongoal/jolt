/*
 * This example shows how to use the low-level Vulkan renderer API to display a rotating cube with a slightly
 * more organised (although still highly inefficient) render loop.
 */
#include <chrono>
#include <jolt/jolt.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define APP_TITLE "Vulkan cube"

using namespace jolt;
using namespace jolt::graphics::vulkan;

static constexpr const uint64_t ns_to_ms = 1'000'000;

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct RenderState {
    Renderer *renderer;
    VkViewport *viewport;
    VkRect2D *scissor;
    CommandPool *cmd_pool;
    ActionSynchro *submit_synchro;
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkQueue graphics_queue;
    BufferDeviceAlloc *vertex_buffer_alloc;
    BufferDeviceAlloc *index_buffer_alloc;
    UniformBufferObject *ubo;
};

struct VertexAttr {
    using attrib_desc_collection = collections::Vector<VkVertexInputAttributeDescription>;

    glm::vec3 pos;
    glm::vec4 color;

    static VkVertexInputBindingDescription get_vertex_binding_description() {
        return {
          0,                          // binding
          sizeof(VertexAttr),         // stride
          VK_VERTEX_INPUT_RATE_VERTEX // inputRate
        };
    }

    static attrib_desc_collection get_vertex_attribute_descriptions() {
        attrib_desc_collection attrs;

        attrs.push({
          0,                          // location
          0,                          // binding
          VK_FORMAT_R32G32B32_SFLOAT, // format
          offsetof(VertexAttr, pos)   // offset
        });

        attrs.push({
          1,                             // location
          0,                             // binding
          VK_FORMAT_R32G32B32A32_SFLOAT, // format
          offsetof(VertexAttr, color)    // offset
        });

        return attrs;
    }
};

collections::StaticArray<VertexAttr, 8> cube_verts{
  {{-0.25f, -0.25f, -0.25f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // 0
  {{-0.25f, 0.25f, -0.25f}, {0.0f, 1.0f, 0.0f, 1.0f}},  // 1
  {{0.25f, 0.25f, -0.25f}, {0.0f, 0.0f, 1.0f, 1.0f}},   // 2
  {{0.25f, -0.25f, -0.25f}, {1.0f, 1.0f, 0.0f, 1.0f}},  // 3
  {{-0.25f, -0.25f, 0.25f}, {1.0f, 0.0f, 1.0f, 1.0f}},  // 4
  {{-0.25f, 0.25f, 0.25f}, {0.0f, 1.0f, 1.0f, 1.0f}},   // 5
  {{0.25f, 0.25f, 0.25f}, {0.0f, 1.0f, 1.0f, 1.0f}},    // 6
  {{0.25f, -0.25f, 0.25f}, {0.0f, 1.0f, 1.0f, 1.0f}}    // 7
};

collections::StaticArray<uint16_t, 12 * 3> cube_faces{
  0, 1, 3, // front 1
  1, 2, 3, // front 2
  1, 5, 2, // bottom 1
  2, 5, 6, // bottom 2
  7, 6, 4, // back 1
  4, 6, 5, // back 2
  4, 5, 0, // left 1
  0, 5, 1, // left 2
  4, 0, 7, // top 1
  7, 0, 3, // top 2
  3, 2, 7, // right 1
  7, 2, 6  // right 2
};

void main_loop(Renderer &renderer);

void render(RenderState &state, collections::Vector<VkCommandBuffer> &out_cmds) {
    CommandBuffer cmd = state.cmd_pool->allocate_single_command_buffer(true);

    cmd.begin_record();
    cmd.cmd_begin_render_pass(true);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipeline);
    vkCmdSetViewport(cmd, 0, 1, state.viewport);
    vkCmdSetScissor(cmd, 0, 1, state.scissor);
    vkCmdPushConstants(
      cmd, state.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBufferObject), state.ubo);

    VkBuffer vertex_buf = *state.vertex_buffer_alloc;
    VkBuffer index_buf = *state.index_buffer_alloc;
    VkDeviceSize vertex_offset = *state.vertex_buffer_alloc;
    VkDeviceSize index_offset = *state.index_buffer_alloc;

    vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buf, &vertex_offset);
    vkCmdBindIndexBuffer(cmd, index_buf, index_offset, VK_INDEX_TYPE_UINT16);

    // vkCmdDraw(cmd, 3, 1, 0, 0);
    vkCmdDrawIndexed(cmd, cube_faces.get_length(), 1, 0, 0, 0);

    cmd.cmd_end_render_pass();
    cmd.end_record();

    cmd.submit(state.graphics_queue, *state.submit_synchro);

    out_cmds.push(cmd);
}

void update(RenderState &state) {
    static auto const start_time = std::chrono::high_resolution_clock::now();
    auto const current_time = std::chrono::high_resolution_clock::now();
    float const elapsed_time =
      std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
    uint32_t const width = state.renderer->get_window()->get_surface_capabilities().currentExtent.width;
    uint32_t const height = state.renderer->get_window()->get_surface_capabilities().currentExtent.height;
    static float fov_deg = 45.0f;

    if(input::is_key_down(input::KeyCode::Add)) {
        fov_deg = max(fov_deg - 1, 15.0f);
    } else if(input::is_key_down(input::KeyCode::Subtract)) {
        fov_deg = min(fov_deg + 1, 70.0f);
    }

    state.ubo->model =
      glm::rotate(glm::mat4(1.0f), elapsed_time * glm::radians(90.0f), glm::vec3(0.0, -1.0f, 0.0f));
    state.ubo->view =
      glm::lookAt(glm::vec3(0.0f, 0.0f, -2.5f), glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    state.ubo->proj = glm::perspective(
      glm::radians(fov_deg), static_cast<float>(width) / static_cast<float>(height), 0.1f, 10.0f);

    state.ubo->proj[1][1] *= -1; // Correct for vulkan having -y upwards
}

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

    // Memory
    graphics::vulkan::Arena gpu_allocator{
      renderer,
      1024,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT};

    // Buffers
    BufferDeviceAlloc cube_verts_buf =
      gpu_allocator.allocate(sizeof(cube_verts), alignof(decltype(cube_verts)));
    BufferDeviceAlloc cube_index_buf =
      gpu_allocator.allocate(sizeof(cube_faces), alignof(decltype(cube_faces)));

    jltassert2(cube_verts_buf != InvalidBufferDeviceAlloc, "Not enough memory to allocate vertex buffer.");
    jltassert2(cube_index_buf != InvalidBufferDeviceAlloc, "Not enough memory to allocate index buffer.");

    {
        StagingBuffer staging_buf{renderer, gqueue, 256};
        Transfer transfer;

        transfer.data.data = cube_verts;
        transfer.size = static_cast<VkDeviceSize>(sizeof(cube_verts));
        transfer.offset = cube_verts_buf;
        transfer.subject.buffer = cube_verts_buf;
        transfer.src_queue = gqueue;
        transfer.dst_queue = gqueue;
        transfer.src_access_mask = VK_ACCESS_MEMORY_WRITE_BIT;
        transfer.dst_access_mask = VK_ACCESS_MEMORY_READ_BIT;
        transfer.src_stage_mask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        transfer.dst_stage_mask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;

        BufferUploadOp vert_upload_op = staging_buf.upload_buffer(transfer);

        vert_upload_op.transfer();

        transfer.data.data = cube_faces;
        transfer.size = static_cast<VkDeviceSize>(sizeof(cube_faces));
        transfer.offset = cube_index_buf;
        transfer.subject.buffer = cube_index_buf;

        BufferUploadOp idx_upload_op = staging_buf.upload_buffer(transfer);

        idx_upload_op.transfer();
    }

    // Shaders
    path::Path vertex_shader_path = "/build/src/shaders/vertex/simple-pos-color.vert.spv";
    hash::hash_t vertex_shader_hash = vertex_shader_path.hash<ShaderManager::hash_function>();

    path::Path fragment_shader_path = "/build/src/shaders/fragment/passthrough.frag.spv";
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
    DescriptorManager::desc_set_layout_vector desc_set_layouts;
    DescriptorManager::push_const_range_vector push_const_ranges;
    DescriptorManager::descriptor_set_layout_binding_vector ubo_descriptor_set_layout_bindings;

    push_const_ranges.push({
      VK_SHADER_STAGE_VERTEX_BIT, // stageFlags
      0,                          // offset
      sizeof(UniformBufferObject) // size
    });

    VkPipelineLayout pipeline_layout =
      desc_manager.create_pipeline_layout(desc_set_layouts, push_const_ranges);

    // Pipeline
    GraphicsPipelineManager pipeline_manager{renderer};

    pipelines::DefaultGraphicsPipelineConfiguration pipeline_cfg{
      renderer, pipeline_layout, vertex_shader, fragment_shader};

    pipeline_cfg.m_vertex_binding_descriptions.push(VertexAttr::get_vertex_binding_description());
    pipeline_cfg.m_vertex_attribute_descriptions = VertexAttr::get_vertex_attribute_descriptions();

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

    // Viewport & scissor
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

    // Render state
    RenderState state;
    UniformBufferObject ubo;

    state.renderer = &renderer;
    state.cmd_pool = &cmd_pool;
    state.viewport = &viewport;
    state.scissor = &scissor;
    state.submit_synchro = &submit_synchro;
    state.pipeline = pipeline;
    state.pipeline_layout = pipeline_layout;
    state.graphics_queue = gqueue;
    state.vertex_buffer_alloc = &cube_verts_buf;
    state.index_buffer_alloc = &cube_index_buf;
    state.ubo = &ubo;

    collections::Vector<VkCommandBuffer> cmd_bufs;

    // Command execution
    do {
        if(ui_window.is_minimized()) {
            jolt::threading::sleep(50);
            continue;
        }

        state.renderer->get_presentation_target()->acquire_next_image(&sem_acquire, &fence_acquire);

        update(state);

        fence_acquire.wait(500 * ns_to_ms);

        render(state, cmd_bufs);

        renderer.get_presentation_target()->present_active_image(present_synchro);

        fence_submit.wait(500 * ns_to_ms);
        fence_acquire.reset();
        fence_submit.reset();

        cmd_pool.free_raw_command_buffers(&cmd_bufs[0], cmd_bufs.get_length());

        cmd_pool.reset(false);
        cmd_bufs.clear();
    } while(ui_window.cycle() && !renderer.is_lost());

    desc_manager.destroy_pipeline_layout(pipeline_layout);
}
