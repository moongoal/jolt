#ifndef JLT_API_HPP
#define JLT_API_HPP

#ifdef JLT_INTERNAL
    #define JLTAPI __attribute__((dllexport))
#else
    #define JLTAPI __attribute__((dllimport))
#endif // JLT_INTERNAL

#define JLT_INLINE __attribute__((always_inline))

#endif /* JLT_API_HPP */
