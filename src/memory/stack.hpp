#ifndef JLT_MEMORY_STACK_HPP
#define JLT_MEMORY_STACK_HPP

#include <cstdint>
#include <util.hpp>
#include "heap.hpp"
#include "checks.hpp"
#include "defs.hpp"

namespace jolt {
    namespace memory {
        struct StackAllocHeader {
            uint32_t const m_alloc_sz;
            uint32_t const m_alloc_offset;

            StackAllocHeader(uint32_t const alloc_sz, uint32_t const offset) :
                m_alloc_sz(alloc_sz), m_alloc_offset(offset) {}
        };

        class JLTAPI Stack : public Heap {
            uint8_t *m_ptr_top; // Pointer to the top of the stack

          public:
            explicit Stack(size_t const memory_size) : Heap(memory_size) {
                m_ptr_top = reinterpret_cast<uint8_t *>(get_base());
            }

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
            size_t get_allocated_size() const {
                return m_ptr_top - reinterpret_cast<uint8_t *>(get_base());
            }

            /**
             * Get a pointer to the top of the stack.
             */
            void *get_top() const { return m_ptr_top; }

            /**
             * Get the size of free committed memory.
             */
            size_t get_free_committed_size() const {
                return get_committed_size() - get_allocated_size();
            }

            /**
             * Ensure the free memory is consistent. If not, abort.
             *
             * @param ptr The pointer to the memory region to check.
             * @param size The size of the memory region.
             */
            void ensure_free_memory_consistency(void *const ptr, size_t const size) const {
                jltassert(JLT_CHECK_MEM_USE_AFTER_FREE(ptr, size));
            }
        };
    } // namespace memory
} // namespace jolt
#endif // JLT_MEMORY_STACK_HPP
