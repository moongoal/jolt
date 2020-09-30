#include <jolt/debug.hpp>
#include <jolt/util.hpp>
#include "stack.hpp"

namespace jolt {
    namespace memory {
        void *Stack::allocate(uint32_t const size, flags_t const flags, uint32_t const alignment) {
            size_t const sz_free = get_free_committed_size();
            void *const ptr_alloc = align_raw_ptr(m_ptr_top + sizeof(AllocHeader), alignment);
            AllocHeader *const ptr_hdr = get_header(ptr_alloc);
            size_t const padding =
              reinterpret_cast<uint8_t *>(ptr_alloc) - m_ptr_top - sizeof(AllocHeader);
            size_t const total_alloc_sz =
              reinterpret_cast<uint8_t *>(ptr_alloc) + size - m_ptr_top + JLT_MEM_CANARY_VALUE_SIZE;

            ensure_free_memory_consistency(m_ptr_top, sz_free);

            if(sz_free < total_alloc_sz) {
                commit(total_alloc_sz - sz_free); // Extend by the excess
            }

            new(ptr_hdr) AllocHeader(size, flags, static_cast<uint32_t>(padding), alignment);

            m_ptr_top += get_total_allocation_size(size, padding);

            JLT_FILL_OVERFLOW(ptr_alloc, size);

            return ptr_alloc;
        }

        void Stack::free(void *const ptr) {
            jltassert(is_top(ptr));

            AllocHeader *const ptr_hdr = get_header(ptr);
            size_t const total_alloc_size = get_total_allocation_size(ptr);

            JLT_CHECK_OVERFLOW(ptr, ptr_hdr->m_alloc_sz);

            m_ptr_top -= total_alloc_size;

            JLT_FILL_AFTER_FREE(m_ptr_top, total_alloc_size);
        }

        void Stack::reallocate(void *const ptr, size_t const new_size) {
            jltassert(ptr);
            jltassert(new_size);
            jltassert(is_top(ptr));

            AllocHeader *const ptr_hdr = get_header(ptr);

            if(new_size < ptr_hdr->m_alloc_sz) {
                size_t const top_diff = ptr_hdr->m_alloc_sz - new_size;
                m_ptr_top -= top_diff;

                JLT_FILL_AFTER_FREE(m_ptr_top, top_diff);
            } else {
                uint8_t *const ptr_far_end =
                  reinterpret_cast<uint8_t *>(get_base()) + get_committed_size();
                size_t const top_diff = new_size - ptr_hdr->m_alloc_sz;
                m_ptr_top += top_diff;

                if(m_ptr_top > ptr_far_end) {
                    commit(m_ptr_top - ptr_far_end);
                }
            }

            ptr_hdr->m_alloc_sz = new_size;

            JLT_FILL_OVERFLOW(ptr, new_size);
        }

        bool Stack::is_top(void *const ptr) const {
            AllocHeader *const ptr_hdr = get_header(ptr);

            return ptr_hdr->m_alloc_sz + reinterpret_cast<uint8_t *>(ptr)
                     + JLT_MEM_CANARY_VALUE_SIZE
                   == m_ptr_top;
        }
    } // namespace memory
} // namespace jolt
