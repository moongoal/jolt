#ifndef JLT_ALLOCATOR_H
#define JLT_ALLOCATOR_H

#include <cstdint>
#include <util.hpp>
#include "defs.hpp"

namespace jolt {
    namespace memory {
        constexpr size_t BIG_OBJECT_MIN_SIZE = 2 * 1024 * 1024; // 2 MiB

        struct FreeListNode {
            size_t size;
            FreeListNode *prev;
            FreeListNode *next;

            FreeListNode(size_t const node_size,
                         FreeListNode *const prev_node,
                         FreeListNode *const next_node) :
                size(node_size),
                prev(prev_node), next(next_node) {}

            FreeListNode(FreeListNode const &other) = delete;
            FreeListNode &operator=(FreeListNode const &other) = delete;
        };

        struct Allocation {
            uint32_t const m_size;   // The size of the allocation.
            uint16_t const m_offset; // The offset from the origin of the raw allocation to the
                                     // beginning of this structure.
            flags_t const m_flags;   // The allocation flags. TODO: Is this needed?

            Allocation(uint32_t size, uint16_t offset, flags_t flags) :
                m_size(size), m_offset(offset), m_flags(flags) {}
        };

        /**
         * Initialize the allocator.
         *
         * @param max_memory The total maximum memory available to the
         * application.
         */
        void initialize(size_t const max_memory);

        /**
         * Finalize the allocator.
         *
         * When calling this function, any memory allocated through this
         * allocator will be freed.
         */
        void finalize();

        /**
         * The real allocation function. Don't call this. Call `allocate()` instead.
         *
         * @param size The total size of the memory to allocate.
         * @param flags The allocation flags.
         * @param alignment The alignment requirements for the allocated memory.
         */
        void *_allocate(size_t const size, flags_t const flags, size_t const alignment);

        /**
         * The real free function. Don't call this. Call `free()` instead.
         *
         * @param ptr A pointer to the beginning of the memory location to free.
         */
        void _free(void *const ptr);

        /**
         * Allocate memory for an object of type T. This function will not construct the object. Use
         * `construct()` after allocating the memory to construct the object.
         *
         * @tparam T The type of the object to allocate.
         * @param n The number of elements of size `T` to allocate.
         * @param flags The allocation flags.
         * @param alignment The alignment requirements for the allocated memory.
         */
        template<typename T>
        T *allocate(size_t const n = 1,
                    flags_t const flags = ALLOC_NONE,
                    size_t const alignment = alignof(T)) {
            return reinterpret_cast<T *>(
                _allocate(n * sizeof(T),
                          choose(flags, flags | ALLOC_BIG, sizeof(T) < BIG_OBJECT_MIN_SIZE),
                          alignment));
        }

        /**
         * Construct an object.
         *
         * @tparam T The object type to construct.
         * @tparam Params The constructor parameters types.
         * @param ptr A pointer to the memory location where the object will be constructed.
         * @param ctor_params The values of the parameters that will be passed to the constructor.
         *
         * @return The same value as `ptr`.
         */
        template<typename T, typename... Params>
        T *construct(T *const ptr, Params... ctor_params) {
            return new(ptr) T(ctor_params...);
        }

        /**
         * Free a location in memory given its pointer. The object's constructor will be called
         * first.
         *
         * @param ptr A pointer to the beginning of the memory location to free.
         */
        template<typename T>
        void free(T *const ptr) {
            ptr->~T();
            _free(ptr);
        }
    } // namespace memory
} // namespace jolt

#endif /* JLT_ALLOCATOR_H */
