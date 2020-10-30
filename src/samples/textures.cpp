/*
 * This example shows how to use the low-level Vulkan renderer API to display a rotating plane with a slightly
 * more organised (although still highly inefficient) render loop.
 */
#include <chrono>
#include <jolt/jolt.hpp>
#include <jolt/media/png.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define APP_TITLE "Vulkan textures"

using namespace jolt;
using namespace jolt::graphics::vulkan;

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
    Buffer *plane_buffer;
    VkDeviceSize plane_buffer_index_offset;
    UniformBufferObject *ubo;
    VkDescriptorSet sampler_desc_set;
};

struct VertexAttr {
    using attrib_desc_collection = collections::Vector<VkVertexInputAttributeDescription>;

    glm::vec3 pos;
    glm::vec4 color;
    glm::vec2 uv;

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

        attrs.push({
          2,                       // location
          0,                       // binding
          VK_FORMAT_R32G32_SFLOAT, // format
          offsetof(VertexAttr, uv) // offset
        });

        return attrs;
    }
};

struct {
    collections::StaticArray<VertexAttr, 4> verts{
      {{-0.25f, -0.25f, -0.25f}, {0.8f, 0.8f, 0.8f, 0.8f}, {0.0f, 0.0f}}, // 0
      {{-0.25f, 0.25f, -0.25f}, {0.8f, 0.8f, 0.8f, 0.8f}, {0.0f, 1.0f}},  // 1
      {{0.25f, 0.25f, -0.25f}, {0.8f, 0.8f, 0.8f, 0.8f}, {1.0f, 1.0f}},   // 2
      {{0.25f, -0.25f, -0.25f}, {0.8f, 0.8f, 0.8f, 0.8f}, {1.0f, 0.0f}}   // 3
    };

