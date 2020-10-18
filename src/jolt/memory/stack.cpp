#include <cstring>
#include <jolt/debug.hpp>
#include <jolt/util.hpp>
#include "stack.hpp"

namespace jolt {
    namespace memory {
        void *Stack::get_top_allocation() {
            if(m_ptr_top != get_base()) {
                return *(reinterpret_cast<void **>(m_ptr_top) - 1);
            }

            return nullptr; // Stack is empty
        }

        void *Stack::allocate(uint32_t const size, flags_t const flags, uint32_t const alignment) {
            size_t const sz_free = get_free_committed_size();
            void *const ptr_alloc = align_raw_ptr(m_ptr_top + sizeof(AllocHeader), alignment);
            AllocHeader *const ptr_hdr = get_header(ptr_alloc);
            size_t const padding = reinterpret_cast<uint8_t *>(ptr_alloc) - m_ptr_top - sizeof(AllocHeader);
            size_t const total_alloc_sz = reinterpret_cast<uint8_t *>(ptr_alloc) + size - m_ptr_top
                                          + JLT_MEM_CANARY_VALUE_SIZE + sizeof(void *);

            ensure_free_memory_consistency(m_ptr_top, sz_free);

            if(sz_free < total_alloc_sz) {
                commit(total_alloc_sz - sz_free); // Extend by the excess
            }

            // Header
            new(ptr_hdr) AllocHeader(size, flags, static_cast<uint32_t>(padding), alignment);

            // Footer
            auto footer_ptr = reinterpret_cast<void **>(
              reinterpret_cast<uint8_t *>(ptr_alloc) + size + JLT_MEM_CANARY_VALUE_SIZE);

            *footer_ptr = ptr_alloc; // Footer is returned pointer
            // m_ptr_top += get_total_allocation_size(size, padding);
            m_ptr_top += total_alloc_sz;

            JLT_FILL_OVERFLOW(ptr_alloc, size);

            return ptr_alloc;
        }

        void Stack::free_top_finalized() {
            for(void *top_alloc = get_top_allocation(); top_alloc; top_alloc = get_top_allocation()) {
                AllocHeader *hdr = get_header(top_alloc);

                if((hdr->m_flags & ALLOC_FINALIZED) == ALLOC_FINALIZED) {
                    free_single_alloc(top_alloc);
                } else {
                    return; // Finished stream of finalized allocations
                }
            }
        }

        void Stack::free_single_alloc(void *const ptr) {
            jltassert(is_top(ptr));

            AllocHeader *const ptr_hdr = get_header(ptr);
            size_t const total_alloc_size = get_total_allocation_size(ptr);

            JLT_CHECK_OVERFLOW(ptr, ptr_hdr->m_alloc_sz);

            m_ptr_top -= total_alloc_size;

            JLT_FILL_AFTER_FREE(m_ptr_top, total_alloc_size);
        }

        void Stack::free(void *const ptr) {
            AllocHeader *const ptr_hdr = get_header(ptr);

            if(is_top(ptr)) {
                ptr_hdr->m_flags |= ALLOC_FINALIZED;

                free_top_finalized();
            } else {
                jltassert((ptr_hdr->m_flags & ALLOC_FINALIZED) != ALLOC_FINALIZED);

                ptr_hdr->m_flags |= ALLOC_FINALIZED;
            }
        }

        void Stack::realloc_shrink_top(size_t const new_size, AllocHeader *const ptr_hdr) {
            size_t const top_diff = ptr_hdr->m_alloc_sz - new_size;
            m_ptr_top -= top_diff;

            JLT_FILL_AFTER_FREE(m_ptr_top, top_diff);
        }

        void Stack::realloc_grow_top(size_t const new_size, AllocHeader *const ptr_hdr) {
            uint8_t *const ptr_far_end = reinterpret_cast<uint8_t *>(get_base()) + get_committed_size();
            size_t const top_diff = new_size - ptr_hdr->m_alloc_sz;
            m_ptr_top += top_diff;

            if(m_ptr_top > ptr_far_end) {
                commit(m_ptr_top - ptr_far_end);
            }
        }

        void *Stack::reallocate(void *const ptr, size_t const new_size) {
            jltassert(ptr);
            jltassert(new_size);

            AllocHeader *const ptr_hdr = get_header(ptr);

            if(is_top(ptr)) {
                if(new_size < ptr_hdr->m_alloc_sz) {
                    realloc_shrink_top(new_size, ptr_hdr);
                } else {
                    realloc_grow_top(new_size, ptr_hdr);
                }

                // Update header
                ptr_hdr->m_alloc_sz = new_size;

                // Update footer
                auto alloc_end_ptr = reinterpret_cast<uint8_t *>(ptr) + ptr_hdr->m_alloc_sz
                                     + JLT_MEM_CANARY_VALUE_SIZE + sizeof(void *);
                auto footer_ptr = reinterpret_cast<void **>(alloc_end_ptr) - 1;

                *footer_ptr = ptr;

                JLT_FILL_OVERFLOW(ptr, new_size);
            } else { // Region is not top
                if(new_size > ptr_hdr->m_alloc_sz) {
                    void *new_ptr = allocate(new_size, ptr_hdr->m_flags, ptr_hdr->m_alignment);

                    memmove(new_ptr, ptr, ptr_hdr->m_alloc_sz);

                    free(ptr); // Do not swap allocate and free here

                    return new_ptr;
                }
            }

            return ptr;
        }

        bool Stack::is_top(void *const ptr) const {
            AllocHeader *const ptr_hdr = get_header(ptr);

            return ptr_hdr->m_alloc_sz + reinterpret_cast<uint8_t *>(ptr) + JLT_MEM_CANARY_VALUE_SIZE
                     + sizeof(void *)
                   == m_ptr_top;
        }
    } // namespace memory
} // namespace jolt
