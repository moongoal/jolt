#include <debug.hpp>
#include <util.hpp>
#include "stack.hpp"

namespace jolt {
    namespace memory {
        void *Stack::allocate(uint32_t const size, flags_t const flags, uint32_t const alignment) {
            size_t const sz_committed = get_committed_size();
            size_t const sz_allocated = get_allocated_size();
            size_t const sz_free = sz_committed - sz_allocated;

            void *const ptr_alloc = align_raw_ptr(m_ptr_top + sizeof(StackAllocHeader), alignment);
            StackAllocHeader *const ptr_hdr = reinterpret_cast<StackAllocHeader *>(ptr_alloc) - 1;
            size_t const padding =
                reinterpret_cast<uint8_t *>(ptr_alloc) - m_ptr_top - sizeof(StackAllocHeader);
            size_t const total_alloc_sz = reinterpret_cast<uint8_t *>(ptr_alloc) + size - m_ptr_top
#ifdef JLT_WITH_MEM_CHECKS
                                          + JLT_MEM_CANARY_VALUE_SIZE
#endif // JLT_WITH_MEM_CHECKS
                ;

            ensure_free_memory_consistency(m_ptr_top, sz_free);

            if(sz_free < total_alloc_sz) {
                commit(total_alloc_sz - sz_free); // Extend by the excess
            }

            new(ptr_hdr) StackAllocHeader(size, static_cast<uint32_t>(padding));

            m_ptr_top += size + padding + sizeof(StackAllocHeader)
#ifdef JLT_WITH_MEM_CHECKS
                         + JLT_MEM_CANARY_VALUE_SIZE
#endif // JLT_WITH_MEM_CHECKS
                ;

            JLT_FILL_OVERFLOW(ptr_alloc, size);

            return ptr_alloc;
        }

        void Stack::free(void *const ptr) {
            auto *const ptr_hdr = reinterpret_cast<StackAllocHeader *>(ptr) - 1;

            jltassert(ptr_hdr->m_alloc_sz + reinterpret_cast<uint8_t *>(ptr)
#ifdef JLT_WITH_MEM_CHECKS
                          + JLT_MEM_CANARY_VALUE_SIZE
#endif // JLT_WITH_MEM_CHECKS
                      == m_ptr_top);

            size_t const total_alloc_size = ptr_hdr->m_alloc_sz + ptr_hdr->m_alloc_offset +
                                            sizeof(StackAllocHeader)
#ifdef JLT_WITH_MEM_CHECKS
                                            + JLT_MEM_CANARY_VALUE_SIZE
#endif // JLT_WITH_MEM_CHECKS
                ;

            JLT_CHECK_OVERFLOW(ptr, ptr_hdr->m_alloc_sz);

            m_ptr_top -= total_alloc_size;

            JLT_FILL_AFTER_FREE(m_ptr_top, total_alloc_size);
        }
    } // namespace memory
} // namespace jolt
