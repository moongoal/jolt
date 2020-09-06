#include <Windows.h>
#include <debug.hpp>
#include <threading/thread.hpp>
#include "allocator.hpp"

using namespace jolt::threading;

namespace jolt {
    namespace memory {
        static AllocatorSlot g_alloc_slots[ALLOCATOR_SLOTS];

        inline uint32_t map_thread_id_to_allocator_slot(thread_id id) {
            return id % ALLOCATOR_SLOTS;
        }

        AllocatorSlot::AllocatorSlot() :
            m_sm_alloc(SMALL_HEAP_MEMORY_SIZE), m_bg_alloc(BIG_HEAP_MEMORY_SIZE),
            m_persist(PERSISTENT_MEMORY_SIZE), m_scratch(SCRATCH_MEMORY_SIZE) {}

        // void initialize(size_t const max_memory) {}

        // void finalize() {}

        void *_allocate(const size_t size, flags_t const flags, size_t const alignment) {
            thread_id const tid = Thread::current().get_id();
            uint32_t const slot_idx = map_thread_id_to_allocator_slot(tid);
            AllocatorSlot &slot = g_alloc_slots[slot_idx];

            if(flags & ALLOC_SCRATCH) {
                return slot.m_scratch.allocate(size, flags, alignment);
            }

            if(flags & ALLOC_PERSIST) {
                return slot.m_persist.allocate(size, flags, alignment);
            }

            if(flags & ALLOC_BIG) {
                return slot.m_bg_alloc.allocate(size, flags, alignment);
            }

            return slot.m_sm_alloc.allocate(size, flags, alignment);
        }

        void free(void *const ptr) {
            thread_id const tid = Thread::current().get_id();
            uint32_t const slot_idx = map_thread_id_to_allocator_slot(tid);
            AllocatorSlot &slot = g_alloc_slots[slot_idx];
            AllocHeader *const hdr_ptr = get_alloc_header(ptr);

            if(hdr_ptr->m_flags & ALLOC_SCRATCH) {
                return slot.m_scratch.free(ptr);
            }

            if(hdr_ptr->m_flags & ALLOC_PERSIST) {
                return slot.m_persist.free(ptr);
            }

            if(hdr_ptr->m_flags & ALLOC_BIG) {
                return slot.m_bg_alloc.free(ptr);
            }
        }
    } // namespace memory
} // namespace jolt
