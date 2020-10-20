#ifndef JLT_ALLOCATOR_H
#define JLT_ALLOCATOR_H

#include <cstdint>
#include <type_traits>
#include <utility>
#include <jolt/threading/lock.hpp>
#include <jolt/threading/lockguard.hpp>
#include <jolt/api.hpp>
#include "defs.hpp"
#include "arena.hpp"
#include "stack.hpp"

/**
 * Shortcut to jolt::memory::allocate.
 *
 * @see jolt::memory::allocate().
 */
#define jltalloc(object_type, ...) jolt::memory::allocate<object_type>(__VA_ARGS__)

/**
 * Shortcut to jolt::memory::allocate.
 *
 * @see jolt::memory::allocate_array().
 */
#define jltallocarray(object_type, n, ...) jolt::memory::allocate_array<object_type>(n, __VA_ARGS__)

/**
 * Construct an already allocated object.
 *
 * @param ptr Pointer the the allocated memory.
 * @param ... Arguments to pass to the object's constructor.
 *
 * @return Pointer to the constructed memory.
 */
#define jltconstruct(ptr, ...) jolt::memory::construct(ptr, __VA_ARGS__)

/**
 * Shortcut to jolt::memory::allocate_and_construct.
 *
 * @see jolt::memory::allocate_and_construct().
 */
#define jltnew(object_type, ...) jolt::memory::allocate_and_construct<object_type>(__VA_ARGS__)

/**
 * Shortcut to jolt::memory::free.
 *
 * @see jolt::memory::free().
 */
#define jltfree(ptr) jolt::memory::free(ptr)

/**
 * Shortcut to jolt::memory::free_array.
 *
 * @see jolt::memory::free_array().
 */
#define jltfreearray(ptr) jolt::memory::free_array(ptr)

namespace jolt {
    namespace memory {
        constexpr size_t BIG_OBJECT_MIN_SIZE = 2 * 1024 * 1024; // 2 MiB
        constexpr size_t ALLOCATOR_SLOTS = 4;                   // Number of slots available to the engine
        constexpr size_t SMALL_HEAP_MEMORY_SIZE = 4LL * 1024 * 1024 * 1024; // 4 GiB
        constexpr size_t BIG_HEAP_MEMORY_SIZE = 4LL * 1024 * 1024 * 1024;   // 4 GiB
        constexpr size_t PERSISTENT_MEMORY_SIZE = 4LL * 1024 * 1024 * 1024; // 4 GiB
        constexpr size_t SCRATCH_MEMORY_SIZE = 256LL * 1024 * 1024;         // 256 MiB
        constexpr size_t JLT_ALLOC_FLAGS_STACK_LEN = 256;                   // Length of force-flags stack

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
         * Array header for allocations performed with `allocate_array()`.
         */
        template<typename T>
        struct ArrayHeader {
            using value_type = T;
            using pointer = T *;

            size_t m_length; //< Number of items.
            pointer m_data;  //< Pointer to the items.
        };

        /**
         * The real allocation function. Don't call this. Call `allocate()` instead.
         *
         * @param size The total size of the memory to allocate.
         * @param flags The allocation flags.
         * @param alignment The alignment requirements for the allocated memory.
         */
        JLT_NODISCARD void *JLTAPI _allocate(size_t const size, flags_t const flags, size_t const alignment);

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
         * Save the previously enabled flags and substitute those with those of a memory region.
         *
         * @param ptr The pointer to the memory region.
         *
         * @remarks Only pointers returned by `allocate()` or `reallocate()` represent valid memory
         * regions for this command. Any other pointer will cause undefined behaviour.
         */
        void JLTAPI push_force_flags(void *const ptr);

        /**
         * Restore the last previously saved flags set (saved by using `push_force_flags()`.
         *
         * @see push_force_flags()
         */
        void JLTAPI pop_force_flags();

        /**
         * Return the current forced flags.
         */
        JLT_NODISCARD flags_t JLTAPI get_current_force_flags();

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
         * @param flags The allocation flags.
         * @param alignment The alignment requirements for the allocated memory.
         */
        template<typename T>
        JLT_NODISCARD T *allocate(flags_t flags = ALLOC_NONE, size_t alignment = alignof(T)) {
            flags |= get_current_force_flags();
            flags |= choose<flags_t>(0, ALLOC_BIG, sizeof(T) < BIG_OBJECT_MIN_SIZE);
            alignment = max(alignment, __STDCPP_DEFAULT_NEW_ALIGNMENT__);

            return reinterpret_cast<T *>(_allocate(sizeof(T), flags, alignment));
        }