    collections::StaticArray<uint16_t, 6> faces{0, 1, 3, 1, 2, 3};
} plane;

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

    VkBuffer vertex_buf = state.plane_buffer->get_buffer();
    VkBuffer index_buf = state.plane_buffer->get_buffer();
    VkDeviceSize vertex_offset = 0;
    VkDeviceSize index_offset = state.plane_buffer_index_offset;

    vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buf, &vertex_offset);
    vkCmdBindIndexBuffer(cmd, index_buf, index_offset, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(
      cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipeline_layout, 0, 1, &state.sampler_desc_set, 0, nullptr);
    vkCmdDrawIndexed(cmd, plane.faces.get_length(), 1, 0, 0, 0);

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
    static float fov_deg = 25.0f;

    if(input::is_key_down(input::KeyCode::Add)) {
        fov_deg = max(fov_deg - 1, 15.0f);
    } else if(input::is_key_down(input::KeyCode::Subtract)) {
        fov_deg = min(fov_deg + 1, 70.0f);
    }

    state.ubo->model = glm::rotate(
      glm::mat4(1.0f), glm::sin(elapsed_time / 2) * glm::radians(45.0f), glm::vec3(0.0, -1.0f, 0.0f));
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
    BufferAllocator buf_allocator{renderer};
    VkMemoryPropertyFlags const buf_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkBufferUsageFlags const buf_usage =
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VkDeviceSize plane_buf_size = sizeof(plane);

    // Buffers
    Buffer plane_buffer = buf_allocator.allocate(plane_buf_size, buf_flags, buf_usage);

    // Textures
    TextureAllocator tex_allocator{renderer};
    io::FileStream image_stream{JLT_ASSETS_DIR "/images/polish-forest.png", io::MODE_READ};
    media::Image image = media::load_image_png(image_stream);
    media::ImageHeader const &image_header = image.get_header();
    Texture texture = tex_allocator.create_builder(image_header.width, image_header.height).build_texture();

    // Upload
    {
        TransferFactory xfer_factory{renderer, gqueue};

        TransferDescriptor desc_verts;
        desc_verts.resource_type = TransferResourceType::Buffer;
        desc_verts.handle.buffer = plane_buffer.get_buffer();
        desc_verts.info.buffer_info.offset = 0;
        desc_verts.data.upload_data = &plane;
        desc_verts.size = static_cast<VkDeviceSize>(sizeof(plane));

        TransferDescriptor desc_texture{};
        desc_texture.resource_type = TransferResourceType::Image;
        desc_texture.handle.image = texture.get_image();
        desc_texture.info.image_info.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        desc_texture.info.image_info.extent = {image_header.width, image_header.height, 1};
        desc_texture.info.image_info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        desc_texture.info.image_info.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        desc_texture.data.upload_data = image.get_data();
        desc_texture.size = image.get_size();

        xfer_factory.add_resource_transfer(desc_verts);
        xfer_factory.add_resource_transfer(desc_texture);

        xfer_factory.build_upload_transfer().transfer_all();
    }

    // Shaders
    path::Path vertex_shader_path = "/build/src/shaders/vertex/simple-pos-color-uv.vert.spv";
    hash::hash_t vertex_shader_hash = vertex_shader_path.hash<ShaderManager::hash_function>();

    path::Path fragment_shader_path = "/build/src/shaders/fragment/color-uv-multiply.frag.spv";
    hash::hash_t fragment_shader_hash = fragment_shader_path.hash<ShaderManager::hash_function>();

    vfs::VirtualFileSystem vfs;
    ShaderManager shader_manager{renderer, vfs};

    shader_manager.register_shader(vertex_shader_path);
    shader_manager.register_shader(fragment_shader_path);
    renderer.set_shader_manager(&shader_manager);

    VkShaderModule const vertex_shader = shader_manager.get_vulkan_shader(vertex_shader_hash);
    VkShaderModule const fragment_shader = shader_manager.get_vulkan_shader(fragment_shader_hash);

    DescriptorManager::pool_size_vector pool_sizes;

    pool_sizes.push({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});

    DescriptorManager desc_manager{renderer, 1, pool_sizes};
    DescriptorManager::push_const_range_vector push_const_ranges;
    DescriptorManager::descriptor_set_layout_vector desc_set_layouts;
    DescriptorManager::descriptor_set_layout_binding_vector sampler_descriptor_set_layout_bindings;

    VkSampler const sampler = texture.get_sampler();

    sampler_descriptor_set_layout_bindings.push({
      1,                                         // binding
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
      1,                                         // descriptorCount
      VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
      nullptr //&sampler                                   // pImmutableSamplers
    });

#define DESC_SET_SAMPLER 0
    desc_set_layouts.push(desc_manager.create_descriptor_set_layout(sampler_descriptor_set_layout_bindings));

    push_const_ranges.push({
      VK_SHADER_STAGE_VERTEX_BIT, // stageFlags
      0,                          // offset
      sizeof(UniformBufferObject) // size
    });

    VkPipelineLayout pipeline_layout =
      desc_manager.create_pipeline_layout(desc_set_layouts, push_const_ranges);

    DescriptorManager::descriptor_set_vector descriptor_sets =
      desc_manager.allocate_descriptor_sets(desc_set_layouts);

    {
        VkDescriptorImageInfo image_info{
          sampler,                                  // sampler
          texture.get_view(),                       // imageView
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // imageLayout
        };

        VkWriteDescriptorSet wds{
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
          nullptr,                                   // pNext
          descriptor_sets[DESC_SET_SAMPLER],         // dstSet
          1,                                         // dstBinding
          0,                                         // dstArrayElement
          1,                                         // descriptorCount
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
          &image_info,                               // pImageInfo
          nullptr,                                   // pBufferInfo
          nullptr                                    // pTexelBufferInfo
        };

        vkUpdateDescriptorSets(renderer.get_device(), 1, &wds, 0, nullptr);
    }

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
    state.plane_buffer = &plane_buffer;
    state.plane_buffer_index_offset = offsetof(decltype(plane), faces);
    state.ubo = &ubo;
    state.sampler_desc_set = descriptor_sets[DESC_SET_SAMPLER];

    collections::Vector<VkCommandBuffer> cmd_bufs;

    // Command execution
    do {
        if(ui_window.is_minimized()) {
            jolt::threading::sleep(50);
            continue;
        }

        state.renderer->get_presentation_target()->acquire_next_image(&sem_acquire, &fence_acquire);

        update(state);

        fence_acquire.wait(SYNCHRO_WAIT_MAX);

        render(state, cmd_bufs);

        renderer.get_presentation_target()->present_active_image(present_synchro);

        fence_submit.wait(SYNCHRO_WAIT_MAX);
        fence_acquire.reset();
        fence_submit.reset();

        cmd_pool.free_raw_command_buffers(&cmd_bufs[0], cmd_bufs.get_length());

        cmd_pool.reset(false);
        cmd_bufs.clear();
    } while(ui_window.cycle() && !renderer.is_lost());

    desc_manager.free_descriptor_sets(descriptor_sets);

    for(auto const &layout : desc_set_layouts) { desc_manager.destroy_descriptor_set_layout(layout); }

    tex_allocator.free(texture);
    buf_allocator.free(plane_buffer);
    desc_manager.destroy_pipeline_layout(pipeline_layout);
}
