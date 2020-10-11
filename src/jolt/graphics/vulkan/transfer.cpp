#include <utility>
#include <cstring>
#include <jolt/util.hpp>
#include <jolt/collections/array.hpp>
#include <jolt/debug.hpp>
#include <jolt/graphics/vulkan.hpp>

namespace jolt {
    namespace graphics {
        namespace vulkan {
            StagingBuffer::StagingBuffer(
              Renderer const &renderer, VkQueue const queue, VkDeviceSize const size) :
              m_renderer{renderer},
              m_queue{queue}, m_buffer_size{size}, m_fence{renderer, true},
              m_cmdpool{renderer, true, true, renderer.get_queue_family_index(queue)} {
                initialize();
            }

            void StagingBuffer::initialize() {
                uint32_t q_fam_idx = m_renderer.get_queue_family_index(m_queue);

                VkBufferCreateInfo cinfo{
                  VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,                                // sType
                  nullptr,                                                             // pNext
                  0,                                                                   // flags
                  m_buffer_size,                                                       // size
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, // usage
                  VK_SHARING_MODE_EXCLUSIVE,                                           // sharingMode
                  1,         // queueFamilyIndexCount
                  &q_fam_idx // pQueueFamilyIndices
                };

                VkResult result =
                  vkCreateBuffer(m_renderer.get_device(), &cinfo, get_vulkan_allocator(), &m_buffer);
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

                result = vkAllocateMemory(m_renderer.get_device(), &ainfo, get_vulkan_allocator(), &m_memory);

                result = vkBindBufferMemory(m_renderer.get_device(), m_buffer, m_memory, 0);
                jltassert2(result == VK_SUCCESS, "Unable to bind staging buffer memory");

                result = vkMapMemory(m_renderer.get_device(), m_memory, 0, m_buffer_size, 0, &m_map_ptr);
                jltassert2(result == VK_SUCCESS, "Unable to map staging buffer memory");
            }

            uint32_t StagingBuffer::choose_memory_type() const {
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
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0)
                       | COHERENT_BIT;
            }

