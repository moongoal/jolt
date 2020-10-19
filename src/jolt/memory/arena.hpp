#ifndef JLT_MEMORY_HEAP_HPP
#define JLT_MEMORY_HEAP_HPP

#include <jolt/api.hpp>
#include "checks.hpp"
#include "heap.hpp"
#include "defs.hpp"

namespace jolt {
    namespace memory {
        struct ArenaFreeListNode {
            size_t m_size;
            ArenaFreeListNode *m_prev, *m_next;

#ifdef JLT_WITH_MEM_CHECKS
            JLT_MEM_OVERFLOW_CANARY_VALUE_TYPE m_free_canary = JLT_MEM_ARENA_FLN_CANARY_VALUE;
#endif // JLT_WITH_MEM_CHECKS

            ArenaFreeListNode(
              size_t const size, ArenaFreeListNode *const prev, ArenaFreeListNode *const next) :
              m_size{size},
              m_prev{prev}, m_next{next} {}
        };

        class JLTAPI Arena : public Heap {
            ArenaFreeListNode *m_free_list;
            size_t m_allocated_size;

            /**
             * Find a free list node available containing at least the specified amount of memory.
             *
             * @return A pointer to an ArenaFreeListNode or nullptr if no candidate is suitable.
             */
            JLT_NODISCARD ArenaFreeListNode *find_free_list_node(size_t size) const;

            /**
             * Find the left-closest free list node available for a given pointer.
             *
             * @return A pointer to an ArenaFreeListNode or nullptr if no candidate is suitable.
             * @remark "left-closest" assumes a left-to-right ordering of memory addresses where an
             * address A at the left of an address B has a lower memory address.
             */
            JLT_NODISCARD ArenaFreeListNode *find_left_closest_node(void *const ptr) const;

            /**
             * Find the left-closest free list node available for a given pointer and size.
             *
             * @return A pointer to an ArenaFreeListNode or nullptr if no candidate is suitable.
             * @remark "right-closest" assumes a left-to-right ordering of memory addresses where an
             * address A at the left of an address B has a lower memory address.
             */
            JLT_NODISCARD ArenaFreeListNode *
            find_right_closest_node(void *const ptr, size_t const size) const;

            JLT_NODISCARD void *
            reallocate_shrink(void *const ptr, uint32_t const new_size, AllocHeader *const ptr_hdr);

            JLT_NODISCARD void *
            reallocate_grow(void *const ptr, uint32_t const new_size, AllocHeader *const ptr_hdr);

          public:
            JLT_NODISCARD explicit Arena(size_t const memory_size);

            /**
             * Allocation function.
             *
             * @param size The total size of the memory to allocate.
             * @param flags Allocation flags.
             * @param alignment The alignment requirements for the allocated memory.
             */
            JLT_NODISCARD void *allocate(uint32_t const size, flags_t const flags, uint32_t const alignment);

            /**
             * Free a location in memory given its pointer. Only the latest allocation on the stack
             * can be freed at a given time.
             *
             * @param ptr A pointer to the beginning of the memory location to free.
             */
            void free(void *const ptr);

            /**
             * Get the amount of memory that is currently allocated.
             */
            JLT_NODISCARD size_t get_allocated_size() const { return m_allocated_size; }

            /**
             * Return a node in the free list.
             * @note The returned node is not necessarily the first in the list.
             */
            JLT_NODISCARD ArenaFreeListNode *get_free_list() const { return m_free_list; }

            /**
             * Ensure that a block of unallocated memory has not been written to.
             * This method will abort upon failure.
             *
             * @param node A pointer to the free list node representing the block of unallocated
             * memory to check.
             */
            void ensure_free_memory_consistency(ArenaFreeListNode *const node) const;

            /**
             * Reallocate a pre-allocated chunk of memory, resizing it.
             *
             * @param ptr Pointer to the memory to reallocate.
             * @param new_size The new size of the new allocation.
             *
             * @return A pointer to the new allocation.
             */
            JLT_NODISCARD void *reallocate(void *const ptr, uint32_t const new_size);

            /**
             * Return a value stating whether reallocating a pointer will result in the memory
             * allocation to be displaced.
             *
             * @param ptr The pointer to the memory allocation.
             * @param new_size The new size to test.
             *
             * @return True if reallocating `ptr` will result in a different pointer being returned
             * by `reallocate()`. False otherwise.
             */
            JLT_NODISCARD bool will_relocate(void *const ptr, uint32_t const new_size) const;

            /**
             * Return the allocation header for a given pointer.
             *
             * @param ptr Pointer to the base of a memory allocation.
             *
             * @note This function will not check whether the pointer represents
             * the base of the allocation or not and will return garbage if it
             * doesn't.
             */
            JLT_NODISCARD static AllocHeader *get_header(void *const ptr) {
                return reinterpret_cast<AllocHeader *>(ptr) - 1;
            }

            /**
             * Return the total allocation size for a given pointer.
             *
             * @param ptr Pointer to the base of a memory allocation.
             *
             * @return The sum of padding, allocation size, header and overflow canary.
             * @note This function will not check whether the pointer represents
             * the base of the allocation or not and will return garbage if it
             * doesn't.
             */
            JLT_NODISCARD static uint32_t get_total_allocation_size(void *const ptr) {
                AllocHeader *const ptr_hdr = get_header(ptr);

                return get_total_allocation_size(ptr_hdr->m_alloc_sz, ptr_hdr->m_alloc_offset);
            }

            /**
             * Return the total allocation size for a given set of parameters.
             *
             * @param size The size of the allocation (data only).
             * @param padding The amount of padding used to align the allocation.
             *
             * @return The sum of padding, allocation size, header and overflow canary.
             * @note This function will not check whether the pointer represents
             * the base of the allocation or not and will return garbage if it
             * doesn't.
             */
            JLT_NODISCARD static uint32_t
            get_total_allocation_size(uint32_t const size, uint32_t const padding) {
                return size + padding + sizeof(AllocHeader) + JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE;
            }
        };
    } // namespace memory
} // namespace jolt

#endif /* JLT_MEMORY_HEAP_HPP */
