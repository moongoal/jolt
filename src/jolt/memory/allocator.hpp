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
         * Force some allocator flags ON.
         *
         * When using the allocator, these enabled flags will always be active regardless of the
         * flags parameter passed to the function.
         *
         * @param flags The flags to be forced ON.
         *
         * @remarks The effects of this function are not cumulative. Calling it multiple times will
         * disable the previous forced flags and enable this new passed set.
         */
        void JLTAPI force_flags(flags_t const flags);

        /**
         * Save the previously enabled flags and substitute those with a new set.
         *
         * @param flags The flags to be forced ON.
         */
        void JLTAPI push_force_flags(flags_t const flags);

        /**
         * Restore the last previously saved flags set (saved by using `push_force_flags()`.
         *
         * @see push_force_flags()
         */
        void JLTAPI pop_force_flags();

        /**
         * Return the current forced flags.
         */
        flags_t JLTAPI get_current_force_flags();

        /**
         * Diable any forced flag currently active.
         *
         * @remarks This has the same effect as calling `force_flags(ALLOC_NONE)`.
         */
        inline void reset_force_flags() { force_flags(ALLOC_NONE); }

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
          size_t const n = 1, flags_t flags = ALLOC_NONE, size_t const alignment = alignof(T)) {
            flags |= get_current_force_flags();

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
            push_force_flags(get_alloc_flags(ptr));

            auto ptr_new = new(ptr) T(ctor_params...);

            pop_force_flags();

            return ptr_new;
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

        inline flags_t get_alloc_flags(void *const ptr) { return get_alloc_header(ptr)->m_flags; }

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
                    flags_t old_flags = get_alloc_flags(ptr);
                    // TODO: Alignment should be propagated here
                    T *const data_new = allocate<T>(new_length, old_flags);

                    for(size_t i = 0; i < new_length; ++i) {
                        construct(data_new + i, std::move(ptr[i]));
                        ptr[i].~T();
                    }

                    jolt::memory::free(ptr);

                    return data_new;
                }
            }

            // TODO: Alignment should be propagated here
            return reinterpret_cast<T *>(_reallocate(ptr, new_length * sizeof(T), alignof(T)));
        }
    } // namespace memory
} // namespace jolt

#endif /* JLT_ALLOCATOR_H */
