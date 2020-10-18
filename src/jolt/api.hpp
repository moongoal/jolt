#ifndef JLT_API_HPP
#define JLT_API_HPP

#ifdef JLT_INTERNAL
    #define JLTAPI __attribute__((dllexport))
#else
    #define JLTAPI __attribute__((dllimport))
#endif // JLT_INTERNAL

#define JLT_INLINE __attribute__((always_inline))

#define JLT_MAYBE_UNUSED [[maybe_unused]]

#endif /* JLT_API_HPP */
