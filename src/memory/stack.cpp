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
            size_t const padding = reinterpret_cast<char *>(ptr_alloc) - m_ptr_top - sizeof(StackAllocHeader);
            size_t const total_alloc_sz = reinterpret_cast<char *>(ptr_alloc) + size - m_ptr_top;

            if(sz_free < total_alloc_sz) {
                extend(total_alloc_sz - sz_free); // Extend by the excess
            }

            new(ptr_hdr) StackAllocHeader(size, static_cast<uint32_t>(padding));

            m_ptr_top += size + padding + sizeof(StackAllocHeader);

            return ptr_alloc;
        }

        void Stack::free(void *const ptr) {
            auto *const ptr_hdr = reinterpret_cast<StackAllocHeader *>(ptr) - 1;

            jltassert(ptr_hdr->m_alloc_sz + reinterpret_cast<char *>(ptr) == m_ptr_top);
            m_ptr_top -= ptr_hdr->m_alloc_sz + ptr_hdr->m_alloc_offset + sizeof(StackAllocHeader);
        }
    } // namespace memory
} // namespace jolt
