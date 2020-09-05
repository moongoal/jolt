#ifndef JLT_MEMORY_HEAP_HPP
#define JLT_MEMORY_HEAP_HPP

#include <util.hpp>
#include "checks.hpp"
#include "defs.hpp"
#include "heap.hpp"

namespace jolt {
    namespace memory {
        struct ArenaAllocHeader {
            uint32_t const m_alloc_sz;
            uint32_t const m_alloc_offset;

#ifdef JLT_WITH_MEM_CHECKS
            JLT_MEM_CANARY_VALUE_TYPE m_free_canary = JLT_MEM_CANARY_VALUE;
#endif // JLT_WITH_MEM_CHECKS

            ArenaAllocHeader(uint32_t const alloc_sz, uint32_t const offset) :
                m_alloc_sz(alloc_sz), m_alloc_offset(offset) {}
        };

        struct ArenaFreeListNode {
            ArenaFreeListNode *m_prev, *m_next;
            size_t m_size;

            ArenaFreeListNode(size_t const size,
                              ArenaFreeListNode *const prev,
                              ArenaFreeListNode *const next) :
                m_size(size),
                m_prev(prev), m_next(next) {}
        };

        class JLTAPI Arena : public Heap {
            ArenaFreeListNode *m_free_list;
            size_t m_allocated_size;

            /**
             * Find a free list node available containing at least the specified amount of memory.
             *
             * @return A pointer to an ArenaFreeListNode or nullptr if no candidate is suitable.
             */
            ArenaFreeListNode *find_free_list_node(size_t size) const;

            /**
             * Find the left-closest free list node available for a given pointer.
             *
             * @return A pointer to an ArenaFreeListNode or nullptr if no candidate is suitable.
             * @remark "left-closest" assumes a left-to-right ordering of memory addresses where an
             * address A at the left of an address B has a lower memory address.
             */
            ArenaFreeListNode *find_left_closest_node(void *const ptr) const;

            /**
             * Find the left-closest free list node available for a given pointer and size.
             *
             * @return A pointer to an ArenaFreeListNode or nullptr if no candidate is suitable.
             * @remark "right-closest" assumes a left-to-right ordering of memory addresses where an
             * address A at the left of an address B has a lower memory address.
             */
            ArenaFreeListNode *find_right_closest_node(void *const ptr, size_t const size) const;

          public:
            explicit Arena(size_t const memory_size);

            /**
             * Allocation function.
             *
             * @param size The total size of the memory to allocate.
             * @param flags The allocation flags.
             * @param alignment The alignment requirements for the allocated memory.
             */
            void *allocate(uint32_t const size, flags_t const flags, uint32_t const alignment);

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
            size_t get_allocated_size() const { return m_allocated_size; }

            /**
             * Return a node in the free list.
             * @note The returned node is not necessarily the first in the list.
             */
            ArenaFreeListNode *get_free_list() const { return m_free_list; }

            void ensure_free_memory_consistency(ArenaFreeListNode *const node) const;
        };
    } // namespace memory
} // namespace jolt

#endif /* JLT_MEMORY_HEAP_HPP */
