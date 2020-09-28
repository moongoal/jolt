#ifndef JLT_ALLOCATOR_H
#define JLT_ALLOCATOR_H

#include <cstdint>
#include <type_traits>
#include <utility>
#include <jolt/threading/lock.hpp>
#include <jolt/threading/lockguard.hpp>
#include <jolt/util.hpp>
#include "defs.hpp"
#include "arena.hpp"
#include "stack.hpp"

namespace jolt {
    namespace memory {
        constexpr size_t BIG_OBJECT_MIN_SIZE = 2 * 1024 * 1024; // 2 MiB
        constexpr size_t ALLOCATOR_SLOTS = 4; // Number of slots available to the engine
        constexpr size_t SMALL_HEAP_MEMORY_SIZE = 4LL * 1024 * 1024 * 1024; // 4 GiB
        constexpr size_t BIG_HEAP_MEMORY_SIZE = 4LL * 1024 * 1024 * 1024;   // 4 GiB
        constexpr size_t PERSISTENT_MEMORY_SIZE = 4LL * 1024 * 1024 * 1024; // 4 GiB
        constexpr size_t SCRATCH_MEMORY_SIZE = 256LL * 1024 * 1024;         // 256 MiB

        /**
         * Allocator slot. Each thread will share its slot with its siblings based
         * on a hash function.
         */
        struct JLTAPI AllocatorSlot {
            Arena m_sm_alloc, m_bg_alloc;
            Stack m_persist, m_scratch;
            jolt::threading::Lock m_lock;

            AllocatorSlot();
        };

        /**
         * The real allocation function. Don't call this. Call `allocate()` instead.
         *
         * @param size The total size of the memory to allocate.
         * @param flags The allocation flags.
         * @param alignment The alignment requirements for the allocated memory.
         */
        void *JLTAPI _allocate(size_t const size, flags_t const flags, size_t const alignment);

        /**
         * The real free function. Don't call this. Call `free()` instead.
         *
         * @param ptr A pointer to the beginning of the memory location to free.
         */
        void JLTAPI _free(void *const ptr);

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
        T *allocate(
          size_t const n = 1,
          flags_t const flags = ALLOC_NONE,
          size_t const alignment = alignof(T)) {
            return reinterpret_cast<T *>(_allocate(
              n * sizeof(T),
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
         * @param n The number of items in the array (1 for non-arrays or arrays where the data has
         * already been destroyed).
         */
        template<typename T>
        void free(T *const ptr, size_t const n = 1) {
            if constexpr(!std::is_void<T>::value && !std::is_trivial<T>::value) {
                for(size_t i = 0; i < n; ++i) { ptr[i].~T(); }
            }

            _free(ptr);
        }

        bool JLTAPI will_relocate(void *const ptr, size_t const new_size);

        inline AllocHeader *get_alloc_header(void *const ptr) {
            return reinterpret_cast<AllocHeader *>(ptr) - 1;
        }

        size_t JLTAPI get_allocated_size();

        /**
         * Return the allocator slot for the calling thread.
         */
        JLTAPI AllocatorSlot &get_allocator_slot();

        void JLTAPI *_reallocate(void *const ptr, size_t const new_size, size_t const alignment);

        template<typename T>
        T *reallocate(T *const ptr, size_t const new_length) {
            AllocatorSlot &slot = get_allocator_slot();
            jolt::threading::LockGuard lock{slot.m_lock};

            if constexpr(std::is_trivial<T>::value) {
                return reinterpret_cast<T *>(_reallocate(ptr, new_length * sizeof(T), alignof(T)));
            } else {
                if(will_relocate(ptr, new_length)) {
                    T *const data_new = allocate<T>(new_length);

                    for(size_t i = 0; i < new_length; ++i) {
                        construct(data_new + i, std::move(ptr[i]));
                        ptr[i].~T();
                    }

                    jolt::memory::free(ptr);

                    return data_new;
                }
            }
            return reinterpret_cast<T *>(_reallocate(ptr, new_length * sizeof(T), alignof(T)));
        }
    } // namespace memory
} // namespace jolt

#endif /* JLT_ALLOCATOR_H */