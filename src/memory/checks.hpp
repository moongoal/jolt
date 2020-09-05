#ifndef JLT_MEMORY_CHECKS_HPP
#define JLT_MEMORY_CHECKS_HPP

#include <cstring>
#include <features.hpp>
#include <util.hpp>
#include <debug.hpp>

#define JLT_MEM_FILLER_VALUE 0xfe
#define JLT_MEM_CANARY_VALUE 0x0000deadbeef0000
#define JLT_MEM_CANARY_VALUE_TYPE uint64_t
#define JLT_MEM_CANARY_VALUE_SIZE sizeof(JLT_MEM_CANARY_VALUE_TYPE)

#ifdef JLT_WITH_MEM_CHECKS
    #define JLT_CHECK_MEM_USE_AFTER_FREE(ptr, size) jolt::memory::check_use_after_free(ptr, size)
    #define JLT_FILL_AFTER_FREE(ptr, size) memset(ptr, JLT_MEM_FILLER_VALUE, size)
    #define JLT_FILL_OVERFLOW(ptr, size)                                                           \
        do {                                                                                       \
            *reinterpret_cast<JLT_MEM_CANARY_VALUE_TYPE *>(reinterpret_cast<uint8_t *>(ptr) +      \
                                                           size) = JLT_MEM_CANARY_VALUE;           \
        } while(false)
    #define JLT_CHECK_OVERFLOW(ptr, size)                                                          \
        jltassert(*reinterpret_cast<JLT_MEM_CANARY_VALUE_TYPE *>(                                  \
                      reinterpret_cast<uint8_t *>(ptr) + size) == JLT_MEM_CANARY_VALUE)
#else // JLT_WITH_MEM_CHECKS
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
    }  // namespace memory
} // namespace jolt

#endif /* JLT_MEMORY_CHECKS_HPP */