        /**
         * Allocate memory for an array of objects of type T. This function will not construct the objects.
         * Use `construct()` after allocating the memory to construct the objects.
         *
         * @tparam T The type of the object to allocate.
         * @param n The number of elements of type `T` to allocate.
         * @param flags The allocation flags.
         * @param alignment The alignment requirements for the allocated memory.
         */
        template<typename T>
        JLT_NODISCARD T *
        allocate_array(size_t const n, flags_t flags = ALLOC_NONE, size_t alignment = alignof(T)) {
            flags |= get_current_force_flags();
            flags |= choose<flags_t>(0, ALLOC_BIG, sizeof(T) < BIG_OBJECT_MIN_SIZE);
            alignment = max(alignment, __STDCPP_DEFAULT_NEW_ALIGNMENT__);

            void *const ptr = _allocate(sizeof(T) * n + sizeof(size_t), flags, alignment);
            auto const len_ptr = reinterpret_cast<size_t *>(ptr);

            *len_ptr = n;

            return reinterpret_cast<T *>(len_ptr + 1);
        }

        /**
         * Simplified one-step allocation and construction function for a single object.
         *
         * @param ctor_params The parameters to be passed to the object constructor.
         *
         * @return A newly allocated and constructed object.
         */
        template<typename T, typename... Params>
        JLT_NODISCARD T *allocate_and_construct(Params &&...ctor_params) {
            T *ptr = allocate<T>();

            return construct(ptr, std::forward<Params>(ctor_params)...);
        }

        /**
         * Construct an object.
         *
         * @tparam T The object type to construct.
         * @tparam Params The constructor parameters types.
         * @param ptr A pointer to the memory location where the object will be constructed.
         * @param ctor_params The values of the parameters that will be passed to the
         constructor.
         *
         * @return The same value as `ptr`.
         */
        template<typename T, typename... Params>
        T *construct(T *const ptr, Params &&...ctor_params) {
            return new(ptr) T(std::forward<Params>(ctor_params)...);
        }

        /**
         * Free a location in memory given its pointer. The object's constructor will be called
         * first.
         *
         * @param ptr A pointer to the beginning of the memory location to free.
         * @param n The number of elements in the array.
         */
        template<typename T>
        void free(T *const ptr) {
            if constexpr(!std::is_void<T>::value && !std::is_trivial<T>::value) {
                ptr->~T();
            }

            _free(ptr);
        }

        /**
         * Free a location in memory given its pointer. The object's constructor will be called
         * first.
         *
         * @param ptr A pointer to the beginning of the memory location to free.
         * @param n The number of elements in the array.
         */
        template<typename T>
        void free_array(T *const ptr) {
            auto const len_ptr = reinterpret_cast<size_t *>(ptr) - 1;
            size_t const n = *len_ptr;

            if constexpr(!std::is_void<T>::value && !std::is_trivial<T>::value) {
                for(size_t i = 0; i < n; ++i) { ptr[i].~T(); }
            }

            _free(len_ptr);
        }

        bool JLTAPI will_relocate(void *const ptr, size_t const new_size);

        JLT_NODISCARD inline AllocHeader *get_alloc_header(void *const ptr) {
            return reinterpret_cast<AllocHeader *>(ptr) - 1;
        }

        inline flags_t get_alloc_flags(void *const ptr) { return get_alloc_header(ptr)->m_flags; }

        size_t JLTAPI get_allocated_size();

        JLT_NODISCARD inline size_t get_array_length(void *ptr) {
            return *(reinterpret_cast<size_t *>(ptr) - 1);
        }

        /**
         * Return the allocator slot for the calling thread.
         */
        JLTAPI AllocatorSlot &get_allocator_slot();

        JLT_NODISCARD void JLTAPI *_reallocate(void *const ptr, size_t const new_size);

        /**
         * Reallocate a previously allocated memory region, shrinking or growing its size.
         *
         * If the size of the memory region is increased, the previously allocated elements are
         * preserved. If the size of the memory region is shrunk, the previously allocated elements
         * are preserved for the first part of the region that will remain allocated.
         *
         * @tparam T The type of the element stored at `ptr`.
         *
         * @param ptr The pointer to the memory region to reallcate.
         * @param new_length The new number of elements in the memory region.
         *
         * @return A possibly new pointer to the reallocated memory region.
         */
        template<typename T>
        JLT_NODISCARD T *reallocate(T *const ptr, size_t const new_length) {
            AllocatorSlot &slot = get_allocator_slot();
            AllocHeader &hdr = *get_alloc_header(ptr); // store old length
            threading::LockGuard lock{slot.m_lock};
            auto const old_len_ptr = reinterpret_cast<size_t *>(ptr) - 1;
            size_t const old_length = *old_len_ptr;
            size_t const new_size = new_length * sizeof(T) + sizeof(size_t);

            if constexpr(!std::is_trivial<T>::value) {
                if(will_relocate(ptr, new_length)) {
                    T *const data_new = allocate_array<T>(new_length, hdr.m_flags, hdr.m_alignment);

                    for(size_t i = 0; i < min(new_length, old_length); ++i) {
                        construct(data_new + i, std::move(ptr[i]));
                    }

                    free_array(ptr);

                    return data_new;
                }
            }

            return reinterpret_cast<T *>(_reallocate(ptr, new_size));
        }
    } // namespace memory
} // namespace jolt

#endif /* JLT_ALLOCATOR_H */
