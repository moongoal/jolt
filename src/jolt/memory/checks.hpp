#ifndef JLT_MEMORY_CHECKS_HPP
#define JLT_MEMORY_CHECKS_HPP

#include <cstring>
#include <jolt/features.hpp>
#include <jolt/api.hpp>
#include <jolt/debug.hpp>

/**
 * Free memory filler value.
 */
#define JLT_MEM_FILLER_VALUE 0xfe

/**
 * Memory overflow canary value.
 */
#define JLT_MEM_OVERFLOW_CANARY_VALUE 0x0000deadbeef0000

/**
 * Arena free list node canary value.
 */
#define JLT_MEM_ARENA_FLN_CANARY_VALUE 0x0000454552460000 // FREE

/**
 * Allocation header canary value.
 */
#define JLT_MEM_ALLOC_HDR_CANARY_VALUE 0x0000444145480000 // HEAD

/**
 * Memory overflow canary value type.
 */
#define JLT_MEM_OVERFLOW_CANARY_VALUE_TYPE uint64_t

#ifdef JLT_WITH_MEM_CHECKS
    /**
     * Memory overflow canary value size.
     */
    #define JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE sizeof(JLT_MEM_OVERFLOW_CANARY_VALUE_TYPE)

    /**
     * Check the memory hasn't be used after being freed.
     *
     * @param ptr Pointer to the beginning of the memory allocation.
     * @param size Size of the memory allocation.
     *
     * @return A boolean value stating whether the memory is sane or not.
     */
    #define JLT_CHECK_MEM_USE_AFTER_FREE(ptr, size) jolt::memory::check_use_after_free(ptr, size)

    /**
     * Fill empty memory with filler values to check its usage after being freed.
     *
     * @param ptr Pointer to the beginning of the memory allocation.
     * @param size Size of the memory allocation.
     */
    #define JLT_FILL_AFTER_FREE(ptr, size) memset(ptr, JLT_MEM_FILLER_VALUE, size)

    /**
     * Fill memory region's overflow canary value.
     *
     * @param ptr Pointer to the beginning of the memory allocation.
     * @param size Size of the memory allocation.
     */
    #define JLT_FILL_OVERFLOW(ptr, size)                                                                     \
        do {                                                                                                 \
            *reinterpret_cast<JLT_MEM_OVERFLOW_CANARY_VALUE_TYPE *>(                                         \
              reinterpret_cast<uint8_t *>(ptr) + size) = JLT_MEM_OVERFLOW_CANARY_VALUE;                      \
        } while(false)

    /**
     * Check a memory chunk hasn't been used beyond its allocated size.
     * This macro will cause the application to abort upon failure.
     *
     * @param ptr Pointer to the beginning of the memory allocation.
     * @param size Size of the memory allocation.
     */
    #define JLT_CHECK_OVERFLOW(ptr, size)                                                                    \
        jltassert(                                                                                           \
          *reinterpret_cast<JLT_MEM_OVERFLOW_CANARY_VALUE_TYPE *>(reinterpret_cast<uint8_t *>(ptr) + size)   \
          == JLT_MEM_OVERFLOW_CANARY_VALUE)
#else // JLT_WITH_MEM_CHECKS
    #define JLT_MEM_OVERFLOW_CANARY_VALUE_SIZE 0
    #define JLT_CHECK_MEM_USE_AFTER_FREE(ptr, size)
    #define JLT_FILL_AFTER_FREE(ptr, size)
    #define JLT_FILL_OVERFLOW(ptr, size)
    #define JLT_CHECK_OVERFLOW(ptr, size)
#endif // JLT_WITH_MEM_CHECKS

namespace jolt {
    namespace memory {
#ifdef JLT_WITH_MEM_CHECKS
        bool check_use_after_free(void *const ptr, size_t const size) JLTAPI;
#endif // JLT_WITH_MEM_CHECKS
    } // namespace memory
} // namespace jolt

#endif /* JLT_MEMORY_CHECKS_HPP */
