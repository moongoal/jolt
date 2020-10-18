#ifndef JLT_MEMORY_STACK_HPP
#define JLT_MEMORY_STACK_HPP

#include <cstdint>
#include <jolt/api.hpp>
#include "heap.hpp"
#include "checks.hpp"
#include "defs.hpp"

namespace jolt {
    namespace memory {
        class JLTAPI Stack : public Heap {
            uint8_t *m_ptr_top; // Pointer to the top of the stack

            /**
             * Free any finalized allocation at the top of the stack.
             */
            void free_top_finalized();

            /**
             * Return the allocated region at the top of the stack.
             *
             * @return A pointer to the last allocation on the stack or `nullptr` if the stack is
             * empty.
             */
            void *get_top_allocation();

            /**
             * Free a single allocated region.
             */
            void free_single_alloc(void *const ptr);

            void realloc_shrink_top(size_t const new_size, AllocHeader *const ptr_hdr);
            void realloc_grow_top(size_t const new_size, AllocHeader *const ptr_hdr);

          public:
            explicit Stack(size_t const memory_size) : Heap(memory_size) {
                m_ptr_top = reinterpret_cast<uint8_t *>(get_base());
            }

            /**
             * Allocation function.
             *
             * @param size The total size of the memory to allocate.
             * @param flags Allocation flags.
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
            size_t get_allocated_size() const { return m_ptr_top - reinterpret_cast<uint8_t *>(get_base()); }

            /**
             * Get a pointer to the top of the stack.
             */
            void *get_top() const { return m_ptr_top; }

            /**
             * Get the size of free committed memory.
             */
            size_t get_free_committed_size() const { return get_committed_size() - get_allocated_size(); }

            /**
             * Ensure the free memory is consistent. If not, abort.
             *
             * @param ptr The pointer to the memory region to check.
             * @param size The size of the memory region.
             */
            void ensure_free_memory_consistency(void *const ptr, size_t const size) const {
                jltassert(JLT_CHECK_MEM_USE_AFTER_FREE(ptr, size));
            }

            /**
             * Reallocate a pre-allocated chunk of memory, resizing it.
             *
             * @param ptr Pointer to the memory to reallocate.
             * @param new_size Size of the new allocation.
             */
            void *reallocate(void *const ptr, size_t const new_size);

            /**
             * Check whether a given memory location is at the top of the stack.
             *
             * @param ptr The pointer to the memory to check.
             *
             * @return True if the memory location is at the top of the stack, false otherwise.
             */
            bool is_top(void *const ptr) const;

            /**
             * Return the allocation header for a given pointer.
             *
             * @param ptr Pointer to the base of a memory allocation.
             *
             * @note This function will not check whether the pointer represents
             * the base of the allocation or not and will return garbage if it
             * doesn't.
             */
            static AllocHeader *get_header(void *const ptr) {
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
            static size_t get_total_allocation_size(void *const ptr) {
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
            static size_t get_total_allocation_size(size_t const size, size_t const padding) {
                return size + padding + sizeof(AllocHeader) + JLT_MEM_CANARY_VALUE_SIZE + sizeof(void *);
            }

            bool will_relocate(void *const ptr, size_t new_size) const {
                AllocHeader *hdr_ptr = get_header(ptr);

                return is_top(ptr) && (new_size > hdr_ptr->m_alloc_sz);
            }
        };
    } // namespace memory
} // namespace jolt
#endif // JLT_MEMORY_STACK_HPP
