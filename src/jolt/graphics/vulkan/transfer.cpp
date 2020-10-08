#include <utility>
#include <cstring>
#include <jolt/util.hpp>
#include <jolt/collections/array.hpp>
#include <jolt/debug.hpp>
#include <jolt/graphics/vulkan.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            Transfer::Transfer(
              Renderer const &renderer, VkQueue const queue, VkDeviceSize const size) :
              m_renderer{renderer},
              m_queue{queue}, m_buffer_size{size}, m_fence{renderer, true},
              m_cmdpool{renderer, false, true, renderer.get_queue_family_index(queue)} {
                initialize();
            }

            void Transfer::initialize() {
                uint32_t q_fam_idx = m_renderer.get_queue_family_index(m_queue);

                VkBufferCreateInfo cinfo{
                  VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // sType
                  nullptr,                              // pNext
                  0,                                    // flags
                  m_buffer_size,                        // size
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,     // usage
                  VK_SHARING_MODE_EXCLUSIVE,            // sharingMode
                  1,                                    // queueFamilyIndexCount
                  &q_fam_idx                            // pQueueFamilyIndices
                };

                VkResult result = vkCreateBuffer(
                  m_renderer.get_device(), &cinfo, get_vulkan_allocator(), &m_buffer);
                jltassert2(result == VK_SUCCESS, "Unable to create staging buffer");

                uint32_t mem_type_idx = choose_memory_type();
                const bool m_memory_is_coherent = mem_type_idx & COHERENT_BIT;
                mem_type_idx &= 0x0000ffffUL;

                VkMemoryAllocateInfo ainfo{
                  VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // sType
                  nullptr,                                // pNext
                  m_buffer_size,                          // allocationSize
                  mem_type_idx                            // memoryTypeIndex
                };

                result = vkAllocateMemory(
                  m_renderer.get_device(), &ainfo, get_vulkan_allocator(), &m_memory);

                result =
                  vkMapMemory(m_renderer.get_device(), m_memory, 0, m_buffer_size, 0, &m_map_ptr);
                jltassert2(result == VK_SUCCESS, "Unable to map staging buffer memory");

                result = vkBindBufferMemory(m_renderer.get_device(), m_buffer, m_memory, 0);
                jltassert2(result == VK_SUCCESS, "Unable to bind staging buffer memory");
            }

            uint32_t Transfer::choose_memory_type() const {
                uint32_t mt = m_renderer.get_memory_type_index(
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

                if(mt != JLT_VULKAN_INVALID32) {
                    return mt;
                }

                mt = m_renderer.get_memory_type_index(
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT
                  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                if(mt != JLT_VULKAN_INVALID32) {
                    return mt | COHERENT_BIT;
                }

                // Directly return as the standard guarantees the presence of a memory type with
                // these requirements
                return m_renderer.get_memory_type_index(
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         0)
                       | COHERENT_BIT;
            }

            void Transfer::dispose() {
                if(m_map_ptr) {
                    vkUnmapMemory(m_renderer.get_device(), m_memory);
                    m_map_ptr = nullptr;
                }

                if(m_buffer != VK_NULL_HANDLE) {
                    vkDestroyBuffer(m_renderer.get_device(), m_buffer, get_vulkan_allocator());
                    m_buffer = VK_NULL_HANDLE;
                }

                if(m_memory != VK_NULL_HANDLE) {
                    vkFreeMemory(m_renderer.get_device(), m_memory, get_vulkan_allocator());
                    m_memory = VK_NULL_HANDLE;
                }
            }

            BufferTransferOp Transfer::transfer_to_buffer(
              const void *const data,
              VkDeviceSize const size,
              VkBuffer const buffer,
              VkDeviceSize const offset,
              VkQueue const queue,
              VkPipelineStageFlags const dst_stage_mask) {
                return BufferTransferOp{*this, data, size, queue, buffer, dst_stage_mask};
            }

            TransferOp::TransferOp(
              Transfer &transfer,
              void const *const ptr,
              VkDeviceSize const size,
              VkQueue const tgt_queue,
              VkPipelineStageFlags const dst_stage_mask) :
              m_transfer{transfer},
              m_ptr{reinterpret_cast<uint8_t const *>(ptr)}, m_total_size{size},
              m_tgt_q_fam_idx{transfer.get_renderer().get_queue_family_index(tgt_queue)},
              m_cmdbuf{transfer.get_command_pool().allocate_single_command_buffer(true)},
              m_finished{false}, m_offset{0}, m_dst_stage_mask{dst_stage_mask} {}

            TransferOp::TransferOp(TransferOp &&other) :
              m_transfer{other.m_transfer}, m_ptr{other.m_ptr}, m_total_size{other.m_total_size},
              m_tgt_q_fam_idx{other.m_tgt_q_fam_idx}, m_cmdbuf{std::move(other.m_cmdbuf)},
              m_finished{other.m_finished}, m_offset{other.m_offset}, m_dst_stage_mask{
                                                                        other.m_dst_stage_mask} {
                other.m_ptr = nullptr;
            }

            TransferOp::~TransferOp() {
                if(m_ptr) { // Not moved
                    m_transfer.get_command_pool().free_single_command_buffer(m_cmdbuf);
                }
            }

            VkDeviceSize TransferOp::host_begin_next_transfer() {
                VkDeviceSize const bufsz =
                  min(m_transfer.get_buffer_size(), m_total_size - m_offset);
                VkResult result;

                memcpy(m_transfer.get_buffer_ptr(), m_ptr + m_offset, bufsz);

                if(!m_transfer.is_coherent()) {
                    VkMappedMemoryRange mrange{
                      VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, // sType
                      nullptr,                               // pNext
                      m_transfer.get_memory(),               // memory
                      0,                                     // offset
                      bufsz                                  // size
                    };

                    result =
                      vkFlushMappedMemoryRanges(m_transfer.get_renderer().get_device(), 1, &mrange);
                    jltassert2(result == VK_SUCCESS, "Unable to flush staging buffer");
                }

                return bufsz;
            }

            bool TransferOp::transfer_single_block() {
                jltassert(m_ptr != nullptr);

                if(!m_finished) {
                    if(m_transfer.get_fence().wait(10)) {
                        uint32_t q_fam_idx =
                          m_transfer.get_renderer().get_queue_family_index(m_transfer.get_queue());
                        ActionSynchro synchro;
                        Fence &fence = m_transfer.get_fence();
                        VkDeviceSize bufsz = host_begin_next_transfer();
                        bool const last_chunk = !(bufsz == m_transfer.get_buffer_size());

                        fence.reset();
                        m_cmdbuf.reset(false);

                        // There is no need to signal the fence unless there is more data to
                        // transfer.
                        if(!last_chunk) {
                            synchro.fence = fence;
                        }

                        VkBufferMemoryBarrier mem_barrier_transfer{
                          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
                          nullptr,                                 // pNext
                          VK_ACCESS_HOST_WRITE_BIT,                // srcAccessMask
                          VK_ACCESS_TRANSFER_READ_BIT,             // dstAccessMask
                          q_fam_idx,                               // srcQueueFamilyIndex,
                          q_fam_idx,                               // dstQueueFamilyIndex,
                          m_transfer.get_buffer(),                 // buffer
                          0,                                       // offset
                          bufsz                                    // size
                        };

                        m_cmdbuf.begin_record();

                        vkCmdPipelineBarrier(
                          m_cmdbuf,
                          VK_PIPELINE_STAGE_HOST_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          0,
                          0,
                          nullptr,
                          1,
                          &mem_barrier_transfer,
                          0,
                          nullptr);

                        device_copy_to_destination(last_chunk, q_fam_idx, bufsz);

                        m_cmdbuf.end_record();

                        m_offset += bufsz;

                        m_cmdbuf.submit(m_transfer.get_queue(), synchro);
                    }

                    m_finished = m_offset >= m_total_size;
                }

                return m_finished;
            }

            BufferTransferOp::BufferTransferOp(
              Transfer &transfer,
              void const *const ptr,
              VkDeviceSize const size,
              VkQueue const tgt_queue,
              VkBuffer const tgt_buffer,
              VkPipelineStageFlags const dst_stage_mask) :
              TransferOp{transfer, ptr, size, tgt_queue, dst_stage_mask},
              m_tgt_buffer{tgt_buffer} {}

            BufferTransferOp::BufferTransferOp(BufferTransferOp &&other) :
              TransferOp{std::move(other)}, m_tgt_buffer{other.m_tgt_buffer} {}

            void BufferTransferOp::device_copy_to_destination(
              bool const last_chunk, uint32_t const q_fam_idx, VkDeviceSize const bufsz) {
                VkBufferCopy copy{
                  0,        // srcOffset
                  m_offset, // dstOffset
                  bufsz     // size
                };

                VkBufferMemoryBarrier mem_barrier_copy{
                  VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
                  nullptr,                                 // pNext
                  VK_ACCESS_TRANSFER_WRITE_BIT,            // srcAccessMask
                  VK_ACCESS_MEMORY_READ_BIT,               // dstAccessMask
                  q_fam_idx,                               // srcQueueFamilyIndex,
                  m_tgt_q_fam_idx,                         // dstQueueFamilyIndex,
                  m_tgt_buffer,                            // buffer
                  choose(static_cast<VkDeviceSize>(0), m_offset, last_chunk), // offset
                  choose(m_total_size, bufsz, last_chunk)                     // size
                };

                vkCmdCopyBuffer(
                  m_cmdbuf.get_buffer(), m_transfer.get_buffer(), m_tgt_buffer, 1, &copy);

                vkCmdPipelineBarrier(
                  m_cmdbuf,
                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                  choose(
                    m_dst_stage_mask,
                    static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_TRANSFER_BIT),
                    last_chunk),
                  0,
                  0,
                  nullptr,
                  1,
                  &mem_barrier_copy,
                  0,
                  nullptr);
            }
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt
