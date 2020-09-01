#ifndef JLT_UTIL_HPP
#define JLT_UTIL_HPP

#include <cstdint>

#define JLT_TO_STRING(x) #x
#define JLT_QUOTE(x) JLT_TO_STRING(x)

#ifdef JLT_INTERNAL
    #define JLTAPI __attribute__((dllexport))
#else
    #define JLTAPI __attribute__((dllimport))
#endif // JLT_INTERNAL

namespace jolt {
    template<typename T>
    constexpr T choose(T const a, T const b, bool const condition) {
        const uint64_t mask = ~static_cast<uint64_t>(0) + condition;

        return static_cast<T>((static_cast<uint64_t>(b) & mask) |
                              (static_cast<uint64_t>(a) & ~mask));
    }

    void *align_raw_ptr(void *const ptr, size_t const alignment) JLTAPI;
} // namespace jolt

#endif /* JLT_UTIL_HPP */
