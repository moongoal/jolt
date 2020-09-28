#include <Windows.h>
#include <jolt/debug.hpp>
#include <jolt/util.hpp>
#include <jolt/features.hpp>
#include "checks.hpp"
#include "heap.hpp"

namespace jolt {
    namespace memory {
        Heap::Heap(size_t const sz, void *const base) :
            m_base_ptr(::VirtualAlloc(base, sz, MEM_RESERVE, PAGE_READWRITE)), m_size(sz),
            m_committed_size(0) {
            jltassert(m_base_ptr);
        }

        Heap::~Heap() { ::VirtualFree((LPVOID)get_base(), (SIZE_T)0, MEM_RELEASE); }

        void *Heap::commit(size_t const ext_sz) {
            jltassert(get_committed_size() + ext_sz <= get_size());
            size_t const real_ext_sz = max(MIN_ALLOC_SIZE, ext_sz);
            void *const commit_ptr = reinterpret_cast<uint8_t *>(get_base()) + get_committed_size();

            void *const ptr = ::VirtualAlloc(commit_ptr,
                                             static_cast<SIZE_T>(real_ext_sz),
                                             MEM_COMMIT,
                                             PAGE_READWRITE);

            jltassert(ptr);

#ifdef JLT_WITH_MEM_CHECKS
            JLT_FILL_AFTER_FREE(commit_ptr, real_ext_sz);
#endif // JLT_WITH_MEM_CHECKS

            m_committed_size += real_ext_sz;

            return ptr;
        }
    } // namespace memory
} // namespace jolt
