#ifndef JLT_API_HPP
#define JLT_API_HPP

#ifdef JLT_INTERNAL
    #define JLTAPI __attribute__((dllexport))
#else
    #define JLTAPI __attribute__((dllimport))
#endif // JLT_INTERNAL

#define JLT_INLINE __attribute__((always_inline))

#define JLT_MAYBE_UNUSED [[maybe_unused]]
#define JLT_NODISCARD [[nodiscard]]

// Required to prevent clangd from treating unknown attributes as errors.
#if __clang_major__ >= 12
    #warning Remove if/else block

    #define JLT_LIKELY [[likely]]
    #define JLT_UNLIKELY [[unlikely]]
#else // __clang_major__ >= 12
    #define JLT_LIKELY
    #define JLT_UNLIKELY
#endif // __clang_major__ >= 12

#endif /* JLT_API_HPP */