            void StagingBuffer::dispose() {
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

            TransferOp::TransferOp(StagingBuffer &staging, Transfer const &transfer) :
              m_staging{staging}, m_xfer{transfer},
              m_finished{false}, m_cmdbuf{staging.get_command_pool().allocate_single_command_buffer(true)},
              m_src_q_fam_idx{staging.get_renderer().get_queue_family_index(transfer.src_queue)},
              m_dst_q_fam_idx{staging.get_renderer().get_queue_family_index(transfer.dst_queue)},
              m_cur_offset{transfer.offset} {}

            TransferOp::TransferOp(TransferOp &&other) :
              m_staging{other.m_staging}, m_xfer{std::move(other.m_xfer)}, m_finished{other.m_finished},
              m_cmdbuf{std::move(other.m_cmdbuf)}, m_src_q_fam_idx{other.m_src_q_fam_idx},
              m_dst_q_fam_idx{other.m_dst_q_fam_idx}, m_cur_offset{other.m_cur_offset} {
                m_cur_offset = JLT_VULKAN_INVALIDSZ;
            }

            TransferOp::~TransferOp() {
                if(!was_moved()) { // Not moved
                    m_staging.get_command_pool().free_single_command_buffer(m_cmdbuf);
                }
            }

            BufferUploadOp StagingBuffer::upload_buffer(Transfer const &transfer) {
                return BufferUploadOp{*this, transfer};
            }

            BufferDownloadOp StagingBuffer::download_buffer(Transfer const &transfer) {
                return BufferDownloadOp{*this, transfer};
            }

            UploadOp::UploadOp(StagingBuffer &staging, Transfer const &transfer) :
              TransferOp{staging, transfer} {}

            VkDeviceSize UploadOp::host_begin_next_transfer() {
                auto data_ptr = reinterpret_cast<uint8_t const *>(m_xfer.data.const_data);
                VkDeviceSize const bufsz =
                  min(m_staging.get_buffer_size(), m_xfer.size + m_xfer.offset - m_cur_offset);
                VkResult result;

                memcpy(m_staging.get_buffer_ptr(), data_ptr + m_cur_offset, bufsz);

                if(!m_staging.is_coherent()) {
                    VkMappedMemoryRange mrange{
                      VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, // sType
                      nullptr,                               // pNext
                      m_staging.get_memory(),                // memory
                      0,                                     // offset
                      VK_WHOLE_SIZE                          // size
                    };

                    result = vkFlushMappedMemoryRanges(m_staging.get_renderer().get_device(), 1, &mrange);
                    jltassert2(result == VK_SUCCESS, "Unable to flush staging buffer");
                }

                return bufsz;
            }

            bool UploadOp::transfer_single_block() {
                if(!m_finished) {
                    Fence &fence = m_staging.get_fence();

                    if(fence.wait(10)) {
                        VkDeviceSize const buf_end = m_xfer.offset + m_xfer.size;
                        m_finished = m_cur_offset >= buf_end;

                        if(!m_finished) {
                            ActionSynchro synchro;
                            uint32_t const q_fam_idx =
                              m_staging.get_renderer().get_queue_family_index(m_staging.get_queue());

                            fence.reset();
                            m_cmdbuf.reset(false);

                            VkDeviceSize const bufsz = host_begin_next_transfer();
                            bool const last_chunk = buf_end - m_cur_offset <= m_staging.get_buffer_size();
                            synchro.fence = fence;

                            m_cmdbuf.begin_record();

                            device_copy_to_destination(last_chunk, q_fam_idx, bufsz);

                            m_cmdbuf.end_record();
                            m_cmdbuf.submit(m_staging.get_queue(), synchro);

                            m_cur_offset += bufsz;
                        }
                    }
                }

                return m_finished;
            }

            BufferUploadOp::BufferUploadOp(StagingBuffer &staging, Transfer const &transfer) :
              UploadOp{staging, transfer} {}

            void BufferUploadOp::device_copy_to_destination(
              bool const last_chunk, uint32_t const q_fam_idx, VkDeviceSize const bufsz) {
                bool const first_chunk = m_xfer.offset == m_cur_offset;
                bool const needs_own_acq =
                  q_fam_idx != m_src_q_fam_idx
                  && first_chunk; //< Does this buffer need queue family ownership acquire?
                bool const needs_own_rel =
                  q_fam_idx != m_dst_q_fam_idx
                  && last_chunk; //< Does this buffer need queue family ownership release?

                VkBufferCopy copy{
                  0,            // srcOffset
                  m_cur_offset, // dstOffset
                  bufsz         // size
                };

                // Barriers between flush & copy
                collections::StaticArray<VkBufferMemoryBarrier, 3> mem_barrier_flush{
                  {
                    // Staging buffer
                    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
                    nullptr,                                 // pNext
                    VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT
                      | VK_ACCESS_HOST_WRITE_BIT, // srcAccessMask
                    VK_ACCESS_TRANSFER_READ_BIT,  // dstAccessMask
                    q_fam_idx,                    // srcQueueFamilyIndex,
                    q_fam_idx,                    // dstQueueFamilyIndex,
                    m_staging.get_buffer(),       // buffer
                    0,                            // offset
                    VK_WHOLE_SIZE                 // size
                  },
                  {
                    // Destination buffer (+ ownership release)
                    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
                    nullptr,                                 // pNext
                    choose<VkAccessFlags>(VK_ACCESS_MEMORY_WRITE_BIT, 0, needs_own_acq)
                      | m_xfer.src_access_mask,                                            // srcAccessMask
                    choose<VkAccessFlags>(VK_ACCESS_TRANSFER_WRITE_BIT, 0, needs_own_acq), // dstAccessMask
                    m_src_q_fam_idx,       // srcQueueFamilyIndex,
                    q_fam_idx,             // dstQueueFamilyIndex,
                    m_xfer.subject.buffer, // buffer
                    m_xfer.offset,         // offset
                    m_xfer.size            // size
                  },
                  {
                    // Destination buffer (ownership acquire)
                    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
                    nullptr,                                 // pNext
                    0,                                       // srcAccessMask
                    VK_ACCESS_TRANSFER_WRITE_BIT,            // dstAccessMask
                    m_src_q_fam_idx,                         // srcQueueFamilyIndex,
                    q_fam_idx,                               // dstQueueFamilyIndex,
                    m_xfer.subject.buffer,                   // buffer
                    m_xfer.offset,                           // offset
                    m_xfer.size                              // size
                  }};

                collections::StaticArray<VkBufferMemoryBarrier, 2> mem_barrier_xfer{
                  {
                    // Destination buffer (ownership release)
                    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
                    nullptr,                                 // pNext
                    VK_ACCESS_MEMORY_WRITE_BIT,              // srcAccessMask
                    0,                                       // dstAccessMask
                    q_fam_idx,                               // srcQueueFamilyIndex
                    m_dst_q_fam_idx,                         // dstQueueFamilyIndex
                    m_xfer.subject.buffer,                   // buffer
                    m_xfer.offset,                           // offset
                    m_xfer.size                              // size
                  },
                  {
                    // Destination buffer (ownership acquire)
                    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
                    nullptr,                                 // pNext
                    0,                                       // srcAccessMask
                    m_xfer.dst_access_mask,                  // dstAccessMask
                    q_fam_idx,                               // srcQueueFamilyIndex
                    m_dst_q_fam_idx,                         // dstQueueFamilyIndex
                    m_xfer.subject.buffer,                   // buffer
                    m_xfer.offset,                           // offset
                    m_xfer.size                              // size
                  }};

                // Acquire ownership from previous using queue
                vkCmdPipelineBarrier(
                  m_cmdbuf,
                  VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_HOST_BIT | m_xfer.src_stage_mask,
                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                  0,
                  0,
                  nullptr,
                  2,
                  mem_barrier_flush,
                  0,
                  nullptr);

                if(needs_own_acq) {
                    vkCmdPipelineBarrier(
                      m_cmdbuf,
                      m_xfer.src_stage_mask,
                      VK_PIPELINE_STAGE_TRANSFER_BIT,
                      0,
                      0,
                      nullptr,
                      1,
                      mem_barrier_flush + 2,
                      0,
                      nullptr);
                }

                // Do ya' thing
                vkCmdCopyBuffer(
                  m_cmdbuf.get_buffer(), m_staging.get_buffer(), m_xfer.subject.buffer, 1, &copy);

                // staging ownership to target queue
                vkCmdPipelineBarrier(
                  m_cmdbuf,
                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                  m_xfer.dst_stage_mask,
                  0,
                  0,
                  nullptr,
                  1,
                  mem_barrier_xfer,
                  0,
                  nullptr);

                if(needs_own_rel) {
                    vkCmdPipelineBarrier(
                      m_cmdbuf,
                      VK_PIPELINE_STAGE_TRANSFER_BIT,
                      m_xfer.dst_stage_mask,
                      0,
                      0,
                      nullptr,
                      1,
                      mem_barrier_xfer + 1,
                      0,
                      nullptr);
                }
            }

            DownloadOp::DownloadOp(StagingBuffer &staging, Transfer const &transfer) :
              TransferOp{staging, transfer}, m_bufsz{0} {}

            DownloadOp::DownloadOp(DownloadOp &&other) :
              TransferOp{std::move(other)}, m_bufsz{other.m_bufsz} {}

            bool DownloadOp::transfer_single_block() {
                if(!m_finished) {
                    Fence &fence = m_staging.get_fence();

                    if(fence.wait(10)) {
                        bool const first_chunk = m_xfer.offset == m_cur_offset;
                        uint32_t q_fam_idx =
                          m_staging.get_renderer().get_queue_family_index(m_staging.get_queue());
                        ActionSynchro synchro;

                        fence.reset();
                        m_cmdbuf.reset(false);

                        if(!first_chunk) {
                            host_copy_to_destination(m_bufsz);

                            m_finished = m_cur_offset >= m_xfer.offset + m_xfer.size;
                        }

                        if(!m_finished) {
                            m_cmdbuf.begin_record();

                            m_bufsz = device_begin_next_transfer(q_fam_idx);

                            m_cmdbuf.end_record();

                            m_cur_offset += m_bufsz;
                            synchro.fence = fence;

                            m_cmdbuf.submit(m_staging.get_queue(), synchro);
                        }
                    }
                }

                return m_finished;
            }

            VkDeviceSize BufferDownloadOp::device_begin_next_transfer(uint32_t const q_fam_idx) {
                VkDeviceSize const buf_sz_to_end = m_xfer.size + m_xfer.offset - m_cur_offset;
                VkDeviceSize const bufsz = min(buf_sz_to_end, m_staging.get_buffer_size());
                bool const first_chunk = m_xfer.offset == m_cur_offset;
                bool const last_chunk = buf_sz_to_end <= m_staging.get_buffer_size();
                bool const needs_own_acq =
                  q_fam_idx != m_src_q_fam_idx
                  && first_chunk; //< Does this buffer need queue family ownership acquire?
                bool const needs_own_rel =
                  q_fam_idx != m_dst_q_fam_idx
                  && last_chunk; //< Does this buffer need queue family ownership release?

                VkBufferCopy region{
                  m_cur_offset, // srcOffset
                  0,            // dstOffset
                  bufsz         // size
                };

                collections::StaticArray<VkBufferMemoryBarrier, 3> membar_before_xfer{
                  {
                    // Staging buffer
                    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,                    // sType
                    nullptr,                                                    // pNext
                    VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT, // srcAccessMask
                    VK_ACCESS_TRANSFER_WRITE_BIT,                               // dstAccessMask
                    q_fam_idx,                                                  // srcQueueFamilyIndex
                    q_fam_idx,                                                  // dstQueueFamilyIndex
                    m_staging.get_buffer(),                                     // buffer
                    0,                                                          // offset
                    VK_WHOLE_SIZE                                               // size
                  },
                  {
                    // Source buffer (+ prev ownership release)
                    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
                    nullptr,                                 // pNext
                    choose<VkAccessFlags>(VK_ACCESS_MEMORY_WRITE_BIT, 0, needs_own_acq)
                      | m_xfer.src_access_mask, // srcAccessMask
                    choose<VkAccessFlags>(
                      0,
                      VK_ACCESS_TRANSFER_READ_BIT,
                      needs_own_acq),      // dstAccessMask
                    m_src_q_fam_idx,       // srcQueueFamilyIndex
                    q_fam_idx,             // dstQueueFamilyIndex
                    m_xfer.subject.buffer, // buffer
                    m_xfer.offset,         // offset
                    m_xfer.size            // size
                  },
                  {
                    // Source buffer (ownership acquire)
                    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
                    nullptr,                                 // pNext
                    0,                                       // srcAccessMask
                    VK_ACCESS_TRANSFER_READ_BIT,             // dstAccessMask
                    m_src_q_fam_idx,                         // srcQueueFamilyIndex
                    q_fam_idx,                               // dstQueueFamilyIndex
                    m_xfer.subject.buffer,                   // buffer
                    m_xfer.offset,                           // offset
                    m_xfer.size                              // size
                  }};

                vkCmdPipelineBarrier(
                  m_cmdbuf,
                  m_xfer.src_stage_mask | VK_PIPELINE_STAGE_TRANSFER_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                  0,
                  0,
                  nullptr,
                  2,
                  membar_before_xfer,
                  0,
                  nullptr);

                if(needs_own_acq) {
                    vkCmdPipelineBarrier(
                      m_cmdbuf,
                      m_xfer.src_stage_mask | VK_PIPELINE_STAGE_TRANSFER_BIT,
                      VK_PIPELINE_STAGE_TRANSFER_BIT,
                      0,
                      0,
                      nullptr,
                      1,
                      membar_before_xfer + 2,
                      0,
                      nullptr);
                }

                vkCmdCopyBuffer(m_cmdbuf, m_xfer.subject.buffer, m_staging.get_buffer(), 1, &region);

                collections::StaticArray<VkBufferMemoryBarrier, 3> membar_after_xfer{
                  {
                    // Staging buffer
                    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,            // sType
                    nullptr,                                            // pNext
                    VK_ACCESS_TRANSFER_WRITE_BIT,                       // srcAccessMask
                    VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT, // dstAccessMask
                    q_fam_idx,                                          // srcQueueFamilyIndex
                    q_fam_idx,                                          // dstQueueFamilyIndex
                    m_staging.get_buffer(),                             // buffer
                    0,                                                  // offset
                    VK_WHOLE_SIZE                                       // size
                  },
                  {
                    // Source buffer (final ownership release)
                    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
                    nullptr,                                 // pNext
                    VK_ACCESS_MEMORY_WRITE_BIT,              // srcAccessMask
                    0,                                       // dstAccessMask
                    q_fam_idx,                               // srcQueueFamilyIndex
                    m_dst_q_fam_idx,                         // dstQueueFamilyIndex
                    m_xfer.subject.buffer,                   // buffer
                    m_xfer.offset,                           // offset
                    m_xfer.size                              // size
                  },
                  {
                    // Source buffer (final ownership release)
                    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
                    nullptr,                                 // pNext
                    0,                                       // srcAccessMask
                    m_xfer.dst_access_mask,                  // dstAccessMask
                    q_fam_idx,                               // srcQueueFamilyIndex
                    m_dst_q_fam_idx,                         // dstQueueFamilyIndex
                    m_xfer.subject.buffer,                   // buffer
                    m_xfer.offset,                           // offset
                    m_xfer.size                              // size
                  }};

                vkCmdPipelineBarrier(
                  m_cmdbuf,
                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                  VK_PIPELINE_STAGE_HOST_BIT,
                  0,
                  0,
                  nullptr,
                  1,
                  membar_after_xfer,
                  0,
                  nullptr);

                if(needs_own_rel) {
                    vkCmdPipelineBarrier(
                      m_cmdbuf,
                      VK_PIPELINE_STAGE_TRANSFER_BIT,
                      VK_PIPELINE_STAGE_HOST_BIT,
                      0,
                      0,
                      nullptr,
                      1,
                      membar_after_xfer + 1,
                      0,
                      nullptr);

                    vkCmdPipelineBarrier(
                      m_cmdbuf,
                      VK_PIPELINE_STAGE_TRANSFER_BIT,
                      VK_PIPELINE_STAGE_HOST_BIT,
                      0,
                      0,
                      nullptr,
                      1,
                      membar_after_xfer + 2,
                      0,
                      nullptr);
                }

                return bufsz;
            }

            void DownloadOp::host_copy_to_destination(VkDeviceSize const bufsz) {
                if(!m_staging.is_coherent()) {
                    VkMappedMemoryRange memrange{
                      VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, // sType
                      nullptr,                               // pNext
                      m_staging.get_memory(),                // memory
                      0,                                     // offset
                      VK_WHOLE_SIZE                          // size
                    };

                    VkResult result =
                      vkInvalidateMappedMemoryRanges(m_staging.get_renderer().get_device(), 1, &memrange);
                    jltassert2(result == VK_SUCCESS, "Unable to invalidate staging buffer");
                }

                memcpy(
                  reinterpret_cast<uint8_t *>(m_xfer.data.data) + m_cur_offset - m_xfer.offset - m_bufsz,
                  m_staging.get_buffer_ptr(),
                  bufsz);
            }

            BufferDownloadOp::BufferDownloadOp(StagingBuffer &staging, Transfer const &transfer) :
              DownloadOp{staging, transfer} {}
        } // namespace vulkan
    }     // namespace graphics
} // namespace jolt
