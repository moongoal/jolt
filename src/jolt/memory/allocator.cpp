#include <Windows.h>
#include <jolt/debug.hpp>
#include <jolt/threading/thread.hpp>
#include "allocator.hpp"

using namespace jolt::threading;

namespace jolt {
    namespace memory {
        static AllocatorSlot g_alloc_slots[ALLOCATOR_SLOTS];
        static thread_local flags_t flags_override = ALLOC_NONE;
        static thread_local flags_t flags_stack[JLT_ALLOC_FLAGS_STACK_LEN];
        static thread_local size_t flags_stack_top = 0;

        inline uint32_t map_thread_id_to_allocator_slot(thread_id id) {
            return id % ALLOCATOR_SLOTS;
        }

        AllocatorSlot &get_allocator_slot() {
            thread_id const tid = Thread::get_current().get_id();
            uint32_t const slot_idx = map_thread_id_to_allocator_slot(tid);
            return g_alloc_slots[slot_idx];
        }

        AllocatorSlot::AllocatorSlot() :
          m_sm_alloc{SMALL_HEAP_MEMORY_SIZE}, m_bg_alloc{BIG_HEAP_MEMORY_SIZE},
          m_persist{PERSISTENT_MEMORY_SIZE}, m_scratch{SCRATCH_MEMORY_SIZE} {}

        void *_allocate(const size_t size, flags_t const flags, size_t const alignment) {
            AllocatorSlot &slot = get_allocator_slot();
            LockGuard lock{slot.m_lock};

            if((flags & ALLOC_SCRATCH) == ALLOC_SCRATCH) {
                return slot.m_scratch.allocate(size, flags, alignment);
            }

            if((flags & ALLOC_PERSIST) == ALLOC_PERSIST) {
                return slot.m_persist.allocate(size, flags, alignment);
            }

            if((flags & ALLOC_BIG) == ALLOC_BIG) {
                return slot.m_bg_alloc.allocate(size, flags, alignment);
            }

            return slot.m_sm_alloc.allocate(size, flags, alignment);
        }

        static inline bool is_allocation_from_slot(void *const ptr, AllocatorSlot &slot) {
            AllocHeader *const hdr_ptr = get_alloc_header(ptr);

            if((hdr_ptr->m_flags & ALLOC_SCRATCH) == ALLOC_SCRATCH) {
                return slot.m_scratch.owns_ptr(ptr);
            }

            if((hdr_ptr->m_flags & ALLOC_PERSIST) == ALLOC_PERSIST) {
                return slot.m_persist.owns_ptr(ptr);
            }

            if((hdr_ptr->m_flags & ALLOC_BIG) == ALLOC_BIG) {
                return slot.m_bg_alloc.owns_ptr(ptr);
            }

            return slot.m_sm_alloc.owns_ptr(ptr);
        }

        static inline AllocatorSlot &get_slot_for_allocation(void *const ptr) {
            AllocatorSlot *slot = &get_allocator_slot();
            AllocatorSlot *const thread_slot = slot;
            AllocHeader *const hdr_ptr = get_alloc_header(ptr);

            if(!is_allocation_from_slot(ptr, *slot)) {
                size_t i;

                for(i = 0; i < ALLOCATOR_SLOTS - 1; ++i) {
                    slot = &g_alloc_slots[i];

                    if(slot != thread_slot && is_allocation_from_slot(ptr, *slot)) {
                        break;
                    }
                }

                // Last slot doesn't need to be checked.
                // If the allocator is used correctly, any freed allocation *will* belong to one of
                // the slots and if it's not in the first N - 1 slots, then it must necessarily be
                // in the Nth one. This means we don't need to run the check again for the last slot
                // and we can therefore spare some cycles :)
                slot = choose(&g_alloc_slots[ALLOCATOR_SLOTS - 1], slot, i == ALLOCATOR_SLOTS - 1);
            }

            return *slot;
        }

        void _free(void *const ptr) {
            AllocatorSlot &slot = get_allocator_slot();
            flags_t flags = get_alloc_flags(ptr);
            LockGuard lock{slot.m_lock};

            if((flags & ALLOC_SCRATCH) == ALLOC_SCRATCH) {
                return slot.m_scratch.free(ptr);
            }

            if((flags & ALLOC_PERSIST) == ALLOC_PERSIST) {
                return slot.m_persist.free(ptr);
            }

            if((flags & ALLOC_BIG) == ALLOC_BIG) {
                return slot.m_bg_alloc.free(ptr);
            }

            slot.m_sm_alloc.free(ptr);
        }

        size_t get_allocated_size() {
            size_t sz = 0;

            for(size_t i = 0; i < ALLOCATOR_SLOTS; ++i) {
                AllocatorSlot const &slot = g_alloc_slots[i];

                sz += slot.m_bg_alloc.get_allocated_size();
                sz += slot.m_sm_alloc.get_allocated_size();
                sz += slot.m_persist.get_allocated_size();
                sz += slot.m_scratch.get_allocated_size();
            }

            return sz;
        }

        void *_reallocate(void *const ptr, size_t const new_size) {
            AllocatorSlot &slot = get_allocator_slot();
            AllocHeader *const hdr_ptr = get_alloc_header(ptr);

            if((hdr_ptr->m_flags & ALLOC_SCRATCH) == ALLOC_SCRATCH) {
                return slot.m_scratch.reallocate(ptr, new_size);
            }

            if((hdr_ptr->m_flags & ALLOC_PERSIST) == ALLOC_PERSIST) {
                return slot.m_persist.reallocate(ptr, new_size);
            }

            if((hdr_ptr->m_flags & ALLOC_BIG) == ALLOC_BIG) {
                return slot.m_bg_alloc.reallocate(ptr, new_size, hdr_ptr->m_alignment);
            }

            return slot.m_sm_alloc.reallocate(ptr, new_size, hdr_ptr->m_alignment);
        }

        bool will_relocate(void *const ptr, size_t const new_size) {
            AllocatorSlot &slot = get_allocator_slot();
            AllocHeader *const hdr_ptr = get_alloc_header(ptr);

            if((hdr_ptr->m_flags & ALLOC_SCRATCH) == ALLOC_SCRATCH) {
                return slot.m_scratch.will_relocate(ptr, new_size);
            }

            if((hdr_ptr->m_flags & ALLOC_PERSIST) == ALLOC_PERSIST) {
                return slot.m_persist.will_relocate(ptr, new_size);
            }

            if((hdr_ptr->m_flags & ALLOC_BIG) == ALLOC_BIG) {
                return slot.m_bg_alloc.will_relocate(ptr, new_size);
            }

            return slot.m_sm_alloc.will_relocate(ptr, new_size);
        }

        void force_flags(flags_t const flags) { flags_override = flags; }

        void push_force_flags(flags_t const flags) {
            jltassert(flags_stack_top < JLT_ALLOC_FLAGS_STACK_LEN);

            flags_stack[flags_stack_top++] = flags_override;
            force_flags(flags);
        }

        void JLTAPI push_force_flags(void *const ptr) { push_force_flags(get_alloc_flags(ptr)); }

        void pop_force_flags() {
            jltassert(flags_stack_top);

            force_flags(flags_stack[--flags_stack_top]);
        }

        flags_t get_current_force_flags() { return flags_override; }
    } // namespace memory
} // namespace jolt
