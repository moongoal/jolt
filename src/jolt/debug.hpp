#ifndef JLT_ASSERT_H
#define JLT_ASSERT_H

#include <jolt/debug/console.hpp>

#ifdef NDEBUG
    #define jltassert(x) ((void)0)
    #define jltassert2(x) ((void)0)
#else
    #define jltassert(x)                                                                           \
        do {                                                                                       \
            if(!(x)) {                                                                             \
                __asm__ volatile("int $0x03");                                                     \
            }                                                                                      \
        } while(false)

    #define jltassert2(x, msg)                                                                     \
        do {                                                                                       \
            if(!(x)) {                                                                             \
                jolt::console.err(msg);                                                            \
                __asm__ volatile("int $0x03");                                                     \
            }                                                                                      \
        } while(false)
#endif // NDEBUG

#include <jolt/debug/console.hpp>

#endif /* JLT_ASSERT_H */
