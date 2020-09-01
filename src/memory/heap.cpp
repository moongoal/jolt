#include <Windows.h>
#include <debug.hpp>
#include <util.hpp>
#include "heap.hpp"

namespace jolt {
    namespace memory {
        Heap::Heap(size_t const sz, void *const base) :
            m_base_ptr(::VirtualAlloc(base, sz, MEM_RESERVE, PAGE_READWRITE)), m_size(sz),
            m_committed_size(0) {
            jltassert(m_base_ptr);
        }

        Heap::~Heap() { ::VirtualFree((LPVOID)get_base(), (SIZE_T)0, MEM_RELEASE); }

        void *Heap::extend(size_t const ext_sz) {
            jltassert(get_committed_size() + ext_sz <= get_size());
            size_t const real_ext_sz = choose(MIN_ALLOC_SIZE, ext_sz, ext_sz < MIN_ALLOC_SIZE);

            void *const ptr =
                ::VirtualAlloc(reinterpret_cast<char *>(get_base()) + get_committed_size(),
                               static_cast<SIZE_T>(real_ext_sz),
                               MEM_COMMIT,
                               PAGE_READWRITE);

            jltassert(ptr);

            m_committed_size += real_ext_sz;

            return ptr;
        }
    } // namespace memory
} // namespace jolt
