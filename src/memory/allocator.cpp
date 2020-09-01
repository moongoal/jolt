#include <Windows.h>
#include <debug.hpp>
#include "allocator.hpp"

static void *s_mem_ptr = nullptr;
static size_t s_mem_sz;

namespace jolt {
    namespace memory {
        static FreeListNode *free_list = nullptr;

        void initialize(size_t const max_memory) {
            s_mem_ptr =
                ::VirtualAlloc(NULL, (SIZE_T)max_memory, MEM_RESERVE, FALSE);
            s_mem_sz = max_memory;

            jltassert(s_mem_ptr);

            free_list = new (s_mem_ptr) FreeListNode(s_mem_sz, nullptr, nullptr);
        }

        void finalize() {
            if (s_mem_ptr) {
                ::VirtualFree(s_mem_ptr, 0, MEM_RELEASE);
                s_mem_ptr = nullptr;
            }
        }

        void *_allocate(const size_t size, flags_t const flags,
                        size_t const alignment) {
            return nullptr;
        }

        void free(void *const ptr) {

        }
    } // namespace memory
} // namespace jolt
